#include <common.h>
#include <net/tcp.h>
#include <net/http_request.h>
#include <net/http_handler.h>

static const u8 local_ip[4] = { 192, 0, 2, 1 };
static const u8 peer_ip[4] = { 192, 0, 2, 2 };
static unsigned checks, failures;

#define CHECK(expr) do { checks++; if (!(expr)) { \
 fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); failures++; \
} } while (0)

struct wire {
 unsigned count;
 u8 bytes[TCP_CORE_WIRE_MAX];
 u16 len;
};

static int emit(void *arg, const u8 src[4], const u8 dst[4],
                const u8 *segment, u16 len)
{
 struct wire *w = arg;
 (void)src; (void)dst;
 if (!w || len > sizeof(w->bytes)) return -1;
 memcpy(w->bytes, segment, len);
 w->len = len;
 w->count++;
 return 0;
}

static void put16(u8 *p, u16 v) { p[0]=(u8)(v>>8); p[1]=(u8)v; }
static void put32(u8 *p, u32 v)
{
 p[0]=(u8)(v>>24); p[1]=(u8)(v>>16); p[2]=(u8)(v>>8); p[3]=(u8)v;
}
static u32 get32(const u8 *p)
{
 return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];
}

static u16 make_segment(u8 *out, u16 sport, u16 dport, u32 seq, u32 ack,
                        u8 flags, u16 window, const u8 *payload, u16 plen)
{
 u16 len = (u16)(20 + plen);
 memset(out, 0, len);
 put16(out, sport); put16(out+2, dport); put32(out+4, seq);
 put32(out+8, ack); out[12]=0x50; out[13]=flags;
 put16(out+14, window);
 if (plen) memcpy(out+20, payload, plen);
 put16(out+16, tcp_checksum_ipv4(peer_ip, local_ip, out, len));
 return len;
}

static int input(struct tcp_core_connection *c, struct wire *w,
                 u32 seq, u32 ack, u8 flags, u16 window,
                 const u8 *payload, u16 plen, u32 now)
{
 u8 segment[TCP_CORE_WIRE_MAX];
 u16 len=make_segment(segment, 49152, 80, seq, ack, flags,
                      window, payload, plen);
 return tcp_core_input(c, peer_ip, local_ip, segment, len, now, emit, w);
}

static void connected(struct tcp_core_connection *c, struct wire *w)
{
 u8 syn[20];
 CHECK(tcp_core_listen(c, local_ip, 80, 1000)==0);
 (void)make_segment(syn,49152,80,5000,0,TCP_FLAG_SYN,4096,NULL,0);
 CHECK(tcp_core_input(c,peer_ip,local_ip,syn,20,10,emit,w)==1);
 CHECK(c->state==TCP_CORE_SYN_RECEIVED);
 CHECK(w->count==1 && (w->bytes[13]&(TCP_FLAG_SYN|TCP_FLAG_ACK))==
       (TCP_FLAG_SYN|TCP_FLAG_ACK));
 CHECK(input(c,w,5001,1001,TCP_FLAG_ACK,4096,NULL,0,20)==1);
 CHECK(c->state==TCP_CORE_ESTABLISHED);
}

