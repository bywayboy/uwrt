#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uv.h>
#include <ctype.h>
#if defined(__linux__)

#endif
#include "automem.h"
#include "utils.h"
#include "cfg.h"
#include "webserver.h"

char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

#if defined(_MSC_VER)
#define S_ISDIR(m) (((m) & 0170000) == (0040000))  
#endif

int mstrcmp(char * a, char * b) {
	char c;
	int r=0;
	while (c = *a++) {
		if (r = c - *b++)
			break;
	}
	return r;
}

char * analize_url(webrequest_t * request, uint32_t * code) {
	automem_t mem;
	struct stat st;
	const config_t * cfg = config_get();
	uint32_t i=0, l = request->nurl;
	char * buf, *p,*bp;
	p = buf = malloc(request->nurl + cfg->lwwwroot + 12);

	memcpy(buf, cfg->wwwroot, cfg->lwwwroot);
	p += cfg->lwwwroot;
	bp = p;
	while (i < l) {
		unsigned char c;
		switch (c= request->_url[i]) {
		case '#':
		case '?':
		case '\0':
			goto parse_finish;
		case '%':
			*p++ = from_hex(request->_url[i + 1]) << 4 | from_hex(request->_url[i + 2]);
			i += 2;
			break;
		default:
			*p++ = c;
			break;
		}
		i++;
	}
parse_finish:
	if (0 == mstrcmp(cfg->cgi, bp)) {
		//ÊÇCGI½Å±¾.
		memmove(buf, bp + cfg->lcgi, l-cfg->lcgi);
		buf[l - cfg->lcgi] = '\0';
		*code = 600; // is script;
		return request->file = buf;
	}
	*p = '\0';
	p--;
	puts(buf);
	if (p[0] == '\\' || p[0] == '/')p[0] = '\0';
	puts(buf);
	while (0 == stat(buf, &st))
	{
		if (S_ISDIR(st.st_mode)) {
			strcpy(p, "/index.html");
		}
	}
	*code = 404;
	return buf;
}