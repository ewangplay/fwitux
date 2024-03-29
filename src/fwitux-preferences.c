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

#include <libfwitux/fwitux-conf.h>
#include <libfwitux/fwitux-xml.h>

#include "fwitux.h"
#include "fwitux-app.h"
#include "fwitux-preferences.h"
#include "fwitux-spell.h"

#define XML_FILE "prefs_dlg.xml"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *notebook;
	GtkWidget *treeview_spell_checker;
	GtkWidget *combo_default_timeline;
	GtkWidget *combo_reload;

	/* Checkbuttons */
	GtkWidget *expand_status;
	GtkWidget *auto_connect;
	GtkWidget *notify;
	GtkWidget *sound_notify;
	GtkWidget *use_realname;
	GtkWidget *spell_check;

	GList     *notify_ids;
} FwituxPrefs;

typedef struct _reload_time {
	gint   minutes;
	gchar *display_text;
} reload_time;

reload_time reload_list[] = {
	{3, N_("3 minutes")},
	{5, N_("5 minutes")},
	{15, N_("15 minutes")},
	{30, N_("30 minutes")},
	{60, N_("hour")},
	{0, NULL}
};

enum {
	COL_LANG_ENABLED,
	COL_LANG_CODE,
	COL_LANG_NAME,
	COL_LANG_COUNT
};

enum {
	COL_COMBO_VISIBLE_NAME,
	COL_COMBO_NAME,
	COL_COMBO_COUNT
};

static void     preferences_setup_widgets          (FwituxPrefs            *prefs);
static void     preferences_languages_add          (FwituxPrefs            *prefs);
static void     preferences_languages_save         (FwituxPrefs            *prefs);
static gboolean preferences_languages_save_foreach (GtkTreeModel           *model,
													GtkTreePath            *path,
													GtkTreeIter            *iter,
													gchar                 **languages);
static void     preferences_languages_load         (FwituxPrefs            *prefs);
static gboolean preferences_languages_load_foreach (GtkTreeModel           *model,
													GtkTreePath            *path,
													GtkTreeIter            *iter,
													gchar                 **languages);
static void preferences_languages_cell_toggled_cb  (GtkCellRendererToggle  *cell,
												    gchar                  *path_string,
												    FwituxPrefs            *prefs);
static void     preferences_timeline_setup         (FwituxPrefs            *prefs);
static void     preferences_widget_sync_bool       (const gchar            *key,
													GtkWidget              *widget);
static void   preferences_widget_sync_string_combo (const gchar            *key,
													GtkWidget              *widget);
static void   preferences_widget_sync_int_combo    (const gchar            *key,
													GtkWidget              *widget);
static void     preferences_notify_bool_cb         (FwituxConf             *conf,
													const gchar            *key,
													gpointer                user_data);
static void     preferences_notify_string_combo_cb (FwituxConf             *conf,
													const gchar            *key,
													gpointer                user_data);
static void     preferences_notify_int_combo_cb    (FwituxConf             *conf,
													const gchar            *key,
													gpointer                user_data);
static void     preferences_notify_sensitivity_cb  (FwituxConf             *conf,
													const gchar            *key,
													gpointer                user_data);
static void     preferences_hookup_toggle_button   (FwituxPrefs            *prefs,
													const gchar            *key,
													GtkWidget              *widget);
static void     preferences_hookup_string_combo    (FwituxPrefs            *prefs,
													const gchar            *key,
													GtkWidget              *widget);
static void     preferences_hookup_int_combo       (FwituxPrefs            *prefs,
													const gchar            *key,
													GtkWidget              *widget);
static void     preferences_hookup_sensitivity     (FwituxPrefs            *prefs,
													const gchar            *key,
													GtkWidget              *widget);
static void preferences_response_cb                (GtkWidget              *widget,
													gint                    response,
													FwituxPrefs            *prefs);
static void preferences_toggle_button_toggled_cb   (GtkWidget              *button,
													gpointer                user_data);
static void preferences_string_combo_changed_cb    (GtkWidget              *button,
													gpointer                user_data);
static void preferences_int_combo_changed_cb       (GtkWidget              *combo,
													gpointer                user_data);
static void preferences_destroy_cb                 (GtkWidget              *widget,
													FwituxPrefs            *prefs);

