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

#ifndef __FWITUX_FRIENDS_DIALOG_H__
#define __FWITUX_FRIENDS_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>

typedef struct {
	GtkWidget    *dialog;
	GtkTreeView  *view;
	GtkTreeModel *store;
    GtkButton    *btn_remove;
} FwituxLists;

void fwitux_friends_lists_dialog_show       (GtkWindow *parent);
void fwitux_followed_lists_dialog_show       (GtkWindow *parent);
void fwitux_followers_lists_dialog_show       (GtkWindow *parent);
void fwitux_lists_dialog_load_lists (FwituxLists * lists, GList     *users);

extern FwituxLists *friends_lists;
extern FwituxLists *followed_lists;
extern FwituxLists *followers_lists;

#endif /* __FWITUX_FRIENDS_DIALOG_H__ */
