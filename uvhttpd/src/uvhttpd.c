#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>
#include "cfg.h"
#include "webserver.h"

static uv_loop_t * loop = NULL;
static webserver_t s;
int main(int argc, char * argv[])
{
	int ret;
	config_init();
	loop = uv_default_loop();

	if(0 == (ret = webserver_init(loop,&s)))
		uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
