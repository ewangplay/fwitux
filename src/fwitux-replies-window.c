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

#include <config.h>
#include <sys/stat.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-paths.h>
#include <libfwitux/fwitux-xml.h>

#include "fwitux.h"
#include "fwitux-replies-window.h"
#include "fwitux-label.h"
#include "fwitux-network.h"
#include "fwitux-ui-utils.h"
#include "fwitux-reply-list.h"

#define GET_PRIV(obj)           \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_REPLIES_WINDOW, FwituxRepliesWindowPriv))

#define DEBUG_DOMAIN_SETUP       "RepliesWindowSetup"

struct _FwituxRepliesWindowPriv {
	/* Main widgets */
	GtkWidget         *window;
    GtkWidget         *scrolled_window;
	FwituxReplyList   *listview;
	GtkWidget         *statusbar;

	/* Expand messages widgets */
	GtkWidget         *expand_box;
	GtkWidget         *expand_image;
	GtkWidget         *expand_title;
	GtkWidget         *expand_label;
};

static void	    fwitux_replies_window_class_init		 (FwituxRepliesWindowClass *klass);
static void     fwitux_replies_window_init			     (FwituxRepliesWindow *replies_window);
static void     replies_window_finalize                  (GObject               *object);
static void     replies_window_reset_geometry            (GtkWidget             *main_window);
static void     replies_window_setup                     (const gchar *status_id);
static void     replies_window_destroy_cb                (GtkWidget             *window,
												             FwituxRepliesWindow   *replies_window_loc);

static FwituxRepliesWindow  *replies_window = NULL;

G_DEFINE_TYPE (FwituxRepliesWindow, fwitux_replies_window, G_TYPE_OBJECT);

static void
fwitux_replies_window_class_init (FwituxRepliesWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = replies_window_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxRepliesWindowPriv));
}

static void
fwitux_replies_window_init (FwituxRepliesWindow *singleton_replies_window)
{
	FwituxRepliesWindowPriv *priv;

	replies_window = singleton_replies_window;

	priv = GET_PRIV (replies_window);
}

static void
replies_window_finalize (GObject *object)
{
	FwituxRepliesWindow	       *replies_window;
	FwituxRepliesWindowPriv      *priv;	
	
	replies_window = FWITUX_REPLIES_WINDOW (object);
	priv = GET_PRIV (replies_window);

	G_OBJECT_CLASS (fwitux_replies_window_parent_class)->finalize (object);
}

static void
replies_window_reset_geometry (GtkWidget *main_window)
{
	FwituxConf *conf;
	gint        x, y, w, h;
    GdkScreen  *screen;
    gint        screen_width, screen_height;

    /* get main window geometry from gconf */
	fwitux_debug (DEBUG_DOMAIN_SETUP, "Loading window geometry...");

	conf = fwitux_conf_get ();

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WINDOW_HEIGHT,
						 &h);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WINDOW_WIDTH,
						 &w);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_X,
						 &x);

	fwitux_conf_get_int (conf,
						 FWITUX_PREFS_UI_WIN_POS_Y,
						 &y);


    /* get screen geometry */
    screen = gdk_screen_get_default();
    screen_width = gdk_screen_get_width(screen);
    screen_height = gdk_screen_get_height(screen);

    /* set the reply list window */
	if (w >=1 && h >= 1) {
		/*
		 * Use the defaults from the glade file
		 * if we don't have good w, h geometry.
		 */
		 fwitux_debug (DEBUG_DOMAIN_SETUP,
					   "Configuring window default size w:%d, h: %d", w, h);
		 //gtk_window_resize (GTK_WINDOW (main_window), w, h);
         gtk_widget_set_size_request(main_window, w, h);
	}

	if (x >= 0 && y >= 0) {
		/*
		 * Let the window manager position it
		 * if we don't have good x, y coordinates.
		 */
		fwitux_debug (DEBUG_DOMAIN_SETUP,
					  "Configuring window default position x:%d, y:%d", x, y);
        if (x + w < screen_width) {
            gtk_window_move (GTK_WINDOW (main_window), x + w, y);
        }
        else if (x - w > 0) {
            gtk_window_move (GTK_WINDOW (main_window), x - w, y);
        }
	}
}

