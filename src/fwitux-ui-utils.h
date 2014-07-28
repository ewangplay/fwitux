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

#ifndef __FWITUX_UI_UTILS_H__
#define __FWITUX_UI_UTILS_H__

#include <gtk/gtk.h>

/* Windows */
gboolean     fwitux_window_get_is_visible          (GtkWindow        *window);
gboolean     fwitux_window_get_is_present          (GtkWindow        *window);
void         fwitux_window_present                 (GtkWindow        *window,
													gboolean          steal_focus);
void         fwitux_help_show                      (GtkWindow        *parent);

#endif /*  __FWITUX_UI_UTILS_H__ */
