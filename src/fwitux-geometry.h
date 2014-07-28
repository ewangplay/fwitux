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

#ifndef __FWITUX_GEOMETRY_H__
#define __FWITUX_GEOMETRY_H__

#include <gtk/gtk.h>

void fwitux_geometry_load_for_main_window (GtkWidget *main_window);
void fwitux_geometry_save_for_main_window (gint       x,
										   gint       y,
										   gint       w,
										   gint       h);

#endif /* __FWITUX_GEOMETRY_H__ */
