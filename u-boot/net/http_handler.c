/*
 * PA-220 bounded read-only HTTP/1.x URI handler.
 * Exact URI allowlist; no decoding, filesystem access, commands or writes.
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <net/http_handler.h>

#define HTTP_STATUS_200 200
#define HTTP_STATUS_400 400
#define HTTP_STATUS_404 404
#define HTTP_STATUS_405 405
#define HTTP_STATUS_500 500

struct http_writer {
 u8 *data;
 u16 capacity;
 u16 length;
 int failed;
};

static const char home_prefix[] =
 "<!doctype html><html><head><meta charset=\"utf-8\">"
 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
 "<title>PA-220 U-BootKit</title></head><body><h1>";
static const char home_suffix[] =
 "</h1><ul><li><a href=\"/api/status\">Status</a></li>"
 "<li><a href=\"/api/board\">Board</a></li>"
 "<li><a href=\"/api/memory\">Memory</a></li>"
 "<li><a href=\"/api/network\">Network</a></li>"
 "</ul></body></html>";
static const char error_400[] = "Bad Request\n";
static const char error_404[] = "Not Found\n";
static const char error_405[] = "Method Not Allowed\n";
static const char error_500[] = "Internal Server Error\n";

struct http_route {
 const char *uri;
 u16 length;
 const char *content_type;
};
static const struct http_route routes[] = {
 { "/", 1, "text/html; charset=utf-8" },
 { "/api/status", 11, "application/json" },
 { "/api/board", 10, "application/json" },
 { "/api/memory", 11, "application/json" },
 { "/api/network", 12, "application/json" }
};

static u16 bounded_length(const char *s, u16 max)
{
 u16 n;
 if (!s) return 0;
 for (n=0; n<max && s[n]; n++) ;
 return n;
}

static void writer_init(struct http_writer *w, u8 *data, u16 capacity)
{
 w->data=data; w->capacity=capacity; w->length=0; w->failed=0;
}

static void writer_bytes(struct http_writer *w, const char *s, u16 n)
{
 if (w->failed) return;
 if (!s || w->length>w->capacity || n>(u16)(w->capacity-w->length)) {
  w->failed=1; return;
 }
 if (n) memcpy(w->data+w->length,s,n);
 w->length+=n;
}

static void writer_cstr(struct http_writer *w, const char *s)
{
 u16 n=bounded_length(s,256);
 if (n==256) { w->failed=1; return; }
 writer_bytes(w,s,n);
}

static void writer_u64(struct http_writer *w, u64 value)
{
 char digits[20];
 u16 n=0;
 do {
  digits[n++]=(char)('0'+value%10);
  value/=10;
 } while (value && n<sizeof(digits));
 while (n) writer_bytes(w,&digits[--n],1);
}

static void writer_json_string(struct http_writer *w,const char *s,u16 max)
{
 u16 i,n;
 u8 c;
 static const char hex[]="0123456789abcdef";
 if (!s) { writer_cstr(w,"null"); return; }
 n=bounded_length(s,max);
 if (n==max) { w->failed=1; return; }
 writer_cstr(w,"\"");
 for (i=0;i<n;i++) {
  c=(u8)s[i];
  if (c=='"' || c=='\\') {
   char pair[2]; pair[0]='\\'; pair[1]=(char)c;
   writer_bytes(w,pair,2);
  } else if (c<0x20) {
   char esc[6]={'\\','u','0','0',hex[c>>4],hex[c&15]};
   writer_bytes(w,esc,6);
  } else {
   writer_bytes(w,&s[i],1);
  }
 }
 writer_cstr(w,"\"");
}

static void writer_html_string(struct http_writer *w,const char *s,u16 max)
{
 u16 i,n;
 if (!s) { w->failed=1; return; }
 n=bounded_length(s,max);
 if (n==max) { w->failed=1; return; }
 for (i=0;i<n;i++) {
  switch (s[i]) {
  case '&': writer_cstr(w,"&amp;"); break;
  case '<': writer_cstr(w,"&lt;"); break;
  case '>': writer_cstr(w,"&gt;"); break;
  case '"': writer_cstr(w,"&quot;"); break;
  case '\'': writer_cstr(w,"&#39;"); break;
  default: writer_bytes(w,&s[i],1); break;
  }
 }
}

static void writer_mac(struct http_writer *w,const u8 mac[6])
{
 static const char hex[]="0123456789abcdef";
 u16 i;
 for (i=0;i<6;i++) {
  char pair[2];
  pair[0]=hex[mac[i]>>4]; pair[1]=hex[mac[i]&15];
  writer_bytes(w,pair,2);
  if (i!=5) writer_cstr(w,":");
 }
}

static void writer_ip(struct http_writer *w,const u8 ip[4])
{
 u16 i;
 for (i=0;i<4;i++) {
  if (i) writer_cstr(w,".");
  writer_u64(w,ip[i]);
 }
}

static int string_equal(const char *a,const char *b,u16 n)
{
 return a && b && !memcmp(a,b,n) && a[n]=='\0';
}

static int method_token(u8 c)
{
 if ((c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z'))
  return 1;
 switch (c) {
 case '!': case '#': case '$': case '%': case '&': case '\'': case '*':
 case '+': case '-': case '.': case '^': case '_': case 0x60:
 case '|': case '~': return 1;
 default: return 0;
 }
}

/* Syntax validation never normalizes or decodes the request-target. */
static int uri_valid(const char *uri,u16 n)
{
 u16 i;
 u8 c;
 if (!uri || !n || uri[0]!='/') return 0;
 for (i=0;i<n;i++) {
  c=(u8)uri[i];
  if ((c>='0'&&c<='9')||(c>='A'&&c<='Z')||
      (c>='a'&&c<='z')||c=='-'||c=='.'||c=='_'||c=='~'||
      c=='/'||c=='?'||c=='&'||c=='='||c==':'||c=='@'||
      c=='+'||c==','||c==';'||c=='$'||c=='!'||c=='\''||
      c=='('||c==')'||c=='*'||c=='['||c==']')
   continue;
  if (c=='%') {
   if (i+2>=n) return 0;
   if (!((uri[i+1]>='0'&&uri[i+1]<='9')||
         (uri[i+1]>='A'&&uri[i+1]<='F')||
         (uri[i+1]>='a'&&uri[i+1]<='f'))||
       !((uri[i+2]>='0'&&uri[i+2]<='9')||
         (uri[i+2]>='A'&&uri[i+2]<='F')||
         (uri[i+2]>='a'&&uri[i+2]<='f')))
    return 0;
   i+=2; continue;
  }
  return 0;
 }
 return 1;
}

