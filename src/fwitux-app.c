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

#include <canberra-gtk.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-paths.h>
#include <libfwitux/fwitux-xml.h>
#ifdef HAVE_GNOME_KEYRING
#include <libfwitux/fwitux-keyring.h>
#endif

#include "fwitux.h"
#include "fwitux-about.h"
#include "fwitux-account-dialog.h"
#include "fwitux-app.h"
#include "fwitux-geometry.h"
#include "fwitux-hint.h"
#include "fwitux-label.h"
#include "fwitux-network.h"
#include "fwitux-preferences.h"
#include "fwitux-message-dialog.h"
#include "fwitux-lists-dialog.h"
#include "fwitux-ui-utils.h"
#include "fwitux-tweet-list.h"

#ifdef HAVE_DBUS
#include "fwitux-dbus.h"
#include <dbus/dbus-glib.h>
#endif

#define GET_PRIV(obj)           \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_APP, FwituxAppPriv))

#define DEBUG_DOMAIN_SETUP       "AppSetup"
#define DEBUG_QUIT

#define TYPE_FOLLOW5 "follow5"

struct _FwituxAppPriv {
	/* Main widgets */
	GtkWidget         *window;
	FwituxTweetList   *listview;
	GtkWidget         *statusbar;

	/*
	 * Widgets that are enabled when
	 * we are connected/disconnected
	 */
	GList             *widgets_connected;
	GList             *widgets_disconnected;

	/* Timeline menu items */
	GSList            *group;
	GtkRadioAction    *menu_public;
	GtkRadioAction    *menu_friends;
	GtkRadioAction    *menu_mine;
	GtkRadioAction    *menu_private;
    GtkRadioAction    *menu_mention;

	GtkAction         *view_friends;
	GtkAction         *view_followed;
	GtkAction         *view_followers;

	/* Status Icon */
	GtkStatusIcon     *status_icon;

	/* Status Icon Popup Menu */
	GtkWidget         *popup_menu;
	GtkToggleAction   *popup_menu_show_app;

	/* Account related data */
	DBusGProxy        *accounts_service;
    DBusGProxy        *account;
	char              *username;
	char              *password;

	/* Misc */
	guint              size_timeout_id;
	
	/* Expand messages widgets */
	GtkWidget         *expand_box;
	GtkWidget         *expand_image;
	GtkWidget         *expand_title;
	GtkWidget         *expand_label;
};

static void	    fwitux_app_class_init			 (FwituxAppClass        *klass);
static void     fwitux_app_init			         (FwituxApp             *app);
static void     app_finalize                     (GObject               *object);
static void     restore_main_window_geometry     (GtkWidget             *main_window);
static void     on_accounts_destroy              (DBusGProxy            *proxy, 
                                                  FwituxApp             *app);
static void     disconnect                       (FwituxApp             *app);
static void     reconnect                        (FwituxApp             *app);
static gboolean update_account                   (DBusGProxy            *account,
                                                  FwituxApp             *app,
                                                  GError **error);
static void     on_account_changed               (DBusGProxy *account,
					                              FwituxApp *app);
static void     on_account_disabled              (DBusGProxy *accounts,
					                              const char *opath,
					                              FwituxApp *app);
static void     on_account_enabled               (DBusGProxy *accounts,
					                              const char *opath,
					                              FwituxApp *app);
static void     app_setup                        (void);
static void     main_window_destroy_cb           (GtkWidget             *window,
												  FwituxApp             *app);
static gboolean main_window_delete_event_cb      (GtkWidget             *window,
												  GdkEvent              *event,
												  FwituxApp             *app);
static void     app_set_radio_group              (FwituxApp             *app,
												  GtkBuilder            *ui);
