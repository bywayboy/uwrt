#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <uci.h>
#include <uv.h>

#include "cfg.h"
#include "webserver.h"
#include "webrequest.h"

#include "utils.h"

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

static void webconn_after_sendmem(uv_write_t* w, int status) {
	automem_t mem;
	webconn_t * conn = container_of((uv_tcp_t*)w->handle, webconn_t, conn);
	automem_init_by_ptr(&mem, w->data, 100);
	automem_uninit(&mem);
	free(w);
}

int webconn_sendmem(webconn_t * conn, automem_t * mem) {
	uv_buf_t buf;
	uv_write_t* w;

	buf = uv_buf_init((char*)mem->pdata, (unsigned int)mem->size);
	w = (uv_write_t*)malloc(sizeof(uv_write_t));
	memset(w, 0, sizeof(uv_write_t));
	w->data = mem->pdata; //带上需要释的数据.

	return uv_write(w, (uv_stream_t *)&conn->conn, &buf, 1, webconn_after_sendmem);
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
	webrequest_t * req = (webrequest_t *)calloc(1, sizeof(webrequest_t));
	if (NULL != req) {
		req->_header_value_start = 0;
		req->mime = "text/html";
		automem_init(&req->buf, 256);
	}
	return req;
}
webrequest_t * webrequest_get(webrequest_t * r){r->ref++;return r;}
webrequest_t * webrequest_put(webrequest_t * r) {
	r->ref--;
	if (0 == r->ref) {
		if (NULL != r->file)free(r->file);
		automem_uninit(&r->buf);
		if (NULL != r->_url)free(r->_url);
		r = NULL;
	}
	return r;
}

static int webserver_onurl(http_parser* parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;
	if (NULL == conn->request)
		request = conn->request = webrequest_get(webrequest_create(conn));
	if (NULL == request && !uv_is_closing((uv_handle_t *)&conn->conn)) {
		uv_close((uv_handle_t *)&conn->conn, webconn_onclose);
		return 0;
	}
	if (length > 0)
		automem_append_voidp(&request->buf, at, length);

	if (parser->state == 32)
	{
		request->_url = memdup(request->buf.pdata, request->buf.size);
		request->params = strchr(request->_url, '?');
		if (NULL == request->params)
			request->params = strchr(request->_url, '#');
		request->nurl = request->buf.size; 
		if (request->params) {
			request->nparams = request->nurl - (request->params - request->_url);
			request->nurl = request->buf.size - request->nparams;
		}
		
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
			if (0 == strcasecmp(request->buf.pdata, "IF-MODIFIED-SINCE")) {
				char * ifv = (char *)request->buf.pdata + request->_header_value_start, n = 0;
				struct tm tm;
				int tot = request->buf.size - request->_header_value_start;
				// If-Modified-Since:  Tue, 21 Jul 2015 08:43:47 GMT
				while (tot--) {

					if (ifv[tot] == ' ')n++;
				}
				strptime(ifv, n == 4 ? "%a, %d %b %Y %H:%M:%S %Z" : "%a, %d %b %Y %H:%M:%S %z", &tm);
				tm.tm_isdst = 0;
				request->st_mtime = mkgmtime(&tm);
			}
			break;
		}
		request->_header_value_start = 0;
		request->buf.size = 0;
	}
	return 0;
}

/*
	对URL 进行分析.
*/
char * analize_url(webrequest_t * request, uint32_t * code);
static void webserver_go(webconn_t * conn, webrequest_t * request) {
	static const char * Last_Modified = "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n";
	uint32_t code;
	analize_url(request, &code);
	if (request->is_cgi) {
		webrequest_push(request, conn);
	}
	else {
		automem_t mem;
		automem_init(&mem, 512);
		automem_init_headers(&mem, code);
		switch (code) {
		case 404:
			automem_append_contents(&mem, "404 Not Found.",sizeof("404 Not Found.")-1);
			break;
		case 304:
			automem_append_header_date(&mem, Last_Modified, request->st_mtime);
			break;
		case 200:
			automem_append_mime(&mem, request->mime);
			automem_append_content_length(&mem, request->st_size);
			automem_append_header_date(&mem, Last_Modified, request->st_mtime);
			break;
		}
		automem_append_voidp(&mem, "\r\n", 2);
		webconn_sendmem(conn, &mem);
		if (200 == code) {
			webcon_writefile(conn, request->file,request->st_size);
		}
	}
	webrequest_put(request);
}

static int webserver_on_header_complete(http_parser* parser)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;
	if ((ULLONG_MAX == parser->content_length || 0 == parser->content_length) && request->_url) {
		if (parser->upgrade) {
			//TODO: WebSocket.

			return 0;
		}
		conn->request = NULL;
		webserver_go(conn,request);
	}
	return 0;
}
static int webserver_on_body(http_parser * parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;

	conn->request = NULL;
	webserver_go(conn,request);
	return 0;
}


static int webserver_on_status(http_parser * parser, const char *at, size_t length)
{
	webconn_t * conn = container_of(parser, webconn_t, parser);
	webrequest_t * request = conn->request;

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