static int route_find(const char *uri,u16 n)
{
 u16 i;
 for (i=0;i<sizeof(routes)/sizeof(routes[0]);i++)
  if (n==routes[i].length && !memcmp(uri,routes[i].uri,n))
   return (int)i;
 return -1;
}

static void build_home(struct http_writer *w,const struct http_handler_info *info)
{
 writer_bytes(w,home_prefix,sizeof(home_prefix)-1);
 writer_html_string(w,info->board_name,HTTP_HANDLER_TEXT_MAX);
 writer_bytes(w,home_suffix,sizeof(home_suffix)-1);
}

static void build_status(struct http_writer *w,const struct http_handler_info *info)
{
 writer_cstr(w,"{\"state\":\"running\",\"uptime_ms\":");
 writer_u64(w,info->uptime_ms);
 writer_cstr(w,"}");
}

static void build_board(struct http_writer *w,const struct http_handler_info *info)
{
 writer_cstr(w,"{\"board\":");
 writer_json_string(w,info->board_name,HTTP_HANDLER_TEXT_MAX);
 writer_cstr(w,",\"board_type\":");
 writer_u64(w,info->board_type);
 writer_cstr(w,",\"cpu\":");
 writer_json_string(w,info->cpu_name,HTTP_HANDLER_TEXT_MAX);
 writer_cstr(w,",\"cpu_clock_hz\":");
 writer_u64(w,info->cpu_clock_hz);
 writer_cstr(w,"}");
}

