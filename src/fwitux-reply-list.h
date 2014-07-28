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

#ifndef __FWITUX_REPLY_LIST_H__
#define __FWITUX_REPLY_LIST_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * FwituxReplyList 
 */ 
#define FWITUX_TYPE_REPLY_LIST         (fwitux_reply_list_get_type ())
#define FWITUX_REPLY_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_REPLY_LIST, FwituxReplyList))
#define FWITUX_REPLY_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_REPLY_LIST, FwituxReplyListClass))
#define FWITUX_IS_REPLY_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_REPLY_LIST))
#define FWITUX_IS_REPLY_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_REPLY_LIST))
#define FWITUX_REPLY_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_TWEET_LIST, FwituxReplyListClass))

typedef struct _FwituxReplyList      FwituxReplyList;
typedef struct _FwituxReplyListClass FwituxReplyListClass;
typedef struct _FwituxReplyListPriv  FwituxReplyListPriv;

struct _FwituxReplyList {
	GtkTreeView            parent;
};

struct _FwituxReplyListClass {
	GtkTreeViewClass       parent_class;
};

GType                   fwitux_reply_list_get_type           (void) G_GNUC_CONST;
FwituxReplyList *       fwitux_reply_list_new                (void);
void                    fwitux_reply_list_set_status_id      (const gchar *status_id);
GtkListStore *          fwitux_reply_list_get_store          (void);

G_END_DECLS

#endif /* __FWITUX_REPLY_LIST_H__ */
