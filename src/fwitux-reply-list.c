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

#include <glib/gi18n.h>

#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-debug.h>

#include "fwitux.h"
#include "fwitux-reply-list.h"
#include "fwitux-message-dialog.h"
#include "fwitux-label.h"
#include "fwitux-network.h"

#define DEBUG_DOMAIN "ReplyList"

enum 
{
    LEFT_BUTTON = 1,    //left mouse button
    MIDDLE_BUTTON,      //middle mouse button
    RIGHT_BUTTON        //right mouse button
};

#define GET_PRIV(obj)           \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_REPLY_LIST, FwituxReplyListPriv))

struct _FwituxReplyListPriv {
	GtkListStore      *store;
	GtkTreeViewColumn *text_column;
	GtkCellRenderer   *text_renderer;

    gchar             *status_id;

    guint             press_button;
    guint32           press_time;
};

static void   fwitux_reply_list_class_init        (FwituxReplyListClass *klass);
static void   fwitux_reply_list_init              (FwituxReplyList      *reply_list);

static void   reply_list_create_model      (FwituxReplyList      *list);
static void   reply_list_create_view        (FwituxReplyList      *list);

static void   reply_list_reply_message     (const gchar *reply_status_id, const gchar * reply_user_name);

static void   reply_list_finalize          (GObject              *obj);

static void   reply_list_size_cb           (GtkWidget            *widget,
                                            GtkAllocation        *allocation,
                                            gpointer              user_data);
static void   reply_list_changed_cb        (GtkWidget            *widget,
                                            gpointer              user_data);

static gboolean reply_list_button_press_cb(GtkWidget * widget,
                                            GdkEventButton * event,
                                            gpointer user_data);

static void reply_reply_cb(GtkMenuItem *menu_item, gpointer user_data);
static void reply_delete_cb(GtkMenuItem *menu_item, gpointer user_data);


static FwituxReplyList *list = NULL;

G_DEFINE_TYPE (FwituxReplyList, fwitux_reply_list, GTK_TYPE_TREE_VIEW);

static void
fwitux_reply_list_class_init (FwituxReplyListClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = reply_list_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxReplyListPriv));
}

static void
fwitux_reply_list_init (FwituxReplyList *reply_list)
{
	FwituxReplyListPriv *priv;

	list = reply_list;

	priv = GET_PRIV (list);

    /* init */
    priv->status_id = NULL;

	reply_list_create_model (list);
	reply_list_create_view (list);

	g_signal_connect (reply_list,
					  "size_allocate",
					  G_CALLBACK (reply_list_size_cb),
					  reply_list);
	g_signal_connect (reply_list,
					  "cursor-changed",
					  G_CALLBACK (reply_list_changed_cb),
					  reply_list);
	g_signal_connect (reply_list,
					  "button-press-event",
					  G_CALLBACK (reply_list_button_press_cb),
					  reply_list);
}

static void
reply_list_finalize (GObject *object)
{
	FwituxReplyListPriv *priv;

	priv = GET_PRIV (object);

    /* clean */
    if(priv->status_id)
        g_free (priv->status_id);

    if(priv->store)
        g_object_unref (priv->store);

	G_OBJECT_CLASS (fwitux_reply_list_parent_class)->finalize (object);
}

static void
reply_list_create_model (FwituxReplyList *list)
{
	FwituxReplyListPriv *priv;
	GtkTreeModel        *model;

	priv = GET_PRIV (list);

	if (priv->store) {
		g_object_unref (priv->store);
	}

	priv->store =
		gtk_list_store_new (N_REPLY_COLUMNS,
							GDK_TYPE_PIXBUF,  /* User photo image */
							G_TYPE_STRING,    /* Display string */
							G_TYPE_STRING,    /* User name string */
							G_TYPE_STRING,    /* Status created date string */
							G_TYPE_STRING,    /* Status text string */
							G_TYPE_STRING,    /* Status id string */
                            G_TYPE_STRING,    /* User id string */
                            G_TYPE_STRING);    /* Status source type */

	/* save normal model */
	model = GTK_TREE_MODEL (priv->store);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
}

static void
reply_list_create_view (FwituxReplyList *list)
{
	FwituxReplyListPriv *priv;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*avatar_column;
	GtkTreeViewColumn   *reply_column;

	priv = GET_PRIV (list);
	
	g_object_set (list,
				  "rules-hint", TRUE,
				  "reorderable", FALSE,
				  "headers-visible", FALSE,
				  NULL);

    /* create user photo column */
	renderer = gtk_cell_renderer_pixbuf_new ();
	avatar_column = gtk_tree_view_column_new_with_attributes (NULL,
												  renderer,
												  "pixbuf", PIXBUF_USER_PHOTO,
												  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), avatar_column);

    /* create formated tweet column */
	renderer = gtk_cell_renderer_text_new ();
	reply_column = gtk_tree_view_column_new_with_attributes (NULL,
												  renderer,
												  "markup", STRING_FORMATED_TWEET,
												  NULL);
	gtk_tree_view_column_set_sizing (reply_column, GTK_TREE_VIEW_COLUMN_FIXED);
	g_object_set (renderer,
				  "ypad", 0,
				  "xpad", 5,
				  "yalign", 0.0,
				  "wrap-mode", PANGO_WRAP_WORD_CHAR,
				  NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), reply_column);
	
	priv->text_column = reply_column;
	priv->text_renderer = renderer;
}