static void test_01_listen_and_syn(void)
{
 struct tcp_core_connection c; struct wire w={0}; u8 s[20];
 CHECK(tcp_core_listen(&c,local_ip,80,123)==0);
 CHECK(c.state==TCP_CORE_LISTEN && c.snd_nxt==123);
 (void)make_segment(s,49152,81,1,0,TCP_FLAG_SYN,1000,NULL,0);
 CHECK(tcp_core_input(&c,peer_ip,local_ip,s,20,0,emit,&w)==0);
 CHECK(c.state==TCP_CORE_LISTEN);
}
static void test_02_handshake(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 CHECK(c.rcv_nxt==5001 && c.snd_una==1001);
}
static void test_03_bad_checksum(void)
{
 struct tcp_core_connection c; struct wire w={0}; u8 s[20];
 tcp_core_listen(&c,local_ip,80,100);
 make_segment(s,49152,80,1,0,TCP_FLAG_SYN,1000,NULL,0); s[16]^=1;
 CHECK(tcp_core_input(&c,peer_ip,local_ip,s,20,0,emit,&w)<0);
 CHECK(c.state==TCP_CORE_LISTEN);
}
static void test_04_receive_payload(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="abc"; u8 b[8];
 connected(&c,&w);
 CHECK(input(&c,&w,5001,1001,TCP_FLAG_ACK|TCP_FLAG_PSH,4096,p,3,30)==0);
 CHECK(c.rx_len==3 && c.rcv_nxt==5004);
 CHECK(tcp_core_read(&c,b,sizeof(b))==3 && !memcmp(b,p,3));
}
static void test_05_read_empty(void)
{
 struct tcp_core_connection c; u8 b[1]; tcp_core_init(&c);
 CHECK(tcp_core_read(&c,b,sizeof(b))==0);
}
static void test_06_out_of_order(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="x";
 connected(&c,&w);
 CHECK(input(&c,&w,5002,1001,TCP_FLAG_ACK|TCP_FLAG_PSH,4096,p,1,30)==0);
 CHECK(c.rx_len==0 && c.rcv_nxt==5001);
 CHECK(w.bytes[13]&TCP_FLAG_ACK);
}
static void test_07_receive_fin_payload(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="last"; u8 b[8];
 connected(&c,&w);
 CHECK(input(&c,&w,5001,1001,TCP_FLAG_ACK|TCP_FLAG_FIN|TCP_FLAG_PSH,
             4096,p,4,30)==0);
 CHECK(c.state==TCP_CORE_CLOSE_WAIT && c.rx_len==4 && c.fin_received);
 CHECK(tcp_core_read(&c,b,sizeof(b))==4 && !memcmp(b,p,4));
}
static void test_08_write_and_pending(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="reply";
 connected(&c,&w);
 CHECK(tcp_core_write(&c,p,5,30,emit,&w)==5);
 CHECK(c.pending && c.snd_nxt==1006);
 CHECK(tcp_core_write(&c,p,5,31,emit,&w)<0);
}
static void test_09_partial_ack(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="0123456789";
 connected(&c,&w); CHECK(tcp_core_write(&c,p,10,30,emit,&w)==10);
 CHECK(input(&c,&w,5001,1006,TCP_FLAG_ACK,4096,NULL,0,40)==0);
 CHECK(c.pending && c.retry_len==5 && c.retry_seq==1006);
 CHECK(!memcmp(c.retry_data,"56789",5));
}
static void test_10_full_ack(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="abc";
 connected(&c,&w); CHECK(tcp_core_write(&c,p,3,30,emit,&w)==3);
 CHECK(input(&c,&w,5001,1004,TCP_FLAG_ACK,4096,NULL,0,40)==0);
 CHECK(!c.pending && c.snd_una==1004);
}
static void test_11_zero_window(void)
{
 struct tcp_core_connection c; struct wire w={0}; const u8 p[]="x";
 connected(&c,&w); c.peer_window=0;
 CHECK(tcp_core_write(&c,p,1,30,emit,&w)==0 && !c.pending);
}
static void test_12_write_mss_limit(void)
{
 struct tcp_core_connection c; struct wire w={0}; u8 p[TCP_CORE_TX_CAPACITY+1];
 connected(&c,&w); memset(p,0,sizeof(p));
 CHECK(tcp_core_write(&c,p,sizeof(p),30,emit,&w)<0);
}
static void test_13_close_established(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 CHECK(tcp_core_close(&c,30,emit,&w)==0);
 CHECK(c.state==TCP_CORE_FIN_WAIT_1 && c.pending);
 CHECK(w.bytes[13]&(TCP_FLAG_FIN|TCP_FLAG_ACK));
}
static void test_14_fin_ack_to_fin_wait_2(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 CHECK(tcp_core_close(&c,30,emit,&w)==0);
 CHECK(input(&c,&w,5001,1002,TCP_FLAG_ACK,4096,NULL,0,40)==0);
 CHECK(c.state==TCP_CORE_FIN_WAIT_2 && !c.pending);
}
static void test_15_peer_fin_to_time_wait(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 tcp_core_close(&c,30,emit,&w);
 input(&c,&w,5001,1002,TCP_FLAG_ACK,4096,NULL,0,40);
 CHECK(input(&c,&w,5001,1002,TCP_FLAG_ACK|TCP_FLAG_FIN,4096,NULL,0,50)==0);
 CHECK(c.state==TCP_CORE_TIME_WAIT && c.time_wait_deadline==60050);
}
static void test_16_time_wait_expiry(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 tcp_core_close(&c,30,emit,&w); input(&c,&w,5001,1002,TCP_FLAG_ACK,4096,NULL,0,40);
 input(&c,&w,5001,1002,TCP_FLAG_ACK|TCP_FLAG_FIN,4096,NULL,0,50);
 CHECK(tcp_core_poll(&c,60050,emit,&w)==0 && c.state==TCP_CORE_LISTEN);
}
static void test_17_close_wait_last_ack(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 input(&c,&w,5001,1001,TCP_FLAG_ACK|TCP_FLAG_FIN,4096,NULL,0,30);
 CHECK(c.state==TCP_CORE_CLOSE_WAIT);
 CHECK(tcp_core_close(&c,40,emit,&w)==0 && c.state==TCP_CORE_LAST_ACK);
}
static void test_18_last_ack_complete(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 input(&c,&w,5001,1001,TCP_FLAG_ACK|TCP_FLAG_FIN,4096,NULL,0,30);
 tcp_core_close(&c,40,emit,&w);
 CHECK(input(&c,&w,5002,1002,TCP_FLAG_ACK,4096,NULL,0,50)==1);
 CHECK(c.state==TCP_CORE_LISTEN);
}
static void test_19_syn_retransmit(void)
{
 struct tcp_core_connection c; struct wire w={0}; u8 s[20];
 tcp_core_listen(&c,local_ip,80,100);
 make_segment(s,49152,80,5000,0,TCP_FLAG_SYN,4096,NULL,0);
 tcp_core_input(&c,peer_ip,local_ip,s,20,0,emit,&w);
 CHECK(tcp_core_poll(&c,1000,emit,&w)==1 && c.retries==1 && w.count==2);
}
static void test_20_retry_exhaustion(void)
{
 struct tcp_core_connection c; struct wire w={0}; u8 s[20]; unsigned i;
 tcp_core_listen(&c,local_ip,80,100);
 make_segment(s,49152,80,5000,0,TCP_FLAG_SYN,4096,NULL,0);
 tcp_core_input(&c,peer_ip,local_ip,s,20,0,emit,&w);
 for(i=1;i<=6 && c.state!=TCP_CORE_LISTEN;i++)
  tcp_core_poll(&c,i*1000,emit,&w);
 CHECK(c.state==TCP_CORE_LISTEN);
}
static void test_21_idle_timeout(void)
{
 struct tcp_core_connection c; struct wire w={0}; connected(&c,&w);
 CHECK(tcp_core_poll(&c,30020,emit,&w)==1 && c.state==TCP_CORE_LISTEN);
}
static void test_22_sequence_wrap_helpers(void)
{
 CHECK(tcp_seq_after(1,0xffffffffU));
 CHECK(tcp_seq_before(0xffffffffU,1));
}
static void test_23_parser_fragmented(void)
{
 struct http_request_parser p; const char *a="GET /api/status HTTP/1.1\r\nHost: x\r\n";
 const char *b="\r\n"; http_request_parser_init(&p);
 CHECK(http_request_parser_feed(&p,(const u8 *)a,strlen(a))==HTTP_REQ_NEED_MORE);
 CHECK(http_request_parser_feed(&p,(const u8 *)b,strlen(b))==HTTP_REQ_COMPLETE);
 CHECK(!strcmp(p.request.method,"GET") && !strcmp(p.request.uri,"/api/status"));
}
static void test_24_parser_bad_crlf(void)
{
 struct http_request_parser p; const char *s="GET / HTTP/1.1\n\n";
 http_request_parser_init(&p);
 CHECK(http_request_parser_feed(&p,(const u8 *)s,strlen(s))==HTTP_REQ_ERROR);
 CHECK(p.error==HTTP_REQ_ERR_BAD_CRLF);
}
static void test_25_parser_reject_body_and_te(void)
{
 struct http_request_parser p; const char *s="POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n";
 http_request_parser_init(&p);
 CHECK(http_request_parser_feed(&p,(const u8 *)s,strlen(s))==HTTP_REQ_ERROR);
 CHECK(p.error==HTTP_REQ_ERR_TRANSFER_ENCODING_NOT_SUPPORTED);
}
static void test_26_parser_reject_trailing(void)
{
 struct http_request_parser p; const char *s="GET / HTTP/1.1\r\n\r\nX";
 http_request_parser_init(&p);
 CHECK(http_request_parser_feed(&p,(const u8 *)s,strlen(s))==HTTP_REQ_ERROR);
 CHECK(p.error==HTTP_REQ_ERR_BODY_TOO_LARGE);
}
static void test_27_handler_routes_and_head(void)
{
 struct http_request q; struct http_handler_info info; u8 out[HTTP_HANDLER_TX_CAPACITY]; u16 n;
 memset(&q,0,sizeof(q)); memset(&info,0,sizeof(info));
 strcpy(q.method,"GET"); strcpy(q.uri,"/api/status"); strcpy(q.version,"HTTP/1.1");
 strcpy(info.board_name,"PA-220"); info.flags=HTTP_INFO_MEMORY_VALID; info.memory_total_bytes=1024;
 CHECK(http_handler_build(&q,&info,out,sizeof(out),&n)==0);
 CHECK(n>20 && !memcmp(out,"HTTP/1.1 200 OK",15));
 strcpy(q.method,"HEAD");
 CHECK(http_handler_build(&q,&info,out,sizeof(out),&n)==0);
 CHECK(n>0);
}
static void test_28_handler_errors(void)
{
 struct http_request q; struct http_handler_info info; u8 out[HTTP_HANDLER_TX_CAPACITY]; u16 n;
 memset(&q,0,sizeof(q)); memset(&info,0,sizeof(info));
 strcpy(q.method,"POST"); strcpy(q.uri,"/"); strcpy(q.version,"HTTP/1.1");
 CHECK(http_handler_build(&q,&info,out,sizeof(out),&n)==0);
 CHECK(!memcmp(out,"HTTP/1.1 405",12));
 strcpy(q.method,"GET"); strcpy(q.uri,"/no-such-uri");
 CHECK(http_handler_build(&q,&info,out,sizeof(out),&n)==0);
 CHECK(!memcmp(out,"HTTP/1.1 404",12));
}

