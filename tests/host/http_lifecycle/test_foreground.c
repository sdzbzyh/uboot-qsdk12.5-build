#include <common.h>
#include <command.h>
#include <net.h>
#include <net/tcp.h>
#include <net/tcp_ipv4_tx.h>
#include <net/http_handler.h>
#include <asm/global_data.h>

extern int do_pa220_web(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

static const u8 local_ip[4] = { 192, 0, 2, 1 };
static const u8 peer_ip[4] = { 192, 0, 2, 2 };
static const u8 peer_mac[6] = { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 };
static const char request_text[] =
 "GET /api/status HTTP/1.1\r\nHost: host\r\n\r\n";

IPaddr_t NetOurIP;
static struct eth_device device;
static struct bd_info board_data;
static struct global_data global_data_storage;
struct global_data *gd = &global_data_storage;

static void (*rx_handler)(const uchar *, unsigned, IPaddr_t, IPaddr_t,
                          const uchar [6]);
static u8 last_output[TCP_CORE_WIRE_MAX];
static u16 last_output_len;
static u8 response_capture[HTTP_HANDLER_TX_CAPACITY * 2];
static u16 response_capture_len;
static unsigned response_bytes_sent;
static unsigned response_bytes_acked;
static unsigned response_bytes_acked_at_fin;
static unsigned output_count;
static unsigned rx_events;
static unsigned eth_init_count;
static unsigned eth_halt_count;
static unsigned handler_register_count;
static int synack_seen;
static int fin_seen;
static int fin_acked;
static int service_complete;
static u32 fake_time = 100;

static void put16(u8 *p, u16 v) { p[0]=(u8)(v>>8); p[1]=(u8)v; }
static void put32(u8 *p, u32 v)
{
 p[0]=(u8)(v>>24); p[1]=(u8)(v>>16); p[2]=(u8)(v>>8); p[3]=(u8)v;
}
static u32 get32(const u8 *p)
{
 return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];
}
static u32 ip_value(const u8 ip[4])
{
 u32 value;
 memcpy(&value,ip,4);
 return value;
}
static u16 build_segment(u8 *out,u32 seq,u32 ack,u8 flags,
                         const u8 *payload,u16 payload_len)
{
 u16 len=(u16)(20+payload_len);
 memset(out,0,len);
 put16(out,49152); put16(out+2,80); put32(out+4,seq); put32(out+8,ack);
 out[12]=0x50; out[13]=flags; put16(out+14,4096);
 if(payload_len) memcpy(out+20,payload,payload_len);
 put16(out+16,tcp_checksum_ipv4(peer_ip,local_ip,out,len));
 return len;
}
static u32 last_server_end(void)
{
 u16 hlen;
 u32 end;
 u8 flags;
 if(last_output_len<20) return 0;
 hlen=(u16)((last_output[12]>>4)*4);
 if(hlen>last_output_len) return 0;
 flags=last_output[13];
 end=get32(last_output+4)+(last_output_len-hlen);
 if(flags&TCP_FLAG_SYN) end++;
 if(flags&TCP_FLAG_FIN) end++;
 return end;
}
static int contains_bytes(const u8 *hay,u16 hay_len,const char *needle)
{
 size_t n=strlen(needle);
 u16 i;
 if(n>hay_len) return 0;
 for(i=0;i<=hay_len-n;i++)
  if(!memcmp(hay+i,needle,n)) return 1;
 return 0;
}

void tcp_ipv4_tx_clear(struct tcp_ipv4_tx_peer *peer)
{
 if(peer) memset(peer,0,sizeof(*peer));
}
int tcp_ipv4_tx_learn_peer(struct tcp_ipv4_tx_peer *peer,
                           const u8 src[4],const u8 mac[6])
{
 if(!peer||memcmp(src,peer_ip,4)||memcmp(mac,peer_mac,6)) return -1;
 memcpy(peer->ip,src,4); memcpy(peer->mac,mac,6); peer->valid=1;
 return 0;
}
int tcp_ipv4_tx_peer_matches(const struct tcp_ipv4_tx_peer *peer,
                             const u8 src[4],const u8 mac[6])
{
 return peer&&peer->valid&&!memcmp(peer->ip,src,4)&&!memcmp(peer->mac,mac,6);
}
int tcp_ipv4_tx_emit(void *arg,const u8 src[4],const u8 dst[4],
                     const u8 *segment,u16 len)
{
 struct tcp_ipv4_tx_peer *peer=arg;
 u16 hlen;
 u8 flags;
 if(!peer||!peer->valid||memcmp(src,local_ip,4)||memcmp(dst,peer_ip,4)||
    len<20||len>sizeof(last_output)) return -1;
 memcpy(last_output,segment,len); last_output_len=len; output_count++;
 hlen=(u16)((segment[12]>>4)*4); flags=segment[13];
 if((flags&(TCP_FLAG_SYN|TCP_FLAG_ACK))==(TCP_FLAG_SYN|TCP_FLAG_ACK))
  synack_seen=1;
 if((flags&TCP_FLAG_PSH)&&len>=hlen) {
  u16 payload=(u16)(len-hlen);
  if(response_capture_len+payload<=sizeof(response_capture)) {
   memcpy(response_capture+response_capture_len,segment+hlen,payload);
   response_capture_len+=payload;
   response_bytes_sent+=payload;
  }
 }
 if(flags&TCP_FLAG_FIN) {
  fin_seen=1;
  response_bytes_acked_at_fin=response_bytes_acked;
 }
 return 0;
}

