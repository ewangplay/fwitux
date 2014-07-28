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

#include <glib/gi18n.h>

#include <gdk/gdkkeysyms.h>

#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-debug.h>
#include <libfwitux/fwitux-xml.h>

#include "fwitux.h"
#include "fwitux-message-dialog.h"
#include "fwitux-spell.h"
#include "fwitux-spell-dialog.h"
#include "fwitux-network.h"

#define DEBUG_DOMAIN_SETUP    "SendMessage"
#define XML_FILE              "message_dlg.xml"

/* Let's use the preferred maximum character count */
#define MAX_CHARACTER_COUNT     200

#define GET_PRIV(obj)          \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_MESSAGE_DIALOG, FwituxMessageDialogPriv))

typedef struct {
	GtkWidget         *textview;
	gchar             *word;
	GtkTextIter        start;
	GtkTextIter        end;
} FwituxMessageSpell;

struct _FwituxMessageDialogPriv {
	/* Main widgets */
	GtkWidget         *dialog;
	GtkWidget         *textview;
	GtkWidget         *label;
	GtkWidget         *send_button;
	GtkWidget         *friends_combo;
	GtkWidget         *friends_label;

	gboolean           show_friends;
    gchar             *reply_status_id;
    gint               source_type;
};

static void	fwitux_message_dialog_class_init		     (FwituxMessageDialogClass *klass);
static void fwitux_message_dialog_init			         (FwituxMessageDialog      *dialog);

static void message_finalize                     (GObject              *object);
static void message_setup                        (GtkWindow            *parent);
static void message_set_characters_available     (GtkTextBuffer        *buffer,
												  FwituxMessageDialog      *dialog);
static void message_text_buffer_changed_cb       (GtkTextBuffer        *buffer,
											      FwituxMessageDialog      *dialog);
static void message_text_populate_popup_cb       (GtkTextView          *view,
												  GtkMenu              *menu,
												  FwituxMessageDialog      *dialog);
static void message_text_check_word_spelling_cb  (GtkMenuItem          *menuitem,
												  FwituxMessageSpell   *message_spell);
static FwituxMessageSpell *message_spell_new     (GtkWidget            *window,
												  const gchar          *word,
												  GtkTextIter           start,
												  GtkTextIter           end);
static void message_spell_free                   (FwituxMessageSpell   *message_spell);
static void message_destroy_cb                   (GtkWidget            *widget,
												  FwituxMessageDialog      *dialog_loc);
static void message_response_cb                  (GtkWidget            *widget,
												  gint                  response,
												  FwituxMessageDialog      *dialog);

static FwituxMessageDialog  *dialog = NULL;

G_DEFINE_TYPE (FwituxMessageDialog, fwitux_message_dialog, G_TYPE_OBJECT);

static void
fwitux_message_dialog_class_init (FwituxMessageDialogClass *klass)
{
	GObjectClass  *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = message_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxMessageDialogPriv));
}

static void
fwitux_message_dialog_init (FwituxMessageDialog *singleton_message)
{
    FwituxMessageDialogPriv    *priv;

	dialog = singleton_message;
    
    priv = GET_PRIV(dialog);

    priv->reply_status_id = NULL;
    priv->source_type = 0;
}

static void
message_finalize (GObject *object)
{	
	G_OBJECT_CLASS (fwitux_message_dialog_parent_class)->finalize (object);
}

