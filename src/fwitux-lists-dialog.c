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

#include "config.h"

#include <libfwitux/fwitux-xml.h>

#include "fwitux.h"
#include "fwitux-lists-dialog.h"
#include "fwitux-network.h"

#define XML_FILE "lists_dlg.xml"

enum {
	USER_ID,
	USER_NAME,
    USER_SEX,
    USER_LOCATION,
	USER_POINTER
};

FwituxLists *friends_lists = NULL;
FwituxLists *followed_lists = NULL;
FwituxLists *followers_lists = NULL;

static void lists_remove_friend_response_cb (GtkButton   *button,
                                           FwituxLists *lists);
static void lists_remove_follower_response_cb (GtkButton   *button,
                                           FwituxLists *lists);
static void lists_response_cb     (GtkWidget   *widget,
								   gint         response,
								   FwituxLists *lists);
static void lists_destroy_cb      (GtkWidget   *widget,
								   FwituxLists *lists);
static void list_follower_activated_cb (GtkTreeView  *tree_view,
							            GtkTreePath       *path,
							            GtkTreeViewColumn *column,
							            FwituxLists       *lists);

static void
lists_response_cb (GtkWidget     *widget,
				   gint           response,
				   FwituxLists   *lists)
{
	gtk_widget_destroy (widget);
}

static void
lists_destroy_cb (GtkWidget    *widget,
				  FwituxLists  *lists)
{
	g_free (lists);
}

static void
lists_remove_friend_response_cb (GtkButton   *button,
                               FwituxLists *lists)
{
	GtkTreeSelection *sel;
	GtkTreeIter       iter;
	FwituxUser       *user;

	/* Get selected Iter */
	sel = gtk_tree_view_get_selection (lists->view);
	
	if (!gtk_tree_selection_get_selected (sel, NULL, &iter))
		return;

	gtk_tree_model_get (lists->store,
						&iter,
						USER_POINTER, &user,
						-1);

	gtk_list_store_remove (GTK_LIST_STORE (lists->store), &iter);

	fwitux_network_del_friend (user);
}

static void
lists_remove_follower_response_cb (GtkButton   *button,
                                   FwituxLists *lists)
{
	GtkTreeSelection *sel;
	GtkTreeIter       iter;
	FwituxUser       *user;

	/* Get selected Iter */
	sel = gtk_tree_view_get_selection (lists->view);
	
	if (!gtk_tree_selection_get_selected (sel, NULL, &iter))
		return;

	gtk_tree_model_get (lists->store,
						&iter,
						USER_POINTER, &user,
						-1);

	gtk_list_store_remove (GTK_LIST_STORE (lists->store), &iter);

	fwitux_network_del_follower (user);
}

static void
list_follower_activated_cb (GtkTreeView       *tree_view,
							GtkTreePath       *path,
							GtkTreeViewColumn *column,
							FwituxLists       *lists)
{
	GtkTreeIter  iter;
	gchar       *user_id;

	gtk_tree_model_get_iter (GTK_TREE_MODEL (lists->store),
							 &iter,
							 path);

	gtk_tree_model_get (GTK_TREE_MODEL (lists->store),
						&iter,
						USER_ID, &user_id,
						-1);

	/* Retrive timeline */
	fwitux_network_get_user_timeline (user_id);

	g_free (user_id);
}

void
fwitux_lists_dialog_load_lists (FwituxLists * lists, GList *users)
{
	FwituxUser  *user;
	GtkTreeIter  iter;
	GList       *list;

    if(!lists)
        return;

	/* Following */
	for (list = users; list; list = list->next)
	{
		user = (FwituxUser *)list->data;
		gtk_list_store_append (GTK_LIST_STORE (lists->store), &iter);
		gtk_list_store_set (GTK_LIST_STORE (lists->store),
							&iter,
							USER_ID, user->id,
							USER_NAME, user->name,
                            USER_SEX, user->sex,
                            USER_LOCATION, user->location,
							USER_POINTER, user,
							-1);
	}

	/* Enable window */
	gdk_window_set_cursor (GTK_WIDGET (lists->dialog)->window, NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (lists->dialog), TRUE);
}

