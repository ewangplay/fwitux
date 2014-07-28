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


#ifndef __FWITUX_DEBUG_H__
#define __FWITUX_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

#ifdef G_HAVE_ISO_VARARGS
#  ifdef FWITUX_DISABLE_DEBUG
#    define fwitux_debug(...)
#  else
#    define fwitux_debug(...) fwitux_debug_impl (__VA_ARGS__)
#  endif
#elif defined(G_HAVE_GNUC_VARARGS)
#  if FWITUX_DISABLE_DEBUG
#    define fwitux_debug(fmt...)
#  else
#    define fwitux_debug(fmt...) fwitux_debug_impl(fmt)
#  endif
#else
#  if FWITUX_DISABLE_DEBUG
#    define fwitux_debug(x)
#  else
#    define fwitux_debug fwitux_debug_impl
#  endif
#endif

void fwitux_debug_impl (const gchar *domain, const gchar *msg, ...);

G_END_DECLS

#endif /* __FWITUX_DEBUG_H__ */

