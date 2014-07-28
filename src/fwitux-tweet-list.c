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
#include "fwitux-tweet-list.h"
#include "fwitux-message-dialog.h"
#include "fwitux-label.h"
#include "fwitux-app.h"
#include "fwitux-replies-window.h"
#include "fwitux-network.h"

#define DEBUG_DOMAIN "TweetList"

enum 
{
    LEFT_BUTTON = 1,    //left mouse button
    MIDDLE_BUTTON,      //middle mouse button
    RIGHT_BUTTON        //right mouse button
};

#define GET_PRIV(obj)           \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_TWEET_LIST, FwituxTweetListPriv))

struct _FwituxTweetListPriv {
	GtkListStore      *store;
	GtkTreeViewColumn *text_column;
	GtkCellRenderer   *text_renderer;

    guint press_button;
    guint32 press_time;
};

static void   fwitux_tweet_list_class_init        (FwituxTweetListClass *klass);
static void   fwitux_tweet_list_init              (FwituxTweetList      *tweet);

static void   tweet_list_create_model      (FwituxTweetList      *list);
static void   tweet_list_setup_view        (FwituxTweetList      *list);
static void   tweet_list_reply_message     (const gchar *status_id);

static void   tweet_list_finalize          (GObject              *obj);

static void   tweet_list_size_cb           (GtkWidget            *widget,
                                            GtkAllocation        *allocation,
                                            gpointer              user_data);
static void   tweet_list_changed_cb        (GtkWidget            *widget,
                                            gpointer              user_data);
static void   tweet_list_activated_cb      (GtkTreeView          *tree_view,
                                            GtkTreePath          *path,
                                            GtkTreeViewColumn    *column,
                                            gpointer              user_data);
static gboolean tweet_list_button_press_cb(GtkWidget * widget,
                                            GdkEventButton * event,
                                            gpointer user_data);
static void tweet_message_user_cb(GtkMenuItem *menu_item, gpointer user_data);
static void tweet_reply_cb(GtkMenuItem *menu_item, gpointer user_data);
static void tweet_forward_cb(GtkMenuItem *menu_item, gpointer user_data);
static void tweet_view_replies_cb(GtkMenuItem *menu_item, gpointer user_data);
static void tweet_delete_cb(GtkMenuItem *menu_item, gpointer user_data);

static FwituxTweetList *list = NULL;

G_DEFINE_TYPE (FwituxTweetList, fwitux_tweet_list, GTK_TYPE_TREE_VIEW);

static void
fwitux_tweet_list_class_init (FwituxTweetListClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tweet_list_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxTweetListPriv));
}

static void
fwitux_tweet_list_init (FwituxTweetList *tweet)
{
	FwituxTweetListPriv *priv;

	list = tweet;

	priv = GET_PRIV (list);

	tweet_list_create_model (list);
	tweet_list_setup_view (list);

	g_signal_connect (tweet,
					  "size_allocate",
					  G_CALLBACK (tweet_list_size_cb),
					  tweet);
	g_signal_connect (tweet,
					  "cursor-changed",
					  G_CALLBACK (tweet_list_changed_cb),
					  tweet);
	g_signal_connect (tweet,
					  "row-activated",
					  G_CALLBACK (tweet_list_activated_cb),
					  tweet);
	g_signal_connect (tweet,
					  "button-press-event",
					  G_CALLBACK (tweet_list_button_press_cb),
					  tweet);
}

static void
tweet_list_finalize (GObject *object)
{
	FwituxTweetListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->store);

	G_OBJECT_CLASS (fwitux_tweet_list_parent_class)->finalize (object);
}

static void
tweet_list_create_model (FwituxTweetList *list)
{
	FwituxTweetListPriv *priv;
	GtkTreeModel        *model;

	priv = GET_PRIV (list);

	if (priv->store) {
		g_object_unref (priv->store);
	}

	priv->store =
		gtk_list_store_new (N_COLUMNS,
							GDK_TYPE_PIXBUF,  /* User photo image */
							G_TYPE_STRING,    /* Display string */
							G_TYPE_STRING,    /* User name string */
							G_TYPE_STRING,    /* Status created date string */
							G_TYPE_STRING,    /* Status text string */
							G_TYPE_STRING,    /* Status id string */
                            G_TYPE_STRING,    /* User id string */
                            G_TYPE_STRING,    /* Status source type */
                            G_TYPE_STRING,    /* Status link url */
                            G_TYPE_STRING);   /* Status image address */
	/* save normal model */
	model = GTK_TREE_MODEL (priv->store);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
}

