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
 * This file contains the code for handling uci config delta files
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include "unistd_2008.h"
#else
	#include <stdbool.h>
	#include <unistd.h>
	#include <sys/file.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <tchar.h>
	#include <wchar.h>
	#include <io.h>
	#include <windows.h>
#else
	#include <sys/file.h>
	#define  _asprintf asprintf
#endif

#include "uci.h"
#include "uci_internal.h"



/* record a change that was done to a package */
void
uci_add_delta(struct uci_context *ctx, struct uci_list *list, int cmd, const char *section, const char *option, const char *value)
{
	struct uci_delta *h;
	int size = strlen(section) + 1;
	char *ptr;

	if (value)
		size += strlen(value) + 1;

	h = uci_alloc_element(ctx, delta, option, size);
	ptr = uci_dataptr(h);
	h->cmd = cmd;
	h->section = strcpy(ptr, section);
	if (value) {
		ptr += strlen(ptr) + 1;
		h->value = strcpy(ptr, value);
	}
	uci_list_add(list, &h->e.list);
}

void
uci_free_delta(struct uci_delta *h)
{
	if (!h)
		return;
	if ((h->section != NULL) &&
		(h->section != uci_dataptr(h))) {
		free(h->section);
		free(h->value);
	}
	uci_free_element(&h->e);
}


int uci_set_savedir(struct uci_context *ctx, const char *dir)
{
	char *sdir;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, dir != NULL);

	sdir = uci_strdup(ctx, dir);
	if (ctx->savedir != uci_savedir)
		free(ctx->savedir);
	ctx->savedir = sdir;
	return 0;
}

int uci_add_delta_path(struct uci_context *ctx, const char *dir)
{
	struct uci_element *e;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, dir != NULL);
	e = uci_alloc_generic(ctx, UCI_TYPE_PATH, dir, sizeof(struct uci_element));
	uci_list_add(&ctx->delta_path, &e->list);

	return 0;
}

static __inline int uci_parse_delta_tuple(struct uci_context *ctx, char **buf, struct uci_ptr *ptr)
{
	int c = UCI_CMD_CHANGE;

	switch(**buf) {
	case '^':
		c = UCI_CMD_REORDER;
		break;
	case '-':
		c = UCI_CMD_REMOVE;
		break;
	case '@':
		c = UCI_CMD_RENAME;
		break;
	case '+':
		/* UCI_CMD_ADD is used for anonymous sections or list values */
		c = UCI_CMD_ADD;
		break;
	case '|':
		c = UCI_CMD_LIST_ADD;
		break;
	case '_':
		c = UCI_CMD_LIST_DEL;
		break;
	}

	if (c != UCI_CMD_CHANGE)
		*buf += 1;

	//UCI_INTERNAL(uci_parse_ptr, ctx, ptr, *buf);
	do {
		ctx->internal = true;
		uci_parse_ptr(ctx, ptr, *buf);
	} while (0);


	if (!ptr->section)
		goto error0;
	if (ptr->flags & UCI_LOOKUP_EXTENDED)
		goto error0;

	switch(c) {
	case UCI_CMD_REORDER:
		if (!ptr->value || ptr->option)
			goto error0;
		break;
	case UCI_CMD_RENAME:
		if (!ptr->value || !uci_validate_name(ptr->value))
			goto error0;
		break;
	case UCI_CMD_LIST_ADD:
		if (!ptr->option)
			goto error0;
	case UCI_CMD_LIST_DEL:
		if (!ptr->option)
			goto error0;
	}

	return c;

error0:
	UCI_THROW(ctx, UCI_ERR_INVAL);
	return 0;
}