void
fwitux_friends_lists_dialog_show (GtkWindow *parent)
{
	GtkBuilder *ui;
	GList      *users;

	if (friends_lists) {
		gtk_window_present (GTK_WINDOW (friends_lists->dialog));
		return;
	}

	friends_lists = g_new0 (FwituxLists, 1);

	/* Get widgets */
	ui = fwitux_xml_get_file (XML_FILE,
						"lists_dialog", &friends_lists->dialog,
						"lists_view", &friends_lists->view,
                        "btn_remove_relationship", &friends_lists->btn_remove,
						NULL);

	friends_lists->store = gtk_tree_view_get_model (friends_lists->view);

	/* Connect the signals */
	fwitux_xml_connect (ui, friends_lists,
						"lists_dialog", "destroy", lists_destroy_cb,
						"lists_dialog", "response", lists_response_cb,
						"btn_remove_relationship", "clicked", lists_remove_friend_response_cb,
						"lists_view", "row-activated", list_follower_activated_cb,
						NULL);

	g_object_unref (ui);

	/* Set the parent */
	g_object_add_weak_pointer (G_OBJECT (friends_lists->dialog), (gpointer) &friends_lists);
	gtk_window_set_transient_for (GTK_WINDOW (friends_lists->dialog), parent);

	/* Now that we're done setting up, let's show the widget */
	gtk_widget_show (friends_lists->dialog);

	/* Load lists */
	users = fwitux_network_get_friends ();
    if (users){
        fwitux_lists_dialog_load_lists (friends_lists, users);
    } else {
        GdkCursor *cursor;
        /* Disable window while retrieving lists */
        cursor = gdk_cursor_new (GDK_WATCH);
        gdk_window_set_cursor (GTK_WIDGET (friends_lists->dialog)->window, cursor);
        gtk_widget_set_sensitive (friends_lists->dialog, FALSE);
    }
}

void
fwitux_followed_lists_dialog_show (GtkWindow *parent)
{
	GtkBuilder *ui;
	GList      *users;

	if (followed_lists) {
		gtk_window_present (GTK_WINDOW (followed_lists->dialog));
		return;
	}

	followed_lists = g_new0 (FwituxLists, 1);

	/* Get widgets */
	ui = fwitux_xml_get_file (XML_FILE,
						"lists_dialog", &followed_lists->dialog,
						"lists_view", &followed_lists->view,
                        "btn_remove_relationship", &followed_lists->btn_remove,
						NULL);

	followed_lists->store = gtk_tree_view_get_model (followed_lists->view);

	/* Connect the signals */
	fwitux_xml_connect (ui, followed_lists,
						"lists_dialog", "destroy", lists_destroy_cb,
						"lists_dialog", "response", lists_response_cb,
						"lists_view", "row-activated", list_follower_activated_cb,
						NULL);

	g_object_unref (ui);

	/* Set the parent */
	g_object_add_weak_pointer (G_OBJECT (followed_lists->dialog), (gpointer) &followed_lists);
	gtk_window_set_transient_for (GTK_WINDOW (followed_lists->dialog), parent);

    /* hide the remove button */
    gtk_widget_hide (GTK_WIDGET (followed_lists->btn_remove));

	/* Now that we're done setting up, let's show the widget */
	gtk_widget_show (followed_lists->dialog);

	/* Load lists */
	users = fwitux_network_get_followed ();
    if (users){
        fwitux_lists_dialog_load_lists (followed_lists, users);
    } else {
        GdkCursor *cursor;
        /* Disable window while retrieving lists */
        cursor = gdk_cursor_new (GDK_WATCH);
        gdk_window_set_cursor (GTK_WIDGET (followed_lists->dialog)->window, cursor);
        gtk_widget_set_sensitive (followed_lists->dialog, FALSE);
    }
}

void
fwitux_followers_lists_dialog_show (GtkWindow *parent)
{
	GtkBuilder *ui;
	GList      *users;

	if (followers_lists) {
		gtk_window_present (GTK_WINDOW (followers_lists->dialog));
		return;
	}

	followers_lists = g_new0 (FwituxLists, 1);

	/* Get widgets */
	ui = fwitux_xml_get_file (XML_FILE,
						"lists_dialog", &followers_lists->dialog,
						"lists_view", &followers_lists->view,
                        "btn_remove_relationship", &followers_lists->btn_remove,
						NULL);

	followers_lists->store = gtk_tree_view_get_model (followers_lists->view);

	/* Connect the signals */
	fwitux_xml_connect (ui, followers_lists,
						"lists_dialog", "destroy", lists_destroy_cb,
						"lists_dialog", "response", lists_response_cb,
						"btn_remove_relationship", "clicked", lists_remove_follower_response_cb,
						"lists_view", "row-activated", list_follower_activated_cb,
						NULL);

	g_object_unref (ui);

	/* Set the parent */
	g_object_add_weak_pointer (G_OBJECT (followers_lists->dialog), (gpointer) &followers_lists);
	gtk_window_set_transient_for (GTK_WINDOW (followers_lists->dialog), parent);

	/* Now that we're done setting up, let's show the widget */
	gtk_widget_show (followers_lists->dialog);

	/* Load lists */
	users = fwitux_network_get_followers ();
    if (users){
        fwitux_lists_dialog_load_lists (followers_lists, users);
    } else {
        GdkCursor *cursor;
        /* Disable window while retrieving lists */
        cursor = gdk_cursor_new (GDK_WATCH);
        gdk_window_set_cursor (GTK_WIDGET (followers_lists->dialog)->window, cursor);
        gtk_widget_set_sensitive (followers_lists->dialog, FALSE);
    }
}

