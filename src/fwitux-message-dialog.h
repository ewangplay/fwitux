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

#ifndef __FWITUX_MESSAGE_DIALOG_H__
#define __FWITUX_MESSAGE_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FWITUX_TYPE_MESSAGE_DIALOG     	        (fwitux_message_dialog_get_type ())
#define FWITUX_MESSAGE_DIALOG(o)		    	(G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_MESSAGE_DIALOG, FwituxMessageDialog))
#define FWITUX_MESSAGE_DIALOG_CLASS(k)	   	 	(G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_MESSAGE_DIALOG, FwituxMessageDialogClass))
#define FWITUX_IS_MESSAGE_DIALOG(o)	    	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_MESSAGE_DIALOG))
#define FWITUX_IS_MESSAGE_DIALOG_CLASS(k)  	    (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_MESSAGE_DIALOG))
#define FWITUX_IS_MESSAGE_DIALOG_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_MESSAGE_DIALOG, FwituxMessageDialogClass))

typedef struct _FwituxMessageDialog      	FwituxMessageDialog;
typedef struct _FwituxMessageDialogClass 	FwituxMessageDialogClass;
typedef struct _FwituxMessageDialogPriv  	FwituxMessageDialogPriv;

struct _FwituxMessageDialog {
        GObject        parent;
};

struct _FwituxMessageDialogClass {
        GObjectClass parent_class;
};

GType             fwitux_message_dialog_get_type         (void) G_GNUC_CONST;
void              fwitux_message_dialog_show (GtkWindow   *parent);
void              fwitux_message_dialog_correct_word     (GtkWidget   *textview,
									                      GtkTextIter  start,
									                      GtkTextIter  end,
									                      const gchar *new_word);
void              fwitux_message_dialog_set_friends      (GList       *friends);
void              fwitux_message_dialog_show_friends     (gboolean     show_friends);
void              fwitux_message_dialog_set_message      (const gchar *message);
void              fwitux_message_dialog_set_reply_status_id (const gchar *status_id);
void              fwitux_message_dialog_set_caption      (const gchar *caption);
void              fwitux_message_dialog_set_source_type (gint source_type);
G_END_DECLS

#endif /* __FWITUX_MESSAGE_DIALOG_H__ */
