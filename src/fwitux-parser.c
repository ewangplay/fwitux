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

/*
 * Just make sure we include the prototype for strptime as well
 */
#define _XOPEN_SOURCE
#include <time.h>
#include <string.h> /* for g_memmove - memmove */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-conf.h>

#include "fwitux.h"
#include "fwitux-app.h"
#include "fwitux-replies-window.h"
#include "fwitux-network.h"
#include "fwitux-parser.h"
#include "fwitux-tweet-list.h"
#include "fwitux-reply-list.h"

#define DEBUG_DOMAIN_SETUP       "Parser" 

typedef struct 
{
	FwituxUser	*user;              //留言用户
	gchar		*text;              //留言内容
	gchar		*created_at;        //留言时间
	gchar		*id;                //留言ID
    gchar       *source;            //分享方式
    gchar       *link;              //分享链接
    gchar       *receiver;          //接收人
    gchar       *image_address;     //图片地址
    gchar       *reply_count;        //回复数量
} FwituxStatus;

static FwituxUser	*parser_fwitux_node_user   (xmlNode     *a_node);
static FwituxStatus	*parser_fwitux_node_status (xmlNode     *a_node);
static xmlDoc       *parser_fwitux_parse       (const gchar  *data,
												gssize       length,
												xmlNode    **first_element);

static gint         parser_get_time (const gchar * datetime);
static gchar		*parser_convert_time       (const gchar	*datetime);
static gboolean     display_notification      (gpointer     tweet);
static void         parser_free_status        (FwituxStatus *status);

/* created time of the newest tweet showed */
static gint			last_tweet_time = 0;

static xmlDoc*
parser_fwitux_parse (const gchar  *data,
					 gssize       length,
					 xmlNode    **first_element)
{
	xmlDoc	*doc = NULL;
	xmlNode	*root_element = NULL;

	/* Read the XML */
	doc = xmlReadMemory (data, length, "xml", "UTF-8", 0);
	if (doc == NULL) {
		fwitux_debug (DEBUG_DOMAIN_SETUP,
					  "failed to read xml data");
		return NULL;
	}

	/* Get first element */
	root_element = xmlDocGetRootElement (doc);
	if (root_element == NULL) {
		fwitux_debug (DEBUG_DOMAIN_SETUP,
					  "failed getting first element of xml data");
		xmlFreeDoc (doc);
		return NULL;
	} else {
		*first_element = root_element;
	}

	return doc;
}


/* Parse a user-list XML ( friends, followers,... ) */
GList *
fwitux_parser_users_list (const gchar *data,
						  gssize       length)
{
	xmlDoc		*doc = NULL;
	xmlNode		*root_element = NULL;
	xmlNode		*cur_node = NULL;

	GList		*friends = NULL;

	FwituxUser 	*user;

	/* parse the xml */
	doc = parser_fwitux_parse (data, length, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return NULL;
	}

	/* get users */
	for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		if (g_str_equal (cur_node->name, "user")){
			/* parse user */
			user = parser_fwitux_node_user (cur_node->children);
			/* add to list */
			friends = g_list_append (friends, user);
		} else if (g_str_equal (cur_node->name, "users")){
			cur_node = cur_node->children;
		}
	} /* End of loop */

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return friends;
}


/* Parse a xml user node. Ex: add/del users responses */
FwituxUser *
fwitux_parser_single_user (const gchar *data,
						   gssize       length)
{
	xmlDoc		*doc = NULL;
	xmlNode		*root_element = NULL;
	FwituxUser 	*user = NULL;
	
	/* parse the xml */
	doc = parser_fwitux_parse (data, length, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return NULL;
	}

	if (g_str_equal (root_element->name, "user")) {
		user = parser_fwitux_node_user (root_element->children);
	}

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();
	
	return user;
}

static gboolean
display_notification (gpointer tweet)
{
	fwitux_app_notify (tweet);
	g_free (tweet);

	return FALSE;
}