static void
preferences_setup_widgets (FwituxPrefs *prefs)
{
	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_TWEETS_SHOW_NAMES,
									  prefs->use_realname);

	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_UI_EXPAND_MESSAGES,
									  prefs->expand_status);

	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_AUTH_AUTO_LOGIN,
									  prefs->auto_connect);

	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_UI_NOTIFICATION,
									  prefs->notify);

	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_UI_SOUND,
									  prefs->sound_notify);

	preferences_hookup_toggle_button (prefs,
									  FWITUX_PREFS_UI_SPELL,
									  prefs->spell_check);

	preferences_hookup_sensitivity (prefs,
									FWITUX_PREFS_UI_SPELL_LANGUAGES,
									prefs->treeview_spell_checker);

	preferences_hookup_string_combo (prefs,
									 FWITUX_PREFS_TWEETS_HOME_TIMELINE,
									 prefs->combo_default_timeline);

	preferences_hookup_int_combo (prefs,
								  FWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
								  prefs->combo_reload);
}
static void
preferences_languages_setup (FwituxPrefs *prefs)
{
	GtkTreeView       *view;
	GtkListStore      *store;
	GtkTreeSelection  *selection;
	GtkTreeModel      *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
	guint              col_offset;

	view = GTK_TREE_VIEW (prefs->treeview_spell_checker);

	store = gtk_list_store_new (COL_LANG_COUNT,
								G_TYPE_BOOLEAN, /*enabled */
								G_TYPE_STRING,  /* code */
								G_TYPE_STRING); /* name */

	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));

	selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	model = GTK_TREE_MODEL (store);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled",
					  G_CALLBACK (preferences_languages_cell_toggled_cb),
					  prefs);

	column = gtk_tree_view_column_new_with_attributes (NULL,
													   renderer,
													   "active", COL_LANG_ENABLED,
													   NULL);

	gtk_tree_view_append_column (view, column);

	renderer = gtk_cell_renderer_text_new ();
	col_offset = gtk_tree_view_insert_column_with_attributes (view,
															  -1, _("Language"),
															  renderer,
															  "text", COL_LANG_NAME,
															  NULL);

	g_object_set_data (G_OBJECT (renderer),
					   "column", GINT_TO_POINTER (COL_LANG_NAME));

	column = gtk_tree_view_get_column (view, col_offset -1);
	gtk_tree_view_column_set_sort_column_id (column, COL_LANG_NAME);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

	g_object_unref (store);
}

static void
preferences_languages_add (FwituxPrefs *prefs)
{
	GtkTreeView  *view;
	GtkListStore *store;
	GList        *codes;
	GList        *l;

	view = GTK_TREE_VIEW (prefs->treeview_spell_checker);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (view));

	codes = fwitux_spell_get_language_codes ();
	for (l = codes; l; l = l->next) {
		GtkTreeIter  iter;
		const gchar *code;
		const gchar *name;

		code = l->data;
		name = fwitux_spell_get_language_name (code);
		if (!name) {
			continue;
		}

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
							COL_LANG_CODE, code,
							COL_LANG_NAME, name,
							-1);
	}

	fwitux_spell_free_language_codes (codes);
}

static void
preferences_languages_save (FwituxPrefs *prefs)
{
	GtkTreeView  *view;
	GtkTreeModel *model;

	gchar        *languages = NULL;

	view = GTK_TREE_VIEW (prefs->treeview_spell_checker);
	model = gtk_tree_view_get_model (view);

	gtk_tree_model_foreach (model,
							(GtkTreeModelForeachFunc)
							preferences_languages_save_foreach,
							&languages);

	if (!languages) {
		/* Default to english */
		languages = g_strdup ("en");
	}

	fwitux_conf_set_string (fwitux_conf_get (),
							FWITUX_PREFS_UI_SPELL_LANGUAGES,
							languages);
	g_free (languages);
}

static gboolean
preferences_languages_save_foreach (GtkTreeModel  *model,
									GtkTreePath   *path,
									GtkTreeIter   *iter,
									gchar        **languages)
{
	gboolean  enabled;
	gchar    *code;

	if (!languages) {
		return TRUE;
	}

	gtk_tree_model_get (model, iter, COL_LANG_ENABLED, &enabled, -1);
	if (!enabled) {
		return FALSE;
	}

	gtk_tree_model_get (model, iter, COL_LANG_CODE, &code, -1);
	if (!code) {
		return FALSE;
	}

	if (!(*languages)) {
		*languages = g_strdup (code);
	} else {
		gchar *str = *languages;
		*languages = g_strdup_printf ("%s,%s", str, code);
		g_free (str);
	}

	g_free (code);

	return FALSE;
}