static void uci_parse_delta_line(struct uci_context * ctx, struct uci_package * p, char * buf)
{
	struct uci_element *e = NULL;
	struct uci_ptr ptr;
	int cmd;

	cmd = uci_parse_delta_tuple(ctx, &buf, &ptr);
	if (strcmp(ptr.package, p->e.name) != 0)
		goto _error;

	if (ctx->flags & UCI_FLAG_SAVED_DELTA)
		uci_add_delta(ctx, &p->saved_delta, cmd, ptr.section, ptr.option, ptr.value);

	switch(cmd) {
	case UCI_CMD_REORDER:
		uci_expand_ptr(ctx, &ptr, true);
		if (!ptr.s)
			UCI_THROW(ctx, UCI_ERR_NOTFOUND);
		//UCI_INTERNAL(uci_reorder_section, ctx, ptr.s, strtoul(ptr.value, NULL, 10));
		do {
			ctx->internal = true;
			uci_reorder_section(ctx,  ptr.s, strtoul(ptr.value, NULL, 10));
		} while (0);
		break;
	case UCI_CMD_RENAME:
		//UCI_INTERNAL(uci_rename, ctx, &ptr);
		do {
			ctx->internal = true;
			uci_rename(ctx, &ptr);
		} while (0);
		break;
	case UCI_CMD_REMOVE:
		//UCI_INTERNAL(uci_delete, ctx, &ptr);
		do {
			ctx->internal = true;
			uci_delete(ctx, &ptr);
		} while (0);
		break;
	case UCI_CMD_LIST_ADD:
		//UCI_INTERNAL(uci_add_list, ctx, &ptr);
		do {
				ctx->internal = true;
				uci_add_list(ctx, &ptr);
		} while (0);
		break;
	case UCI_CMD_LIST_DEL:
		//UCI_INTERNAL(uci_del_list, ctx, &ptr);
		do {
			ctx->internal = true;
			uci_del_list(ctx, &ptr);
		} while (0);
		break;
	case UCI_CMD_ADD:
	case UCI_CMD_CHANGE:
		//UCI_INTERNAL(uci_set, ctx, &ptr);
		do {
			ctx->internal = true;
			uci_set(ctx, &ptr);
		} while (0);

		e = ptr.last;
		if (!ptr.option && e && (cmd == UCI_CMD_ADD))
			uci_to_section(e)->anonymous = true;
		break;
	}
	return;
_error:
	UCI_THROW(ctx, UCI_ERR_PARSE);
}

/* returns the number of changes that were successfully parsed */
static int uci_parse_delta(struct uci_context *ctx, FILE *stream, struct uci_package *p)
{
	struct uci_parse_context *pctx;
	int changes = 0;

	/* make sure no memory from previous parse attempts is leaked */
	uci_cleanup(ctx);

	pctx = (struct uci_parse_context *) uci_malloc(ctx, sizeof(struct uci_parse_context));
	ctx->pctx = pctx;
	pctx->file = stream;

	while (!feof(pctx->file)) {
		uci_getln(ctx, 0);
		if (!pctx->buf[0])
			continue;

		/*
		 * ignore parse errors in single lines, we want to preserve as much
		 * delta as possible
		 */
		UCI_TRAP_SAVE(ctx, errorx);
		uci_parse_delta_line(ctx, p, pctx->buf);
		UCI_TRAP_RESTORE(ctx);
		changes++;
errorx:
		continue;
	}

	/* no error happened, we can get rid of the parser context now */
	uci_cleanup(ctx);
	return changes;
}

/* returns the number of changes that were successfully parsed */
static int uci_load_delta_file(struct uci_context *ctx, struct uci_package *p, char *filename, FILE **f, bool flush)
{
	FILE *stream = NULL;
	int changes = 0;

	UCI_TRAP_SAVE(ctx, done);
	stream = uci_open_stream(ctx, filename, SEEK_SET, flush, false);
	if (p)
		changes = uci_parse_delta(ctx, stream, p);
	UCI_TRAP_RESTORE(ctx);
done:
	if (f)
		*f = stream;
	else if (stream)
		uci_close_stream(stream);
	return changes;
}

