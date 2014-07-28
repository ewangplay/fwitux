/*
 * Copyright (C) 2010-2012 Wang Xiaohui <ewangplay@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include "config.h"

#include <stdarg.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

/*
 * Set FWITUX_DEBUG to a colon/comma/space separated list of domains, or "all"
 * to get all debug output.
 */

#include "fwitux-debug.h"

static gchar    **debug_strv;
static gboolean   all_domains = FALSE;

static void
debug_init (void)
{
	static gboolean inited = FALSE;

	if (!inited) {
		const gchar *env;
		gint         i;

		env = g_getenv ("FWITUX_DEBUG");

		if (env) {
			debug_strv = g_strsplit_set (env, ":, ", 0);
		} else {
			debug_strv = NULL;
		}

		for (i = 0; debug_strv && debug_strv[i]; i++) {
			if (strcmp ("all", debug_strv[i]) == 0) {
				all_domains = TRUE;
			}
		}

		inited = TRUE;
	}
}

void
fwitux_debug_impl (const gchar *domain, const gchar *msg, ...)
{
	gint i;

	g_return_if_fail (domain != NULL);
	g_return_if_fail (msg != NULL);

	debug_init ();

	for (i = 0; debug_strv && debug_strv[i]; i++) {
		if (all_domains || strcmp (domain, debug_strv[i]) == 0) {
			va_list args;

			g_print ("%s: ", domain);

			va_start (args, msg);
			g_vprintf (msg, args);
			va_end (args);

			g_print ("\n");
			break;
		}
	}
}