static void
preferences_languages_load (FwituxPrefs *prefs)
{
	GtkTreeView   *view;
	GtkTreeModel  *model;
	gchar         *value;
	gchar        **vlanguages;

	if (!fwitux_conf_get_string (fwitux_conf_get (),
								 FWITUX_PREFS_UI_SPELL_LANGUAGES,
								 &value) || !value) {
		return;
	}

	vlanguages = g_strsplit (value, ",", -1);
	g_free (value);

	view = GTK_TREE_VIEW (prefs->treeview_spell_checker);
	model = gtk_tree_view_get_model (view);

	gtk_tree_model_foreach (model,
							(GtkTreeModelForeachFunc)
							preferences_languages_load_foreach,
							vlanguages);

	g_strfreev (vlanguages);
}

static gboolean
preferences_languages_load_foreach (GtkTreeModel  *model,
									GtkTreePath   *path,
									GtkTreeIter   *iter,
									gchar        **languages)
{
	gchar    *code;
	gchar    *lang;
	gint      i;
	gboolean  found = FALSE;

	if (!languages) {
		return TRUE;
	}

	gtk_tree_model_get (model, iter, COL_LANG_CODE, &code, -1);
	if (!code) {
		return FALSE;
	}

	for (i = 0, lang = languages[i]; lang; lang = languages[++i]) {
		if (strcmp (lang, code) == 0) {
			found = TRUE;
		}
	}

	gtk_list_store_set (GTK_LIST_STORE (model), iter, COL_LANG_ENABLED, found, -1);

	return FALSE;
}

static void
preferences_languages_cell_toggled_cb (GtkCellRendererToggle *cell,
									   gchar                 *path_string,
									   FwituxPrefs           *prefs)
{
	GtkTreeView  *view;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	gboolean      enabled;

	view = GTK_TREE_VIEW (prefs->treeview_spell_checker);
	model = gtk_tree_view_get_model (view);
	store = GTK_LIST_STORE (model);

	path = gtk_tree_path_new_from_string (path_string);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, COL_LANG_ENABLED, &enabled, -1);

	enabled ^= 1;

	gtk_list_store_set (store, &iter, COL_LANG_ENABLED, enabled, -1);
	gtk_tree_path_free (path);

	preferences_languages_save (prefs);
}

static void
preferences_notify_bool_cb (FwituxConf  *conf,
							const gchar *key,
							gpointer     user_data)
{
	preferences_widget_sync_bool (key, user_data);
}

static void
preferences_timeline_setup (FwituxPrefs *prefs)
{
	static const gchar *timelines[] = {
		FWITUX_API_TIMELINE_PUBLIC, N_("Public"),
		FWITUX_API_TIMELINE_FRIENDS, N_("Friend"),
		FWITUX_API_TIMELINE_USER, N_("Mine"),
		FWITUX_API_PRIVATE_FRIEND, N_("Private"),
        FWITUX_API_MENTIONS_ME, N_("@Me"),
		NULL
	};

	GtkListStore    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	gint             i;

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (prefs->combo_default_timeline),
								renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (prefs->combo_default_timeline),
									renderer, "text", COL_COMBO_VISIBLE_NAME, NULL);

	model = gtk_list_store_new (COL_COMBO_COUNT,
								G_TYPE_STRING,
								G_TYPE_STRING);

	gtk_combo_box_set_model (GTK_COMBO_BOX (prefs->combo_default_timeline),
							 GTK_TREE_MODEL (model));

	for (i = 0; timelines[i]; i += 2) {
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
							COL_COMBO_VISIBLE_NAME, _(timelines[i + 1]),
							COL_COMBO_NAME, timelines[i],
							-1);
	}

	g_object_unref (model);
}

static void
preferences_reload_setup (FwituxPrefs *prefs)
{
	GtkListStore    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	reload_time     *options;

	options = reload_list;

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (prefs->combo_reload),
								renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (prefs->combo_reload),
									renderer, "text", COL_COMBO_VISIBLE_NAME, NULL);

	model = gtk_list_store_new (COL_COMBO_COUNT,
								G_TYPE_STRING,
								G_TYPE_INT);

	gtk_combo_box_set_model (GTK_COMBO_BOX (prefs->combo_reload),
							 GTK_TREE_MODEL (model));

	while (options->display_text != NULL) {
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
							COL_COMBO_VISIBLE_NAME, _(options->display_text),
							COL_COMBO_NAME, options->minutes,
							-1);
		options++;
	}

	g_object_unref (model);
}

static void
preferences_widget_sync_bool (const gchar *key, GtkWidget *widget)
{
	gboolean value;

	if (fwitux_conf_get_bool (fwitux_conf_get (), key, &value)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
	}
}