static void
tweet_list_setup_view (FwituxTweetList *list)
{
	FwituxTweetListPriv *priv;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*avatar_column;
	GtkTreeViewColumn   *tweet_column;

	priv = GET_PRIV (list);
	
	g_object_set (list,
				  "rules-hint", TRUE,
				  "reorderable", FALSE,
				  "headers-visible", FALSE,
				  NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	avatar_column =
		gtk_tree_view_column_new_with_attributes (NULL,
												  renderer,
												  "pixbuf", PIXBUF_USER_PHOTO,
												  NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), avatar_column);

	renderer = gtk_cell_renderer_text_new ();
	tweet_column =
		gtk_tree_view_column_new_with_attributes (NULL,
												  renderer,
												  "markup", STRING_FORMATED_TWEET,
												  NULL);
	gtk_tree_view_column_set_sizing (tweet_column,
									 GTK_TREE_VIEW_COLUMN_FIXED);
	g_object_set (renderer,
				  "ypad", 0,
				  "xpad", 5,
				  "yalign", 0.0,
				  "wrap-mode", PANGO_WRAP_WORD_CHAR,
				  NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), tweet_column);
	
	priv->text_column = tweet_column;
	priv->text_renderer = renderer;
}

static void
tweet_list_changed_cb (GtkWidget *widget,
                       gpointer   user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;

    GtkTreeView         *tree_view;
    GtkTreeSelection    *tree_selection;
	GtkTreeIter          iter;
	GdkPixbuf           *pixbuf;

	gchar               *user_name, *status_text, *status_created;
    gchar               *status_id, *status_source, *status_link, *status_image_addr;
	gchar               *label;
    gchar               *login_username;

	gboolean             expand;

    GtkMenu             *popup_menu;
    GtkImageMenuItem    *menu_item;
    GtkImage            *item_image;
	FwituxConf          *conf;


    tree_view = GTK_TREE_VIEW(widget);
    t = FWITUX_TWEET_LIST (user_data);
    priv = GET_PRIV (t);

    if (priv->press_button == LEFT_BUTTON)
    {
        /* get the expand flag */
        conf = fwitux_conf_get();
        fwitux_conf_get_bool (conf,
                              FWITUX_PREFS_UI_EXPAND_MESSAGES,
                              &expand);

        if (!expand)
            return;
        
        /* Get selected Iter */
        tree_selection = gtk_tree_view_get_selection (tree_view);
        if (!gtk_tree_selection_get_selected (tree_selection, NULL, &iter))
            return;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                            &iter,
                            STRING_USER_NAME, &user_name,
                            STRING_STATUS_TEXT, &status_text,
                            STRING_STATUS_CREATED, &status_created,
                            PIXBUF_USER_PHOTO, &pixbuf,
                            STRING_STATUS_SOURCE, &status_source,
                            STRING_STATUS_LINK, &status_link,
                            STRING_STATUS_IMAGE_ADDRESS, &status_image_addr,
                            -1);

        fwitux_app_show_expand_message (user_name, status_created, 
                                 status_source, status_text, 
                                 status_link, status_image_addr, pixbuf);

        g_free (user_name);
        g_free (status_text);
        g_free (status_created);
        g_free (status_source);
        g_free (status_link);
        g_free (status_image_addr);
    }
    else if (priv->press_button == MIDDLE_BUTTON)
    {
        /* reply the status */

        /* get the selected status id */
        tree_selection = gtk_tree_view_get_selection(tree_view);
        if(! gtk_tree_selection_get_selected(tree_selection, NULL, &iter) )
            return;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                            &iter,
                            STRING_STATUS_ID, &status_id,
                            -1);

        /* diaplay the reply message dialog */
        tweet_list_reply_message(status_id);

        g_free (status_id);
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

        /* create the replies view menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("View Replies") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image(menu_item, GTK_WIDGET(item_image));
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(tweet_view_replies_cb), list);
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));

        /* create the @username menu item */
        label = g_strdup_printf("@%s", user_name);
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label(label) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image(menu_item, GTK_WIDGET( item_image ));
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(tweet_message_user_cb), list);
        gtk_widget_set_sensitive(GTK_WIDGET (menu_item), !g_str_equal(user_name, login_username));
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));
        g_free(label);

        /* create the reply menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("Reply") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image( menu_item, GTK_WIDGET( item_image ) );
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(tweet_reply_cb), list);
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));

        /* create the forward menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("Forward") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image(menu_item, GTK_WIDGET( item_image ));
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(tweet_forward_cb), list);
        gtk_widget_set_sensitive(GTK_WIDGET (menu_item), !g_str_equal(user_name, login_username));
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));

        /* create the delete menu item */
        menu_item = GTK_IMAGE_MENU_ITEM( gtk_image_menu_item_new_with_label( _("Delete") ) );
        item_image = GTK_IMAGE( gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_MENU) );
        gtk_image_menu_item_set_image(menu_item, GTK_WIDGET( item_image ));
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(tweet_delete_cb), list);
        gtk_widget_set_sensitive(GTK_WIDGET (menu_item), g_str_equal(user_name, login_username));
        gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menu_item));
        gtk_widget_show(GTK_WIDGET (menu_item));

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
tweet_list_size_cb (GtkWidget     *widget,
                    GtkAllocation *allocation,
                    gpointer       user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
	gint                 w;

	t = FWITUX_TWEET_LIST (user_data);	
	priv = GET_PRIV (t);

	w = gtk_tree_view_column_get_width (priv->text_column);

	g_object_set (priv->text_renderer,
				  "wrap-width", w-10,
				  NULL);
}