int main(void)
{
 void (*tests[])(void)={
 test_01_listen_and_syn,test_02_handshake,test_03_bad_checksum,
 test_04_receive_payload,test_05_read_empty,test_06_out_of_order,
 test_07_receive_fin_payload,test_08_write_and_pending,test_09_partial_ack,
 test_10_full_ack,test_11_zero_window,test_12_write_mss_limit,
 test_13_close_established,test_14_fin_ack_to_fin_wait_2,test_15_peer_fin_to_time_wait,
 test_16_time_wait_expiry,test_17_close_wait_last_ack,test_18_last_ack_complete,
 test_19_syn_retransmit,test_20_retry_exhaustion,test_21_idle_timeout,
 test_22_sequence_wrap_helpers,test_23_parser_fragmented,test_24_parser_bad_crlf,
 test_25_parser_reject_body_and_te,test_26_parser_reject_trailing,
 test_27_handler_routes_and_head,test_28_handler_errors
 };
 unsigned i;
 for(i=0;i<ARRAY_SIZE(tests);i++) {
  unsigned before=failures;
  tests[i]();
  printf("%02u %s\n",i+1,failures==before?"PASS":"FAIL");
 }
 printf("P5-2C HOST TESTS: %u checks, %u failures, 28 scenarios\n",checks,failures);
 return failures?1:0;
}
