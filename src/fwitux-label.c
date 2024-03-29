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

#include <string.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <libsexy/sexy-url-label.h>

#include "fwitux.h"
#include "fwitux-label.h"
#include "fwitux-network.h"

static void   fwitux_label_class_init   (FwituxLabelClass *clas);
static void   fwitux_label_init         (FwituxLabel      *label);
static void   fwitux_label_finalize     (GObject          *object);

static void  label_url_activated_cb     (GtkWidget        *url_label,
                                         gchar            *url,
                                         gpointer          user_data);
static char* label_msg_get_string       (const char       * message);

G_DEFINE_TYPE (FwituxLabel, fwitux_label, SEXY_TYPE_URL_LABEL);

static void
fwitux_label_class_init (FwituxLabelClass *clas)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (clas);

	/* Add private */

	object_class->finalize = fwitux_label_finalize;
}

static void
fwitux_label_init (FwituxLabel *label)
{
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD_CHAR);
	
	g_object_set (label,
				  "xalign", 0.0,
				  "yalign", 0.0,
				  "xpad", 2,
				  "ypad", 2,
				  NULL);
	
	g_signal_connect (label,
					  "url-activated",
					  G_CALLBACK (label_url_activated_cb),
					  NULL);
}

static void
fwitux_label_finalize (GObject *object)
{
	/* Cleanup code */
}

static void
label_url_activated_cb  (GtkWidget *url_label,
						 gchar     *url,
						 gpointer   user_data)
{
	if (g_app_info_launch_default_for_uri (url, NULL, NULL) == FALSE) {
			g_warning ("Couldn't show URL: '%s'", url);
	}
}

GtkWidget *
fwitux_label_new (void)
{
	return g_object_new (FWITUX_TYPE_LABEL, NULL);
}

void
fwitux_label_set_text (FwituxLabel  *nav,
					   const gchar  *text)
{
	gchar *parsed_text;
	
	if (!text)
		return;
	
	parsed_text = label_msg_get_string (text);
	
	sexy_url_label_set_markup (SEXY_URL_LABEL (nav), parsed_text);
	
	g_free (parsed_text);
}

static gboolean
url_check_word (char *word, int len)
{
#define D(x) (x), ((sizeof (x)) - 1)
	static const struct {
		const char *s;
		gint        len;
	}
	
	prefix[] = {
		{ D("ftp.") },
		{ D("www.") },
		{ D("ftp://") },
		{ D("http://") },
		{ D("https://") },
		{ D("@") },
	};
#undef D

	gint 		i;
	
	for (i = 0; i < G_N_ELEMENTS(prefix); i++) {
		int l;

		l = prefix[i].len;
		if (len > l) {
			gint j;

			/* This is pretty much strncasecmp(). */
			for (j = 0; j < l; j++)	{
				unsigned char c = word[j];
				
				if (g_ascii_tolower (c) != prefix[i].s[j])
					break;
			}
			if (j == l)
				return TRUE;
		}
	}
	
	return FALSE;
}

static gssize
find_first_non_username(const char *str)
{
	gssize i;

	for (i = 0; str[i]; ++i) {
        /*
		if (!(g_ascii_isalnum(str[i]) || str[i] == '_')) {
			return i;
		}
        */
        if (g_ascii_isspace(str[i]))
        {
            return i;
        }
	}
	return -1;
}

static char*
label_msg_get_string (const char* message)
{
	gchar **tokens;
	gchar  *result;
	gchar  *temp;
	gint 	i;
	
	if (G_STR_EMPTY (message)) {
		return NULL;
	}
	
	/* TODO: Do we need to escape out <>&"' so that pango markup doesn't get confused? */
	
	/* surround urls with <a> markup so that sexy-url-label can link it */
	//tokens = g_strsplit_set (message, " \t\n", 0);
	tokens = g_strsplit_set (message, " ", 0);
	for (i = 0; tokens[i]; i++) {
		if (url_check_word (tokens[i], strlen (tokens[i]))) {
			if (tokens[i][0] == '@') {
				gssize end;

				end  = find_first_non_username (&tokens[i][1]) + 1;
				if (end == 0) {
					temp =
						g_strdup_printf ("<a href=\"http://www.follow5.com/f5/mwfm/profile?an=%s\">%s</a>",
										 &tokens[i][1],
										 tokens[i]);
				} else {
					gchar delim;

					delim = tokens[i][end];
					tokens[i][end] = '\0';
					temp =
						g_strdup_printf ("<a href=\"http://www.follow5.com/f5/mwfm/profile?an=%s\">%s</a>%c%s",
										 &tokens[i][1], tokens[i], delim, &tokens[i][end+1]);
				}
			} else {
				temp = g_strdup_printf ("<a href=\"%s\">%s</a>",
										tokens[i], tokens[i]);
			}
			g_free (tokens[i]);
			tokens[i] = temp;
		}
	}
	result = g_strjoinv(" ", tokens);
	g_strfreev (tokens);
	
	return result;	
}
