/*
 * Copyright (c) 2004 Darren Tucker.
 *
 * Based originally on asprintf.c from OpenBSD:
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller AT courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/* Include vasprintf() if not on your OS. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>


#define INIT_SZ 128

int _v_asprintf(char **strp, const char *fmt, va_list ap)
{
	FILE *dev_null;
	int arg_len;
#if defined(_WIN32) || defined(_WIN64)
	dev_null = fopen("nul", "w"); 
#else
	dev_null = fopen("/dev/null", "w");
#endif
	arg_len = vprintf(fmt, ap);
	printf("arglen = %d\n", arg_len);
	arg_len = vfprintf(dev_null, fmt, ap);
	if (arg_len != -1) {
		*strp = (char *)malloc((size_t)arg_len + 1);
		arg_len = vsprintf(*strp, fmt, ap);
	}
	else *strp = NULL;
	fclose(dev_null);
	return arg_len;
}

int _asprintf(char **strp, const char *fmt, ...)
{
	int result;
    
	va_list args;
	va_start(args, fmt);
	result = _v_asprintf(strp, fmt, args);
	va_end(args);
	return result;
}


char *_bprintf(const char *fmt, ...)
{
	char *strp = NULL;
	
	va_list args;
	va_start(args, fmt);
	_v_asprintf(&strp, fmt, args);
	va_end(args);
	
	return strp;
}