void net_init(void) { }
void eth_halt(void) { eth_halt_count++; }
void eth_set_current(void) { }
struct eth_device *eth_get_dev(void) { return &device; }
int eth_init(struct bd_info *bd) { (void)bd; eth_init_count++; return 0; }
u32 get_timer(u32 base) { (void)base; return fake_time++; }
void net_set_tcp_handler(void (*handler)(const uchar *,unsigned,IPaddr_t,
                                         IPaddr_t,const uchar [6]))
{
 rx_handler=handler;
 handler_register_count++;
}
int ctrlc(void) { return service_complete; }

static void inject(u32 seq,u32 ack,u8 flags,const u8 *payload,u16 payload_len)
{
 u8 segment[TCP_CORE_WIRE_MAX];
 u16 len;
 if(!rx_handler) { fprintf(stderr,"RX handler unexpectedly NULL\n"); exit(2); }
 len=build_segment(segment,seq,ack,flags,payload,payload_len);
 rx_handler(segment,len,ip_value(peer_ip),ip_value(local_ip),peer_mac);
}

void eth_rx(void)
{
 u32 ack;
 if(rx_events==0) {
  inject(5000,0,TCP_FLAG_SYN,NULL,0);
 } else if(rx_events==1) {
  ack=last_server_end();
  inject(5001,ack,TCP_FLAG_ACK,NULL,0);
 } else if(rx_events==2) {
  ack=last_server_end();
  inject(5001,ack,TCP_FLAG_ACK|TCP_FLAG_PSH|TCP_FLAG_FIN,
         (const u8 *)request_text,(u16)strlen(request_text));
 } else {
  u8 flags=last_output_len>=20?last_output[13]:0;
  u16 hlen=last_output_len>=20?(u16)((last_output[12]>>4)*4):20;
  u16 payload=last_output_len>=hlen?(u16)(last_output_len-hlen):0;
  ack=last_server_end();
  if(flags&TCP_FLAG_PSH) response_bytes_acked+=payload;
  if(flags&TCP_FLAG_FIN) fin_acked=1;
  inject(5001+(u32)strlen(request_text)+1,ack,TCP_FLAG_ACK,NULL,0);
  if(flags&TCP_FLAG_FIN) service_complete=1;
 }
 rx_events++;
}

int main(void)
{
 char *argv[]={"pa220","web"};
 int rc;
 memcpy(&NetOurIP,local_ip,4);
 memset(&device,0,sizeof(device));
 strcpy(device.name,"eth0");
 memcpy(device.enetaddr,(u8[]){0x02,0xaa,0xbb,0xcc,0xdd,0xee},6);
 device.state=ETH_STATE_ACTIVE;
 board_data.bi_memstart=0;
 board_data.bi_memsize=1024ULL*1024ULL*1024ULL;
 global_data_storage.bd=&board_data;
 global_data_storage.cpu_clk=800000000ULL;

 rc=do_pa220_web(NULL,0,2,argv);
 CHECK_UNUSED:
 if(rc!=0) { fprintf(stderr,"foreground command returned %d\n",rc); return 1; }
 if(!synack_seen||!response_bytes_sent||!fin_seen||!fin_acked) {
  fprintf(stderr,"missing lifecycle event: synack=%d response=%u fin=%d fin_ack=%d\n",
          synack_seen,response_bytes_sent,fin_seen,fin_acked); return 1;
 }
 if(response_bytes_acked!=response_bytes_sent ||
    response_bytes_acked_at_fin!=response_bytes_sent) {
  fprintf(stderr,"FIN sent before response fully ACKed: sent=%u acked=%u at_fin=%u\n",
          response_bytes_sent,response_bytes_acked,response_bytes_acked_at_fin);
  return 1;
 }
 if(!contains_bytes(response_capture,response_capture_len,"HTTP/1.1 200 OK")) {
  fprintf(stderr,"HTTP 200 response missing\n"); return 1;
 }
 if(!contains_bytes(response_capture,response_capture_len,"PA-220")) {
  fprintf(stderr,"device response body missing\n"); return 1;
 }
 if(rx_handler||handler_register_count!=2||eth_init_count!=1||eth_halt_count!=2) {
  fprintf(stderr,"foreground cleanup invariant failed\n"); return 1;
 }
 printf("FOREGROUND HOST: PASS; rx_events=%u outputs=%u response_bytes=%u; "
        "FIN after all response ACKs; cleanup verified\n",
        rx_events,output_count,response_bytes_sent);
 return 0;
}
