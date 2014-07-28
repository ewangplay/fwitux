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

#ifndef __FWITUX_DBUS_H__
#define __FWITUX_DBUS_H__

#include <glib.h>

gboolean fwitux_dbus_nm_init      (void);
void     fwitux_dbus_nm_finalize  (void);
gboolean fwitux_dbus_nm_get_state (gboolean *connected);

#endif /* __FWITUX_DBUS_H__ */
