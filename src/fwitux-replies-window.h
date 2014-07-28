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

#ifndef __FWITUX_REPLIES_WINDOW_H__
#define __FWITUX_REPLIES_WINDOW_H__

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libfwitux/fwitux-paths.h>

#include "fwitux-parser.h"

G_BEGIN_DECLS

#define FWITUX_TYPE_REPLIES_WINDOW              (fwitux_replies_window_get_type ())
#define FWITUX_REPLIES_WINDOW(o)			    (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_REPLIES_WINDOW, FwituxRepliesWindow))
#define FWITUX_REPLIES_WINDOW_CLASS(k)		    (G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_REPLIES_WINDOW, FwituxRepliesWindowClass))
#define FWITUX_IS_REPLIES_WINDOW(o)		        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_REPLIES_WINDOW))
#define FWITUX_IS_REPLIES_WINDOW_CLASS(k)	    (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_REPLIES_WINDOW))
#define FWITUX_IS_REPLIES_WINDOW_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_REPLIES_WINDOW, FwituxRepliesWindowClass))

typedef struct _FwituxRepliesWindow      	FwituxRepliesWindow;
typedef struct _FwituxRepliesWindowClass 	FwituxRepliesWindowClass;
typedef struct _FwituxRepliesWindowPriv  	FwituxRepliesWindowPriv;

struct _FwituxRepliesWindow {
        GObject        parent;
};

struct _FwituxRepliesWindowClass {
        GObjectClass parent_class;
};

GType                   fwitux_replies_window_get_type             (void) G_GNUC_CONST;
void                    fwitux_replies_window_show                 (const gchar *status_id);
void				    fwitux_replies_window_set_statusbar_msg	   (const gchar *message);
void                    fwitux_replies_window_set_image            (const gchar  *file,
                                                                       GtkTreeIter   iter);
void                    fwitux_replies_window_set_expand_message  (const gchar  *name,
                                                                       const gchar  *date,
                                                                       const gchar  *source,
                                                                       const gchar  *reply_count,
                                                                       const gchar  *tweet,
                                                                       const gchar  *link,
                                                                       const gchar  *image_addr,
                                                                       const gchar  *user_image_url);
G_END_DECLS

#endif /*_FWITUX_REPLIES_WINDOW_H_*/
