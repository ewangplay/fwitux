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

#include <gconf/gconf-client.h>

#include "fwitux-conf.h"
#include "fwitux-debug.h"

#define DEBUG_DOMAIN "Config"

#define FWITUX_CONF_ROOT        "/apps/fwitux"
#define DESKTOP_INTERFACE_ROOT  "/desktop/gnome/interface"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FWITUX_TYPE_CONF, FwituxConfPriv))

typedef struct {
	GConfClient *gconf_client;
} FwituxConfPriv;

typedef struct {
	FwituxConf           *conf;
	FwituxConfNotifyFunc  func;
	gpointer              user_data;
} FwituxConfNotifyData;

static void conf_finalize (GObject *object);

G_DEFINE_TYPE (FwituxConf, fwitux_conf, G_TYPE_OBJECT);

static FwituxConf *global_conf = NULL;

static void
fwitux_conf_class_init (FwituxConfClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = conf_finalize;

	g_type_class_add_private (object_class, sizeof (FwituxConfPriv));
}

static void
fwitux_conf_init (FwituxConf *conf)
{
	FwituxConfPriv *priv;

	priv = GET_PRIV (conf);

	priv->gconf_client = gconf_client_get_default ();

	gconf_client_add_dir (priv->gconf_client,
						  FWITUX_CONF_ROOT,
						  GCONF_CLIENT_PRELOAD_ONELEVEL,
						  NULL);
	gconf_client_add_dir (priv->gconf_client,
						  DESKTOP_INTERFACE_ROOT,
						  GCONF_CLIENT_PRELOAD_NONE,
						  NULL);
}

static void
conf_finalize (GObject *object)
{
	FwituxConfPriv *priv;

	priv = GET_PRIV (object);

	gconf_client_remove_dir (priv->gconf_client,
							 FWITUX_CONF_ROOT,
							 NULL);
	gconf_client_remove_dir (priv->gconf_client,
							 DESKTOP_INTERFACE_ROOT,
							 NULL);

	g_object_unref (priv->gconf_client);

	G_OBJECT_CLASS (fwitux_conf_parent_class)->finalize (object);
}

FwituxConf *
fwitux_conf_get (void)
{
	if (!global_conf) {
		global_conf = g_object_new (FWITUX_TYPE_CONF, NULL);
	}

	return global_conf;
}

void
fwitux_conf_shutdown (void)
{
	if (global_conf) {
		g_object_unref (global_conf);
		global_conf = NULL;
	}
}

gboolean
fwitux_conf_set_int (FwituxConf  *conf,
					 const gchar *key,
					 gint         value)
{
	FwituxConfPriv *priv;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	fwitux_debug (DEBUG_DOMAIN, "Setting int:'%s' to %d", key, value);

	priv = GET_PRIV (conf);

	return gconf_client_set_int (priv->gconf_client,
								 key,
								 value,
								 NULL);
}

gboolean
fwitux_conf_get_int (FwituxConf  *conf,
					 const gchar *key,
					 gint        *value)
{
	FwituxConfPriv *priv;
	GError         *error = NULL;

	*value = 0;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	priv = GET_PRIV (conf);

	*value = gconf_client_get_int (priv->gconf_client,
								   key,
								   &error);

	fwitux_debug (DEBUG_DOMAIN, "Getting int:'%s' (=%d), error:'%s'",
				  key, *value, error ? error->message : "None");

	if (error) {
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

gboolean
fwitux_conf_set_bool (FwituxConf  *conf,
					  const gchar *key,
					  gboolean     value)
{
	FwituxConfPriv *priv;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	fwitux_debug (DEBUG_DOMAIN, "Setting bool:'%s' to %d ---> %s",
				  key, value, value ? "true" : "false");

	priv = GET_PRIV (conf);

	return gconf_client_set_bool (priv->gconf_client,
								  key,
								  value,
								  NULL);
}

gboolean
fwitux_conf_get_bool (FwituxConf  *conf,
					  const gchar *key,
					  gboolean    *value)
{
	FwituxConfPriv *priv;
	GError         *error = NULL;

	*value = FALSE;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	priv = GET_PRIV (conf);

	*value = gconf_client_get_bool (priv->gconf_client,
									key,
									&error);

	fwitux_debug (DEBUG_DOMAIN, "Getting bool:'%s' (=%d ---> %s), error:'%s'",
				  key, *value, *value ? "true" : "false",
				  error ? error->message : "None");

	if (error) {
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

gboolean
fwitux_conf_set_string (FwituxConf  *conf,
						const gchar *key,
						const gchar *value)
{
	FwituxConfPriv *priv;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	fwitux_debug (DEBUG_DOMAIN, "Setting string:'%s' to '%s'",
				  key, value);

	priv = GET_PRIV (conf);

	return gconf_client_set_string (priv->gconf_client,
									key,
									value,
									NULL);
}

gboolean
fwitux_conf_get_string (FwituxConf   *conf,
						const gchar  *key,
						gchar       **value)
{
	FwituxConfPriv *priv;
	GError         *error = NULL;

	*value = NULL;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	priv = GET_PRIV (conf);

	*value = gconf_client_get_string (priv->gconf_client,
									  key,
									  &error);

	fwitux_debug (DEBUG_DOMAIN, "Getting string:'%s' (='%s'), error:'%s'",
				  key, *value, error ? error->message : "None");

	if (error) {
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

gboolean
fwitux_conf_set_string_list (FwituxConf  *conf,
							 const gchar *key,
							 GSList      *value)
{
	FwituxConfPriv *priv;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	priv = GET_PRIV (conf);

	return gconf_client_set_list (priv->gconf_client,
								  key,
								  GCONF_VALUE_STRING,
								  value,
								  NULL);
}

gboolean
fwitux_conf_get_string_list (FwituxConf   *conf,
							 const gchar  *key,
							 GSList      **value)
{
	FwituxConfPriv *priv;
	GError         *error = NULL;

	*value = NULL;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	priv = GET_PRIV (conf);

	*value = gconf_client_get_list (priv->gconf_client,
									key,
									GCONF_VALUE_STRING,
									&error);
	if (error) {
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

static void
conf_notify_data_free (FwituxConfNotifyData *data)
{
	g_object_unref (data->conf);
	g_slice_free (FwituxConfNotifyData, data);
}

static void
conf_notify_func (GConfClient *client,
				  guint        id,
				  GConfEntry  *entry,
				  gpointer     user_data)
{
	FwituxConfNotifyData *data;

	data = user_data;

	data->func (data->conf,
				gconf_entry_get_key (entry),
				data->user_data);
}

guint
fwitux_conf_notify_add (FwituxConf           *conf,
						const gchar          *key,
						FwituxConfNotifyFunc  func,
						gpointer              user_data)
{
	FwituxConfPriv       *priv;
	guint                 id;
	FwituxConfNotifyData *data;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), 0);

	priv = GET_PRIV (conf);

	data = g_slice_new (FwituxConfNotifyData);
	data->func = func;
	data->user_data = user_data;
	data->conf = g_object_ref (conf);

	id = gconf_client_notify_add (priv->gconf_client,
								  key,
								  conf_notify_func,
								  data,
								  (GFreeFunc) conf_notify_data_free,
								  NULL);

	return id;
}

gboolean
fwitux_conf_notify_remove (FwituxConf *conf,
						   guint       id)
{
	FwituxConfPriv *priv;

	g_return_val_if_fail (FWITUX_IS_CONF (conf), FALSE);

	priv = GET_PRIV (conf);

	gconf_client_notify_remove (priv->gconf_client, id);

	return TRUE;
}

