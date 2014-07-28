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

#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-conf.h>

#include "fwitux.h"
#include "fwitux-geometry.h"

#define DEBUG_DOMAIN "Geometry"

void
fwitux_geometry_save_for_main_window (gint x, gint y,
									  gint w, gint h)
{
	FwituxConf *conf;

	fwitux_debug (DEBUG_DOMAIN, "Saving for main window: x:%d, y:%d, w:%d, h:%d",
				  x, y, w, h);

	conf = fwitux_conf_get ();

	fwitux_conf_set_int (conf,
						 FWITUX_PREFS_UI_WINDOW_HEIGHT,
						 h);

	fwitux_conf_set_int (conf,
						 FWITUX_PREFS_UI_WINDOW_WIDTH,
						 w);

	fwitux_conf_set_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_X,
						 x);

	fwitux_conf_set_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_Y,
						 y);

}
 
void
fwitux_geometry_load_for_main_window (GtkWidget *main_window)
{
	FwituxConf *conf;
	gint        x, y, w, h;

	fwitux_debug (DEBUG_DOMAIN, "Loading window geometry...");

	conf = fwitux_conf_get ();

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WINDOW_HEIGHT,
						 &h);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WINDOW_WIDTH,
						 &w);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_X,
						 &x);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_Y,
						 &y);

	if (w >=1 && h >= 1) {
		/*
		 * Use the defaults from the glade file
		 * if we don't have good w, h geometry.
		 */
		 fwitux_debug (DEBUG_DOMAIN,
					   "Configuring window default size w:%d, h: %d", w, h);
		 gtk_window_resize (GTK_WINDOW (main_window), w, h);
	}

	if (x >= 0 && y >= 0) {
		/*
		 * Let the window manager position it
		 * if we don't have good x, y coordinates.
		 */
		fwitux_debug (DEBUG_DOMAIN,
					  "Configuring window default position x:%d, y:%d", x, y);
		gtk_window_move (GTK_WINDOW (main_window), x, y);
	}
 }
