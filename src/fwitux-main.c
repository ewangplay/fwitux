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

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <libnotify/notify.h>

#include <libfwitux/fwitux-paths.h>

#include "fwitux-app.h"
#include "fwitux-network.h"

int
main (int argc, char *argv[])
{
	gchar *localedir;

	localedir = fwitux_paths_get_locale_path ();
	bindtextdomain (GETTEXT_PACKAGE, localedir);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_free (localedir);

	g_set_application_name (_("fwitux"));

	if (!g_thread_supported ()) g_thread_init (NULL);

	gtk_init (&argc, &argv);

	gtk_window_set_default_icon_name ("fwitux");

	/* Start the network */
	fwitux_network_new ();

	/* Start libnotify */
	notify_init ("fwitux");

	/* Create the ui */
	fwitux_app_create ();

	gtk_main ();

	/* Close libnotify */
	notify_uninit ();

	/* Close the network */
	fwitux_network_close ();

	/* Clean up the ui */
	g_object_unref (fwitux_app_get ());

	return 0;
}
