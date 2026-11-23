#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/http_handler.h>
#include <common.h>
#include <net.h>

gd_t *gd;
struct eth_device *http_test_eth_current;
u32 NetOurIP;

ulong get_timer(ulong base)
{
 (void)base;
 return 123456UL;
}

static int assertions;
static int failures;
#define CHECK(x) do { assertions++; if (!(x)) { failures++; \
 printf("FAIL line %d: %s\n",__LINE__,#x); } } while (0)

static int parse_request(const char *raw,struct http_request *request)
{
 struct http_request_parser parser;
 enum http_req_status result;
 http_request_parser_init(&parser);
 result=http_request_parser_feed(&parser,(const u8 *)raw,(u16)strlen(raw));
 if(result!=HTTP_REQ_COMPLETE) return -1;
 *request=parser.request;
 return 0;
}

static int build_raw(const char *raw,const struct http_handler_info *info,
 u8 *out,u16 cap,u16 *length)
{
 struct http_request request;
 if(parse_request(raw,&request)) return -1;
 return http_handler_build(&request,info,out,cap,length);
}

static int has(const u8 *data,u16 length,const char *needle)
{
 u16 n=(u16)strlen(needle),i;
 if(n>length) return 0;
 for(i=0;i<=length-n;i++)
  if(!memcmp(data+i,needle,n)) return 1;
 return 0;
}

static int split_body(const u8 *data,u16 length,const u8 **body,u16 *body_len)
{
 u16 i;
 for(i=0;i+3<length;i++)
  if(!memcmp(data+i,"\r\n\r\n",4)) {
   *body=data+i+4; *body_len=(u16)(length-i-4); return 0;
  }
 return -1;
}

static int get_content_length(const u8 *data,u16 length,u16 *value)
{
 u16 i;
 for(i=0;i+16<length;i++)
  if(!memcmp(data+i,"Content-Length: ",16)) {
   u32 n=0; i+=16;
   while(i<length&&data[i]>='0'&&data[i]<='9') {
    n=n*10+data[i]-'0'; i++;
   }
   *value=(u16)n; return 0;
  }
 return -1;
}

static void fixture(struct http_handler_info *info)
{
 memset(info,0,sizeof(*info));
 strcpy(info->board_name,"Palo Alto PA-220");
 strcpy(info->cpu_name,"Cavium Octeon CN7130");
 strcpy(info->interface_name,"octrgmii0");
 info->board_type=20021;
 info->cpu_clock_hz=1000000000U;
 info->uptime_ms=123456U;
 info->memory_start=0x10000000ULL;
 info->memory_total_bytes=0x20000000ULL;
 info->flags=HTTP_INFO_MEMORY_VALID|HTTP_INFO_ETH_PRESENT|
             HTTP_INFO_MAC_VALID|HTTP_INFO_IP_VALID;
 info->eth_state=HTTP_INFO_ETH_ACTIVE;
 info->mac[0]=0x00; info->mac[1]=0x11; info->mac[2]=0x22;
 info->mac[3]=0x33; info->mac[4]=0x44; info->mac[5]=0x55;
 info->ip[0]=192; info->ip[1]=168; info->ip[2]=1; info->ip[3]=2;
}

static void route_test(const char *name,const char *raw,
 const char *expected,const char *body_text)
{
 struct http_handler_info info;
 u8 out[HTTP_HANDLER_TX_CAPACITY];
 u16 length=0;
 fixture(&info);
 CHECK(build_raw(raw,&info,out,sizeof(out),&length)==0);
 CHECK(length>0);
 CHECK(has(out,length,expected));
 if(body_text) CHECK(has(out,length,body_text));
 printf("%s: PASS\n",name);
}

int main(void)
{
 struct http_handler_info info;
 struct http_request request;
 u8 get[HTTP_HANDLER_TX_CAPACITY],head[HTTP_HANDLER_TX_CAPACITY];
 u8 out[HTTP_HANDLER_TX_CAPACITY];
 u16 get_len=0,head_len=0,cl_get=0,cl_head=0,body_len=0,exact=0;
 const u8 *body;
 int rc;

 fixture(&info);
 route_test("01 GET /","GET / HTTP/1.1\r\nHost: x\r\n\r\n",
            "200 OK","/api/network");
 route_test("02 HEAD /","HEAD / HTTP/1.1\r\nHost: x\r\n\r\n",
            "200 OK",NULL);
 route_test("03 GET status","GET /api/status HTTP/1.1\r\n\r\n",
            "200 OK","\"uptime_ms\":123456");
 route_test("04 HEAD status","HEAD /api/status HTTP/1.1\r\n\r\n",
            "200 OK",NULL);
 route_test("05 GET board","GET /api/board HTTP/1.1\r\n\r\n",
            "200 OK","CN7130");
 route_test("06 GET memory","GET /api/memory HTTP/1.1\r\n\r\n",
            "200 OK","\"dram_total_bytes\":536870912");
 route_test("07 GET network","GET /api/network HTTP/1.1\r\n\r\n",
            "200 OK","octrgmii0");
 route_test("08 unknown URI","GET /missing HTTP/1.1\r\n\r\n",
            "404 Not Found","Not Found");
 route_test("09 prefix bypass","GET /api/statusx HTTP/1.1\r\n\r\n",
            "404 Not Found",NULL);
 route_test("10 extra path","GET /api/status/extra HTTP/1.1\r\n\r\n",
            "404 Not Found",NULL);
 route_test("11 encoded path","GET /api/%73tatus HTTP/1.1\r\n\r\n",
            "404 Not Found",NULL);
 route_test("12 query string","GET /api/status?x=1 HTTP/1.1\r\n\r\n",
            "404 Not Found",NULL);
 route_test("13 POST","POST / HTTP/1.1\r\n\r\n",
            "405 Method Not Allowed","Allow: GET, HEAD");
 route_test("14 PUT","PUT / HTTP/1.1\r\n\r\n",
            "405 Method Not Allowed","Allow: GET, HEAD");
 route_test("15 DELETE","DELETE / HTTP/1.1\r\n\r\n",
            "405 Method Not Allowed","Allow: GET, HEAD");

 CHECK(parse_request("GET /api/status HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,&info,get,sizeof(get),&get_len)==0);
 CHECK(parse_request("HEAD /api/status HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,&info,head,sizeof(head),&head_len)==0);
 CHECK(has(head,head_len,"200 OK"));
 CHECK(has(head,head_len,"Content-Type: application/json"));
 CHECK(has(head,head_len,"Connection: close"));
 CHECK(get_content_length(get,get_len,&cl_get)==0);
 CHECK(get_content_length(head,head_len,&cl_head)==0);
 CHECK(cl_head==cl_get);
 CHECK(split_body(head,head_len,&body,&body_len)==0);
 CHECK(body_len==0);
 CHECK(split_body(get,get_len,&body,&body_len)==0);
 CHECK(body_len==cl_get);
 printf("16 HEAD body suppressed and GET length retained: PASS\n");
 printf("17 GET body bytes equal Content-Length: PASS\n");

 CHECK(parse_request("GET /api/status HTTP/1.1\r\n\r\n",&request)==0);
 rc=http_handler_build(&request,&info,out,24,&exact);
 CHECK(rc!=0&&exact==0);
 printf("18 undersized output buffer: PASS\n");

 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 rc=http_handler_build(&request,&info,out,exact,&head_len);
 CHECK(rc==0&&head_len==exact);
 rc=http_handler_build(&request,&info,out,(u16)(exact-1),&head_len);
 CHECK(rc!=0&&head_len==0);
 printf("19 exact output boundary: PASS\n");

 strcpy(info.interface_name,"eth\"\\\n");
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"eth\\\"\\\\\\u000a"));
 printf("20 JSON escaping: PASS\n");

 strcpy(info.board_name,"A<&\"'>");
 CHECK(parse_request("GET / HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"A&lt;&amp;&quot;&#39;&gt;"));
 printf("21 HTML escaping: PASS\n");

 route_test("22 HTTP/1.0","GET /api/status HTTP/1.0\r\n\r\n",
            "HTTP/1.0 200 OK","\"state\":\"running\"");
 route_test("23 HTTP/1.1","GET /api/status HTTP/1.1\r\n\r\n",
            "HTTP/1.1 200 OK","\"state\":\"running\"");

 CHECK(parse_request("G ET / HTTP/1.1\r\n\r\n",&request)!=0);
 memset(&request,0,sizeof(request));
 strcpy(request.method,"G ET"); strcpy(request.uri,"/");
 strcpy(request.version,"HTTP/1.1");
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"400 Bad Request"));
 printf("24 invalid Method: PASS\n");

 memset(&request,0,sizeof(request));
 strcpy(request.method,"GET"); strcpy(request.uri,"/bad\\path");
 strcpy(request.version,"HTTP/1.1");
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"400 Bad Request"));
 printf("25 invalid URI: PASS\n");

 CHECK(http_handler_build(NULL,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"400 Bad Request"));
 CHECK(parse_request("GET /api/status HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,NULL,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"500 Internal Server Error"));
 printf("26 null request and unavailable info: PASS\n");

 CHECK(parse_request("GET /api/memory HTTP/1.1\r\n\r\n",&request)==0);
 info.flags=(u8)(info.flags&~HTTP_INFO_MEMORY_VALID);
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"500 Internal Server Error"));
 printf("27 unavailable memory maps to 500: PASS\n");

 {
  bd_t bd;
  gd_t global;
  struct eth_device dev;
  memset(&bd,0,sizeof(bd));
  memset(&global,0,sizeof(global));
  memset(&dev,0,sizeof(dev));
  bd.bi_memstart=0x10000000UL;
  bd.bi_memsize=0x20000000ULL;
  global.bd=&bd;
  global.cpu_clk=800000000UL;
  gd=&global;
  strcpy(dev.name,"octrgmii0");
  dev.state=ETH_STATE_ACTIVE;
  dev.enetaddr[0]=0x00; dev.enetaddr[5]=0x55;
  http_test_eth_current=&dev;
  NetOurIP=0x0201a8c0U;
  CHECK(http_handler_get_device_info(&info)==0);
  CHECK(info.memory_total_bytes==0x20000000ULL);
  CHECK(info.cpu_clock_hz==800000000U);
  CHECK(info.uptime_ms==123456U);
  CHECK(info.flags&HTTP_INFO_ETH_PRESENT);
  CHECK(info.flags&HTTP_INFO_IP_VALID);
  CHECK(!strcmp(info.interface_name,"octrgmii0"));
  CHECK(parse_request("GET /api/network HTTP/1.1\r\n\r\n",&request)==0);
  CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
  CHECK(has(out,exact,"192.168.1.2"));
  printf("28 U-Boot read-only data provider: PASS\n");
 }

 fixture(&info);
 CHECK(parse_request("GET /api/network HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(has(out,exact,"octrgmii0"));
 CHECK(parse_request("GET /api/status HTTP/1.1\r\n\r\n",&request)==0);
 CHECK(http_handler_build(&request,&info,out,sizeof(out),&exact)==0);
 CHECK(!has(out,exact,"octrgmii0"));
 printf("29 repeated calls have no state residue: PASS\n");

 printf("Assertions: %d, failures: %d\n",assertions,failures);
 return failures?1:0;
}
