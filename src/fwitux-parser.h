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
#ifndef __FWITUX_PARSER_H__
#define __FWITUX_PARSER_H__

#include <gtk/gtk.h>

typedef struct {
	gchar	*id;
	gchar	*name;
	gchar	*image_url;
    gchar   *url;
    gchar   *location;
    gchar   *description;
    gchar   *sex;
    gchar   *birthday;
    gchar   *mobile;
    gchar   *qq;
    gchar   *msn;
    gchar   *email;
    gchar   *created_at;
    int     favourites_count;   //收藏数量
    int     followers_count;    //关注我的数量
    int     following_count;    //我关注的数量
    int     friends_count;      //好友数量
    int     statuses_count;     //分享数量
} FwituxUser;


/* Returns a liststore for the main treeview to show tweets */
gboolean fwitux_parser_timeline (const gchar *data, 
								 gssize       length);

/* Returns a liststore for the reply treeview to show replies */
gboolean fwitux_parser_reply_timeline (const gchar *data, 
								 gssize       length);

/* Returns a Glist with friends. Can be used to 
   build the friends menu, on direct messages dialog, etc.. */
GList *fwitux_parser_users_list (const gchar *data,
								 gssize       length);

/* Parse a xml user node. Ex: add/del users responses */
FwituxUser *fwitux_parser_single_user (const gchar *data,
									   gssize       length);

/* To free a User struct */
void parser_free_user (FwituxUser *user);

/* Restet the ID of the last tweet showed */
void parser_reset_last_tweet_time (void);

#endif /*  __FWITUX_PARSER_H__ */