static void     app_connect_cb                   (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_disconnect_cb                (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_new_message_cb               (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_send_direct_message_cb       (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_quit_cb                      (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_refresh_cb                   (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_account_cb                   (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_preferences_cb               (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_public_timeline_cb           (GtkRadioAction        *action,
												  GtkRadioAction        *current,
												  FwituxApp             *app);
static void     app_friends_timeline_cb          (GtkRadioAction        *action,
												  GtkRadioAction        *current,
												  FwituxApp             *app);
static void     app_mine_timeline_cb             (GtkRadioAction        *action,
												  GtkRadioAction        *current,
												  FwituxApp             *app);
static void     app_private_friend_cb           (GtkRadioAction        *action,
												  GtkRadioAction        *current,
												  FwituxApp             *app);
static void     app_mention_me_cb               (GtkRadioAction *action,
                                                  GtkRadioAction *current,
                                                  FwituxApp *app);
static void     app_fwitux_view_friends_cb       (GtkAction             *action,
												  FwituxApp             *app);
static void     app_fwitux_view_followed_cb       (GtkAction             *action,
												  FwituxApp             *app);
static void     app_fwitux_view_followers_cb       (GtkAction             *action,
												  FwituxApp             *app);
static void     app_about_cb                     (GtkWidget             *window,
												  FwituxApp             *app);
static void     app_help_contents_cb             (GtkWidget             *widget,
												  FwituxApp             *app);
static void     app_status_icon_activate_cb      (GtkStatusIcon         *status_icon,
												  FwituxApp             *app);
static void     app_status_icon_popup_menu_cb    (GtkStatusIcon         *status_icon,
												  guint                  button,
												  guint                  activate_time,
												  FwituxApp             *app);
static gboolean request_accounts                 (FwituxApp *a, GError **error);
static void     app_connection_items_setup       (FwituxApp             *app,
												  GtkBuilder            *ui);
static void     app_login                        (FwituxApp             *app);
static void     app_set_default_timeline         (FwituxApp             *app,
												  gchar                 *timeline);
static void     app_retrieve_default_timeline    (void);
static void     app_status_icon_create_menu      (void);
static void     app_status_icon_create           (void);
static void     app_check_dir                    (void);
static void     fwitux_app_toggle_visibility     (void);
static gboolean configure_event_timeout_cb       (GtkWidget             *widget);
static gboolean app_window_configure_event_cb    (GtkWidget             *widget,
												  GdkEventConfigure     *event,
												  FwituxApp             *app);

static FwituxApp  *app = NULL;

G_DEFINE_TYPE (FwituxApp, fwitux_app, G_TYPE_OBJECT);

static void
fwitux_app_class_init (FwituxAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = app_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxAppPriv));
}

static void
fwitux_app_init (FwituxApp *singleton_app)
{
	FwituxAppPriv *priv;

	app = singleton_app;

	priv = GET_PRIV (app);

	priv->widgets_connected = NULL;
	priv->widgets_disconnected = NULL;
	priv->group = NULL;
}

static void
app_finalize (GObject *object)
{
	FwituxApp	       *app;
	FwituxAppPriv      *priv;	
	
	app = FWITUX_APP (object);
	priv = GET_PRIV (app);

	if (priv->size_timeout_id) {
		g_source_remove (priv->size_timeout_id);
	}

	g_list_free (priv->widgets_connected);
	g_list_free (priv->widgets_disconnected);
	g_slist_free (priv->group);

#ifdef HAVE_DBUS
	fwitux_dbus_nm_finalize ();
#endif

	fwitux_conf_shutdown ();
	
	G_OBJECT_CLASS (fwitux_app_parent_class)->finalize (object);
}

static void
restore_main_window_geometry (GtkWidget *main_window)
{
	fwitux_geometry_load_for_main_window (main_window);
}

static void
on_accounts_destroy (DBusGProxy *proxy, FwituxApp *app)
{
	FwituxAppPriv *priv = GET_PRIV (app);
    priv->account = NULL;
	priv->accounts_service = NULL;
}

static void
disconnect (FwituxApp *app)
{
	GtkListStore *store = fwitux_tweet_list_get_store ();
	gtk_list_store_clear (store);
	fwitux_network_logout ();
	fwitux_app_state_on_connection (FALSE);
}

static void
reconnect (FwituxApp *app)
{
	FwituxConf *conf;
	gboolean login;

	disconnect (app);
	conf = fwitux_conf_get ();

	/*Check to see if we should automatically login */
	fwitux_conf_get_bool (conf,
						  FWITUX_PREFS_AUTH_AUTO_LOGIN,
						  &login);

	if (!login)
		return;

	app_login (app);
}

static gboolean
update_account (DBusGProxy  *account,
				FwituxApp   *app,
				GError     **error)
{
	FwituxAppPriv *priv;
	gchar         *username;
	gchar         *password;


	priv = GET_PRIV (app);

    /* the account is changing */
    if (priv->account && strcmp (g_strdup (dbus_g_proxy_get_path (account)),
								 g_strdup (dbus_g_proxy_get_path (priv->account))) != 0) {
	    g_object_unref (priv->account);
        priv->account = NULL;
    }
  
    if (!priv->account) {
        priv->account = account;
        dbus_g_proxy_add_signal (priv->account,
								 "Changed",
								 G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (priv->account,
									 "Changed",
									 G_CALLBACK (on_account_changed),
									 app, NULL);
    }

    /* The username should normally never change on an existing account, but let's update
       it in any case. Since we are only interested in the enabled account,
       and are connected to an account disabled signal, we don't need to check the 
       enabled flag here. */		
    if (!dbus_g_proxy_call (priv->account,
						    "GetUsername",
							error,
							G_TYPE_INVALID,
							G_TYPE_STRING,
							&username,
							G_TYPE_INVALID))
        return FALSE;

	if (!dbus_g_proxy_call (priv->account,
							"GetPassword",
							error,
							G_TYPE_INVALID,
							G_TYPE_STRING,
							&password,
							G_TYPE_INVALID))
        return FALSE;

    priv->username = username;
	priv->password = password;
    return TRUE;
}               

static void
on_account_changed (DBusGProxy *account,
					FwituxApp  *app)
{
	FwituxAppPriv *priv;
	GError        *error = NULL;

	priv = GET_PRIV (app);

    /* we shouldn't be getting a signal about an account that is no longer current, 
       but let's do these checks just in case */
    if (!priv->account || strcmp (g_strdup (dbus_g_proxy_get_path (account)),
								  g_strdup (dbus_g_proxy_get_path (priv->account))) != 0)
		return;

    if (!update_account(account, app, &error)) {
        g_printerr ("failed to update an account that changed: %s", error->message);
        return; 
    }

	reconnect (app);
}

static void
on_account_disabled (DBusGProxy *accounts,
					 const char *opath,
					 FwituxApp  *app)
{
	FwituxAppPriv *priv;
	GError        *error = NULL;

	priv = GET_PRIV (app);

	if (!priv->account || strcmp (opath, g_strdup (dbus_g_proxy_get_path (priv->account))) != 0)
		return;

	if (!request_accounts (app, &error)) {
		g_warning ("Failed to get accounts: %s", error->message);
		g_clear_error (&error);
		return;
	}

	reconnect (app);
}

static void
on_account_enabled (DBusGProxy *accounts,
					const char *opath,
					FwituxApp  *app)
{
	GError *error = NULL;
	FwituxAppPriv *priv;
	priv = GET_PRIV (app);
	    
    DBusGProxy *account;

    account = dbus_g_proxy_new_for_name_owner (dbus_g_bus_get (DBUS_BUS_SESSION, NULL),
											   "org.gnome.OnlineAccounts",
											   opath,
											   "org.gnome.OnlineAccounts", 
                                               &error);

    if (!account) {
        g_warning ("Could not get an account object for opath: %s",
				   opath);
		g_clear_error (&error);
		return;
	}

    if (!update_account(account, app, &error)) {
        g_printerr ("failed to update an account that got enabled: %s", error->message);
        return; 
    }

	reconnect (app);
}
	
static void
app_setup (void)
{
	FwituxAppPriv    *priv;
	FwituxConf		 *conf;
	GtkBuilder       *ui;
	GtkWidget        *scrolled_window;
	GtkWidget        *expand_vbox;
	gchar            *timeline;
	gboolean          login;
	gboolean		  hidden;

    GError           *error = NULL;
    guint32           result;
    DBusGProxy       *session_bus;

	fwitux_debug (DEBUG_DOMAIN_SETUP, "Beginning....");

	priv = GET_PRIV (app);

	/* Set up interface */
	fwitux_debug (DEBUG_DOMAIN_SETUP, "Initialising interface");
	ui = fwitux_xml_get_file ("main_window.xml",
							  "main_window", &priv->window,
							  "main_scrolledwindow", &scrolled_window,
							  "main_statusbar", &priv->statusbar,
							  "view_public_timeline", &priv->menu_public,
							  "view_friends_timeline", &priv->menu_friends,
							  "view_my_timeline", &priv->menu_mine,
							  "view_private_friend", &priv->menu_private,
							  "view_mention_me", &priv->menu_mention,
							  "view_friends", &priv->view_friends,
							  "view_followed", &priv->view_followed,
							  "view_followers", &priv->view_followers,
							  "expand_box", &priv->expand_box,
							  "expand_vbox", &expand_vbox,
							  "expand_image", &priv->expand_image,
							  "expand_title", &priv->expand_title,
							  NULL);

	/* Set group for menu radio actions */
	app_set_radio_group (app, ui);

    /* Grab the conf object */
	conf = fwitux_conf_get ();

	/*
	 * Set the default timeline.  This needs
	 * to be done before connecting signals.
	 */
	fwitux_conf_get_string (conf,
							FWITUX_PREFS_TWEETS_HOME_TIMELINE,
							&timeline);
	app_set_default_timeline (app, timeline);
	g_free (timeline);

	/* Connect the signals */
	fwitux_xml_connect (ui, app,
						"main_window", "destroy", main_window_destroy_cb,
						"main_window", "delete_event", main_window_delete_event_cb,
						"main_window", "configure_event", app_window_configure_event_cb,
						"follow5_connect", "activate", app_connect_cb,
						"follow5_disconnect", "activate", app_disconnect_cb,
						"follow5_new_message", "activate", app_new_message_cb,
						"follow5_send_direct_message", "activate", app_send_direct_message_cb,
						"follow5_refresh", "activate", app_refresh_cb,
						"follow5_quit", "activate", app_quit_cb,
						"settings_account", "activate", app_account_cb,
						"settings_preferences", "activate", app_preferences_cb,
						"view_public_timeline", "changed", app_public_timeline_cb,
						"view_friends_timeline", "changed", app_friends_timeline_cb,
						"view_my_timeline", "changed", app_mine_timeline_cb,
						"view_private_friend", "changed", app_private_friend_cb,
						"view_mention_me", "changed", app_mention_me_cb,
						"view_friends", "activate", app_fwitux_view_friends_cb,
						"view_followed", "activate", app_fwitux_view_followed_cb,
						"view_followers", "activate", app_fwitux_view_followers_cb,
						"help_contents", "activate", app_help_contents_cb,
						"help_about", "activate", app_about_cb,
						NULL);

	/* Set up connected related widgets */
	app_connection_items_setup (app, ui);
	g_object_unref (ui);

	/* Let's hide the main window, while we are setting up the ui */
	gtk_widget_hide (GTK_WIDGET (priv->window));

#ifdef HAVE_DBUS
	/* Initialize NM */
	fwitux_dbus_nm_init ();
#endif

    priv->accounts_service = NULL;

    session_bus = dbus_g_proxy_new_for_name (dbus_g_bus_get (DBUS_BUS_SESSION, NULL),
                                             "org.freedesktop.DBus",
                                             "/org/freedesktop/DBus",
                                             "org.freedesktop.DBus");

    if (dbus_g_proxy_call (session_bus, "StartServiceByName", &error,
                           G_TYPE_STRING, "org.gnome.OnlineAccounts", 
                           G_TYPE_UINT, 0, G_TYPE_INVALID, 
                           G_TYPE_UINT, &result, G_TYPE_INVALID)) {
	    priv->accounts_service =
			dbus_g_proxy_new_for_name (dbus_g_bus_get (DBUS_BUS_SESSION, NULL),
									   "org.gnome.OnlineAccounts",
									   "/onlineaccounts",
									   "org.gnome.OnlineAccounts");
    }

	if (priv->accounts_service) {
		g_signal_connect (priv->accounts_service, "destroy",
						  G_CALLBACK (on_accounts_destroy), app); 
	    dbus_g_proxy_call (priv->accounts_service,
						   "EnsureAccountType",
						   &error,
						   G_TYPE_STRING, TYPE_FOLLOW5,
                           G_TYPE_STRING, "Follow5",
                           G_TYPE_STRING, "Follow5 username",
                           G_TYPE_STRING, "http://follow5.com",
						   G_TYPE_INVALID);
		dbus_g_proxy_add_signal (priv->accounts_service,
								 "AccountDisabled",
								 DBUS_TYPE_G_OBJECT_PATH,
								 G_TYPE_INVALID);
		dbus_g_proxy_add_signal (priv->accounts_service,
								 "AccountEnabled",
								 DBUS_TYPE_G_OBJECT_PATH,
								 G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (priv->accounts_service,
									 "AccountDisabled",
									 G_CALLBACK (on_account_disabled),
									 app, NULL);
		dbus_g_proxy_connect_signal (priv->accounts_service,
									 "AccountEnabled",
									 G_CALLBACK (on_account_enabled),
									 app, NULL);
	}

	/* Set-up the notification area */
	fwitux_debug (DEBUG_DOMAIN_SETUP,
				  "Configuring notification area widget...");
	app_status_icon_create_menu ();
	app_status_icon_create ();
	
	/* Set the main window geometry */ 	 
	restore_main_window_geometry (priv->window);

	/* Set-up list view */
	priv->listview = fwitux_tweet_list_new ();
	gtk_widget_show (GTK_WIDGET (priv->listview));
	gtk_container_add (GTK_CONTAINER (scrolled_window),
					   GTK_WIDGET (priv->listview));

	/* Set-up expand messages panel */
	priv->expand_label = fwitux_label_new ();
	gtk_widget_show (GTK_WIDGET (priv->expand_label));
	gtk_box_pack_end (GTK_BOX (expand_vbox),
					   GTK_WIDGET (priv->expand_label),
					   TRUE, TRUE, 0);
	gtk_widget_hide (GTK_WIDGET (priv->expand_box));

	/* Initial status of widgets */
	fwitux_app_state_on_connection (FALSE);

	/* Check Fwitux directory and images cache */
	app_check_dir ();
	
	/* Get the gconf value for whether the window should be hidden on start-up */
	fwitux_conf_get_bool (conf,
						  FWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
						  &hidden);
	
	/* Ok, set the window state based on the gconf value */				  
	if (hidden) {
		gtk_widget_hide (priv->window);
	} else {
		gtk_widget_show (priv->window);
	}

	/*Check to see if we should automatically login */
	fwitux_conf_get_bool (conf,
						  FWITUX_PREFS_AUTH_AUTO_LOGIN,
						  &login);

	if (login) 
		app_login (app);
}

static void
main_window_destroy_cb (GtkWidget *window, FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	/* Add any clean-up code here */

#ifdef DEBUG_QUIT
	gtk_main_quit ();
#else
	exit (0);
#endif
}

static gboolean
main_window_delete_event_cb (GtkWidget *window,
							 GdkEvent  *event,
							 FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (gtk_status_icon_is_embedded (priv->status_icon)) {
		fwitux_hint_show (FWITUX_PREFS_HINTS_CLOSE_MAIN_WINDOW,
						  _("Fwitux is still running, it is just hidden."),
						  _("Click on the notification area icon to show Fwitux."),
						   GTK_WINDOW (fwitux_app_get_window ()),
						   NULL, NULL);
		
		fwitux_app_set_visibility (FALSE);

		return TRUE;
	}
	
	if (fwitux_hint_dialog_show (FWITUX_PREFS_HINTS_CLOSE_MAIN_WINDOW,
								_("You were about to quit!"),
								_("Since no system or notification tray has been "
								"found, this action would normally quit Fwitux.\n\n"
								"This is just a reminder, from now on, Fwitux will "
								"quit when performing this action unless you uncheck "
								"the option below."),
								GTK_WINDOW (fwitux_app_get_window ()),
								NULL, NULL)) {
		/* Shown, we don't quit because the callback will
		 * decide that based on the YES|NO response from the
		 * question we are about to ask, since this behaviour
		 * is new.
		 */
		return TRUE;
	}

	/* At this point, we have checked we have:
	 *   - No tray
	 *   - Have NOT shown the hint
	 * So we just quit.
	 */

	return FALSE;
}

static void
app_set_radio_group (FwituxApp  *app,
					 GtkBuilder *ui)
{
	FwituxAppPriv  *priv;
	GtkRadioAction *w;
	gint            i;

	const gchar     *radio_actions[] = {
		"view_public_timeline",
		"view_friends_timeline",
		"view_my_timeline",
		"view_private_friend",
        "view_mention_me"
	};

	priv = GET_PRIV (app);

	for (i = 0; i < G_N_ELEMENTS (radio_actions); i++) {
		w = GTK_RADIO_ACTION (gtk_builder_get_object (ui, radio_actions[i]));
		gtk_radio_action_set_group (w, priv->group);
		priv->group = gtk_radio_action_get_group (w);
	}
}

static void
fwitux_app_toggle_visibility (void)
{
	FwituxAppPriv *priv;
	gboolean       visible;

	priv = GET_PRIV (app);

	visible = fwitux_window_get_is_visible (GTK_WINDOW (priv->window));

	if (visible && gtk_status_icon_is_embedded (priv->status_icon)) {
		gint x, y, w, h;

		gtk_window_get_size (GTK_WINDOW (priv->window), &w, &h);
		gtk_window_get_position (GTK_WINDOW (priv->window), &x, &y);
		gtk_widget_hide (priv->window);

		fwitux_geometry_save_for_main_window (x, y, w, h);

		if (priv->size_timeout_id) {
			g_source_remove (priv->size_timeout_id);
			priv->size_timeout_id = 0;
		}
	} else {
		fwitux_geometry_load_for_main_window (priv->window);
		fwitux_window_present (GTK_WINDOW (priv->window), TRUE);
	}
	/* Save the window visibility state */
	fwitux_conf_set_bool (fwitux_conf_get (),
						  FWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
						  visible);
}

void
fwitux_app_set_visibility (gboolean visible)
{
	GtkWidget *window;

	window = fwitux_app_get_window ();

	fwitux_conf_set_bool (fwitux_conf_get (),
						  FWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
						  !visible);

	if (visible) {
		fwitux_window_present (GTK_WINDOW (window), TRUE);
	} else {
		gtk_widget_hide (window);
	}
}

static void
app_connect_cb (GtkWidget *widget,
				FwituxApp *app)
{
	app_login (app);
}

static void
app_disconnect_cb (GtkWidget *widget,
				   FwituxApp *app)
{
	disconnect (app);
}

static void
app_new_message_cb (GtkWidget *widget,
					FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_message_dialog_show (GTK_WINDOW (priv->window));
	fwitux_message_dialog_show_friends (FALSE);
}

static void
app_send_direct_message_cb (GtkWidget *widget,
							FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_message_dialog_show (GTK_WINDOW (priv->window));
	fwitux_message_dialog_show_friends (TRUE);
}

static void
app_quit_cb (GtkWidget  *widget,
			 FwituxApp  *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	gtk_widget_destroy (priv->window);
}

static void
app_refresh_cb (GtkWidget *window,
				FwituxApp *app)
{
	fwitux_network_refresh ();
}

static void
app_public_timeline_cb (GtkRadioAction *action,
						GtkRadioAction *current,
						FwituxApp      *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->menu_public == current)
    {
		fwitux_network_get_timeline (FWITUX_API_TIMELINE_PUBLIC, NULL);
    }
}

static void
app_friends_timeline_cb (GtkRadioAction *action,
						 GtkRadioAction *current,
						 FwituxApp      *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->menu_friends == current) 
    {
		fwitux_network_get_timeline (FWITUX_API_TIMELINE_FRIENDS, NULL);
    }
}

static void
app_mine_timeline_cb (GtkRadioAction *action,
					  GtkRadioAction *current,
					  FwituxApp      *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->menu_mine == current)
    {
		fwitux_network_get_timeline (FWITUX_API_TIMELINE_USER, NULL);
    }
}

static void
app_private_friend_cb (GtkRadioAction *action, 	 
						GtkRadioAction *current,
						FwituxApp      *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->menu_private == current)
    {
		fwitux_network_get_timeline (FWITUX_API_PRIVATE_FRIEND, NULL);
    }
}

static void
app_mention_me_cb (GtkRadioAction *action,
                   GtkRadioAction *current,
                   FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->menu_mention == current)
    {
		fwitux_network_get_timeline (FWITUX_API_MENTIONS_ME, NULL);
    }
}

static void
app_fwitux_view_friends_cb (GtkAction *action,
							FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_friends_lists_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_fwitux_view_followed_cb (GtkAction *action,
							FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_followed_lists_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_fwitux_view_followers_cb (GtkAction *action,
							FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_followers_lists_dialog_show (GTK_WINDOW (priv->window));
}

static char**
get_account_set_request (FwituxApp *app)
{
	static const char* follow5[2] = { TYPE_FOLLOW5, NULL };

	return (char **)follow5;
}

static void
app_account_cb (GtkWidget *widget,
				FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->accounts_service) {
		dbus_g_proxy_call_no_reply (priv->accounts_service,
									"OpenAccountsDialogWithTypes",
									G_TYPE_STRV, 
									get_account_set_request (app),
                                    G_TYPE_UINT, 
                                    gtk_get_current_event_time(),   
									G_TYPE_INVALID);
	} else {
		fwitux_account_dialog_show (GTK_WINDOW (priv->window));
	}
}

static void
app_preferences_cb (GtkWidget *widget,
					FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_preferences_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_about_cb (GtkWidget *widget,
			  FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_about_dialog_new (GTK_WINDOW (priv->window));
}

static void
app_help_contents_cb (GtkWidget *widget,
					  FwituxApp *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	fwitux_help_show (GTK_WINDOW (priv->window));
}

static void
app_show_hide_cb (GtkWidget *widget,
				  FwituxApp *app)
{
	fwitux_app_toggle_visibility ();
}

static void
app_status_icon_activate_cb (GtkStatusIcon *status_icon,
							 FwituxApp     *app)
{
	fwitux_app_toggle_visibility ();
}

static void
app_status_icon_popup_menu_cb (GtkStatusIcon *status_icon,
							   guint          button,
							   guint          activate_time,
							   FwituxApp     *app)
{
	FwituxAppPriv *priv;
	gboolean       show;

	priv = GET_PRIV (app);

	show = fwitux_window_get_is_visible (GTK_WINDOW (priv->window));

	g_signal_handlers_block_by_func (priv->popup_menu_show_app,
									 app_show_hide_cb, app);

	gtk_toggle_action_set_active (priv->popup_menu_show_app, show);

	g_signal_handlers_unblock_by_func (priv->popup_menu_show_app,
									   app_show_hide_cb, app);

	gtk_menu_popup (GTK_MENU (priv->popup_menu),
					NULL, NULL,
					gtk_status_icon_position_menu,
					priv->status_icon,
					button,
					activate_time);
}

static void
app_status_icon_create_menu (void)
{
	FwituxAppPriv   *priv;
	GtkAction       *new_msg;
	GtkAction       *quit;
	GtkWidget       *w;

	priv = GET_PRIV (app);

	priv->popup_menu_show_app = gtk_toggle_action_new ("tray_show_app",
													   _("_Show Fwitux"),
													   NULL,
													   NULL);
	g_signal_connect (G_OBJECT (priv->popup_menu_show_app),
					  "toggled", G_CALLBACK (app_show_hide_cb),
					  app);

	new_msg = gtk_action_new ("tray_new_message",
							  _("_New Message"),
							  NULL,
							  "gtk-new");
	g_signal_connect (G_OBJECT (new_msg),
					  "activate", G_CALLBACK (app_new_message_cb),
					  app);

	quit = gtk_action_new ("tray_quit",
						   _("_Quit"),
						   NULL,
						   "gtk-quit");
	g_signal_connect (G_OBJECT (quit),
					  "activate", G_CALLBACK (app_quit_cb),
					  app);

	priv->popup_menu = gtk_menu_new ();
	w = gtk_action_create_menu_item (GTK_ACTION (priv->popup_menu_show_app));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), w);
	w = gtk_separator_menu_item_new ();
	gtk_widget_show (w);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), w);
	w = gtk_action_create_menu_item (new_msg);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), w);
	w = gtk_action_create_menu_item (quit);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), w);
}

