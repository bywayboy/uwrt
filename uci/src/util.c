/*
 * libuci - Library for the Unified Configuration Interface
 * Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

/*
 * This file contains misc utility functions and wrappers to standard
 * functions, which throw exceptions upon failure.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
//#include <stdbool.h>
#if defined(_WIN32) || defined(_WIN64)
	#include "unistd_2008.h"
#else
	#include <unistd.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/file.h>
#endif 
#include "uci.h"
#include "uci_internal.h"

#if _MSC_VER >=1200
	#include "flock.h"
#endif


 void *uci_malloc(struct uci_context *ctx, size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr)
		UCI_THROW(ctx, UCI_ERR_MEM);
	memset(ptr, 0, size);

	return ptr;
}

 void *uci_realloc(struct uci_context *ctx, void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (!ptr)
		UCI_THROW(ctx, UCI_ERR_MEM);

	return ptr;
}

 char *uci_strdup(struct uci_context *ctx, const char *str)
{
	char *ptr;
#if defined(_WIN32) || defined(_WIN64)
	ptr = _strdup(str);
#else
	ptr = strdup(str);
#endif
	if (!ptr)
		UCI_THROW(ctx, UCI_ERR_MEM);

	return ptr;
}

/*
 * validate strings for names and types, reject special characters
 * for names, only alphanum and _ is allowed (shell compatibility)
 * for types, we allow more characters
 */
 bool uci_validate_str(const char *str, bool name)
{
	if (!*str)
		return false;

	while (*str) {
		unsigned char c = *str;
		if (!isalnum(c) && c != '_') {
			if (name || (c < 33) || (c > 126))
				return false;
		}
		str++;
	}
	return true;
}

bool uci_validate_text(const char *str)
{
	while (*str) {
		unsigned char c = *str;
		if ((c == '\r') || (c == '\n') ||
			((c < 32) && (c != '\t')))
			return false;
		str++;
	}
	return true;
}

void uci_alloc_parse_context(struct uci_context *ctx)
{
	ctx->pctx = (struct uci_parse_context *) uci_malloc(ctx, sizeof(struct uci_parse_context));
}
#if defined(_MSC_VER)
char* strsep(char** stringp, const char* delim)
{
  char* start = *stringp;
  char* p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
  {
    *stringp = NULL;
  }
  else
  {
    *p = '\0';
    *stringp = p + 1;
  }

  return start;
}
#endif
int uci_parse_ptr(struct uci_context *ctx, struct uci_ptr *ptr, char *str)
{
	char *last = NULL;
	char *tmp;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, str);
	UCI_ASSERT(ctx, ptr);

	memset(ptr, 0, sizeof(struct uci_ptr));

	/* value */
	last = strchr(str, '=');
	if (last) {
		*last = 0;
		last++;
		ptr->value = last;
	}

	ptr->package = strsep(&str, ".");
	if (!ptr->package)
		goto error;

	ptr->section = strsep(&str, ".");
	if (!ptr->section) {
		ptr->target = UCI_TYPE_PACKAGE;
		goto lastval;
	}

	ptr->option = strsep(&str, ".");
	if (!ptr->option) {
		ptr->target = UCI_TYPE_SECTION;
		goto lastval;
	} else {
		ptr->target = UCI_TYPE_OPTION;
	}

	tmp = strsep(&str, ".");
	if (tmp)
		goto error;

lastval:
	if (ptr->package && !uci_validate_package(ptr->package))
		goto error;
	if (ptr->section && !uci_validate_name(ptr->section))
		ptr->flags |= UCI_LOOKUP_EXTENDED;
	if (ptr->option && !uci_validate_name(ptr->option))
		goto error;
	if (ptr->value && !uci_validate_text(ptr->value))
		goto error;

	return 0;

error:
	memset(ptr, 0, sizeof(struct uci_ptr));
	UCI_THROW(ctx, UCI_ERR_PARSE);
}


void uci_parse_error(struct uci_context *ctx, char *pos, char *reason)
{
	struct uci_parse_context *pctx = ctx->pctx;

	pctx->reason = reason;
	pctx->byte = pos - pctx->buf;
	UCI_THROW(ctx, UCI_ERR_PARSE);
}



/*
 * open a stream and go to the right position
 *
 * note: when opening for write and seeking to the beginning of
 * the stream, truncate the file
 */
 FILE *uci_open_stream(struct uci_context *ctx, const char *filename, int pos, bool write, bool create)
{
	struct stat statbuf;
	FILE *file = NULL;
	int fd, ret;
	int mode = (write ? O_RDWR : O_RDONLY);

	if (create)
		mode |= O_CREAT;

	if (!write && ((stat(filename, &statbuf) < 0) ||
		((statbuf.st_mode &  S_IFMT) != S_IFREG))) {
		UCI_THROW(ctx, UCI_ERR_NOTFOUND);
	}
#if defined(_WIN32) || defined(_WIN64)
	fd = _open(filename, mode, UCI_FILEMODE);
#else
	fd = open(filename, mode, UCI_FILEMODE);
#endif
	if (fd < 0)
		goto error;

	ret = flock(fd, (write ? LOCK_EX : LOCK_SH));
	if ((ret < 0) && (errno != ENOSYS))
		goto error;

#if defined(_WIN32) || defined(_WIN64)
	ret = _lseek(fd, 0, pos);
#else
	ret = lseek(fd, 0, pos);
#endif

	if (ret < 0)
		goto error;

#if defined(_WIN32) || defined(_WIN64)
	file = _fdopen(fd, (write ? "w+" : "r"));
#else
	file = fdopen(fd, (write ? "w+" : "r"));
#endif
	if (file)
		goto done;

error:
	UCI_THROW(ctx, UCI_ERR_IO);
done:
	return file;
}

void uci_close_stream(FILE *stream)
{
	int fd;

	if (!stream)
		return;

	fflush(stream);
#if defined(_WIN32) || defined(_WIN64)
	fd = _fileno(stream);
#else
	fd = fileno(stream);
#endif
	flock(fd, LOCK_UN);


	fclose(stream);
}


