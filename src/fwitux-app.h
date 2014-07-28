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

#ifndef __FWITUX_APP_H__
#define __FWITUX_APP_H__

#include <glib-object.h>
#include <glib.h>

#include <gtk/gtk.h>

#include <libfwitux/fwitux-paths.h>

#include "fwitux-parser.h"

G_BEGIN_DECLS

#define FWITUX_TYPE_APP             (fwitux_app_get_type ())
#define FWITUX_APP(o)			    (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_APP, FwituxApp))
#define FWITUX_APP_CLASS(k)		    (G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_APP, FwituxAppClass))
#define FWITUX_IS_APP(o)		    (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_APP))
#define FWITUX_IS_APP_CLASS(k)	    (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_APP))
#define FWITUX_IS_APP_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_APP, FwituxAppClass))

typedef struct _FwituxApp      	FwituxApp;
typedef struct _FwituxAppClass 	FwituxAppClass;
typedef struct _FwituxAppPriv  	FwituxAppPriv;

struct _FwituxApp {
        GObject        parent;
};

struct _FwituxAppClass {
        GObjectClass parent_class;
};

GType               fwitux_app_get_type                     (void) G_GNUC_CONST;
void                fwitux_app_create                       (void);
FwituxApp *	        fwitux_app_get                          (void);
GtkWidget *         fwitux_app_get_window                   (void);
void				fwitux_app_set_visibility				(gboolean	   visible);
void				fwitux_app_set_statusbar_msg	        (const gchar   *message);
void                fwitux_app_notify_sound                 (void);
void				fwitux_app_notify                       (gchar        *msg);
void                fwitux_app_state_on_connection          (gboolean      connected);
void                fwitux_app_set_image                    (const gchar  *file,
                                                             GtkTreeIter   iter);
void                fwitux_app_show_expand_message               (const gchar  *name,
                                                             const gchar  *date,
                                                             const gchar  *source,
                                                             const gchar  *tweet,
                                                             const gchar  *link,
                                                             const gchar  *image_addr,
                                                             GdkPixbuf    *pixbuf);
void                fwitux_app_hide_expand_message          (void);
G_END_DECLS

#endif /*_FWITUX_APP_H_*/