static void
app_status_icon_create (void)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	priv->status_icon = gtk_status_icon_new_from_icon_name ("fwitux");
	g_signal_connect (priv->status_icon,
					  "activate",
					  G_CALLBACK (app_status_icon_activate_cb),
					  app);

	g_signal_connect (priv->status_icon,
					  "popup_menu",
					  G_CALLBACK (app_status_icon_popup_menu_cb),
					  app);

	gtk_status_icon_set_visible (priv->status_icon, TRUE);
}

void
fwitux_app_create (void)
{
	g_object_new (FWITUX_TYPE_APP, NULL);

	app_setup ();
}

FwituxApp *
fwitux_app_get (void)
{
	g_assert (app != NULL);
	
	return app;
}
 
static gboolean
configure_event_timeout_cb (GtkWidget *widget)
{
	FwituxAppPriv *priv;
	gint           x, y, w, h;

	priv = GET_PRIV (app);

	gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
	gtk_window_get_position (GTK_WINDOW (widget), &x, &y);

	fwitux_geometry_save_for_main_window (x, y, w, h);

	priv->size_timeout_id = 0;

	return FALSE;
}

static gboolean
app_window_configure_event_cb (GtkWidget         *widget,
							   GdkEventConfigure *event,
							   FwituxApp         *app)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->size_timeout_id) {
		g_source_remove (priv->size_timeout_id);
	}

	priv->size_timeout_id =
		g_timeout_add (500,
					   (GSourceFunc) configure_event_timeout_cb,
					   widget);

	return FALSE;
}