/* Parse a timeline XML file */
gboolean
fwitux_parser_timeline (const gchar *data, 
						gssize       length)
{
	xmlDoc		    *doc          = NULL;
	xmlNode		    *root_element = NULL;
	xmlNode		    *cur_node     = NULL;

	GtkListStore 	*store        = NULL;
	GtkTreeIter	     iter;

	FwituxStatus 	*status;

	/* Count new tweets */
	gboolean         show_notification = (last_tweet_time > 0);
	gint             last_time = 0;

    /*
	 * On multiple tweet updates we only want to 
	 * play the sound notification once.
	 */
    //gboolean         multiple_new_tweets = FALSE;

	gint             tweet_display_delay = 0;
	const gint       tweet_display_interval = 5;

	/* parse the xml */
	doc = parser_fwitux_parse (data, length, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return FALSE;
	}

	/* Get the fwitux ListStore and clear previous */
	store = fwitux_tweet_list_get_store ();
	gtk_list_store_clear (store);

	/* get tweets or direct messages */
	for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;

		/* Timelines and direct messages */
		if (g_str_equal (cur_node->name, "status") ||
		    g_str_equal (cur_node->name, "direct_message")) {
			gchar *tweet;
            gchar *tweet_base;
			gchar *datetime;
			gint   tweet_time;

			/* Parse node */
			status = parser_fwitux_node_status (cur_node->children);

			tweet_time = parser_get_time (status->created_at);

			/* the first tweet parsed is the 'newest' */
			if (last_time == 0){
				last_time = tweet_time;
			}

			/* Create string for text column */
			datetime = parser_convert_time (status->created_at);
            tweet_base = g_strconcat ("<b>", status->user->name, "</b> - ", 
                                 "<small>", datetime, "</small> - ", 
                                 "<small>", status->source, "</small> - ",
                                 "<small>", _("Reply"), "(<b>", status->reply_count, "</b>)</small>\n",
                                 status->text,
                                 NULL);

            if ( (status->link && strlen(status->link) > 0)
                 && (status->image_address && strlen(status->image_address) > 0) ) {
                tweet = g_strconcat (tweet_base, "\n", 
                                     "<i>", status->link, "</i>\n",
                                     "<i>", status->image_address, "</i>",
                                     NULL);
            } else if (status->link && strlen(status->link) > 0) {
                tweet = g_strconcat (tweet_base, "\n",
                                     "<i>", status->link, "</i>",
                                     NULL);
            } else if (status->image_address && strlen(status->image_address) > 0) {
                tweet = g_strconcat (tweet_base, "\n", 
                                     "<i>", status->image_address, "</i>",
                                     NULL);
            } else {
                tweet = g_strdup(tweet_base);
            }

            /* set notification */
			if (tweet_time > last_tweet_time && show_notification) {
                /*
				if (!multiple_new_tweets) {
					fwitux_app_notify_sound ();
					multiple_new_tweets = TRUE;
				}
                */

				g_timeout_add_seconds (tweet_display_delay,
									   display_notification,
									   g_strdup (tweet));

				tweet_display_delay += tweet_display_interval;
			}


			/* Append to ListStore */
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
								STRING_FORMATED_TWEET, tweet,
								STRING_USER_NAME, status->user->name,
								STRING_STATUS_CREATED, datetime,
								STRING_STATUS_TEXT, status->text,
                                STRING_STATUS_ID, status->id,
								STRING_USER_ID, status->user->id,
                                STRING_STATUS_SOURCE, status->source,
                                STRING_STATUS_LINK, status->link,
                                STRING_STATUS_IMAGE_ADDRESS, status->image_address,
								-1);
			
			/* Get Image */
			fwitux_network_get_image (status->user->image_url,
									  iter);

			/* Free status struct */
            parser_free_status (status);

			/* Free other string */
			g_free (tweet);
            g_free (tweet_base);
			g_free (datetime);
		} else if (g_str_equal (cur_node->name, "statuses") ||
			g_str_equal (cur_node->name, "direct-messages")) {
			cur_node = cur_node->children;
		}

	} /* end of loop */

	/* Remember last tweet time showed */
	if (last_time > 0) {
		last_tweet_time = last_time;
	}

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return TRUE;
}

