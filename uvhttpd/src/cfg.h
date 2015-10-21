#ifndef _UVHTTPD_CFG_H
#define _UVHTTPD_CFG_H

#include "linkhash.h"

#if !defined(__linux)
#define strdup( x ) _strdup( x )
#endif
typedef struct uvconfig config_t;
struct uvconfig {
	char bind[36];
	uint32_t port;
	char * wwwroot;
	void * mime;
};

const config_t * config_get(void);
int config_init(void);
void config_uninit(void);


#endif