static gboolean
request_accounts (FwituxApp  *app,
				  GError    **error)
{
	FwituxAppPriv  *priv;
	guint           i;
	GPtrArray      *accounts = NULL;
	char          **accountset;
    gboolean        succeeded = TRUE;

	priv = GET_PRIV (app);

	accountset = get_account_set_request (app);

	if (!G_STR_EMPTY (priv->username)){
		g_free (priv->username);
	}
	priv->username = NULL;

	if (!G_STR_EMPTY (priv->password)) {
		g_free (priv->password);
	}
	priv->password = NULL;

    if (priv->account) 
        g_object_unref (priv->account);
    priv->account = NULL;
 
	if (!dbus_g_proxy_call (priv->accounts_service,
							"GetEnabledAccountsWithTypes",
							error,
							G_TYPE_STRV,
							accountset,
							G_TYPE_INVALID,
							dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_PROXY),
							&accounts,
							G_TYPE_INVALID))
		return FALSE;

	/* We only use the first account */
	for (i = 0; i < accounts->len && i == 0; i++) {
        succeeded = update_account((DBusGProxy*)g_ptr_array_index (accounts, i), app, error);
	}
	return succeeded;
}

static void
request_username_password (FwituxApp *a)
{
	FwituxAppPriv *priv;
	FwituxConf    *conf;

	priv = GET_PRIV (a);

	conf = fwitux_conf_get ();

	if (!priv->accounts_service) {
		g_free (priv->username);
		priv->username = NULL;
		fwitux_conf_get_string (conf,
								FWITUX_PREFS_AUTH_USER,
								&priv->username);
		g_free (priv->password);
#ifdef HAVE_GNOME_KEYRING
		priv->password = NULL;
		if (G_STR_EMPTY (priv->username)) {
			priv->password = NULL;
		} else {
			if (!(fwitux_keyring_get_password (priv->username, &priv->password))) {
				priv->password = NULL;
			}
		}
#else
		fwitux_conf_get_string (conf,
								FWITUX_PREFS_AUTH_PASSWORD,
								&priv->password);
#endif
	} else {
		GError *error = NULL;
		if (!request_accounts (a, &error)) {
			g_printerr ("failed to get accounts: %s", error->message);
		}
	}
}

