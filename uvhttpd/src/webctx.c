#include <stdio.h>

#include "uv.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "webserver.h"
#include "webrequest.h"
#include "webctx.h"
#include "utils.h"

#define LUA_FMTCTX_CLSNAME	"HttpCtx"

// 获取HTTP头部信息
static int lua_webctx_params(lua_State * L) {

}
// 获取URL GET参数
static int lua_webctx_get(lua_State * L) {

}
// 获取POST参数.
static int lua_webctx_post(lua_State * L) {

}
// 设置HTTP头部信息
static int lua_webctx_header(lua_State * L) {

}
// 获取所有在线的设备.
static int lua_webctx_getdevices(lua_State * L) {

}


static const luaL_Reg lua_webctx[] = {
	{"param", lua_webctx_params},	// 获取HTTP头部信息.
	{"get", lua_webctx_get},		// 获取URL GET参数.
	{"post", lua_webctx_post},		// 获取POST参数.
	{"header",lua_webctx_header},	// 设置HTTP头部信息.
	{"devs", lua_webctx_getdevices},// 获取所有在线的设备.
	{NULL,NULL},
};

int lua_pushwebctx(lua_State * L, webrequest_t * req)
{
	lua_register_class(L, lua_webctx, LUA_FMTCTX_CLSNAME, NULL);
	lua_pushlightuserdata(L, req); // LightUserData 我吃了
	luaL_setmetatable(L, LUA_FMTCTX_CLSNAME);
	return 1;
}