/* Parse the reply timeline XML file */
gboolean
fwitux_parser_reply_timeline (const gchar *data, 
			        		gssize       length)
{
	xmlDoc		    *doc          = NULL;
	xmlNode		    *root_element = NULL;
	xmlNode		    *cur_node     = NULL;
	GtkListStore 	*store        = NULL;
	GtkTreeIter	     iter;

	FwituxStatus 	*status;

	/* parse the xml */
	doc = parser_fwitux_parse (data, length, &root_element);
	if (!doc) {
		xmlCleanupParser ();
		return FALSE;
	}

	/* Get the fwitux ListStore and clear previous */
	store = fwitux_reply_list_get_store ();
	gtk_list_store_clear (store);

	/* get tweets or direct messages */
	for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;

		/* Timelines and direct messages */
		if (g_str_equal (cur_node->name, "reply")) {
			gchar *tweet;
			gchar *datetime;

			/* Parse node */
			status = parser_fwitux_node_status (cur_node->children);

			/* Create string for text column */
			datetime = parser_convert_time (status->created_at);
            tweet = g_strconcat ("<b>", status->user->name, "</b> - ", 
                                 "<small>", datetime, "</small> - ", 
                                 "<small>", status->source, "</small>\n",
                                 status->text,
                                 NULL);

			/* Append to ListStore */
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
								STRING_REPLY_FORMATED_TWEET, tweet,
								STRING_REPLY_USER_NAME, status->user->name,
								STRING_REPLY_STATUS_CREATED, datetime,
								STRING_REPLY_STATUS_TEXT, status->text,
                                STRING_REPLY_STATUS_ID, status->id,
								STRING_REPLY_USER_ID, status->user->id,
                                STRING_REPLY_STATUS_SOURCE, status->source,
								-1);
			
			/* Get Image */
			fwitux_network_get_reply_image (status->user->image_url,
									        iter);

			/* Free status struct */
            parser_free_status (status);

			/* Free other string */
			g_free (tweet);
			g_free (datetime);
		} else if (g_str_equal (cur_node->name, "status")) {
			gchar *tweet;
			gchar *datetime;

			/* Parse node */
			status = parser_fwitux_node_status (cur_node->children);

			/* Create string for text column */
			datetime = parser_convert_time (status->created_at);
            tweet = g_strconcat ("<b>", status->user->name, "</b> - ", 
                                 "<small>", datetime, "</small> - ", 
                                 "<small>", status->source, "</small>\n",
                                 status->text,
                                 NULL);

            /* setup expand label */
            fwitux_replies_window_set_expand_message(status->user->name,
                    datetime,
                    status->source,
                    status->reply_count,
                    status->text,
                    status->link,
                    status->image_address,
                    status->user->image_url);

            /* set reply list status id */
            fwitux_reply_list_set_status_id(status->id);

			/* Free status struct */
            parser_free_status (status);

			/* Free other string */
			g_free (tweet);
			g_free (datetime);

            /* goto children */
			cur_node = cur_node->children;
		} else if (g_str_equal (cur_node->name, "relpyes")) {
			cur_node = cur_node->children;
        }
	} /* end of loop */

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return TRUE;
}