static void build_memory(struct http_writer *w,const struct http_handler_info *info)
{
 writer_cstr(w,"{\"dram_start_bytes\":");
 writer_u64(w,info->memory_start);
 writer_cstr(w,",\"dram_total_bytes\":");
 writer_u64(w,info->memory_total_bytes);
 writer_cstr(w,",\"dram_total_mib\":");
 writer_u64(w,info->memory_total_bytes/1048576ULL);
 writer_cstr(w,"}");
}

static void build_network(struct http_writer *w,const struct http_handler_info *info)
{
 writer_cstr(w,"{\"interface\":");
 if (info->flags&HTTP_INFO_ETH_PRESENT)
  writer_json_string(w,info->interface_name,sizeof(info->interface_name));
 else writer_cstr(w,"null");
 writer_cstr(w,",\"device_state\":");
 if (!(info->flags&HTTP_INFO_ETH_PRESENT))
  writer_json_string(w,"unavailable",12);
 else if (info->eth_state==HTTP_INFO_ETH_ACTIVE)
  writer_json_string(w,"active",7);
 else if (info->eth_state==HTTP_INFO_ETH_PASSIVE)
  writer_json_string(w,"passive",8);
 else writer_json_string(w,"init",5);
 writer_cstr(w,",\"mac\":");
 if (info->flags&HTTP_INFO_MAC_VALID) {
  writer_cstr(w,"\"");
  writer_mac(w,info->mac);
  writer_cstr(w,"\"");
 } else writer_cstr(w,"null");
 writer_cstr(w,",\"ip\":");
 if (info->flags&HTTP_INFO_IP_VALID) {
  writer_cstr(w,"\"");
  writer_ip(w,info->ip);
  writer_cstr(w,"\"");
 } else writer_cstr(w,"null");
 writer_cstr(w,"}");
}

static const char *reason(int status)
{
 switch(status) {
 case HTTP_STATUS_200: return "OK";
 case HTTP_STATUS_400: return "Bad Request";
 case HTTP_STATUS_404: return "Not Found";
 case HTTP_STATUS_405: return "Method Not Allowed";
 default: return "Internal Server Error";
 }
}

static int header_bytes(char *h,u16 *n,const char *s,u16 count)
{
 if (count>(u16)(HTTP_HANDLER_BODY_OFFSET-*n)) return -1;
 memcpy(h+*n,s,count); *n+=count; return 0;
}

static int header_cstr(char *h,u16 *n,const char *s)
{
 u16 count=bounded_length(s,128);
 if (count==128) return -1;
 return header_bytes(h,n,s,count);
}

static int header_u64(char *h,u16 *n,u64 value)
{
 char digits[20];
 u16 count=0;
 do {
  digits[count++]=(char)('0'+value%10);
  value/=10;
 } while(value&&count<sizeof(digits));
 while(count)
  if(header_bytes(h,n,&digits[--count],1)) return -1;
 return 0;
}

static int build_response(const struct http_request *request,int status,
 const char *type,const u8 *body,u16 body_len,int head_only,
 u8 *out,u16 capacity,u16 *out_len)
{
 char header[HTTP_HANDLER_BODY_OFFSET];
 const char *version="HTTP/1.1";
 u16 n=0,total;
 if(request&&string_equal(request->version,"HTTP/1.0",8))
  version="HTTP/1.0";
 if(header_cstr(header,&n,version)||header_cstr(header,&n," ")||
    header_u64(header,&n,(u64)status)||header_cstr(header,&n," ")||
    header_cstr(header,&n,reason(status))||
    header_cstr(header,&n,"\r\nContent-Type: ")||
    header_cstr(header,&n,type)||
    header_cstr(header,&n,"\r\nContent-Length: ")||
    header_u64(header,&n,body_len)||
    header_cstr(header,&n,"\r\nConnection: close\r\n"))
  return -1;
 if(status==HTTP_STATUS_405&&header_cstr(header,&n,"Allow: GET, HEAD\r\n"))
  return -1;
 if(header_cstr(header,&n,"\r\n")) return -1;
 if(n>=HTTP_HANDLER_BODY_OFFSET||n>capacity||body_len>capacity-n)
  return -1;
 total=(u16)(n+(head_only?0:body_len));
 if(!head_only&&body_len) memmove(out+n,body,body_len);
 memcpy(out,header,n);
 *out_len=total;
 return 0;
}