static void
tweet_list_activated_cb (GtkTreeView       *tree_view,
                         GtkTreePath       *path,
                         GtkTreeViewColumn *column,
                         gpointer           user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
	gchar               *status_id;
	GtkTreeIter          iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store),
							 &iter,
							 path);

	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_STATUS_ID, &status_id,
						-1);

    fwitux_replies_window_show(status_id);

	g_free (status_id);
}

static gboolean
tweet_list_button_press_cb(GtkWidget * widget,
                           GdkEventButton * event,
                           gpointer user_data)
{

	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    priv->press_button = event->button;
    priv->press_time = event->time;

    return FALSE;
}

/* @username menu item handler */
static void
tweet_message_user_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
    gchar               *message;
    gchar               *user_name;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_USER_NAME, &user_name,
						-1);

	message = g_strdup_printf ("@%s ", user_name);
	fwitux_message_dialog_show (NULL);
	fwitux_message_dialog_show_friends (FALSE);
	fwitux_message_dialog_set_message (message);

    g_free (user_name);
	g_free (message);
}

/* reply status menu item handler */
static void
tweet_reply_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
    gchar               *status_id;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    /* get the selected status id */
    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_STATUS_ID, &status_id,
						-1);

    /* display the reply message dialog */
    tweet_list_reply_message(status_id);

    g_free (status_id);
}

/* display the reply message dialog */
static void
tweet_list_reply_message(const gchar *status_id)
{
	fwitux_message_dialog_show (NULL);
	fwitux_message_dialog_show_friends (FALSE);
    fwitux_message_dialog_set_reply_status_id(status_id);
    fwitux_message_dialog_set_source_type(MAIN_WINDOW_SOURCE);
    fwitux_message_dialog_set_caption( _("Reply Message") );
}

/* forward status menu item handler */
static void
tweet_forward_cb(GtkMenuItem *menu_item, gpointer user_data)
{
   	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
    gchar               *message;
    gchar               *user_name;
    gchar               *status_text;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_USER_NAME, &user_name,
                        STRING_STATUS_TEXT, &status_text,
						-1);

	message = g_strdup_printf ( _("Forward from %s: %s"), user_name, status_text );
	fwitux_message_dialog_show (NULL);
	fwitux_message_dialog_show_friends (FALSE);
	fwitux_message_dialog_set_message (message);
    fwitux_message_dialog_set_caption( _("Forward Message") );

    g_free (user_name);
	g_free (message);
    g_free (status_text);
}

/* view replies menu item handler */
static void
tweet_view_replies_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
    gchar               *status_id;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_STATUS_ID, &status_id,
						-1);

    
    fwitux_replies_window_show (status_id);

    g_free (status_id);
}

/* delete status menu item handler */
static void
tweet_delete_cb(GtkMenuItem *menu_item, gpointer user_data)
{
	FwituxTweetList     *t;
	FwituxTweetListPriv *priv;
    gchar               *status_id;
    GtkTreeSelection    *tree_selection;
    GtkTreeIter         iter;

	t = FWITUX_TWEET_LIST (user_data);
	priv = GET_PRIV (t);

    tree_selection = gtk_tree_view_get_selection( GTK_TREE_VIEW (t) );
    gtk_tree_selection_get_selected(tree_selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
						&iter,
						STRING_STATUS_ID, &status_id,
						-1);

    fwitux_network_delete_status(status_id, MAIN_WINDOW_SOURCE);

    g_free (status_id);

    /* delete the status from tweet list */
    gtk_list_store_remove(priv->store, &iter);
}


FwituxTweetList *
fwitux_tweet_list_new (void)
{
	return g_object_new (FWITUX_TYPE_TWEET_LIST, NULL);
}

GtkListStore *
fwitux_tweet_list_get_store (void)
{
	FwituxTweetListPriv *priv;
	
	priv = GET_PRIV (list);
	
	return priv->store;
}
