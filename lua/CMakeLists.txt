cmake_minimum_required(VERSION 2.6)

PROJECT(lua C)

ADD_DEFINITIONS("-DLUA_USE_LINUX")

SET(LUA_SOURCES
	src/lapi.c
	src/lauxlib.c
	src/lbaselib.c
	src/lbitlib.c
	src/lcode.c
	src/lcorolib.c
	src/lctype.c
	src/ldblib.c
	src/ldebug.c
	src/ldo.c
	src/ldump.c
	src/lfunc.c
	src/lgc.c
	src/linit.c
	src/liolib.c
	src/llex.c
	src/lmathlib.c
	src/lmem.c
	src/loadlib.c
	src/lobject.c
	src/lopcodes.c
	src/loslib.c
	src/lparser.c
	src/lstate.c
	src/lstring.c
	src/lstrlib.c
	src/ltable.c
	src/ltablib.c
	src/ltm.c
	src/lundump.c
	src/lvm.c
	src/lzio.c
	src/lutf8lib.c
)

ADD_LIBRARY(lua SHARED ${LUA_SOURCES})
TARGET_LINK_LIBRARIES(lua dl pthread m)

INSTALL(TARGETS
	lua DESTINATION /usr/lib
)

 