/* returns the number of changes that were successfully parsed */
 int uci_load_delta(struct uci_context *ctx, struct uci_package *p, bool flush)
{
	struct uci_element *e;
	char *filename = NULL;
	FILE *f = NULL;
	int changes = 0;

	if (!p->has_delta)
		return 0;
#if defined(_WIN32) || defined(_WIN64)
#define _PATH_FORMAT_ "%s\\%s"
#else
#define _PATH_FORMAT_ "%s/%s"
#endif
	uci_foreach_element(&ctx->delta_path, e) {
		if ((_asprintf(&filename, _PATH_FORMAT_, e->name, p->e.name) < 0) || !filename)
			UCI_THROW(ctx, UCI_ERR_MEM);

		uci_load_delta_file(ctx, p, filename, NULL, false);
		free(filename);
	}

	if ((_asprintf(&filename, _PATH_FORMAT_, ctx->savedir, p->e.name) < 0) || !filename)
		UCI_THROW(ctx, UCI_ERR_MEM);

	changes = uci_load_delta_file(ctx, p, filename, &f, flush);
	if (flush && f && (changes > 0)) {
		rewind(f);
#if defined(_WIN32) || defined(_WIN64)
		if (_chsize(_fileno(f), 0) < 0){
#else
		if (ftruncate(fileno(f), 0) < 0){
#endif
			uci_close_stream(f);
			UCI_THROW(ctx, UCI_ERR_IO);
		}
	}
	if (filename)
		free(filename);
	uci_close_stream(f);
	ctx->err = 0;
	return changes;
}

static void uci_filter_delta(struct uci_context *ctx, const char *name, const char *section, const char *option)
{
	struct uci_parse_context *pctx;
	struct uci_element *e, *tmp;
	struct uci_list list;
	char *filename = NULL;
	struct uci_ptr ptr;
	FILE *f = NULL;

	uci_list_init(&list);
	uci_alloc_parse_context(ctx);
	pctx = ctx->pctx;
#if defined(_WIN32) || defined(_WIN64)
#define _PATH_FORMAT_ "%s\\%s"
#else
#define _PATH_FORMAT_ "%s/%s"
#endif
	if ((_asprintf(&filename, _PATH_FORMAT_, ctx->savedir, name) < 0) || !filename)
		UCI_THROW(ctx, UCI_ERR_MEM);

	UCI_TRAP_SAVE(ctx, done);
	f = uci_open_stream(ctx, filename, SEEK_SET, true, false);
	pctx->file = f;
	while (!feof(f)) {
		struct uci_element *e;
		char *buf;

		uci_getln(ctx, 0);
		buf = pctx->buf;
		if (!buf[0])
			continue;

		/* NB: need to allocate the element before the call to
		 * uci_parse_delta_tuple, otherwise the original string
		 * gets modified before it is saved */
		e = uci_alloc_generic(ctx, UCI_TYPE_DELTA, pctx->buf, sizeof(struct uci_element));
		uci_list_add(&list, &e->list);

		uci_parse_delta_tuple(ctx, &buf, &ptr);
		if (section) {
			if (!ptr.section || (strcmp(section, ptr.section) != 0))
				continue;
		}
		if (option) {
			if (!ptr.option || (strcmp(option, ptr.option) != 0))
				continue;
		}
		/* match, drop this element again */
		uci_free_element(e);
	}

	/* rebuild the delta file */
	rewind(f);
#if defined(_WIN32) || defined(_WIN64)
	if (_chsize(_fileno(f), 0) < 0)
#else
	if (ftruncate(fileno(f), 0) < 0)
#endif
		UCI_THROW(ctx, UCI_ERR_IO);
	uci_foreach_element_safe(&list, tmp, e) {
		fprintf(f, "%s\n", e->name);
		uci_free_element(e);
	}
	UCI_TRAP_RESTORE(ctx);

done:
	if (filename)
		free(filename);
	uci_close_stream(pctx->file);
	uci_foreach_element_safe(&list, tmp, e) {
		uci_free_element(e);
	}
	uci_cleanup(ctx);
}

int uci_revert(struct uci_context *ctx, struct uci_ptr *ptr)
{
	char *package = NULL;
	char *section = NULL;
	char *option = NULL;

	UCI_HANDLE_ERR(ctx);
	uci_expand_ptr(ctx, ptr, false);
	UCI_ASSERT(ctx, ptr->p->has_delta);

	/*
	 * - flush unwritten changes
	 * - save the package name
	 * - unload the package
	 * - filter the delta
	 * - reload the package
	 */
	UCI_TRAP_SAVE(ctx, error2);
	//UCI_INTERNAL(uci_save, ctx, ptr->p);
	do {
		ctx->internal = true;
		uci_save(ctx, ptr->p);
	} while (0);

	/* NB: need to clone package, section and option names,
	 * as they may get freed on uci_free_package() */
	package = uci_strdup(ctx, ptr->p->e.name);
	if (ptr->section)
		section = uci_strdup(ctx, ptr->section);
	if (ptr->option)
		option = uci_strdup(ctx, ptr->option);

	uci_free_package(&ptr->p);
	uci_filter_delta(ctx, package, section, option);

	//UCI_INTERNAL(uci_load, ctx, package, &ptr->p);
	
	do { 
		ctx->internal = true;		
		uci_load(ctx, package, &ptr->p);	
	} while (0);

	UCI_TRAP_RESTORE(ctx);
	ctx->err = 0;

error2:
	if (package)
		free(package);
	if (section)
		free(section);
	if (option)
		free(option);
	if (ctx->err)
		UCI_THROW(ctx, ctx->err);
	return 0;
}

int uci_save(struct uci_context *ctx, struct uci_package *p)
{
	FILE *f = NULL;
	char *filename = NULL;
	struct uci_element *e, *tmp;
	struct stat statbuf;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, p != NULL);

	/*
	 * if the config file was outside of the /etc/config path,
	 * don't save the delta to a file, update the real file
	 * directly.
	 * does not modify the uci_package pointer
	 */
	if (!p->has_delta)
		return uci_commit(ctx, &p, false);

	if (uci_list_empty(&p->delta))
		return 0;

	if (stat(ctx->savedir, &statbuf) < 0)
	{
#if defined(_WIN32) || defined(_WIN64)
		CreateDirectoryA(ctx->savedir, NULL);
#else
		mkdir(ctx->savedir, UCI_DIRMODE);
#endif
		
	}
	else if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
		UCI_THROW(ctx, UCI_ERR_IO);

#if defined(_WIN32) || defined(_WIN64)
#define _PATH_FORMAT_ "%s\\%s"
#else
#define _PATH_FORMAT_ "%s/%s"
#endif

	if ((_asprintf(&filename, _PATH_FORMAT_, ctx->savedir, p->e.name) < 0) || !filename)
		UCI_THROW(ctx, UCI_ERR_MEM);

	ctx->err = 0;
	UCI_TRAP_SAVE(ctx, done);
	f = uci_open_stream(ctx, filename, SEEK_END, true, true);
	UCI_TRAP_RESTORE(ctx);

	uci_foreach_element_safe(&p->delta, tmp, e) {
		struct uci_delta *h = uci_to_delta(e);
		char *prefix = "";

		switch(h->cmd) {
		case UCI_CMD_REMOVE:
			prefix = "-";
			break;
		case UCI_CMD_RENAME:
			prefix = "@";
			break;
		case UCI_CMD_ADD:
			prefix = "+";
			break;
		case UCI_CMD_REORDER:
			prefix = "^";
			break;
		case UCI_CMD_LIST_ADD:
			prefix = "|";
			break;
		case UCI_CMD_LIST_DEL:
			prefix = "_";
			break;
		default:
			break;
		}

		fprintf(f, "%s%s.%s", prefix, p->e.name, h->section);
		if (e->name)
			fprintf(f, ".%s", e->name);

		if (h->cmd == UCI_CMD_REMOVE && !h->value)
			fprintf(f, "\n");
		else
			fprintf(f, "=%s\n", h->value);
		uci_free_delta(h);
	}

done:
	uci_close_stream(f);
	if (filename)
		free(filename);
	if (ctx->err)
		UCI_THROW(ctx, ctx->err);

	return 0;
}