static void
app_login (FwituxApp *a)
{
	FwituxAppPriv *priv;

#ifdef HAVE_DBUS
	gboolean connected = TRUE;

	/*
	 * Don't try to connect if we have
	 * Network Manager state and we are NOT connected.
	 */
	if (fwitux_dbus_nm_get_state (&connected) && !connected) {
		return;
	}
#endif
	
	priv = GET_PRIV (a);

	request_username_password (a);

	if (G_STR_EMPTY (priv->username) || G_STR_EMPTY (priv->password)) {
		app_account_cb (NULL, a);
	} else {
		fwitux_network_login (priv->username, priv->password);
		app_retrieve_default_timeline ();
	}
}

/*
 * Function to set the default
 * timeline in the menu.
 */
static void
app_set_default_timeline (FwituxApp *app, gchar *timeline)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	/* This shouldn't happen, but just in case */
	if (G_STR_EMPTY (timeline)) {
		g_warning ("Default timeline in not set");
		return;
	}

	if (strcmp (timeline, FWITUX_API_TIMELINE_FRIENDS) == 0) {
		gtk_radio_action_set_current_value (priv->menu_friends,	1);
	} else if (strcmp (timeline, FWITUX_API_TIMELINE_PUBLIC) == 0) {
		gtk_radio_action_set_current_value (priv->menu_public, 1);
	} else if (strcmp (timeline, FWITUX_API_TIMELINE_USER) == 0) {
		gtk_radio_action_set_current_value (priv->menu_mine, 1);
	} else if (strcmp (timeline, FWITUX_API_PRIVATE_FRIEND) == 0) {
		gtk_radio_action_set_current_value (priv->menu_private, 1);
    } else if (strcmp (timeline, FWITUX_API_MENTIONS_ME) == 0) {
        gtk_radio_action_set_current_value (priv->menu_mention, 1);
	} else {
		/* Let's fallback to friends timeline */
		gtk_radio_action_set_current_value (priv->menu_friends,	1);
	}
}

