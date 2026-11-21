/*
 * PA-220 bounded HTTP/1.x request-head parser.
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __PA220_HTTP_REQUEST_H__
#define __PA220_HTTP_REQUEST_H__
#include <linux/types.h>
#define HTTP_REQ_RX_CAPACITY       4096
#define HTTP_REQ_TX_CAPACITY       1460
#define HTTP_REQ_LINE_MAX          512
#define HTTP_REQ_URI_MAX           384
#define HTTP_REQ_HEADER_COUNT_MAX  16
#define HTTP_REQ_HEADER_LINE_MAX   256
#define HTTP_REQ_HEADER_TOTAL_MAX  2048
#define HTTP_REQ_BODY_MAX          0
enum http_req_status { HTTP_REQ_NEED_MORE = 0, HTTP_REQ_COMPLETE = 1, HTTP_REQ_ERROR = -1 };
enum http_req_error {
 HTTP_REQ_ERR_NONE = 0, HTTP_REQ_ERR_ARGUMENT, HTTP_REQ_ERR_RX_LIMIT,
 HTTP_REQ_ERR_REQUEST_LINE_LIMIT, HTTP_REQ_ERR_HEADER_TOTAL_LIMIT,
 HTTP_REQ_ERR_HEADER_LINE_LIMIT, HTTP_REQ_ERR_HEADER_COUNT_LIMIT,
 HTTP_REQ_ERR_BAD_CRLF, HTTP_REQ_ERR_BAD_REQUEST_LINE, HTTP_REQ_ERR_BAD_METHOD,
 HTTP_REQ_ERR_BAD_URI, HTTP_REQ_ERR_BAD_VERSION, HTTP_REQ_ERR_BAD_HEADER,
 HTTP_REQ_ERR_BAD_HEADER_NAME, HTTP_REQ_ERR_BAD_HEADER_VALUE,
 HTTP_REQ_ERR_DUPLICATE_CONTENT_LENGTH, HTTP_REQ_ERR_BAD_CONTENT_LENGTH,
 HTTP_REQ_ERR_BODY_NOT_SUPPORTED, HTTP_REQ_ERR_CONTENT_LENGTH_TRANSFER_ENCODING,
 HTTP_REQ_ERR_TRANSFER_ENCODING_NOT_SUPPORTED, HTTP_REQ_ERR_BODY_TOO_LARGE,
 HTTP_REQ_ERR_TRAILING_DATA
};
struct http_req_header { char name[HTTP_REQ_HEADER_LINE_MAX]; char value[HTTP_REQ_HEADER_LINE_MAX]; };
struct http_request {
 char method[HTTP_REQ_LINE_MAX];
 char uri[HTTP_REQ_URI_MAX + 1];
 char version[9];
 struct http_req_header headers[HTTP_REQ_HEADER_COUNT_MAX];
 u8 header_count;
 u8 content_length_present;
 u8 transfer_encoding_present;
 u32 content_length;
 u16 request_line_length;
 u16 header_bytes;
 u16 header_end;
};
struct http_request_parser {
 u8 rx[HTTP_REQ_RX_CAPACITY];
 u16 rx_len;
 enum http_req_error error;
 struct http_request request;
};
void http_request_parser_init(struct http_request_parser *parser);
void http_request_parser_reset(struct http_request_parser *parser);
enum http_req_status http_request_parser_feed(struct http_request_parser *parser,
 const u8 *data, u16 length);
const char *http_request_error_string(enum http_req_error error);
#endif
