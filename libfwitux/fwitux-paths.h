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

#ifndef __FWITUX_PATHS_H__
#define __FWITUX_PATHS_H__

#include <glib.h>

G_BEGIN_DECLS

gchar *fwitux_paths_get_xml_path    (const gchar *filename);
gchar *fwitux_paths_get_image_path  (const gchar *filename);
gchar *fwitux_paths_get_locale_path (void);

G_END_DECLS

#endif /* __FWITUX_PATHS_H__ */
