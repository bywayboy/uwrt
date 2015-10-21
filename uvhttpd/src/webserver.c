#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <uci.h>
#include <uv.h>

#include "cfg.h"
#include "webserver.h"

void * memdup(void * src, uint32_t sz) {
	void * dest = calloc(1, sz +1);
	memcpy(dest, src, sz);
	return dest;
}
void uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	suggested_size = 20480;
	buf->base = (char *)malloc(suggested_size);
	buf->len = suggested_size;
}
webconn_t * webconn_create(webserver_t * server) {
	webconn_t * cli = (webconn_t *)calloc(1, sizeof(webconn_t));
	cli->server = server;
	cli->ref = 0;
	cli->proto = WEB_PROTO_HTTP;
	cli->loop = server->loop;
	http_parser_init(&cli->parser, HTTP_REQUEST);
	uv_tcp_init(server->loop, &cli->conn);
	return cli;
}

webconn_t * webconn_get(webconn_t * c) { c->ref++; return c; }
webconn_t * webconn_put(webconn_t * c) {
	c->ref--;
	if (0 == c->ref) {
		//TODO: free
		return NULL;
	}
	return c;
}

static void webconn_onclose(uv_handle_t * handle) {
	webconn_t  * conn = container_of(handle, webconn_t, conn);
}
static void webconn_onshutdown(uv_shutdown_t * req, int status) {
	if (!uv_is_closing((uv_handle_t *)req->handle))
		uv_close((uv_handle_t *)req->handle, webconn_onclose);
}

static void webconn_onread(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	webconn_t  * conn = container_of((uv_tcp_t *)stream, webconn_t, conn);
	if (0 < nread) {
		if (conn->proto == WEB_PROTO_HTTP) {
			size_t  nparsed;
			nparsed = http_parser_execute(&conn->parser, &conn->server->parser_settings, buf->base, nread);
			if (conn->parser.upgrade)
			{
				// do nothing, Portocol switch.
			}
			else if (nparsed != nread) {

				/* Handle error. Usually just close the connection. */
			}
		}
		else if (conn->proto == WEB_PROTO_WEBSOCKET) {
			//wsparser_pushdata(conn->ws_parser, buf->base, nread, webconn_wshandler, (void *)conn);
		}
	}
	else if (0 > nread)
	{
		uv_read_stop(stream);
		switch (nread) {
		case UV_ECONNRESET:
		case UV_EOF:
			uv_close((uv_handle_t *)stream, webconn_onclose);
			break;
		default:
			uv_shutdown(&conn->shutdown, (uv_stream_t *)stream, webconn_onshutdown);
			break;
		}
	}
	if (NULL != buf->base) free(buf->base);
}
static void webserver_ontimer(uv_timer_t * timer) {
	
}

static void webserver_onconnection(uv_stream_t* stream, int status)
{
	webserver_t * server = container_of(stream, webserver_t, s);
	const config_t * cfg = config_get();
	if (0 == status) {
		int r;
		webconn_t * cli = webconn_create(server);
		if (0 == (r = uv_accept(stream, (uv_stream_t *)&cli->conn)))
		{
			if (0 == uv_read_start((uv_stream_t *)&cli->conn, uv_alloc, webconn_onread)) {

				return;
			}
		}
		uv_close((uv_handle_t *)&cli->conn, webconn_onclose);
	}
}
static webrequest_t * webrequest_create(webconn_t * conn) {
	webrequest_t * req = (webrequest_t *)malloc(sizeof(webrequest_t));
	if (NULL != req) {
		req->_header_value_start = 0;
		req->body = NULL;
		req->fstamp = 0;
		req->ref = 0;
		automem_init(&req->buf, 256);
	}
	return req;
}

static int webserver_onurl(http_parser* parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;
	if (NULL == conn->request)
		request = conn->request = webrequest_create(conn);
	if (NULL == request && !uv_is_closing((uv_handle_t *)&conn->conn)) {
		uv_close((uv_handle_t *)&conn->conn, webconn_onclose);
		return 0;
	}
	if (length > 0)
		automem_append_voidp(&request->buf, at, length);

	if (parser->state == 32)
	{
		request->_url = memdup(request->buf.pdata, request->buf.size);
		request->nurl = request->buf.size;
		request->buf.size = 0;
	}
	return 0;
}
static int webserver_on_header_field(http_parser* parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;

	if (length > 0)
		automem_append_voidp(&request->buf, at, length);
	if (parser->state == 44) {
		automem_append_byte(&request->buf, '\0');
		request->_header_value_start = request->buf.size;
	}
	return 0;
}
static int webserver_on_header_value(http_parser* parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;

	if (length > 0)
		automem_append_voidp(&request->buf, at, length);

	if (parser->state == 50) {
		switch (request->buf.pdata[0]) {
		case 'I':
		case 'i':

			break;
		}
		request->_header_value_start = 0;
		request->buf.size = 0;
	}
	return 0;
}
static int webserver_on_header_complete(http_parser* parser)
{
	return 0;
}
static int webserver_on_body(http_parser * parser, const char *at, size_t length)
{
	return 0;
}

static int webserver_on_status(http_parser * parser, const char *at, size_t length)
{
	return 0;
}
int webserver_init(uv_loop_t * loop, webserver_t * server)
{
	int ret;
	struct sockaddr addr;
	const config_t * cfg = config_get();
	server->loop = loop;
	if (0 == (ret = uv_tcp_init(server->loop, &server->s)))
	{
		if (0 == (ret = uv_ip4_addr(cfg->bind, cfg->port, (struct sockaddr_in *)&addr)))
		{
			if (0 == (ret = uv_tcp_bind(&server->s, &addr, 0)))
			{
				uv_timer_init(server->loop, &server->timer);
				uv_timer_start(&server->timer, webserver_ontimer, 2000, 2000);

				server->parser_settings.on_url = webserver_onurl;
				server->parser_settings.on_header_field = webserver_on_header_field;
				server->parser_settings.on_header_value = webserver_on_header_value;
				server->parser_settings.on_headers_complete = webserver_on_header_complete;
				server->parser_settings.on_body = webserver_on_body;
				server->parser_settings.on_status = webserver_on_status;
				if (0 == (ret = uv_listen((uv_stream_t *)&server->s, 2000, webserver_onconnection))) {
					return 0;
				}
			}
		}
		uv_close((uv_handle_t *)&server->s, NULL);
	}
	return ret;
}
void webserver_uninit(webserver_t * server)
{
	
}