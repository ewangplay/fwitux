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

#ifndef __FWITUX_CONF_H__
#define __FWITUX_CONF_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define FWITUX_TYPE_CONF         (fwitux_conf_get_type ())
#define FWITUX_CONF(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_CONF, FwituxConf))
#define FWITUX_CONF_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FWITUX_TYPE_CONF, FwituxConfClass))
#define FWITUX_IS_CONF(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_CONF))
#define FWITUX_IS_CONF_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_CONF))
#define FWITUX_CONF_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_CONF, FwituxConfClass))

typedef struct _FwituxConf      FwituxConf;
typedef struct _FwituxConfClass FwituxConfClass;

struct _FwituxConf  {
	GObject parent;
};

struct _FwituxConfClass {
	GObjectClass parent_class;
};

typedef void (*FwituxConfNotifyFunc) (FwituxConf  *conf, 
									  const gchar *key,
									  gpointer     user_data);

GType       fwitux_conf_get_type        (void) G_GNUC_CONST;
FwituxConf *fwitux_conf_get             (void);
void        fwitux_conf_shutdown        (void);
guint       fwitux_conf_notify_add      (FwituxConf            *conf,
										 const gchar           *key,
										 FwituxConfNotifyFunc   func,
										 gpointer               data);
gboolean    fwitux_conf_notify_remove   (FwituxConf            *conf,
										 guint                  id);
gboolean    fwitux_conf_set_int         (FwituxConf            *conf,
										 const gchar           *key,
										 gint                   value);
gboolean    fwitux_conf_get_int         (FwituxConf            *conf,
										 const gchar           *key,
										 gint                  *value);
gboolean    fwitux_conf_set_bool        (FwituxConf            *conf,
										 const gchar           *key,
										 gboolean               value);
gboolean    fwitux_conf_get_bool        (FwituxConf            *conf,
										 const gchar           *key,
										 gboolean              *value);
gboolean    fwitux_conf_set_string      (FwituxConf            *conf,
										 const gchar           *key,
										 const gchar           *value);
gboolean    fwitux_conf_get_string      (FwituxConf            *conf,
										 const gchar           *key,
										 gchar                **value);
gboolean    fwitux_conf_set_string_list (FwituxConf            *conf,
										 const gchar           *key,
										 GSList                *value);
gboolean    fwitux_conf_get_string_list (FwituxConf            *conf,
										 const gchar           *key,
										 GSList              **value);

G_END_DECLS

#endif /* __FWITUX_CONF_H__ */

