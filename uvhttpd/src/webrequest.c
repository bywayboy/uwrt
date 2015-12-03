#include <stdio.h>
#include <stdint.h>

#include <uv.h>

#include "webserver.h"
#include "webrequest.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


static void webrequest_threadproc(uv_work_t * req)
{
	webrequest_t * request = (webrequest_t *)req->data;
	lua_State * L;
}
static void after_queue_work(uv_work_t* req, int status)
{
	webrequest_t * request = (webrequest_t *)req->data;

	webrequest_put(request);
	free(req);
}

int webrequest_push(webrequest_t * request, webconn_t * conn)
{
	uv_work_t * req = (uv_work_t*)malloc(sizeof(uv_work_t));
	req->data = webrequest_get(request);
	request->conn = webconn_get(conn);
	if (0 != uv_queue_work(conn->loop, req, webrequest_threadproc, after_queue_work))
	{
		request->conn = webconn_put(conn);
		webrequest_put(request);
		free(req);
		return -1;
	}
	return 0;
}