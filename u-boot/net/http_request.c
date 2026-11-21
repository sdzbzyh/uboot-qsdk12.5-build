/*
 * PA-220 bounded, single-request HTTP/1.x request-head parser.
 * No allocation, I/O, URI dispatch, body buffering, or TCP dependency.
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <net/http_request.h>
static int http_is_ctl(u8 c) { return c < 0x20 || c == 0x7f; }
static int http_is_token(u8 c)
{
 if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return 1;
 switch (c) {
 case '!': case '#': case '$': case '%': case '&': case '\'': case '*':
 case '+': case '-': case '.': case '^': case '_': case '`': case '|': case '~': return 1;
 default: return 0;
 }
}
static int http_equal_ci(const char *a, const char *b)
{
 while (*a && *b) {
  char x=*a++, y=*b++;
  if (x>='A' && x<='Z') x=(char)(x+('a'-'A'));
  if (y>='A' && y<='Z') y=(char)(y+('a'-'A'));
  if (x!=y) return 0;
 }
 return *a=='\0' && *b=='\0';
}
static enum http_req_status http_fail(struct http_request_parser *p, enum http_req_error e)
{ p->error=e; return HTTP_REQ_ERROR; }
void http_request_parser_init(struct http_request_parser *p) { if (p) memset(p,0,sizeof(*p)); }
void http_request_parser_reset(struct http_request_parser *p) { http_request_parser_init(p); }
static int http_find_crlf(const u8 *b,u16 start,u16 end,u16 *line_end,enum http_req_error *e)
{
 u16 i;
 for(i=start;i<end;i++) {
  if(b[i]=='\n') { if(i==start || b[i-1]!='\r') {*e=HTTP_REQ_ERR_BAD_CRLF;return -1;} *line_end=i-1;return 1; }
  if(b[i]=='\r' && i+1<end && b[i+1]!='\n') {*e=HTTP_REQ_ERR_BAD_CRLF;return -1;}
 }
 return 0;
}
static int http_parse_request_line(struct http_request_parser *p,u16 end)
{
 u16 i,sp1=0,sp2=0,ml,ul,vl; struct http_request *r=&p->request;
 if(end>HTTP_REQ_LINE_MAX) return -HTTP_REQ_ERR_REQUEST_LINE_LIMIT;
 if(!end) return -HTTP_REQ_ERR_BAD_REQUEST_LINE;
 for(i=0;i<end;i++) {
  u8 ch=p->rx[i];
  if(ch==' ') { if(!sp1) sp1=i; else if(!sp2) sp2=i; else return -HTTP_REQ_ERR_BAD_REQUEST_LINE; }
  else if(http_is_ctl(ch)) return -HTTP_REQ_ERR_BAD_REQUEST_LINE;
 }
 if(!sp1 || !sp2 || sp2<=sp1+1 || sp2==end-1) return -HTTP_REQ_ERR_BAD_REQUEST_LINE;
 ml=sp1; ul=sp2-sp1-1; vl=end-sp2-1;
 if(!ml) return -HTTP_REQ_ERR_BAD_METHOD;
 if(ul>HTTP_REQ_URI_MAX) return -HTTP_REQ_ERR_BAD_URI;
 if(vl!=8) return -HTTP_REQ_ERR_BAD_VERSION;
 for(i=0;i<ml;i++) if(!http_is_token(p->rx[i])) return -HTTP_REQ_ERR_BAD_METHOD;
 for(i=0;i<ul;i++) {u8 ch=p->rx[sp1+1+i]; if(ch<0x21 || ch>0x7e || ch=='#') return -HTTP_REQ_ERR_BAD_URI;}
 if(memcmp(p->rx+sp2+1,"HTTP/1.0",8) && memcmp(p->rx+sp2+1,"HTTP/1.1",8)) return -HTTP_REQ_ERR_BAD_VERSION;
 memcpy(r->method,p->rx,ml); r->method[ml]='\0';
 memcpy(r->uri,p->rx+sp1+1,ul); r->uri[ul]='\0';
 memcpy(r->version,p->rx+sp2+1,8); r->version[8]='\0'; r->request_line_length=end;
 return 0;
}
static int http_parse_content_length(struct http_request_parser *p,const char *v)
{
 u32 n=0; u16 i=0;
 if(p->request.content_length_present) return -HTTP_REQ_ERR_DUPLICATE_CONTENT_LENGTH;
 if(!v[0]) return -HTTP_REQ_ERR_BAD_CONTENT_LENGTH;
 while(v[i]) {
  u32 d;
  if(v[i]<'0' || v[i]>'9') return -HTTP_REQ_ERR_BAD_CONTENT_LENGTH;
  d=(u32)(v[i]-'0');
  if(n>(0xffffffffU-d)/10U) return -HTTP_REQ_ERR_BAD_CONTENT_LENGTH;
  n=n*10U+d; i++;
 }
 p->request.content_length_present=1; p->request.content_length=n;
 return 0;
}
static int http_parse_header(struct http_request_parser *p,u16 start,u16 end)
{
 struct http_request *r=&p->request; struct http_req_header *h;
 u16 colon=start,i,vs,ve; int rc;
 if(end<=start) return 0;
 if(end-start+2>HTTP_REQ_HEADER_LINE_MAX) return -HTTP_REQ_ERR_HEADER_LINE_LIMIT;
 if(p->rx[start]==' ' || p->rx[start]=='\t') return -HTTP_REQ_ERR_BAD_HEADER;
 while(colon<end && p->rx[colon]!=':') colon++;
 if(colon==start || colon==end) return -HTTP_REQ_ERR_BAD_HEADER;
 for(i=start;i<colon;i++) if(!http_is_token(p->rx[i])) return -HTTP_REQ_ERR_BAD_HEADER_NAME;
 vs=colon+1;
 while(vs<end && (p->rx[vs]==' ' || p->rx[vs]=='\t')) vs++;
 ve=end;
 while(ve>vs && (p->rx[ve-1]==' ' || p->rx[ve-1]=='\t')) ve--;
 for(i=vs;i<ve;i++) if(http_is_ctl(p->rx[i]) && p->rx[i]!='\t') return -HTTP_REQ_ERR_BAD_HEADER_VALUE;
 if(r->header_count>=HTTP_REQ_HEADER_COUNT_MAX) return -HTTP_REQ_ERR_HEADER_COUNT_LIMIT;
 h=&r->headers[r->header_count];
 memcpy(h->name,p->rx+start,colon-start); h->name[colon-start]='\0';
 memcpy(h->value,p->rx+vs,ve-vs); h->value[ve-vs]='\0'; r->header_count++;
 if(http_equal_ci(h->name,"Content-Length")) return http_parse_content_length(p,h->value);
 if(http_equal_ci(h->name,"Transfer-Encoding")) { r->transfer_encoding_present=1; return 0; }
 rc=0; return rc;
}
enum http_req_status http_request_parser_feed(struct http_request_parser *p,const u8 *data,u16 length)
{
 u16 old,end,line_end,pos,header_start; enum http_req_error scan=HTTP_REQ_ERR_NONE; int found,rc;
 if(!p) return HTTP_REQ_ERROR;
 if(!data && length) return http_fail(p,HTTP_REQ_ERR_ARGUMENT);
 if(p->error!=HTTP_REQ_ERR_NONE) return HTTP_REQ_ERROR;
 if(p->request.header_end) return length?http_fail(p,HTTP_REQ_ERR_TRAILING_DATA):HTTP_REQ_COMPLETE;
 old=p->rx_len;
 if(length>HTTP_REQ_RX_CAPACITY-old) return http_fail(p,HTTP_REQ_ERR_RX_LIMIT);
 if(length) memcpy(p->rx+old,data,length);
 p->rx_len=old+length; end=p->rx_len;
 found=http_find_crlf(p->rx,0,end,&line_end,&scan);
 if(found<0) return http_fail(p,scan);
 if(!found) {if(end>HTTP_REQ_LINE_MAX+1) return http_fail(p,HTTP_REQ_ERR_REQUEST_LINE_LIMIT);return HTTP_REQ_NEED_MORE;}
 rc=http_parse_request_line(p,line_end); if(rc) return http_fail(p,(enum http_req_error)-rc);
 header_start=line_end+2; pos=header_start;
 while(pos<end) {
  found=http_find_crlf(p->rx,pos,end,&line_end,&scan);
  if(found<0) return http_fail(p,scan);
  if(!found) {
   if(end-header_start>HTTP_REQ_HEADER_TOTAL_MAX) return http_fail(p,HTTP_REQ_ERR_HEADER_TOTAL_LIMIT);
   if(end-pos>HTTP_REQ_HEADER_LINE_MAX+1) return http_fail(p,HTTP_REQ_ERR_HEADER_LINE_LIMIT);
   return HTTP_REQ_NEED_MORE;
  }
  if(line_end==pos) {
   u16 h_end=line_end+2;
   if(h_end-header_start>HTTP_REQ_HEADER_TOTAL_MAX) return http_fail(p,HTTP_REQ_ERR_HEADER_TOTAL_LIMIT);
   p->request.header_bytes=h_end-header_start; p->request.header_end=h_end;
   if(p->request.content_length_present && p->request.transfer_encoding_present)
    return http_fail(p,HTTP_REQ_ERR_CONTENT_LENGTH_TRANSFER_ENCODING);
   if(p->request.transfer_encoding_present)
    return http_fail(p,HTTP_REQ_ERR_TRANSFER_ENCODING_NOT_SUPPORTED);
   if(p->request.content_length>HTTP_REQ_BODY_MAX)
    return http_fail(p,HTTP_REQ_ERR_BODY_NOT_SUPPORTED);
   if(end!=h_end) return http_fail(p,HTTP_REQ_ERR_BODY_TOO_LARGE);
   return HTTP_REQ_COMPLETE;
  }
  rc=http_parse_header(p,pos,line_end); if(rc) return http_fail(p,(enum http_req_error)-rc);
  pos=line_end+2;
  if(pos-header_start>HTTP_REQ_HEADER_TOTAL_MAX) return http_fail(p,HTTP_REQ_ERR_HEADER_TOTAL_LIMIT);
 }
 if(end-header_start>HTTP_REQ_HEADER_TOTAL_MAX) return http_fail(p,HTTP_REQ_ERR_HEADER_TOTAL_LIMIT);
 return HTTP_REQ_NEED_MORE;
}
const char *http_request_error_string(enum http_req_error e)
{
 static const char * const names[]={"none","argument","rx-limit","request-line-limit","header-total-limit","header-line-limit","header-count-limit","bad-crlf","bad-request-line","bad-method","bad-uri","bad-version","bad-header","bad-header-name","bad-header-value","duplicate-content-length","bad-content-length","body-not-supported","content-length-transfer-encoding","transfer-encoding-not-supported","body-too-large","trailing-data"};
 if((unsigned int)e>=sizeof(names)/sizeof(names[0])) return "unknown";
 return names[e];
}