static void
preferences_widget_sync_string_combo (const gchar *key, GtkWidget *widget)
{
	gchar        *value;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      found;

	if (!fwitux_conf_get_string (fwitux_conf_get (), key, &value)) {
		return;
	}

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	found = FALSE;
	if (value && gtk_tree_model_get_iter_first (model, &iter)) {
		gchar *name;

		do {
			gtk_tree_model_get (model, &iter,
								COL_COMBO_NAME, &name,
								-1);

			if (strcmp (name, value) == 0) {
				found = TRUE;
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
				break;
			} else {
				found = FALSE;
			}

			g_free (name);
		} while (gtk_tree_model_iter_next (model, &iter));
	}

	/* Fallback to the first one. */
	if (!found) {
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
		}
	}

	g_free (value);
}

static void
preferences_widget_sync_int_combo (const gchar *key, GtkWidget *widget)
{
	gint          value;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      found;

	if (!fwitux_conf_get_int (fwitux_conf_get (), key, &value)) {
		return;
	}

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	found = FALSE;
	if (value && gtk_tree_model_get_iter_first (model, &iter)) {
		gint minutes;

		do {
			gtk_tree_model_get (model, &iter,
								COL_COMBO_NAME, &minutes,
								-1);

			if (minutes == value) {
				found = TRUE;
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
				break;
			} else {
				found = FALSE;
			}

		} while (gtk_tree_model_iter_next (model, &iter));
	}

	/* Fallback to the first one. */
	if (!found) {
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
		}
	}
}

static void
preferences_notify_string_combo_cb (FwituxConf  *conf,
									const gchar *key,
									gpointer     user_data)
{
	preferences_widget_sync_string_combo (key, user_data);
}

static void
preferences_notify_int_combo_cb (FwituxConf  *conf,
								 const gchar *key,
								 gpointer     user_data)
{
	preferences_widget_sync_int_combo (key, user_data);
}

static void
preferences_notify_sensitivity_cb (FwituxConf  *conf,
								   const gchar *key,
								   gpointer     user_data)
{
	gboolean value;

	if (fwitux_conf_get_bool (conf, key, &value)) {
		gtk_widget_set_sensitive (GTK_WIDGET (user_data), value);
	}
}

static void
preferences_add_id (FwituxPrefs *prefs, guint id)
{
	prefs->notify_ids = g_list_prepend (prefs->notify_ids,
										GUINT_TO_POINTER (id));
}

static void
preferences_hookup_toggle_button (FwituxPrefs *prefs,
								  const gchar *key,
								  GtkWidget   *widget)
{
	guint id;

	preferences_widget_sync_bool (key, widget);

	g_object_set_data_full (G_OBJECT (widget), "key",
							g_strdup (key), g_free);

	g_signal_connect (widget,
					  "toggled",
					  G_CALLBACK (preferences_toggle_button_toggled_cb),
					  NULL);

	id = fwitux_conf_notify_add (fwitux_conf_get (),
								 key,
								 preferences_notify_bool_cb,
								 widget);

	if (id) {
		preferences_add_id (prefs, id);
	}
}

static void
preferences_hookup_string_combo (FwituxPrefs *prefs,
								 const gchar *key,
								 GtkWidget   *widget)
{
	guint id;

	preferences_widget_sync_string_combo (key, widget);

	g_object_set_data_full (G_OBJECT (widget), "key",
							g_strdup (key), g_free);

	g_signal_connect (widget,
					  "changed",
					  G_CALLBACK (preferences_string_combo_changed_cb),
					  NULL);

	id = fwitux_conf_notify_add (fwitux_conf_get (),
								 key,
								 preferences_notify_string_combo_cb,
								 widget);
	if (id) {
		preferences_add_id (prefs, id);
	}
}

static void
preferences_hookup_int_combo (FwituxPrefs *prefs,
								 const gchar *key,
								 GtkWidget   *widget)
{
	guint id;

	preferences_widget_sync_int_combo (key, widget);

	g_object_set_data_full (G_OBJECT (widget), "key",
							g_strdup (key), g_free);

	g_signal_connect (widget,
					  "changed",
					  G_CALLBACK (preferences_int_combo_changed_cb),
					  NULL);

	id = fwitux_conf_notify_add (fwitux_conf_get (),
								 key,
								 preferences_notify_int_combo_cb,
								 widget);

	if (id) {
		preferences_add_id (prefs, id);
	}
}

