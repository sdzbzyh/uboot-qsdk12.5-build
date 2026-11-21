#include <stdio.h>
#include <string.h>
#include <net/http_request.h>
static int checks,failures;
#define C(x) do{checks++;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);failures++;}}while(0)
static enum http_req_status feed(struct http_request_parser*p,const char*s){return http_request_parser_feed(p,(const u8*)s,(u16)strlen(s));}
static void bad(const char*n,const char*s,enum http_req_error e){struct http_request_parser p;enum http_req_status z;http_request_parser_init(&p);z=feed(&p,s);C(z==HTTP_REQ_ERROR);C(p.error==e);printf("%s: %s (%s)\n",n,z==HTTP_REQ_ERROR&&p.error==e?"PASS":"FAIL",http_request_error_string(p.error));}
static void good(const char*n,const char*s,const char*m,const char*u,u8 h){struct http_request_parser p;enum http_req_status z;http_request_parser_init(&p);z=feed(&p,s);C(z==HTTP_REQ_COMPLETE);C(p.request.header_end==strlen(s));C(!strcmp(p.request.method,m));C(!strcmp(p.request.uri,u));C(p.request.header_count==h);printf("%s: %s\n",n,z==HTTP_REQ_COMPLETE?"PASS":"FAIL");}
static void split(const char*n,const char*s,const u16*c,unsigned int nc,u8 expected_headers){struct http_request_parser p;unsigned int i;u16 off=0,total=(u16)strlen(s);enum http_req_status z=HTTP_REQ_NEED_MORE;http_request_parser_init(&p);for(i=0;i<nc;i++){z=http_request_parser_feed(&p,(const u8*)s+off,c[i]);off+=c[i];C(z==HTTP_REQ_NEED_MORE);}z=http_request_parser_feed(&p,(const u8*)s+off,total-off);C(z==HTTP_REQ_COMPLETE);C(!strcmp(p.request.uri,"/fragmented"));C(p.request.header_count==expected_headers);printf("%s: %s\n",n,z==HTTP_REQ_COMPLETE?"PASS":"FAIL");}
int main(void){
 const char*g="GET / HTTP/1.1\r\nHost: example\r\n\r\n",*h="HEAD /status HTTP/1.0\r\n\r\n",*f="GET /fragmented HTTP/1.1\r\n\r\n";
 struct http_request_parser p;enum http_req_status z;u8 over[HTTP_REQ_RX_CAPACITY+1];char b[2400];unsigned int i;
 const u16 a[]={4,5,6,2,3},v[]={8,2,2,2},hd[]={20,5,4},cr[]={25,1};
 good("01 complete GET",g,"GET","/",1);good("02 complete HEAD",h,"HEAD","/status",0);
 split("03 request-line split",f,a,5,0);split("04 URI split",f,v,4,0);
 split("05 header split","GET /fragmented HTTP/1.1\r\nX-A: b\r\n\r\n",hd,3,1);
 split("06 CRLF split",f,cr,2,0);
 good("07 multiple headers","GET / HTTP/1.1\r\nHost: x\r\nAccept: */*\r\nX-Test: yes\r\n\r\n","GET","/",3);
 good("08 header case insensitive","GET / HTTP/1.1\r\ncOnTeNt-LeNgTh: 0\r\nhOsT: x\r\n\r\n","GET","/",2);
 memset(b,'A',sizeof(b));http_request_parser_init(&p);z=http_request_parser_feed(&p,(u8*)b,HTTP_REQ_LINE_MAX+2);C(z==HTTP_REQ_ERROR&&p.error==HTTP_REQ_ERR_REQUEST_LINE_LIMIT);printf("09 request line overlong: %s\n",p.error==HTTP_REQ_ERR_REQUEST_LINE_LIMIT?"PASS":"FAIL");
 memset(b,0,sizeof(b));memcpy(b,"GET /",5);memset(b+5,'a',HTTP_REQ_URI_MAX+1);memcpy(b+5+HTTP_REQ_URI_MAX+1," HTTP/1.1\r\n\r\n",13);http_request_parser_init(&p);z=http_request_parser_feed(&p,(u8*)b,(u16)(5+HTTP_REQ_URI_MAX+1+13));C(z==HTTP_REQ_ERROR&&p.error==HTTP_REQ_ERR_BAD_URI);printf("10 URI overlong: %s\n",p.error==HTTP_REQ_ERR_BAD_URI?"PASS":"FAIL");
 {char x[1024]="GET / HTTP/1.1\r\n";for(i=0;i<17;i++)strcat(x,"X: y\r\n");strcat(x,"\r\n");bad("11 header count limit",x,HTTP_REQ_ERR_HEADER_COUNT_LIMIT);}
 {char x[400]="GET / HTTP/1.1\r\nX:";size_t n=strlen(x);memset(x+n,'a',HTTP_REQ_HEADER_LINE_MAX);x[n+HTTP_REQ_HEADER_LINE_MAX]=0;bad("12 header line limit",x,HTTP_REQ_ERR_HEADER_LINE_LIMIT);}
 {char x[2400]="GET / HTTP/1.1\r\n",line[241];memset(line,'a',240);line[240]=0;for(i=0;i<9;i++){strcat(x,"X:");strcat(x,line);strcat(x,"\r\n");}bad("13 header total limit",x,HTTP_REQ_ERR_HEADER_TOTAL_LIMIT);}
 http_request_parser_init(&p);memset(over,'A',sizeof(over));z=http_request_parser_feed(&p,over,sizeof(over));C(z==HTTP_REQ_ERROR&&p.error==HTTP_REQ_ERR_RX_LIMIT);printf("14 RX overflow: %s\n",p.error==HTTP_REQ_ERR_RX_LIMIT?"PASS":"FAIL");
 {const u8 nul[]={'G','E','T',' ','/',' ','H','T','T','P','/','1','.','1','\r','\n',0,'\r','\n'};http_request_parser_init(&p);z=http_request_parser_feed(&p,nul,sizeof(nul));C(z==HTTP_REQ_ERROR&&p.error==HTTP_REQ_ERR_BAD_HEADER);printf("15 NUL byte: %s\n",z==HTTP_REQ_ERROR&&p.error==HTTP_REQ_ERR_BAD_HEADER?"PASS":"FAIL");}
 bad("33 CR followed by non-LF","GET / HTTP/1.1\rX\n\r\n",HTTP_REQ_ERR_BAD_CRLF);
 bad("34 malformed percent escape","GET /bad%2 HTTP/1.1\r\n\r\n",HTTP_REQ_ERR_BAD_URI);
 bad("35 backslash in URI","GET /bad\\path HTTP/1.1\r\n\r\n",HTTP_REQ_ERR_BAD_URI);
 bad("16 bare LF","GET / HTTP/1.1\n\n",HTTP_REQ_ERR_BAD_CRLF);
 bad("17 malformed request line","GET  / HTTP/1.1\r\n\r\n",HTTP_REQ_ERR_BAD_REQUEST_LINE);
 bad("18 malformed header","GET / HTTP/1.1\r\nBadHeader\r\n\r\n",HTTP_REQ_ERR_BAD_HEADER);
 bad("19 invalid Content-Length","GET / HTTP/1.1\r\nContent-Length: +1\r\n\r\n",HTTP_REQ_ERR_BAD_CONTENT_LENGTH);
 bad("20 duplicate Content-Length","GET / HTTP/1.1\r\nContent-Length: 0\r\nContent-Length: 0\r\n\r\n",HTTP_REQ_ERR_DUPLICATE_CONTENT_LENGTH);
 bad("21 conflicting duplicate Content-Length","GET / HTTP/1.1\r\nContent-Length: 0\r\nContent-Length: 1\r\n\r\n",HTTP_REQ_ERR_DUPLICATE_CONTENT_LENGTH);
 bad("22 CL plus TE","GET / HTTP/1.1\r\nContent-Length: 0\r\nTransfer-Encoding: chunked\r\n\r\n",HTTP_REQ_ERR_CONTENT_LENGTH_TRANSFER_ENCODING);
 bad("23 TE then CL conflict","GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nContent-Length: 0\r\n\r\n",HTTP_REQ_ERR_CONTENT_LENGTH_TRANSFER_ENCODING);
 bad("24 unsupported TE","GET / HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n",HTTP_REQ_ERR_TRANSFER_ENCODING_NOT_SUPPORTED);
 bad("25 CL decimal overflow","GET / HTTP/1.1\r\nContent-Length: 4294967296\r\n\r\n",HTTP_REQ_ERR_BAD_CONTENT_LENGTH);
 bad("26 control in header value","GET / HTTP/1.1\r\nX: a\001b\r\n\r\n",HTTP_REQ_ERR_BAD_HEADER_VALUE);
 bad("27 nonzero body","GET / HTTP/1.1\r\nContent-Length: 1\r\n\r\n",HTTP_REQ_ERR_BODY_NOT_SUPPORTED);
 http_request_parser_init(&p);z=feed(&p,"GET / HTTP/1.1\r\nHost:");C(z==HTTP_REQ_NEED_MORE);printf("28 incomplete NEED_MORE: %s\n",z==HTTP_REQ_NEED_MORE?"PASS":"FAIL");
 bad("29 trailing body","GET / HTTP/1.1\r\n\r\nx",HTTP_REQ_ERR_BODY_TOO_LARGE);
 http_request_parser_init(&p);z=feed(&p,"GET / HTTP/1.1\r\n");C(z==HTTP_REQ_NEED_MORE);http_request_parser_reset(&p);z=feed(&p,h);C(z==HTTP_REQ_COMPLETE&&!strcmp(p.request.method,"HEAD"));printf("30 reset and new request: %s\n",z==HTTP_REQ_COMPLETE?"PASS":"FAIL");
 http_request_parser_init(&p);z=feed(&p,g);C(z==HTTP_REQ_COMPLETE);z=feed(&p,"");C(z==HTTP_REQ_COMPLETE);printf("31 repeated COMPLETE: %s\n",z==HTTP_REQ_COMPLETE?"PASS":"FAIL");
 http_request_parser_init(&p);z=feed(&p,"GET / HTTP/1.1\r\nHost: x\r\n");C(z==HTTP_REQ_NEED_MORE);z=feed(&p,"\r\n");C(z==HTTP_REQ_COMPLETE&&p.request.header_count==1);printf("32 terminator split: %s\n",z==HTTP_REQ_COMPLETE?"PASS":"FAIL");
 printf("PARSER_STRUCT_BYTES=%lu\n",(unsigned long)sizeof(struct http_request_parser));
 printf("CHECKS=%d FAILURES=%d\n",checks,failures);return failures?1:0;
}