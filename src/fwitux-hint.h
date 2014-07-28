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
 
#ifndef __FWITUX_HINT_H__
#define __FWITUX_HINT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean   fwitux_hint_dialog_show	(const gchar         *conf_path,
									 const gchar         *message1,
									 const gchar         *message2,
									 GtkWindow           *parent,
									 GFunc                func,
									 gpointer             user_data);
gboolean   fwitux_hint_show			(const gchar         *conf_path,
									 const gchar         *message1,
									 const gchar         *message2,
									 GtkWindow           *parent,
									 GFunc                func,
									 gpointer             user_data);

G_END_DECLS

#endif /* __FWITUX_HINT_H__ */