static FwituxUser *
parser_fwitux_node_user (xmlNode *a_node)
{
	xmlNode		   *cur_node = NULL;
	xmlBufferPtr	buffer;
	FwituxUser     *user;

	buffer = xmlBufferCreate ();
	user = g_new0 (FwituxUser, 1);

	/* Begin 'users' node loop */
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		if (xmlNodeBufGetContent (buffer, cur_node) != 0)
			continue;

		if (g_str_equal (cur_node->name, "id" )) {
			const xmlChar *tmp;
			tmp = xmlBufferContent (buffer);
			user->id = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "name" )) {
			const xmlChar *tmp;
			tmp = xmlBufferContent (buffer);
			user->name = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "profile_image_url")) {
			const xmlChar *tmp;
			tmp = xmlBufferContent (buffer);
			user->image_url = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "url")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent (buffer);
            user->url = g_strdup ((const gchar*)tmp);
        } else if (g_str_equal (cur_node->name, "location")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent (buffer);
            user->location = g_strdup ((const gchar*)tmp);
        } else if (g_str_equal (cur_node->name, "description")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent (buffer);
            user->description = g_strdup ((const gchar*)tmp);
        } else if (g_str_equal (cur_node->name, "sex")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent (buffer);
            user->sex = g_strdup ((const gchar*)tmp);
        } else if (g_str_equal (cur_node->name, "created_at")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent (buffer);
            user->created_at = g_strdup ((const gchar*)tmp);
        }

		/* Free buffer content */
		xmlBufferEmpty (buffer);

	} /* End of loop */

	/* Free buffer pointer */
	xmlBufferFree (buffer);

	return user;
}


static FwituxStatus *
parser_fwitux_node_status (xmlNode *a_node)
{
	xmlNode		   *cur_node = NULL;
	xmlBufferPtr	buffer;
	FwituxStatus   *status;

	buffer = xmlBufferCreate ();
	status = g_new0 (FwituxStatus, 1);

	/* Begin 'status' or 'direct-messages' loop */
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlNodeBufGetContent (buffer, cur_node) != 0)
			continue;

		if (g_str_equal (cur_node->name, "created_at")) {
			const xmlChar *tmp;
			tmp = xmlBufferContent(buffer);
			status->created_at = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "id")) {
			const xmlChar *tmp;
			tmp = xmlBufferContent(buffer);
			status->id = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "text")) {
			const xmlChar *msg;
			gchar *cur;
			msg = xmlBufferContent (buffer);
			status->text = g_markup_escape_text ((const gchar *)msg, -1);

			/* &amp;lt; becomes &lt; */
            /* &amp;gt; becomes &gt; */
            /* &amp;nbsp; becomes &nbsp; */
			cur = status->text;
			while ((cur = strstr (cur, "&amp;"))) {
				if (strncmp (cur + 5, "lt;", 3) == 0 
                          || strncmp (cur + 5, "gt;", 3) == 0
                          || strncmp (cur + 5, "nbsp;", 5) == 0) {
					g_memmove (cur + 1, cur + 5, strlen (cur + 5) + 1);
                } else {
					cur += 5;
                }
			}

            /* remove the &nbsp; */
			cur = status->text;
			while ((cur = strstr (cur, "&nbsp;"))) {
                *cur = ' ';
                g_memmove (cur + 1, cur + 6, strlen (cur + 6) + 1);
			}

		} else if (g_str_equal (cur_node->name, "sender") ||
				   g_str_equal (cur_node->name, "user")) {
			status->user = parser_fwitux_node_user (cur_node->children);
		} else if (g_str_equal (cur_node->name, "source")) {
			const xmlChar *tmp;
			tmp = xmlBufferContent(buffer);
			status->source = g_strdup ((const gchar *)tmp);
        } else if (g_str_equal (cur_node->name, "link")) {
			const xmlChar *tmp;
			tmp = xmlBufferContent(buffer);
			status->link = g_strdup ((const gchar *)tmp);
        } else if (g_str_equal (cur_node->name, "receiver")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent(buffer);
            status->receiver = g_strdup ((const gchar *)tmp);
        } else if (g_str_equal (cur_node->name, "image_address")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent(buffer);
            status->image_address = g_strdup ((const gchar *)tmp);
        } else if (g_str_equal (cur_node->name, "reply_count")) {
            const xmlChar *tmp;
            tmp = xmlBufferContent(buffer);
            status->reply_count = g_strdup ((const gchar *)tmp);
        }

		/* Free buffer content */
		xmlBufferEmpty (buffer);

	} /* End of loop */

	/* Free buffer pointer */
	xmlBufferFree (buffer);
	
	return status;
}

