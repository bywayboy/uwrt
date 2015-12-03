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

// ��ȡHTTPͷ����Ϣ
static int lua_webctx_params(lua_State * L) {

}
// ��ȡURL GET����
static int lua_webctx_get(lua_State * L) {

}
// ��ȡPOST����.
static int lua_webctx_post(lua_State * L) {

}
// ����HTTPͷ����Ϣ
static int lua_webctx_header(lua_State * L) {

}
// ��ȡ�������ߵ��豸.
static int lua_webctx_getdevices(lua_State * L) {

}


static const luaL_Reg lua_webctx[] = {
	{"param", lua_webctx_params},	// ��ȡHTTPͷ����Ϣ.
	{"get", lua_webctx_get},		// ��ȡURL GET����.
	{"post", lua_webctx_post},		// ��ȡPOST����.
	{"header",lua_webctx_header},	// ����HTTPͷ����Ϣ.
	{"devs", lua_webctx_getdevices},// ��ȡ�������ߵ��豸.
	{NULL,NULL},
};

int lua_pushwebctx(lua_State * L, webrequest_t * req)
{
	lua_register_class(L, lua_webctx, LUA_FMTCTX_CLSNAME, NULL);
	lua_pushlightuserdata(L, req); // LightUserData �ҳ���
	luaL_setmetatable(L, LUA_FMTCTX_CLSNAME);
	return 1;
}