static void
message_setup (GtkWindow  *parent)
{
	FwituxMessageDialogPriv   *priv;
	GtkBuilder            *ui;
	GtkTextBuffer         *buffer;
	const gchar           *standard_msg;
	gchar                 *character_count;
	GtkCellRenderer       *renderer;
	GtkListStore          *model_friends;
    GtkAccelGroup         *accel_group;
	
	priv = GET_PRIV (dialog);

	/* Set up interface */
	fwitux_debug (DEBUG_DOMAIN_SETUP, "Initialising message dialog");

	/* Get widgets */
	ui =
		fwitux_xml_get_file (XML_FILE,
							 "send_message_dialog", &priv->dialog,
							 "send_message_textview", &priv->textview,
							 "char_label", &priv->label,
							 "friends_combo", &priv->friends_combo,
							 "friends_label", &priv->friends_label,
							 "send_button", &priv->send_button,
							 NULL);
	
	/* Connect the signals */
	fwitux_xml_connect (ui, dialog,
						"send_message_dialog", "destroy", message_destroy_cb,
						"send_message_dialog", "response", message_response_cb,
						"send_message_textview", "populate_popup", message_text_populate_popup_cb,
						NULL);

	g_object_unref (ui);

	/* Set the label */
	standard_msg = _("Characters Available");
	character_count =
		g_markup_printf_escaped ("<span size=\"small\">%s: %i</span>",
								 standard_msg, MAX_CHARACTER_COUNT);
	gtk_label_set_markup (GTK_LABEL (priv->label), character_count);
	g_free (character_count);

	/* Connect the signal to the textview */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	g_signal_connect (buffer,
					  "changed",
					  G_CALLBACK (message_text_buffer_changed_cb),
					  dialog);

	/* Create misspelt words identification tag */
	gtk_text_buffer_create_tag (buffer,
								"misspelled",
								"underline", PANGO_UNDERLINE_ERROR,
								NULL);

	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog), parent);

	/* Setup friends combobox's model */
	model_friends = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (priv->friends_combo),
							 GTK_TREE_MODEL (model_friends));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->friends_combo),
								renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->friends_combo),
									renderer, "text", 0, NULL);

	gtk_widget_grab_focus(GTK_WIDGET (priv->textview));
	
    /* register accel key on message dialog */
    accel_group = gtk_accel_group_new();
    gtk_widget_add_accelerator(GTK_WIDGET (priv->send_button), "clicked", accel_group, 
                               GDK_Return,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group(GTK_WINDOW (priv->dialog), accel_group);

	/* Show the dialog */
	gtk_widget_show (GTK_WIDGET (priv->dialog));
}

void
fwitux_message_dialog_show (GtkWindow *parent)
{
	FwituxMessageDialogPriv   *priv;
	
	if (dialog){
		priv = GET_PRIV (dialog);
		gtk_window_present (GTK_WINDOW (priv->dialog));
		return;
	}

	g_object_new (FWITUX_TYPE_MESSAGE_DIALOG, NULL);

	message_setup (parent);
}

void
fwitux_message_dialog_correct_word (GtkWidget   *textview,
							 GtkTextIter  start,
							 GtkTextIter  end,
							 const gchar *new_word)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (textview != NULL);
	g_return_if_fail (new_word != NULL);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_insert (buffer, &start,
							new_word,
							-1);
}

void
fwitux_message_dialog_set_friends (GList *friends)
{
	FwituxMessageDialogPriv *priv;
	GList               *list;
	GtkTreeIter          iter;
	FwituxUser          *user;
	GtkListStore        *model_friends;
    GtkComboBox         *friends_combo;

	priv = GET_PRIV (dialog);

    friends_combo = GTK_COMBO_BOX (priv->friends_combo);

	model_friends = GTK_LIST_STORE (gtk_combo_box_get_model (friends_combo));

	for (list = friends; list; list = list->next) {
		user = (FwituxUser *)list->data;
		gtk_list_store_append (model_friends, &iter);
		gtk_list_store_set (model_friends,
							&iter,
							0, user->name,
                            1, user->id,
							-1);
	}

    /* 设置列表显示第一个元素 */
    gtk_combo_box_set_active(friends_combo, 0);
}

void
fwitux_message_dialog_show_friends (gboolean show_friends)
{
	FwituxMessageDialogPriv *priv = GET_PRIV (dialog);
	priv->show_friends = show_friends;

	if (show_friends){
		GList *friends;
		gtk_widget_show (priv->friends_combo);
		gtk_widget_show (priv->friends_label);
		
		/* Let's populate the combobox */
		friends = fwitux_network_get_friends_for_message_dialog ();
        if (friends){
            fwitux_debug (DEBUG_DOMAIN_SETUP, "Loaded previous friends list");
            fwitux_message_dialog_set_friends (friends);
        } else {
            fwitux_debug (DEBUG_DOMAIN_SETUP, "Fetching friends...");
        }

		return;
	}
	gtk_widget_hide (priv->friends_combo);
	gtk_widget_hide (priv->friends_label);
}

void
fwitux_message_dialog_set_message (const gchar *message)
{
	FwituxMessageDialogPriv *priv = GET_PRIV (dialog);
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));

	gtk_text_buffer_set_text (buffer, message, -1);

	gtk_window_set_focus (GTK_WINDOW (priv->dialog), priv->textview);
}

void 
fwitux_message_dialog_set_reply_status_id(const gchar * status_id)
{
    FwituxMessageDialogPriv    *priv;

    priv = GET_PRIV(dialog);

    if(priv->reply_status_id)
    {
        g_free(priv->reply_status_id);
        priv->reply_status_id = NULL;
    }

    priv->reply_status_id = g_strdup(status_id);
}

