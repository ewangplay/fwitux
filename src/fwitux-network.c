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
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>

#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-conf.h>
#ifdef HAVE_GNOME_KEYRING
#include <libfwitux/fwitux-keyring.h>
#endif

#include "fwitux.h"
#include "fwitux-network.h"
#include "fwitux-parser.h"
#include "fwitux-app.h"
#include "fwitux-replies-window.h"
#include "fwitux-message-dialog.h"
#include "fwitux-lists-dialog.h"

#define DEBUG_DOMAIN	  "Network"
#define FWITUX_HEADER_URL "http://code.google.com/p/fwitux/client.xml"

typedef struct {
	gchar        *src;
	GtkTreeIter   iter;
} FwituxImage;

enum _enum_user_type
{
    FRIENDS_TYPE,
    FOLLOWED_TYPE,
    FOLLOWERS_TYPE
};

static void network_get_data		(const gchar           *url,
									 SoupSessionCallback    callback,
									 gpointer               data);
static void network_post_data		(const gchar           *url,
									 gchar                 *formdata,
									 SoupSessionCallback    callback,
                                     gpointer               data);
static gboolean	network_check_http 	(gint                   status_code);
static void network_parser_free_lists (void);

/* libsoup callbacks */
static void network_cb_on_login		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_post_status		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_send_message	(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_reply_status	(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_delete_status (SoupSession          *session,
                                     SoupMessage           *msg,
                                     gpointer               user_data);
static void network_cb_on_load_timeline	(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_load_reply_timeline (SoupSession *session,
						           SoupMessage *msg,
						           gpointer     user_data);
static void network_cb_on_get_friends (SoupSession *session,
                                     SoupMessage *msg,
                                     gpointer     user_data);
static void network_cb_on_get_users	    (SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_get_image		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_get_reply_image (SoupSession *session,
					          SoupMessage *msg,
					          gpointer     user_data);
static void network_cb_on_add_friend		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_add_follower		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_del_friend		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_del_follower		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_auth_user		(SoupSession           *session,
									 SoupMessage           *msg,
									 SoupAuth              *auth,
									 gboolean               retrying,
									 gpointer               data);

/* Autoreload timeout functions */
static gboolean 	network_timeout			(gpointer user_data);
static void			network_timeout_new		(void);
static gboolean 	network_reply_timeout	(gpointer user_data);
static void			network_reply_timeout_new (void);

static SoupSession			*soup_connection = NULL;
static GList				*user_friends = NULL;
static GList				*user_followed = NULL;
static GList				*user_followers = NULL;
static gboolean				processing = FALSE;
static gchar				*current_timeline = NULL;
static gboolean				processing_reply = FALSE;
static gchar				*current_status_id = NULL;
static guint				timeout_id;
static guint				reply_timeout_id;
static gchar                *global_username = NULL;
static gchar                *global_password = NULL;

static gint                 reply_source_type = 0;
static gint                 delete_source_type = 0;

/* This function must be called at startup */
void
fwitux_network_new (void)
{
	FwituxConf	*conf;
	gboolean	check_proxy = FALSE;
 
	/* Close previous networking */
	if (soup_connection) {
		fwitux_network_close ();
	}

	/* Async connections */
	soup_connection = soup_session_async_new_with_options (SOUP_SESSION_MAX_CONNS,
														   8,
														   NULL);

	fwitux_debug (DEBUG_DOMAIN, "Libsoup (re)started");

	/* Set the proxy, if configuration is set */
	conf = fwitux_conf_get ();
	fwitux_conf_get_bool (conf,
						  FWITUX_PROXY_USE,
						  &check_proxy);

	if (check_proxy) {
		gchar *server, *proxy_uri;
		gint port;

		/* Get proxy */
		fwitux_conf_get_string (conf,
								FWITUX_PROXY_HOST,
								&server);
		fwitux_conf_get_int (conf,
							 FWITUX_PROXY_PORT,
							 &port);

		if (server && server[0]) {
			SoupURI *suri;

			check_proxy = FALSE;
			fwitux_conf_get_bool (conf,
								  FWITUX_PROXY_USE_AUTH,
								  &check_proxy);

			/* Get proxy auth data */
			if (check_proxy) {
				char *user, *password;

				fwitux_conf_get_string (conf,
										FWITUX_PROXY_USER,
										&user);
				fwitux_conf_get_string (conf,
										FWITUX_PROXY_PASS,
										&password);

				proxy_uri = g_strdup_printf ("http://%s:%s@%s:%d",
											 user,
											 password,
											 server,
											 port);

				g_free (user);
				g_free (password);
			} else {
				proxy_uri = g_strdup_printf ("http://%s:%d", 
											 server, port);
			}

			fwitux_debug (DEBUG_DOMAIN, "Proxy uri: %s",
						  proxy_uri);

			/* Setup proxy info */
			suri = soup_uri_new (proxy_uri);
			g_object_set (G_OBJECT (soup_connection),
						  SOUP_SESSION_PROXY_URI,
						  suri,
						  NULL);

			soup_uri_free (suri);
			g_free (server);
			g_free (proxy_uri);
		}
	}
}


/* Cancels requests, and unref libsoup. */
void
fwitux_network_close (void)
{
	/* Close all connections */
	fwitux_network_stop ();

	network_parser_free_lists ();

	g_object_unref (soup_connection);

	if (current_timeline) {
		g_free (current_timeline);
		current_timeline = NULL;
	}

    if (current_status_id) {
        g_free (current_status_id);
        current_status_id = NULL;
    }

	fwitux_debug (DEBUG_DOMAIN, "Libsoup closed");
}


/* Cancels all pending requests in session. */
void
fwitux_network_stop	(void)
{
	fwitux_debug (DEBUG_DOMAIN,"Cancelled all connections");

	soup_session_abort (soup_connection);
}


/* Login in Twitter */
void
fwitux_network_login (const char *username, const char *password)
{
    gchar *formdata;

	fwitux_debug (DEBUG_DOMAIN, "Begin login.. ");

	g_free (global_username);
	global_username = g_strdup (username);
	g_free (global_password);
	global_password = g_strdup (password);

	fwitux_app_set_statusbar_msg (_("Connecting..."));

	/* HTTP Basic Authentication */
	g_signal_connect (soup_connection,
					  "authenticate",
					  G_CALLBACK (network_cb_on_auth_user),
					  NULL);

	/* Verify cedentials */
    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);

    network_post_data(FWITUX_API_LOGIN,
                      formdata,
                      network_cb_on_login,
                      NULL);
}


/* Logout current user */
void fwitux_network_logout (void)
{
	fwitux_network_new ();
	
	fwitux_debug (DEBUG_DOMAIN, "Logout");
}


/* Post a new tweet - text must be Url encoded */
void
fwitux_network_post_status (const gchar *text)
{
	gchar *formdata;

	formdata = g_strdup_printf ("api_key=%s&status=%s", FWITUX_API_KEY, text);

	network_post_data (FWITUX_API_POST_STATUS,
					   formdata,
					   network_cb_on_post_status,
                       NULL);
}


/* Send a direct message to a follower - text must be Url encoded  */
void
fwitux_network_send_message (const gchar *friend_id,
							 const gchar *text)
{
	gchar *formdata;

	formdata = g_strdup_printf ( "api_key=%s&fid=%s&status=%s",
                                 FWITUX_API_KEY, friend_id, text);
	
	network_post_data (FWITUX_API_SEND_MESSAGE,
					   formdata,
					   network_cb_on_send_message,
                       NULL);
}

/* Reply a status */
void
fwitux_network_reply_status (const gchar *reply_status_id, const gchar * text, gint source_type)
{
	gchar *formdata;

    reply_source_type = source_type;

	formdata = g_strdup_printf ( "api_key=%s&id=%s&status=%s",
                                 FWITUX_API_KEY, reply_status_id, text);
	
	network_post_data (FWITUX_API_REPLY_STATUS,
					   formdata,
					   network_cb_on_reply_status,
                       NULL);
}

/* Delete a status */
void 
fwitux_network_delete_status (const gchar *status_id, gint source_type)
{
	gchar *formdata;

    delete_source_type = source_type;

	formdata = g_strdup_printf ( "api_key=%s&id=%s",
                                 FWITUX_API_KEY, status_id);
	
	network_post_data (FWITUX_API_DEL_STAUS,
					   formdata,
					   network_cb_on_delete_status,
                       NULL);
}

void
fwitux_network_refresh (void)
{
    gchar *formdata;

	if (!current_timeline || processing)
		return;

	/* UI */
	fwitux_app_set_statusbar_msg (_("Loading timeline..."));

	processing = TRUE;

    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(current_timeline,
                      formdata,
                      network_cb_on_load_timeline,
                      NULL);
}

/* Get and parse a timeline */
void
fwitux_network_get_timeline (const gchar *url_timeline, gchar *formdata)
{
    gchar *new_timeline = NULL;

	if (processing)
		return;

	parser_reset_last_tweet_time ();

	/* UI */
	fwitux_app_set_statusbar_msg (_("Loading timeline..."));

	processing = TRUE;

    new_timeline = g_strdup(url_timeline);

    if (formdata == NULL) {
        formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    }

    network_post_data(url_timeline,
                      formdata,
                      network_cb_on_load_timeline,
                      new_timeline);
}

/* get and parse the reply timeline */
void fwitux_network_get_reply_timeline (const gchar *status_id)
{
    gchar   *formdata;
    gchar   *new_status_id;

	if (processing_reply)
		return;

	/* UI */
	fwitux_replies_window_set_statusbar_msg (_("Loading replies..."));

	processing_reply = TRUE;

    new_status_id = g_strdup(status_id);
    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, status_id);
    network_post_data(FWITUX_API_TIMELINE_REPLY,
                      formdata,
                      network_cb_on_load_reply_timeline,
                      new_status_id);
}


/* Get a user timeline */
void
fwitux_network_get_user_timeline (const gchar *user_id)
{
    gchar *formdata;

	if(!G_STR_EMPTY (user_id)) {
		formdata = g_strdup_printf ("api_key=%s&id=%s", FWITUX_API_KEY, user_id);
		fwitux_network_get_timeline (FWITUX_API_TIMELINE_USER, formdata);
	}
}

/* Get authenticating user's friends for send message
 * Returns:
 * 		NULL: Friends will be fetched
 * 		GList: The list of friends (fetched previously)
 */
GList *
fwitux_network_get_friends_for_message_dialog (void)
{
    gchar *formdata;

	if (user_friends)
		return user_friends;
	
    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(FWITUX_API_FRIENDS,
                      formdata,
                      network_cb_on_get_friends,
                      NULL);
	return NULL;
}

/* Get authenticating user's friends
 * Returns:
 * 		NULL: Friends will be fetched
 * 		GList: The list of friends (fetched previously)
 */
GList *
fwitux_network_get_friends (void)
{
    gchar *formdata;

	if (user_friends)
		return user_friends;
	
    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(FWITUX_API_FRIENDS,
                      formdata,
                      network_cb_on_get_users,
                      GINT_TO_POINTER(FRIENDS_TYPE));
	return NULL;
}


/* Get the authenticating user's followed
 * Returns:
 * 		NULL: Followed will be fetched
 * 		GList: The list of followed (fetched previously)
 */
GList *
fwitux_network_get_followed (void)
{
    gchar *formdata;

	if (user_followed)
		return user_followed;

    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(FWITUX_API_FOLLOWED,
                      formdata,
                      network_cb_on_get_users,
                      GINT_TO_POINTER(FOLLOWED_TYPE));
	return NULL;
}

/* Get the authenticating user's followings
 * Returns:
 * 		NULL: Followings will be fetched
 * 		GList: The list of followings (fetched previously)
 */
GList *
fwitux_network_get_followers (void)
{
    gchar *formdata;

	if (user_followers)
		return user_followers;

    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(FWITUX_API_FOLLOWERS,
                      formdata,
                      network_cb_on_get_users,
                      GINT_TO_POINTER(FOLLOWERS_TYPE));
	return NULL;
}

/* Get an image from servers */
void
fwitux_network_get_image (const gchar  *url_image,
						  GtkTreeIter   iter)
{
	gchar	*image_file;
	gchar   *image_name;

	FwituxImage *image;

	/* save using the filename */
	image_name = strrchr (url_image, '/');
	if (image_name && image_name[1] != '\0') {
		image_name++;
	} else {
		image_name = "fwitux_unknown_image";
	}

	image_file = g_build_filename (g_get_home_dir(), ".gnome2",
								   FWITUX_CACHE_IMAGES,
								   image_name, NULL);

	/* check if image already exists */
	if (g_file_test (image_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {		
		/* Set image from file here */
		fwitux_app_set_image (image_file, iter);
		g_free (image_file);
		return;
	}

	image = g_new0 (FwituxImage, 1);
	image->src  = g_strdup (image_file);
	image->iter = iter;

	g_free (image_file);

	/* Note: 'image' will be freed in 'network_cb_on_get_image' */
	network_get_data (url_image, network_cb_on_get_image, image);
}

/* Get an reply image from servers */
void
fwitux_network_get_reply_image (const gchar  *url_image,
						  GtkTreeIter   iter)
{
	gchar	*image_file;
	gchar   *image_name;

	FwituxImage *image;

	/* save using the filename */
	image_name = strrchr (url_image, '/');
	if (image_name && image_name[1] != '\0') {
		image_name++;
	} else {
		image_name = "fwitux_unknown_image";
	}

	image_file = g_build_filename (g_get_home_dir(), ".gnome2",
								   FWITUX_CACHE_IMAGES,
								   image_name, NULL);

	/* check if image already exists */
	if (g_file_test (image_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {		
		/* Set image from file here */
		fwitux_replies_window_set_image (image_file, iter);
		g_free (image_file);
		return;
	}

	image = g_new0 (FwituxImage, 1);
	image->src  = g_strdup (image_file);
	image->iter = iter;

	g_free (image_file);

	/* Note: 'image' will be freed in 'network_cb_on_get_image' */
	network_get_data (url_image, network_cb_on_get_reply_image, image);
}

/* Add a user to friends */
void
fwitux_network_add_friend (const gchar *user_id)
{
    gchar *formdata;
	
	if (G_STR_EMPTY (user_id))
		return;
	
    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, user_id);

    network_post_data(FWITUX_API_FRIEND_ADD,
                      formdata,
                      network_cb_on_add_friend,
                      NULL);
}


/* remove a user from friends */
void
fwitux_network_del_friend (FwituxUser *user)
{
    gchar *formdata;
	
	if (!user || !user->id)
		return;
	
    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, user->id);

    network_post_data(FWITUX_API_FRIEND_DEL,
                      formdata,
                      network_cb_on_del_friend,
                      user);
}

/* Add a user to followers */
void
fwitux_network_add_follower (const gchar *user_id)
{
    gchar *formdata;
	
	if (G_STR_EMPTY (user_id))
		return;
	
    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, user_id);

    network_post_data(FWITUX_API_FOLLOWER_ADD,
                      formdata,
                      network_cb_on_add_follower,
                      NULL);
}

/* remove a user from followers */
void
fwitux_network_del_follower (FwituxUser *user)
{
    gchar *formdata;
	
	if (!user || !user->id)
		return;
	
    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, user->id);

    network_post_data(FWITUX_API_FOLLOWER_DEL,
                      formdata,
                      network_cb_on_del_follower,
                      user);
}

/* Get data from net */
static void
network_get_data (const gchar           *url,
				  SoupSessionCallback    callback,
				  gpointer               data)
{
	SoupMessage *msg;

	fwitux_debug (DEBUG_DOMAIN, "Get: %s",url);

	msg = soup_message_new ( "GET", url );

	soup_session_queue_message (soup_connection, msg, callback, data);
}


/* Private: Post data to net */
static void
network_post_data (const gchar           *url,
				   gchar                 *formdata,
				   SoupSessionCallback    callback,
                   gpointer              data)
{
	SoupMessage *msg;

	fwitux_debug (DEBUG_DOMAIN, "Post: %s",url);

	msg = soup_message_new ("POST", url);
	
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client", PACKAGE_NAME);
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client-Version", PACKAGE_VERSION);
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client-URL", FWITUX_HEADER_URL);

	soup_message_set_request (msg, 
							  "application/x-www-form-urlencoded",
							  SOUP_MEMORY_TAKE,
							  formdata,
							  strlen (formdata));

	soup_session_queue_message (soup_connection, msg, callback, data);
}


/* Check HTTP response code */
static gboolean
network_check_http (gint status_code)
{
	if (status_code == 401) {
		fwitux_app_set_statusbar_msg (_("Access denied."));

	} else if (SOUP_STATUS_IS_CLIENT_ERROR (status_code)) {
		fwitux_app_set_statusbar_msg (_("HTTP communication error."));

	} else if(SOUP_STATUS_IS_SERVER_ERROR (status_code)) {
		fwitux_app_set_statusbar_msg (_("Internal server error."));

	} else if (!SOUP_STATUS_IS_SUCCESSFUL (status_code)) {
		fwitux_app_set_statusbar_msg (_("Stopped"));

	} else {
		return TRUE;
	}
	
	return FALSE;
}

/* Callback to free every element on a User list */
static void
network_free_user_list_each (gpointer user,
							 gpointer data)
{
	FwituxUser *usr;

	if (!user)
		return;

	usr = (FwituxUser *)user;
	parser_free_user (user);
}


/* Free a list of Users */
static void
network_parser_free_lists ()
{
	if (user_friends) {
		g_list_foreach (user_friends, network_free_user_list_each, NULL);
		g_list_free (user_friends);
		user_friends = NULL;
		fwitux_debug (DEBUG_DOMAIN,
					  "Friends freed");
	}

	if (user_followed) {
		g_list_foreach (user_followed, network_free_user_list_each, NULL);
		g_list_free (user_followed);
		user_followed = NULL;
		fwitux_debug (DEBUG_DOMAIN,
					  "Followers freed");
	}
}



/* HTTP Basic Authentication */
static void
network_cb_on_auth_user (SoupSession  *session,
					SoupMessage  *msg,
					SoupAuth     *auth,
					gboolean      retrying,
					gpointer      data)
{
	/* Don't bother to continue if there is no user_id */
	if (G_STR_EMPTY (global_username)) {
		return;
	}

	/* verify that the password has been set */
	if (!G_STR_EMPTY (global_password))
		soup_auth_authenticate (auth, global_username, 
								global_password);
}


/* On verify credentials */
static void
network_cb_on_login (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	fwitux_debug (DEBUG_DOMAIN,
				  "Login response: %i",msg->status_code);

	if (network_check_http (msg->status_code)) {
		fwitux_app_state_on_connection (TRUE);
		return;
	}

	fwitux_app_state_on_connection (FALSE);
}


/* On post a tweet */
static void
network_cb_on_post_status (SoupSession *session,
					SoupMessage *msg,
					gpointer     user_data)
{
	if (network_check_http (msg->status_code)) {
		fwitux_app_set_statusbar_msg (_("Status Sent"));
	}
	
	fwitux_debug (DEBUG_DOMAIN,
				  "Tweet response: %i",msg->status_code);
}


/* On send a direct message */
static void
network_cb_on_send_message (SoupSession *session,
					   SoupMessage *msg,
					   gpointer     user_data)
{
	if (network_check_http (msg->status_code)) {
		fwitux_app_set_statusbar_msg (_("Message Sent"));
	}

	fwitux_debug (DEBUG_DOMAIN,
				  "Message response: %i",msg->status_code);
}

/* On reply a status  */
static void
network_cb_on_reply_status (SoupSession *session,
					   SoupMessage *msg,
					   gpointer     user_data)
{
    const gchar    *message = _("Reply Sent");

	if (network_check_http (msg->status_code)) {
        if (reply_source_type == MAIN_WINDOW_SOURCE) {
            fwitux_app_set_statusbar_msg (message);
        }
        else if (reply_source_type == REPLY_WINDOW_SOURCE) {
            fwitux_replies_window_set_statusbar_msg (message);
        }
        else {
            fwitux_app_set_statusbar_msg (message);
        }
	}

	fwitux_debug (DEBUG_DOMAIN,
				  "Message response: %i",msg->status_code);
}

/* On delete a status */
static void 
network_cb_on_delete_status (SoupSession          *session,
                             SoupMessage           *msg,
                             gpointer               user_data)
{
    const gchar    *message = _("Status Deleted");

	if (network_check_http (msg->status_code)) {
        if (delete_source_type == MAIN_WINDOW_SOURCE) {
            fwitux_app_set_statusbar_msg (message);
        }
        else if (delete_source_type == REPLY_WINDOW_SOURCE) {
            fwitux_replies_window_set_statusbar_msg (message);
        }
        else 
        {
            fwitux_app_set_statusbar_msg (message);
        }
	}

	fwitux_debug (DEBUG_DOMAIN,
				  "Message response: %i",msg->status_code);
}

/* On get a timeline */
static void
network_cb_on_load_timeline (SoupSession *session,
						SoupMessage *msg,
						gpointer     user_data)
{
	gchar        *new_timeline = NULL;
	
	if (user_data){
		new_timeline = (gchar *)user_data;
	}

	fwitux_debug (DEBUG_DOMAIN,
				  "Timeline response: %i",msg->status_code);

	/* Timeout */
	network_timeout_new ();

	/* Check response */
	if (!network_check_http (msg->status_code)) {
		if (new_timeline)
			g_free (new_timeline);
		return;
	}

	fwitux_debug (DEBUG_DOMAIN, "Parsing timeline");

	/* Parse and set ListStore */
	if (fwitux_parser_timeline (msg->response_body->data, msg->response_body->length)) {
		fwitux_app_set_statusbar_msg (_("Timeline Loaded"));

		if (new_timeline){
			if (current_timeline)
				g_free (current_timeline);
			current_timeline = g_strdup (new_timeline);
		}
	} else {
		fwitux_app_set_statusbar_msg (_("Timeline Parser Error."));
	}

	if (new_timeline)
		g_free (new_timeline);

    /* process finished */
	processing = FALSE;
}

/* On get the reply timeline */
static void
network_cb_on_load_reply_timeline (SoupSession *session,
						           SoupMessage *msg,
						           gpointer     user_data)
{
    gchar   *new_status_id = NULL;

    if (user_data) {
        new_status_id = (gchar *)user_data;
    }

	fwitux_debug (DEBUG_DOMAIN,
				  "Timeline response: %i",msg->status_code);

	/* Timeout */
	network_reply_timeout_new ();

	/* Check response */
	if (!network_check_http (msg->status_code)) {
        if(new_status_id)
            g_free (new_status_id);
		return;
	}

	fwitux_debug (DEBUG_DOMAIN, "Parsing timeline");

	/* Parse and set ListStore */
	if (fwitux_parser_reply_timeline (msg->response_body->data, msg->response_body->length)) {
		fwitux_replies_window_set_statusbar_msg (_("Replies Loaded"));

        if(new_status_id) {
            if(current_status_id)
                g_free (current_status_id);
            current_status_id = g_strdup(new_status_id);
        }
	} else {
		fwitux_replies_window_set_statusbar_msg (_("Replies Parser Error."));
	}

    if(new_status_id)
        g_free(new_status_id);

    /* process finished */
	processing_reply = FALSE;
}

/* On get user friends */
static void
network_cb_on_get_friends (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	GList    *users;
		
	fwitux_debug (DEBUG_DOMAIN,
				  "Users response: %i",msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse user list */
	fwitux_debug (DEBUG_DOMAIN, "Parsing user list");

	users = fwitux_parser_users_list (msg->response_body->data,
									  msg->response_body->length);

    /* if parse ok, set set frineds list */
	if (users){
        user_friends = users;
        fwitux_message_dialog_set_friends (user_friends);
	} else {
		fwitux_app_set_statusbar_msg (_("Users parser error."));
	}
}

/* On get user followers */
static void
network_cb_on_get_users (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	gint     user_type = GPOINTER_TO_INT(user_data);
	GList    *users;
		
	fwitux_debug (DEBUG_DOMAIN,
				  "Users response: %i",msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse user list */
	fwitux_debug (DEBUG_DOMAIN, "Parsing user list");

	users = fwitux_parser_users_list (msg->response_body->data,
									  msg->response_body->length);

	/* check if it ok, and if it is a followers or following list */
	if (users){
        if (user_type == FRIENDS_TYPE) {
            user_friends = users;

            fwitux_lists_dialog_load_lists (friends_lists, user_friends);
            //fwitux_message_set_followers (user_friends);
        } else if (user_type == FOLLOWED_TYPE) {
            user_followed = users;

            fwitux_lists_dialog_load_lists (followed_lists, user_followed);
        } else if (user_type == FOLLOWERS_TYPE) {
            user_followers = users;

            fwitux_lists_dialog_load_lists (followers_lists, user_followers);
        }
	} else {
		fwitux_app_set_statusbar_msg (_("Users parser error."));
	}
}


/* On get a image */
static void
network_cb_on_get_image (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	FwituxImage *image = (FwituxImage *)user_data;

	fwitux_debug (DEBUG_DOMAIN,
				  "Image response: %i", msg->status_code);

	/* check response */
	if (network_check_http (msg->status_code)) {
		/* Save image data */
		if (g_file_set_contents (image->src,
								 msg->response_body->data,
								 msg->response_body->length,
								 NULL)) {
			/* Set image from file here (image_file) */
			fwitux_app_set_image (image->src,image->iter);
		}
	}

	g_free (image->src);
	g_free (image);
}

/* On get a reply image */
static void
network_cb_on_get_reply_image (SoupSession *session,
					          SoupMessage *msg,
					          gpointer     user_data)
{
	FwituxImage *image = (FwituxImage *)user_data;

	fwitux_debug (DEBUG_DOMAIN,
				  "Image response: %i", msg->status_code);

	/* check response */
	if (network_check_http (msg->status_code)) {
		/* Save image data */
		if (g_file_set_contents (image->src,
								 msg->response_body->data,
								 msg->response_body->length,
								 NULL)) {
			/* Set image from file here (image_file) */
			fwitux_replies_window_set_image (image->src,image->iter);
		}
	}

	g_free (image->src);
	g_free (image);
}

/* On add a follower */
static void
network_cb_on_add_follower (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	FwituxUser *user;

	fwitux_debug (DEBUG_DOMAIN,
				  "Add follower response: %i", msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse new user */
	fwitux_debug (DEBUG_DOMAIN, "Parsing new user");

	user = fwitux_parser_single_user (msg->response_body->data,
									  msg->response_body->length);

	if (user) {
		user_followers = g_list_append ( user_followers, user );
		fwitux_app_set_statusbar_msg (_("Follower Added"));
	} else {
		fwitux_app_set_statusbar_msg (_("Failed to add follower"));
	}
}


/* On add a friend */
static void
network_cb_on_add_friend (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	FwituxUser *user;

	fwitux_debug (DEBUG_DOMAIN,
				  "Add friend response: %i", msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse new user */
	fwitux_debug (DEBUG_DOMAIN, "Parsing new user");

	user = fwitux_parser_single_user (msg->response_body->data,
									  msg->response_body->length);

	if (user) {
		user_friends = g_list_append ( user_friends, user );
		fwitux_app_set_statusbar_msg (_("Friend Added"));
	} else {
		fwitux_app_set_statusbar_msg (_("Failed to add friend"));
	}
}

/* On remove a friend */
static void
network_cb_on_del_friend (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	fwitux_debug (DEBUG_DOMAIN,
				  "Delete friend response: %i", msg->status_code);

	if (network_check_http (msg->status_code)) {		
		fwitux_app_set_statusbar_msg (_("Friend Removed"));
	} else {
		fwitux_app_set_statusbar_msg (_("Failed to remove friend"));
	}
	
	if (user_data) {
		FwituxUser *user = (FwituxUser *)user_data;
		user_friends = g_list_remove (user_friends, user);
		parser_free_user (user);
	}
}

/* On remove a follower */
static void
network_cb_on_del_follower (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	fwitux_debug (DEBUG_DOMAIN,
				  "Delete follower response: %i", msg->status_code);

	if (network_check_http (msg->status_code)) {		
		fwitux_app_set_statusbar_msg (_("Follower Removed"));
	} else {
		fwitux_app_set_statusbar_msg (_("Failed to remove follower"));
	}
	
	if (user_data) {
		FwituxUser *user = (FwituxUser *)user_data;
		user_followers = g_list_remove (user_followers, user);
		parser_free_user (user);
	}
}

/* create timelien timeout */
static void
network_timeout_new (void)
{
	gint minutes;
	guint reload_time;

	if (timeout_id) {
		fwitux_debug (DEBUG_DOMAIN,
					  "Stopping timeout id: %i", timeout_id);

		g_source_remove (timeout_id);
	}

	fwitux_conf_get_int (fwitux_conf_get (),
						 FWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
						 &minutes);

	/* The timeline reload interval shouldn't be less than 3 minutes */
	if (minutes < 3) {
		minutes = 3;
	}

	/* This should be the number of milliseconds */
	reload_time = minutes * 60 * 1000;

	timeout_id = g_timeout_add (reload_time,
								network_timeout,
								NULL);

	fwitux_debug (DEBUG_DOMAIN,
				  "Starting timeout id: %i", timeout_id);
}

/* create reply timeline timeout */
static void
network_reply_timeout_new (void)
{
	gint minutes;
	guint reload_time;

	if (reply_timeout_id) {
		fwitux_debug (DEBUG_DOMAIN,
					  "Stopping timeout id: %i", reply_timeout_id);
		g_source_remove (reply_timeout_id);
	}

	fwitux_conf_get_int (fwitux_conf_get (),
						 FWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
						 &minutes);

	/* The timeline reload interval shouldn't be less than 3 minutes */
	if (minutes < 3) {
		minutes = 3;
	}

	/* This should be the number of milliseconds */
	reload_time = minutes * 60 * 1000;

	reply_timeout_id = g_timeout_add (reload_time,
								network_reply_timeout,
								NULL);

	fwitux_debug (DEBUG_DOMAIN,
				  "Starting timeout id: %i", reply_timeout_id);
}

/* timelien timeout handler */
static gboolean
network_timeout (gpointer user_data)
{
    gchar *formdata;

	if (!current_timeline || processing)
		return FALSE;

	/* UI */
	fwitux_app_set_statusbar_msg (_("Reloading timeline..."));

	fwitux_debug (DEBUG_DOMAIN,
				  "Auto reloading. Timeout: %i", timeout_id);

	processing = TRUE;

    formdata = g_strdup_printf("api_key=%s", FWITUX_API_KEY);
    network_post_data(current_timeline,
                      formdata,
                      network_cb_on_load_timeline,
                      NULL);

	timeout_id = 0;

    /* return FALSE, the timeout is automatically destoryed
     * and this function will not be called again. 
     */
	return FALSE;
}

/* reply timeline timeout handler */
static gboolean
network_reply_timeout (gpointer user_data)
{
    gchar *formdata;

	if (!current_status_id || processing_reply)
		return FALSE;

	/* UI */
	fwitux_replies_window_set_statusbar_msg (_("Reloading replies..."));

	fwitux_debug (DEBUG_DOMAIN,
				  "Auto reloading. Timeout: %i", reply_timeout_id);

	processing_reply = TRUE;

    formdata = g_strdup_printf("api_key=%s&id=%s", FWITUX_API_KEY, current_status_id);
    network_post_data(FWITUX_API_TIMELINE_REPLY,
                      formdata,
                      network_cb_on_load_reply_timeline,
                      NULL);

	reply_timeout_id = 0;

    /* return FALSE, the timeout is automatically destoryed
     * and this function will not be called again. 
     */
	return FALSE;
}

void fwitux_network_remove_reply_source(void)
{
    if(reply_timeout_id) {
		fwitux_debug (DEBUG_DOMAIN,
					  "Stopping timeout id: %i", reply_timeout_id);
        g_source_remove (reply_timeout_id);
        reply_timeout_id = 0;
    }

    if(current_status_id) {
        g_free(current_status_id);
        current_status_id = NULL;
    }
}

