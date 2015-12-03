#ifndef _UVHTTPD_UTIL_H
#define _UVHTTPD_UTIL_H


#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

void lua_register_class(lua_State * L, const luaL_Reg * methods, const char * name, lua_CFunction has_index);

int automem_append_header_date(automem_t * mem, const char * fmt, time_t t);
/*
	HTTP status Header.
	Server: uvhttpd/...
	Connection: keep-alive.
*/
int automem_init_headers(automem_t * mem, int code);

int automem_append_mime(automem_t * mem, const char * mime);
int automem_append_content_length(automem_t *mem, size_t sz);

int automem_append_contents(automem_t * mem, void * bytes, int sz);

int webcon_writefile(webconn_t * conn, const char * file, size_t st_size);

#if defined(_WIN32) || defined(_WIN64)
#include "strptime.h"
#define strcasecmp(a, b) _stricmp(a ,b)
#define mkgmtime( x ) _mkgmtime( x )
#endif
#endif