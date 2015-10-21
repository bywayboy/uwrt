#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <uci.h>

#include "cfg.h"

static struct uci_context * ctx = NULL;
static config_t cfg = {
#if defined(__linux__)
	.bind="0.0.0.0",
	.port=80,
	.wwwroot=NULL
#else
	"0.0.0.0",
	80,
	NULL
#endif
};

#define UCI_CONFIG_FILE	"/etc/config/uvhttpd"

const config_t * config_get(void) {
	return &cfg;
}
static void server_config(struct uci_section * s)
{
	struct uci_element *n = NULL;
	uci_foreach_element(&s->options, n) {
		struct uci_option *o = uci_to_option(n);
		switch (n->name[0]) {
		case 'b':// bind.
			strcpy(cfg.bind, o->v.string);
			break;
		case 'p':
			cfg.port = strtoul(o->v.string, NULL, 10);
			break;
		case 'w':
			cfg.wwwroot = strdup(o->v.string);
			break;
		}
	}
}

static void mime_config(struct uci_section * s) {
	struct uci_element *n = NULL;
	uci_foreach_element(&s->options, n) {
		struct uci_option *o = uci_to_option(n);
		lh_table_insert(cfg.mime, strdup(n->name), strdup(o->v.string));
	}
}

static mime_free(struct lh_entry *e) {
	free(e->k);
	free((void*)e->v);
}

int config_init(void)
{
	struct uci_package * pkg = NULL;

	cfg.mime = lh_kchar_table_new(32, "MIME", mime_free);
	if (UCI_OK == uci_load(ctx, UCI_CONFIG_FILE, &pkg))
	{
		struct uci_element * e;
		uci_foreach_element(&pkg->sections, e)
		{
			struct uci_section * s = uci_to_section(e);
			switch (s->type[0])
			{
			case 's':
				server_config(s);
				break;
			case 'm':
				mime_config(s);
				break;
			}
		}
	}
	return 0L;
}

void config_uninit(void) {

	if (NULL != cfg.mime)
		lh_table_free(cfg.mime);
	if (NULL != cfg.wwwroot)
		free(cfg.wwwroot);
	cfg.mime = NULL;
	cfg.wwwroot = NULL;
}