static void
replies_window_setup (const gchar *status_id)
{
	FwituxRepliesWindowPriv    *priv;
	GtkBuilder                 *ui;
	GtkWidget                  *expand_vbox;

	fwitux_debug (DEBUG_DOMAIN_SETUP, "Beginning....");

	priv = GET_PRIV (replies_window);

	/* Set up interface */
	fwitux_debug (DEBUG_DOMAIN_SETUP, "Initialising interface");
	ui = fwitux_xml_get_file ("replies_window.xml",
							  "replies_window", &priv->window,
							  "replies_scrolledwindow", &priv->scrolled_window,
							  "replies_statusbar", &priv->statusbar,
							  "expand_box", &priv->expand_box,
							  "expand_vbox", &expand_vbox,
							  "expand_image", &priv->expand_image,
							  "expand_title", &priv->expand_title,
							  NULL);

	/* Connect the signals */
	fwitux_xml_connect (ui, replies_window,
						"replies_window", "destroy", replies_window_destroy_cb,
						NULL);

	/* release ui */
	g_object_unref (ui);

	/* Let's hide the replies window, while we are setting up the ui */
	gtk_widget_hide (GTK_WIDGET (priv->window));

    /* set the window title */
    gtk_window_set_title(GTK_WINDOW (priv->window), _("Reply List"));

	/* Set-up list view */
	priv->listview = fwitux_reply_list_new ();
	gtk_widget_show (GTK_WIDGET (priv->listview));
	gtk_container_add (GTK_CONTAINER (priv->scrolled_window),
					   GTK_WIDGET (priv->listview));

	/* Set-up expand messages panel */
	priv->expand_label = fwitux_label_new ();
	gtk_widget_show (GTK_WIDGET (priv->expand_label));
	gtk_box_pack_end (GTK_BOX (expand_vbox),
					   GTK_WIDGET (priv->expand_label),
					   TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET (priv->expand_box));

    /* get reply timeline */
	fwitux_network_get_reply_timeline (status_id);

	/* Set the replies window geometry */ 	 
	replies_window_reset_geometry (priv->window);

	/* Show the replies window  */				  
    gtk_widget_show (priv->window);
}

static void
replies_window_destroy_cb (GtkWidget *window, FwituxRepliesWindow *replies_window_loc)
{
    /* clean the reply resource */
    fwitux_network_remove_reply_source();

    g_object_unref(replies_window);
    replies_window = NULL;
}

void
fwitux_replies_window_show (const gchar *status_id)
{
    FwituxRepliesWindowPriv    *priv;

    if(replies_window) {
        priv = GET_PRIV (replies_window);

        /* load the new reply timeline */
        fwitux_network_get_reply_timeline (status_id);

        /* reset the window geometry */
        replies_window_reset_geometry (priv->window);

        /* raise the window on the desktop */
        gtk_window_present(GTK_WINDOW (priv->window));

        return;
    }

	g_object_new (FWITUX_TYPE_REPLIES_WINDOW, NULL);

	replies_window_setup (status_id);
}

void
fwitux_replies_window_set_statusbar_msg (const gchar *message)
{
	FwituxRepliesWindowPriv *priv;

	priv = GET_PRIV (replies_window);

	/* Avoid some warnings */
	if (!priv->statusbar || !GTK_IS_STATUSBAR (priv->statusbar))
		return;

	/* conext ID will be always 1 */
	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar), 1);
	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar), 1, message);
}

void
fwitux_replies_window_set_image (const gchar *file,
                      GtkTreeIter  iter)
{
	GtkListStore *store;
	GdkPixbuf	 *pixbuf;
	GError		 *error = NULL;

	pixbuf = gdk_pixbuf_new_from_file (file, &error);

	if (!pixbuf){
		fwitux_debug (DEBUG_DOMAIN_SETUP, "Image error: %s: %s",
					  file, error->message);
		g_error_free (error);
		return;
	}
	
	store = fwitux_reply_list_get_store ();

	gtk_list_store_set (store, &iter,
						PIXBUF_USER_PHOTO, pixbuf, -1);
}

void
fwitux_replies_window_set_expand_message (const gchar *name,
                           const gchar *date,
                           const gchar *source,
                           const gchar *reply_count,
                           const gchar *tweet,
                           const gchar *link,
                           const gchar *image_addr,
                           const gchar *user_image_url)
{
	FwituxRepliesWindowPriv *priv;
	gchar 		  *title_text;
    gchar         *tweet_text;
    const gchar   *image_name;
    gchar         *image_file;
	
	priv = GET_PRIV (replies_window);

    /* build title text */
    title_text = g_strconcat("<b>", name, "</b> - ",
                             "<small>", date, "</small> - ",
                             "<small>", source, "</small> - ", 
                             "<small>", _("Reply"), "(<b>", reply_count, "</b>)</small>", NULL);

    /* build body text */
    if ((link && strlen(link) > 0) && (image_addr && strlen(image_addr) > 0)) {
        tweet_text = g_strconcat(tweet, "\n",
                                 "<i> ", link, " </i>\n ",
                                 "<i> ", image_addr, " </i>",
                                 NULL);
    } else if (link && strlen(link) > 0) {
        tweet_text = g_strconcat(tweet, "\n ",
                                 "<i> ", link, " </i>",
                                 NULL);
    } else if (image_addr && strlen(image_addr) > 0) {
        tweet_text = g_strconcat(tweet, "\n ",
                                 "<i> ", image_addr, " </i>",
                                 NULL);
    } else {
        tweet_text = g_strdup(tweet);
    }
		
    /* set text */
	fwitux_label_set_text (FWITUX_LABEL (priv->expand_label), tweet_text);
	gtk_label_set_markup (GTK_LABEL (priv->expand_title), title_text);

    /* free */
	g_free (title_text);
    g_free (tweet_text);
	
    /* set user image */
	image_name = strrchr (user_image_url, '/');
	if (image_name && image_name[1] != '\0') {
		image_name++;
	} else {
		image_name = user_image_url;
	}

	image_file = g_build_filename (g_get_home_dir(), ".gnome2",
								   FWITUX_CACHE_IMAGES,
								   image_name, NULL);

	if (g_file_test (image_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {		
        gtk_image_set_from_file(GTK_IMAGE(priv->expand_image), image_file);
	}

    g_free (image_file);
}