/* Function to retrieve the users default timeline */
static void
app_retrieve_default_timeline (void)
{
	FwituxAppPriv *priv;
	gchar         *timeline;

	priv = GET_PRIV (app);

	fwitux_conf_get_string (fwitux_conf_get (),
							FWITUX_PREFS_TWEETS_HOME_TIMELINE,
							&timeline);

	if (G_STR_EMPTY (timeline)){
		timeline = g_strdup (FWITUX_API_TIMELINE_FRIENDS);
		app_set_default_timeline (app, FWITUX_API_TIMELINE_FRIENDS);
	}

	fwitux_network_get_timeline (timeline, NULL);

	g_free (timeline);
}

static void
app_check_dir (void)
{
	gchar    *file;

	file = g_build_filename (g_get_home_dir (), ".gnome2", FWITUX_CACHE_IMAGES, NULL);

	if (!g_file_test (file, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR)) {
		fwitux_debug (DEBUG_DOMAIN_SETUP, "Making directory: %s", file);
		g_mkdir_with_parents (file, S_IRUSR|S_IWUSR|S_IXUSR);
	}

	g_free (file);
}

static void
app_connection_items_setup (FwituxApp  *app,
							GtkBuilder *ui)
{
	FwituxAppPriv *priv;
	GList         *list;
	GObject       *w;
	gint           i;

	const gchar   *widgets_connected[] = {
		"follow5_disconnect",
		"follow5_new_message",
		"follow5_send_direct_message",
		"follow5_refresh",
		"view1"
	};

	const gchar   *widgets_disconnected[] = {
		"follow5_connect"
	};

	priv = GET_PRIV (app);

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_connected); i++) {
		w = gtk_builder_get_object (ui, widgets_connected[i]);
		list = g_list_prepend (list, w);
	}

	priv->widgets_connected = list;

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_disconnected); i++) {
		w = gtk_builder_get_object (ui, widgets_disconnected[i]);
		list = g_list_prepend (list, w);
	}

	priv->widgets_disconnected = list;
}

