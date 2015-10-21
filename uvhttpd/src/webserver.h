#ifndef _UVHTTPD_WEBSERVER_H
#define _UVHTTPD_WEBSERVER_H
#include "http_parser.h"
#include "automem.h"

typedef struct webserver webserver_t;
typedef struct webconn   webconn_t;

enum {
	WEB_PROTO_HTTP,
	WEB_PROTO_WEBSOCKET,
};
typedef struct webrequest webrequest_t;
struct webrequest {
	int _header_value_start, ref; // private
	time_t fstamp;
	automem_t buf;
	char * _url;
	uint32_t nurl;
	char * body; // body 如果有值，则指向 buf.
//	wsFrame * frame; // WebSocket 协议有效! 数据帧

};
struct webconn{
	uv_tcp_t conn;
	webserver_t * server;
	uv_loop_t * loop;
	int ref;
	enum webServerProto proto;
	http_parser parser;
	uv_shutdown_t shutdown;
	webrequest_t * request;
};

struct webserver {
	uv_tcp_t s;
	uv_loop_t * loop;
	http_parser_settings parser_settings;
	uv_timer_t timer;
};

webconn_t * webconn_get(webconn_t * c);
webconn_t * webconn_put(webconn_t * c);

int webserver_init(uv_loop_t * loop, webserver_t * server);
void webserver_uninit(webserver_t * server);

#endif