static void
preferences_hookup_sensitivity (FwituxPrefs *prefs,
								const gchar *key,
								GtkWidget   *widget)
{
	gboolean value;
	guint    id;

	if (fwitux_conf_get_bool (fwitux_conf_get (), key, &value)) {
		gtk_widget_set_sensitive (widget, value);
	}

	id = fwitux_conf_notify_add (fwitux_conf_get (),
								 key,
								 preferences_notify_sensitivity_cb,
								 widget);

	if (id) {
		preferences_add_id (prefs, id);
	}
}

static void
preferences_toggle_button_toggled_cb (GtkWidget *button,
									  gpointer   user_data)
{
	const gchar *key;

	key = g_object_get_data (G_OBJECT (button), "key");

	fwitux_conf_set_bool (fwitux_conf_get (),
						  key,
						  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));

    /* if the ckeckbutton is expand message checkbutton,
     * and cancel the selection state, then hide the expand message box 
     */
    if (g_str_equal(key, FWITUX_PREFS_UI_EXPAND_MESSAGES)) {
        if(!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
            fwitux_app_hide_expand_message();
        }
    }
}

static void
preferences_string_combo_changed_cb (GtkWidget *combo,
									 gpointer   user_data)
{
	const gchar  *key;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *name;

	key = g_object_get_data (G_OBJECT (combo), "key");

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

		gtk_tree_model_get (model, &iter,
							COL_COMBO_NAME, &name,
							-1);
		fwitux_conf_set_string (fwitux_conf_get (), key, name);
		g_free (name);
	}
}

static void
preferences_int_combo_changed_cb (GtkWidget *combo,
								  gpointer   user_data)
{
	const gchar  *key;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          minutes;

	key = g_object_get_data (G_OBJECT (combo), "key");

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

		gtk_tree_model_get (model, &iter,
							COL_COMBO_NAME, &minutes,
							-1);

		fwitux_conf_set_int (fwitux_conf_get (), key, minutes);
	}
}

static void
preferences_response_cb (GtkWidget   *widget,
						 gint         response,
						 FwituxPrefs *prefs)
{
	gtk_widget_destroy (widget);
}
static void
preferences_destroy_cb (GtkWidget   *widget,
						FwituxPrefs *prefs)
{
	GList *l;

	for (l = prefs->notify_ids; l; l = l->next) {
		guint id;

		id = GPOINTER_TO_UINT (l->data);
		fwitux_conf_notify_remove (fwitux_conf_get (), id);
	}

	g_list_free (prefs->notify_ids);
	g_free (prefs);
}

void
fwitux_preferences_dialog_show (GtkWindow *parent)
{
	static FwituxPrefs *prefs;
	GtkBuilder         *ui;

	if (prefs) {
		gtk_window_present (GTK_WINDOW (prefs->dialog));
		return;
	}

	prefs = g_new0 (FwituxPrefs, 1);

	/* Get widgets */
	ui = fwitux_xml_get_file (XML_FILE,
							  "preferences_dialog", &prefs->dialog,
							  "preferences_notebook", &prefs->notebook,
							  "combobox_timeline", &prefs->combo_default_timeline,
							  "combobox_reload", &prefs->combo_reload,
							  "expand_checkbutton", &prefs->expand_status,
							  "autoconnect_checkbutton", &prefs->auto_connect,
							  "notify_checkbutton", &prefs->notify,
							  "names_checkbutton", &prefs->use_realname,
							  "spell_checkbutton", &prefs->spell_check,
							  "sound_checkbutton", &prefs->sound_notify,
							  "spell_treeview", &prefs->treeview_spell_checker,
							  NULL);

	/* Connect the signals */
	fwitux_xml_connect (ui, prefs,
						"preferences_dialog", "destroy", preferences_destroy_cb,
						"preferences_dialog", "response", preferences_response_cb,
						NULL);

	g_object_unref (ui);

	g_object_add_weak_pointer (G_OBJECT (prefs->dialog), (gpointer) &prefs);
	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent);

	/* Set up the rest of the widget */
	preferences_timeline_setup (prefs);
	preferences_reload_setup (prefs);

	preferences_setup_widgets (prefs);

	preferences_languages_setup (prefs);
	preferences_languages_add (prefs);
	preferences_languages_load (prefs);

	/* If compiled with spelling support, show the notebook page */
	if (fwitux_spell_supported ()) {
		GtkWidget *page;

		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs->notebook), 1);
		gtk_widget_show (page);
	}

    /* hide the following widget becuase they are not used now */
    gtk_widget_hide (prefs->sound_notify);
    gtk_widget_hide (prefs->use_realname);

	gtk_widget_show (prefs->dialog);
}