void
fwitux_app_state_on_connection (gboolean connected)
{
	FwituxAppPriv *priv;
	GList         *l;

	priv = GET_PRIV (app);

	for (l = priv->widgets_connected; l; l = l->next) {
		g_object_set (l->data, "sensitive", connected, NULL);
	}

	for (l = priv->widgets_disconnected; l; l = l->next) {
		g_object_set (l->data, "sensitive", !connected, NULL);
	}
}

GtkWidget *
fwitux_app_get_window (void)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);
	
	return priv->window;
}

void
fwitux_app_set_statusbar_msg (const gchar *message)
{
	FwituxAppPriv *priv;

	priv = GET_PRIV (app);

	/* Avoid some warnings */
	if (!priv->statusbar || !GTK_IS_STATUSBAR (priv->statusbar))
		return;

	/* conext ID will be always 1 */
	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar), 1);
	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar), 1, message);
}

void
fwitux_app_notify_sound (void)
{
	gboolean sound;

	fwitux_conf_get_bool (fwitux_conf_get (),
						  FWITUX_PREFS_UI_SOUND,
						  &sound);

	if (sound) {
		ca_context_play (ca_gtk_context_get (),
						 0,
						 CA_PROP_APPLICATION_NAME, g_get_application_name (),
						 CA_PROP_EVENT_ID, "message-new-instant",
						 CA_PROP_EVENT_DESCRIPTION, _("New tweet received"),
						 NULL);
	}
}

