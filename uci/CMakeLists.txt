cmake_minimum_required(VERSION 2.6)

PROJECT(uci C)

SET(LIB_SOURCES
	src/libuci.c
	src/file.c
	src/util.c 
	src/delta.c
	src/parse.c
	src/asprintf.c
)

INCLUDE_DIRECTORIES(include src)
ADD_LIBRARY(uci SHARED ${LIB_SOURCES})
TARGET_LINK_LIBRARIES(uci)
 

ADD_EXECUTABLE(cli
	src/cli.c
	src/asprintf.c
)
TARGET_LINK_LIBRARIES(cli uci)
SET_TARGET_PROPERTIES(cli PROPERTIES OUTPUT_NAME uci)

ADD_LIBRARY(ucimap STATIC src/ucimap.c)

INSTALL(FILES src/uci.h src/uci_config.h src/ucimap.h
	DESTINATION include
)

INSTALL(TARGETS uci cli
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)