static gint
parser_get_time (const gchar * datetime)
{
    struct tm    post;
	gchar        *oldenv;
    gint         tweet_time;

	oldenv = setlocale (LC_TIME, "C");
	strptime (datetime, "%a %b %d %T +0800 %Y", &post);
    tweet_time =  mktime (&post);
	setlocale (LC_TIME, oldenv);

    return tweet_time;
}

static gchar *
parser_convert_time (const gchar *datetime)
{
	struct tm	 post;
	gint			 seconds_post;
	gint 		 diff;
	gchar        *oldenv;

	time_t		 t = time(NULL);

	tzset ();

	oldenv = setlocale (LC_TIME, "C");

	strptime (datetime, "%a %b %d %T +0800 %Y", &post);
	seconds_post =  mktime (&post);

	setlocale (LC_TIME, oldenv);

	diff = difftime (t, seconds_post);
	
	if (diff < 2) {
		return g_strdup (_("1 second ago"));
	}
	/* Seconds */
	if (diff < 60 ) {
		return g_strdup_printf (_("%i seconds ago"), diff);
	} else if (diff < 120) {
		return g_strdup (_("1 minute ago"));
	} else {
		/* Minutes */
		diff = diff/60;
		if (diff < 60) {
			return g_strdup_printf (_("%i minutes ago"), diff);
		} else if (diff < 120) {
			return g_strdup (_("1 hour ago"));
		} else {
			/* Hours */
			diff = diff/60;
			if (diff < 24) {
				return g_strdup_printf (_("%i hours ago"), diff);
			} else if (diff < 48) {
				return g_strdup (_("1 day ago"));
			} else {
				/* Days */
				diff = diff/24;
				if (diff < 30) {
					return g_strdup_printf (_("%i days ago"), diff);
				} else if (diff < 60) {
					return g_strdup (_("1 month ago"));
				} else {
					return g_strdup_printf (_("%i months ago"), (diff/30));
				}
			}
		}
	}
	return NULL;
}


/* Free a FwituxStatus struct */
static void
parser_free_status (FwituxStatus *status)
{
    if (!status)
        return;

    if (status->text)
        g_free (status->text);
    if (status->created_at)
        g_free (status->created_at);
    if (status->id)
        g_free (status->id);
    if (status->source)
        g_free (status->source);
    if (status->link)
        g_free (status->link);
    if (status->receiver)
        g_free (status->receiver);
    if (status->image_address)
        g_free (status->image_address);
    if (status->reply_count)
        g_free (status->reply_count);

    if (status->user)
        parser_free_user (status->user);
    
    g_free (status);
}

/* Free a FwituxUser struct */
void
parser_free_user (FwituxUser *user)
{
	if (!user)
		return;

	if (user->id)
		g_free (user->id);
	if (user->name)
		g_free (user->name);
	if (user->image_url)
		g_free (user->image_url);
    if (user->url)
        g_free (user->url);
    if (user->location)
        g_free (user->location);
    if (user->description)
        g_free (user->description);
    if (user->sex)
        g_free (user->sex);
    if (user->birthday)
        g_free (user->birthday);
    if (user->mobile)
        g_free (user->mobile);
    if (user->qq)
        g_free (user->qq);
    if (user->msn)
        g_free (user->msn);
    if (user->email)
        g_free (user->email);
    if (user->created_at)
        g_free (user->created_at);

	g_free (user);
}


void
parser_reset_last_tweet_time ()
{
	last_tweet_time = 0;
}