void
fwitux_app_notify (gchar *msg)
{
	gboolean notify;

	fwitux_conf_get_bool (fwitux_conf_get (),
						  FWITUX_PREFS_UI_NOTIFICATION,
						  &notify);

	if (notify) {
		NotifyNotification *notification;
		GError             *error = NULL;

		notification = notify_notification_new (PACKAGE_NAME,
												msg,
												"fwitux",
												NULL);

		notify_notification_set_timeout (notification, 8 * 1000);
		notify_notification_show (notification, &error);

		if (error) {
			fwitux_debug (DEBUG_DOMAIN_SETUP,
						  "Error displaying notification: %s",
						  error->message);
			g_error_free (error);
		}
		g_object_unref (G_OBJECT (notification));
	}
}

void
fwitux_app_set_image (const gchar *file,
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
	
	store = fwitux_tweet_list_get_store ();

	gtk_list_store_set (store, &iter,
						PIXBUF_USER_PHOTO, pixbuf, -1);
}

void
fwitux_app_show_expand_message (const gchar *name,
                           const gchar *date,
                           const gchar *source,
                           const gchar *tweet,
                           const gchar *link,
                           const gchar *image_addr,
                           GdkPixbuf   *pixbuf)
{
	FwituxAppPriv *priv;
	gchar 		  *title_text;
    gchar         *tweet_text;
	
	priv = GET_PRIV (app);

	title_text = g_strdup_printf ("<b>%s</b> - %s - %s", name, date, source);

    if ((link && strlen(link) > 0) && (image_addr && strlen(image_addr) > 0)) {
        tweet_text = g_strdup_printf ("%s\n %s\n %s\n", tweet, link, image_addr);
    } else if (link && strlen(link) > 0) {
        tweet_text = g_strdup_printf ("%s\n %s\n", tweet, link);
    } else if (image_addr && strlen(image_addr) > 0) {
        tweet_text = g_strdup_printf ("%s\n %s\n", tweet, image_addr);
    } else {
        tweet_text = g_strdup_printf ("%s\n", tweet);
    }
		
	fwitux_label_set_text (FWITUX_LABEL (priv->expand_label), tweet_text);
	gtk_label_set_markup (GTK_LABEL (priv->expand_title), title_text);
	g_free (title_text);
    g_free (tweet_text);
	
	if (pixbuf) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->expand_image), pixbuf);
	}
	
	gtk_widget_show (priv->expand_box);
}

void
fwitux_app_hide_expand_message (void)
{
	FwituxAppPriv *priv;
	
	priv = GET_PRIV (app);

	gtk_widget_hide (priv->expand_box);
}