void
fwitux_message_dialog_set_caption(const gchar * caption)
{
	FwituxMessageDialogPriv *priv = GET_PRIV (dialog);
    gtk_window_set_title(GTK_WINDOW(priv->dialog), caption);
}

static gchar *
url_encode_message (gchar *text)
{
	const char        *good;
	static const char  hex[16] = "0123456789ABCDEF";
	GString           *result;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (*text != '\0', NULL);

	/* RFC 3986 */ 
	good = "~-._";

	result = g_string_new (NULL);
	while (*text) {
		unsigned char c = *text++;

		if (g_ascii_isalnum (c) || strchr (good, c))
			g_string_append_c (result, c);
		else {
			g_string_append_c (result, '%');
			g_string_append_c (result, hex[c >> 4]);
			g_string_append_c (result, hex[c & 0xf]);
		}
	}

	return g_string_free (result, FALSE);
}

static void
message_set_characters_available (GtkTextBuffer     *buffer,
								  FwituxMessageDialog   *dialog)
{
	FwituxMessageDialogPriv *priv;
	gint i;
	gint count;
	const gchar *standard_msg;
	gchar *character_count;

	priv = GET_PRIV (dialog);

	i = gtk_text_buffer_get_char_count (buffer);

	count = MAX_CHARACTER_COUNT - i;

	standard_msg = _("Characters Available");

	if (i > MAX_CHARACTER_COUNT) {
		character_count =
			g_markup_printf_escaped ("<span size=\"small\">%s: <span foreground=\"red\">%i</span></span>",
									 standard_msg, count);
		gtk_widget_set_sensitive (priv->send_button, FALSE);
	} else {
		character_count =
			g_markup_printf_escaped ("<span size=\"small\">%s: %i</span>",
									 standard_msg, count);
		gtk_widget_set_sensitive (priv->send_button, TRUE);
	}

	gtk_label_set_markup (GTK_LABEL (priv->label), character_count);
	g_free (character_count);
}

static void
message_text_buffer_changed_cb (GtkTextBuffer    *buffer,
                                FwituxMessageDialog  *dialog)
{
	FwituxMessageDialogPriv   *priv;
	GtkTextIter            start;
	GtkTextIter            end;
	gchar                 *str;
	gboolean               spell_checker = FALSE;

	priv = GET_PRIV (dialog);

	message_set_characters_available (buffer, dialog);

	fwitux_conf_get_bool (fwitux_conf_get (),
						  FWITUX_PREFS_UI_SPELL,
						  &spell_checker);

	gtk_text_buffer_get_start_iter (buffer, &start);

	if (!spell_checker) {
		gtk_text_buffer_get_end_iter (buffer, &end);
		gtk_text_buffer_remove_tag_by_name (buffer, "misspelled", &start, &end);
		return;
	}

	if (!fwitux_spell_supported ()) {
		return;
	}

	while (TRUE) {
		gboolean correct = FALSE;

		/* if at start */
		if (gtk_text_iter_is_start (&start)) {
			end = start;

			if (!gtk_text_iter_forward_word_end (&end)) {
				/* no whole word yet */
				break;
			}
		} else {
			if (!gtk_text_iter_forward_word_end (&end)) {
				/* must be the end of the buffer */
				break;
			}

			start = end;
			gtk_text_iter_backward_word_start (&start);
		}

		str = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

		/* spell check string */
		correct = fwitux_spell_check (str);

		if (!correct) {
			gtk_text_buffer_apply_tag_by_name (buffer, "misspelled",
											   &start, &end);
		} else {
			gtk_text_buffer_remove_tag_by_name (buffer, "misspelled",
												&start, &end);
		}

		g_free (str);

		/* set the start iter to the end iters position */
		start = end;
	}
}