int http_handler_build(const struct http_request *request,
 const struct http_handler_info *info,u8 *out,u16 capacity,u16 *out_len)
{
 struct http_writer body;
 const char *error_body=NULL;
 const char *type="text/plain; charset=utf-8";
 u16 error_len=0,method_len,uri_len;
 int status=HTTP_STATUS_200,head_only=0,route=-1;
 u16 i;

 if(!out_len) return -1;
 *out_len=0;
 if(!out||capacity<HTTP_HANDLER_BODY_OFFSET) return -1;
 out[0]=0;
 if(!request) {
  status=HTTP_STATUS_400; error_body=error_400;
  error_len=sizeof(error_400)-1; goto error_response;
 }
 method_len=bounded_length(request->method,sizeof(request->method));
 uri_len=bounded_length(request->uri,sizeof(request->uri));
 if(!method_len||method_len==sizeof(request->method)||
    !uri_len||uri_len==sizeof(request->uri)||
    (!string_equal(request->version,"HTTP/1.0",8)&&
     !string_equal(request->version,"HTTP/1.1",8))) {
  status=HTTP_STATUS_400; error_body=error_400;
  error_len=sizeof(error_400)-1; goto error_response;
 }
 for(i=0;i<method_len;i++)
  if(!method_token((u8)request->method[i])) {
   status=HTTP_STATUS_400; error_body=error_400;
   error_len=sizeof(error_400)-1; goto error_response;
  }
 if(!uri_valid(request->uri,uri_len)) {
  status=HTTP_STATUS_400; error_body=error_400;
  error_len=sizeof(error_400)-1; goto error_response;
 }
 head_only=string_equal(request->method,"HEAD",4);
 route=route_find(request->uri,uri_len);
 if(route<0) {
  status=HTTP_STATUS_404; error_body=error_404;
  error_len=sizeof(error_404)-1; goto error_response;
 }
 if(!string_equal(request->method,"GET",3)&&!head_only) {
  status=HTTP_STATUS_405; error_body=error_405;
  error_len=sizeof(error_405)-1; goto error_response;
 }
 if(!info) {
  status=HTTP_STATUS_500; error_body=error_500;
  error_len=sizeof(error_500)-1; goto error_response;
 }
 if((route==0||route==2)&&
    bounded_length(info->board_name,HTTP_HANDLER_TEXT_MAX)==0) {
  status=HTTP_STATUS_500; error_body=error_500;
  error_len=sizeof(error_500)-1; goto error_response;
 }
 if(route==3&&(!(info->flags&HTTP_INFO_MEMORY_VALID)||
                !info->memory_total_bytes)) {
  status=HTTP_STATUS_500; error_body=error_500;
  error_len=sizeof(error_500)-1; goto error_response;
 }

 type=routes[route].content_type;
 writer_init(&body,out+HTTP_HANDLER_BODY_OFFSET,
             (u16)(capacity-HTTP_HANDLER_BODY_OFFSET));
 switch(route) {
 case 0: build_home(&body,info); break;
 case 1: build_status(&body,info); break;
 case 2: build_board(&body,info); break;
 case 3: build_memory(&body,info); break;
 case 4: build_network(&body,info); break;
 default: body.failed=1; break;
 }
 /* A render/capacity failure is not a response: publish no partial bytes. */
 if(body.failed) return -1;
 if(build_response(request,status,type,out+HTTP_HANDLER_BODY_OFFSET,
                   body.length,head_only,out,capacity,out_len))
  return -1;
 return 0;

error_response:
 writer_init(&body,out+HTTP_HANDLER_BODY_OFFSET,
             (u16)(capacity-HTTP_HANDLER_BODY_OFFSET));
 writer_bytes(&body,error_body,error_len);
 if(body.failed) return -1;
 if(build_response(request,status,"text/plain; charset=utf-8",
                   out+HTTP_HANDLER_BODY_OFFSET,body.length,head_only,
                   out,capacity,out_len))
  return -1;
 return 0;
}
