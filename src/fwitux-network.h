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
#ifndef __FWITUX_NETWORK_H__
#define __FWITUX_NETWORK_H__

#include <gtk/gtk.h>
#include "fwitux-parser.h"

/* Verify user credentials */
void fwitux_network_login (const char *username, const char *password);

/* Logout current user */
void fwitux_network_logout (void);

/* Post a new tweet */
void fwitux_network_post_status	(const gchar *text);

/* Post a direct message to a friend */
void fwitux_network_send_message (const gchar *friend_id, const gchar *text);

/* Reply a status */
void fwitux_network_reply_status (const gchar *reply_status_id, const gchar * text, gint source_type);

/* Delete a status */
void fwitux_network_delete_status (const gchar *status_id, gint source_type);

/* Parse and load a timeline */
void fwitux_network_get_timeline (const gchar *url_timeline, gchar *formdata);

/* Parse and load a reply timeline */
void fwitux_network_get_reply_timeline (const gchar *status_id);

/* Retrive a user timeline. If user is null, get authenticated user timeline*/
void fwitux_network_get_user_timeline (const gchar *username);

/* Refresh current timeline */
void fwitux_network_refresh	(void);

/* Get authenticating user's friends for send message */
GList *fwitux_network_get_friends_for_message_dialog (void);

/* Get authenticating user's friends */
GList *fwitux_network_get_friends (void);

/* Get the authenticating user's followers */
GList *fwitux_network_get_followed (void);

/* Get the authenticating user's followings */
GList *fwitux_network_get_followers (void);

/* Get an image from servers */
void fwitux_network_get_image (const gchar *url_image, GtkTreeIter iter);

/* Get an image from servers */
void fwitux_network_get_reply_image (const gchar *url_image, GtkTreeIter iter);

/* Add a user to friends */
void fwitux_network_add_friend (const gchar *user_id);

/* Remove a user from friends */
void fwitux_network_del_friend (FwituxUser *user);

/* Add a user to followings */
void fwitux_network_add_follower (const gchar *user_id);

/* Remove a user from friends */
void fwitux_network_del_follower (FwituxUser *user);

/* Networking */
void fwitux_network_new	(void);

void fwitux_network_close (void);

void fwitux_network_stop (void);

void fwitux_network_remove_reply_source (void);

#endif /*  __FWITUX_NETWORK_H__ */