static void
message_text_populate_popup_cb (GtkTextView        *view,
								GtkMenu            *menu,
								FwituxMessageDialog    *dialog)
{
	FwituxMessageDialogPriv   *priv;
	GtkTextBuffer         *buffer;
	GtkTextTagTable       *table;
	GtkTextTag            *tag;
	gint                   x,y;
	GtkTextIter            iter, start, end;
	GtkWidget             *item;
	gchar                 *str = NULL;
	FwituxMessageSpell    *message_spell;

	priv = GET_PRIV (dialog);

	/* Add the spell check menu item */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	table = gtk_text_buffer_get_tag_table (buffer);

	tag = gtk_text_tag_table_lookup (table, "misspelled");

	gtk_widget_get_pointer (GTK_WIDGET (view), &x, &y);

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (view),
										   GTK_TEXT_WINDOW_WIDGET,
										   x, y,
										   &x, &y);

	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (view), &iter, x, y);

	start = end = iter;

	if (gtk_text_iter_backward_to_tag_toggle (&start, tag) &&
		gtk_text_iter_forward_to_tag_toggle (&end, tag)) {
		str = gtk_text_buffer_get_text (buffer,
										&start, &end, FALSE);
	}

	if (G_STR_EMPTY (str)) {
		return;
	}

	message_spell = message_spell_new (priv->textview, str, start, end);

	g_object_set_data_full (G_OBJECT (menu),
							"message_spell", message_spell,
							(GDestroyNotify) message_spell_free);

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new_with_mnemonic (_("_Check Word Spelling..."));
	g_signal_connect (item,
					  "activate",
					  G_CALLBACK (message_text_check_word_spelling_cb),
					  message_spell);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
}

static void
message_text_check_word_spelling_cb (GtkMenuItem        *menuitem,
									 FwituxMessageSpell *chat_spell)
{
	fwitux_spell_dialog_show (chat_spell->textview,
							  chat_spell->start,
							  chat_spell->end,
							  chat_spell->word);
}

static FwituxMessageSpell *
message_spell_new (GtkWidget          *textview,
				   const gchar        *word,
				   GtkTextIter         start,
				   GtkTextIter         end)
{
	FwituxMessageSpell *message_spell;

	message_spell = g_new0 (FwituxMessageSpell, 1);

	message_spell->textview = textview;
	message_spell->word = g_strdup (word);
	message_spell->start = start;
	message_spell->end = end;

	return message_spell;
}

static void
message_spell_free (FwituxMessageSpell *message_spell)
{
	g_free (message_spell->word);
	g_free (message_spell);
}

static void
message_response_cb (GtkWidget          *widget,
					 gint                response,
					 FwituxMessageDialog    *dialog)
{
	FwituxMessageDialogPriv   *priv;

	if (response == GTK_RESPONSE_OK) {	  
		GtkTextBuffer  *buffer;
		GtkTextIter     start_iter;
		GtkTextIter     end_iter;

		priv = GET_PRIV (dialog);

		fwitux_debug (DEBUG_DOMAIN_SETUP, "Posting message to Follow5");

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
		gtk_text_buffer_get_start_iter (buffer, &start_iter);
		gtk_text_buffer_get_end_iter (buffer, &end_iter);

		if (!gtk_text_iter_equal (&start_iter, &end_iter)) {
				gchar          *text;
				gchar          *good_msg;

				text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, TRUE);
				good_msg = url_encode_message (text);
				
				if (priv->show_friends)
				{
					GtkTreeIter   iter;
					gchar        *friend_id;
					GtkComboBox  *combo = GTK_COMBO_BOX (priv->friends_combo);
					GtkTreeModel *model = gtk_combo_box_get_model (combo);
					/* Send a direct message  */
					if (gtk_combo_box_get_active_iter (combo, &iter)){
						/* Get friend id */
						gtk_tree_model_get (model,
											&iter,
											1,
                                            &friend_id,
											-1);
						/* Send the message */
						fwitux_network_send_message (friend_id, good_msg);
						g_free (friend_id);
					}
                } else if (priv->reply_status_id) {
                    /* reply a message */
                    fwitux_network_reply_status (priv->reply_status_id, good_msg, priv->source_type);
                    g_free(priv->reply_status_id);
                    priv->reply_status_id = NULL;
				} else {
					/* Post a tweet */
					fwitux_network_post_status (good_msg);
				}

				g_free (text);
				g_free (good_msg);
		}
	}
	gtk_widget_destroy (widget);
}

static void
message_destroy_cb (GtkWidget         *widget,
					FwituxMessageDialog   *dialog_loc)
{
	FwituxMessageDialogPriv *priv;

	priv = GET_PRIV (dialog);

    /* release the reply status id */
    if(priv->reply_status_id)
    {
        g_free(priv->reply_status_id);
        priv->reply_status_id = NULL;
    }

    /* release the dialog object */
	g_object_unref (dialog);
	dialog = NULL;
}

void fwitux_message_dialog_set_source_type (gint source_type)
{
	FwituxMessageDialogPriv *priv;

	priv = GET_PRIV (dialog);

    priv->source_type = source_type;
}