static void
reply_list_changed_cb (GtkWidget *widget,
                       gpointer   user_data)
{
	FwituxReplyList     *t;
	FwituxReplyListPriv *priv;

    GtkTreeView         *tree_view;
    GtkTreeSelection    *tree_selection;
	GtkTreeIter          iter;

	gchar               *user_name;
    gchar               *login_username;

    GtkMenu             *popup_menu;
    GtkImageMenuItem    *menu_item;
    GtkImage            *item_image;
	FwituxConf          *conf;


    tree_view = GTK_TREE_VIEW(widget);
    t = FWITUX_REPLY_LIST (user_data);
    priv = GET_PRIV (t);

    if (priv->press_button == MIDDLE_BUTTON)
    {
        /* reply the status */

        /* get the selected status id */
        tree_selection = gtk_tree_view_get_selection(tree_view);
        if(! gtk_tree_selection_get_selected(tree_selection, NULL, &iter) )
            return;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                            &iter,
                            STRING_USER_NAME, &user_name,
                            -1);

        /* diaplay the reply message dialog */
        reply_list_reply_message(priv->status_id, user_name);

        g_free (user_name);
    }
    else if (priv->press_button == RIGHT_BUTTON)
    {
        /* get the current status's author */
        tree_selection = gtk_tree_view_get_selection(tree_view);
        if( !gtk_tree_selection_get_selected(tree_selection, NULL, &iter) )
            return;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                            &iter,
                            STRING_USER_NAME, &user_name,
                            -1);

        /* get the current login user name */
        conf = fwitux_conf_get ();
		fwitux_conf_get_string (conf,
								FWITUX_PREFS_AUTH_USER,
								&login_username);

        /* create the right button menu */
        popup_menu = GTK_MENU( gtk_menu_new() );

        /* create the reply menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("Reply") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image( menu_item, GTK_WIDGET( item_image ) );
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(reply_reply_cb), list);
        gtk_widget_set_sensitive(GTK_WIDGET (menu_item), !g_str_equal(user_name, login_username));
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));

        /* create the delete menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("Delete") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image(menu_item, GTK_WIDGET( item_image ));
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(reply_delete_cb), list);
        gtk_widget_set_sensitive(GTK_WIDGET (menu_item), g_str_equal(user_name, login_username));
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        //gtk_widget_show(GTK_WIDGET (menu_item));
        gtk_widget_hide(GTK_WIDGET (menu_item));

        /* free */
        g_free(login_username);
        g_free(user_name);

        /* popup the right button menu */
        gtk_widget_show(GTK_WIDGET(popup_menu));
        gtk_menu_popup(popup_menu,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       priv->press_button,
                       priv->press_time);
    }
}

static void
reply_list_size_cb (GtkWidget     *widget,
                    GtkAllocation *allocation,
                    gpointer       user_data)
{
	FwituxReplyList     *t;
	FwituxReplyListPriv *priv;
	gint                 w;

	t = FWITUX_REPLY_LIST (user_data);	
	priv = GET_PRIV (t);

	w = gtk_tree_view_column_get_width (priv->text_column);

	g_object_set (priv->text_renderer,
				  "wrap-width", w-10,
				  NULL);
}

static gboolean
reply_list_button_press_cb(GtkWidget * widget,
                           GdkEventButton * event,
                           gpointer user_data)
{

	FwituxReplyList     *t;
	FwituxReplyListPriv *priv;

	t = FWITUX_REPLY_LIST (user_data);
	priv = GET_PRIV (t);

    priv->press_button = event->button;
    priv->press_time = event->time;

    return FALSE;
}

/* reply status menu item handler */
static void
reply_reply_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxReplyList     *t;
	FwituxReplyListPriv *priv;
    gchar               *user_name;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_REPLY_LIST (user_data);
	priv = GET_PRIV (t);

    /* get the selected status id */
    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_USER_NAME, &user_name,
						-1);

    /* display the reply message dialog */
    reply_list_reply_message(priv->status_id, user_name);

    g_free (user_name);
}

/* display the reply message dialog */
static void
reply_list_reply_message(const gchar *reply_status_id, const gchar * reply_user_name)
{
    gchar      *message;

	fwitux_message_dialog_show (NULL);
	fwitux_message_dialog_show_friends (FALSE);
    fwitux_message_dialog_set_reply_status_id(reply_status_id);
    fwitux_message_dialog_set_source_type(REPLY_WINDOW_SOURCE);
    fwitux_message_dialog_set_caption( _("Reply Message") );

    message = g_strconcat(_("Reply"), reply_user_name, " ", NULL);
    fwitux_message_dialog_set_message(message);
    g_free (message);
}

/* delete status menu item handler */
static void
reply_delete_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxReplyList     *t;
	FwituxReplyListPriv *priv;
    gchar               *reply_id;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_REPLY_LIST (user_data);
	priv = GET_PRIV (t);

    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_STATUS_ID, &reply_id,
						-1);

    fwitux_network_delete_status(reply_id, REPLY_WINDOW_SOURCE);

    g_free (reply_id);

    /* delete the status from reply list */
    gtk_list_store_remove(priv->store, &iter);
}


FwituxReplyList *
fwitux_reply_list_new (void)
{
	return g_object_new (FWITUX_TYPE_REPLY_LIST, NULL);
}

void
fwitux_reply_list_set_status_id (const gchar *status_id)
{
    FwituxReplyListPriv    *priv;

    priv = GET_PRIV (list);

    /* set status id */
    if (priv->status_id)
        g_free (priv->status_id);

    priv->status_id = g_strdup(status_id);
}


GtkListStore *
fwitux_reply_list_get_store (void)
{
	FwituxReplyListPriv *priv;
	
	priv = GET_PRIV (list);
	
	return priv->store;
}

