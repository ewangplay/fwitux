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

#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-xml.h>
#ifdef HAVE_GNOME_KEYRING
#include <libfwitux/fwitux-keyring.h>
#endif

#include "fwitux.h"
#include "fwitux-account-dialog.h"

#define XML_FILE "account_dlg.xml"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *show_password;
} FwituxAccount;

static void      account_response_cb          (GtkWidget         *widget,
											   gint               response,
											   FwituxAccount     *act);
static void      account_destroy_cb           (GtkWidget         *widget,
											   FwituxAccount     *act);
static void      account_show_password_cb     (GtkWidget         *widget,
											   FwituxAccount     *act);

static void
account_response_cb (GtkWidget     *widget,
					 gint           response,
					 FwituxAccount *act)
{
	if (response == GTK_RESPONSE_OK) {
		FwituxConf *conf;

		conf = fwitux_conf_get ();

		fwitux_conf_set_string (conf,
								FWITUX_PREFS_AUTH_USER,
								gtk_entry_get_text (GTK_ENTRY (act->username)));

#ifdef HAVE_GNOME_KEYRING
		fwitux_keyring_set_password (gtk_entry_get_text (GTK_ENTRY (act->username)),
									 gtk_entry_get_text (GTK_ENTRY (act->password)));
#else
		fwitux_conf_set_string (conf,
								FWITUX_PREFS_AUTH_PASSWORD,
								gtk_entry_get_text (GTK_ENTRY (act->password)));
#endif
	}
	gtk_widget_destroy (widget);
}

static void
account_destroy_cb (GtkWidget     *widget,
					FwituxAccount *act)
{
	g_free (act);
}

static void
account_show_password_cb (GtkWidget     *widget,
						  FwituxAccount *act)
{
	gboolean visible;

	visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (act->show_password));
	gtk_entry_set_visibility (GTK_ENTRY (act->password), visible);
}

void
fwitux_account_dialog_show (GtkWindow *parent)
{
	static FwituxAccount *act;
	GtkBuilder           *ui;
	FwituxConf           *conf;
	gchar                *username;
	gchar                *password;

	if (act) {
		gtk_window_present (GTK_WINDOW (act->dialog));
		return;
	}

	act = g_new0 (FwituxAccount, 1);

	/* Get widgets */
	ui = fwitux_xml_get_file (XML_FILE,
						"account_dialog", &act->dialog,
						"username_entry", &act->username,
						"password_entry", &act->password,
						"show_password_checkbutton", &act->show_password,
						NULL);

	/* Connect the signals */
	fwitux_xml_connect (ui, act,
						"account_dialog", "destroy", account_destroy_cb,
						"account_dialog", "response", account_response_cb,
						"show_password_checkbutton", "toggled", account_show_password_cb,
						NULL);

	g_object_unref (ui);

	g_object_add_weak_pointer (G_OBJECT (act->dialog), (gpointer) &act);

	gtk_window_set_transient_for (GTK_WINDOW (act->dialog), parent);

	/*
	 * Check to see if the username & pasword are already in gconf,
	 * and if so fill in the appropriate entry widget.
	 */
	conf = fwitux_conf_get ();
	fwitux_conf_get_string (conf,
							FWITUX_PREFS_AUTH_USER,
							&username);
	gtk_entry_set_text (GTK_ENTRY (act->username), username ? username : "");

#ifdef HAVE_GNOME_KEYRING
	/* If there is no username, don't bother searching for the password */
	if (G_STR_EMPTY (username)) {
		username = NULL;
		password = NULL;
	} else {
		if (!(fwitux_keyring_get_password (username, &password))) {
			password = NULL;
		}
	}
#else
	fwitux_conf_get_string (conf,
							FWITUX_PREFS_AUTH_PASSWORD,
							&password);
#endif

	gtk_entry_set_text (GTK_ENTRY (act->password), password ? password : "");
	g_free (username);
	g_free (password);

	/* Ok, let's go ahead and show it */
	gtk_widget_show (act->dialog);
}
