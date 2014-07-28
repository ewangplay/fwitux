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

#ifndef __FWITUX_TWEET_LIST_H__
#define __FWITUX_TWEET_LIST_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * FwituxTweetList 
 */ 
#define FWITUX_TYPE_TWEET_LIST         (fwitux_tweet_list_get_type ())
#define FWITUX_TWEET_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_TWEET_LIST, FwituxTweetList))
#define FWITUX_TWEET_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_TWEET_LIST, FwituxTweetListClass))
#define FWITUX_IS_TWEET_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_TWEET_LIST))
#define FWITUX_IS_TWEET_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_TWEET_LIST))
#define FWITUX_TWEET_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_TWEET_LIST, FwituxTweetListClass))

typedef struct _FwituxTweetList      FwituxTweetList;
typedef struct _FwituxTweetListClass FwituxTweetListClass;
typedef struct _FwituxTweetListPriv  FwituxTweetListPriv;

struct _FwituxTweetList {
	GtkTreeView            parent;
};

struct _FwituxTweetListClass {
	GtkTreeViewClass       parent_class;
};

GType                 fwitux_tweet_list_get_type           (void) G_GNUC_CONST;
FwituxTweetList *     fwitux_tweet_list_new                (void);
GtkListStore *        fwitux_tweet_list_get_store          (void);

G_END_DECLS

#endif /* __FWITUX_TWEET_LIST_H__ */
