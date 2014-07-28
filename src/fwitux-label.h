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

#ifndef __FWITUX_LABEL_H__
#define __FWITUX_LABEL_H__

#include <libsexy/sexy-url-label.h>

G_BEGIN_DECLS

/*
 * FwituxLabel
 */ 
#define FWITUX_TYPE_LABEL         (fwitux_label_get_type ())
#define FWITUX_LABEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FWITUX_TYPE_LABEL, FwituxLabel))
#define FWITUX_LABEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FWITUX_TYPE_LABEL, FwituxLabelClass))
#define FWITUX_IS_LABEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FWITUX_TYPE_LABEL))
#define FWITUX_IS_LABEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FWITUX_TYPE_LABEL))
#define FWITUX_LABEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FWITUX_TYPE_LABEL, FwituxLabelClass))

typedef struct _FwituxLabel        FwituxLabel;
typedef struct _FwituxLabelClass   FwituxLabelClass;
typedef struct _FwituxLabelPriv    FwituxLabelPriv;

struct _FwituxLabel {
	SexyUrlLabel           parent;
};

struct _FwituxLabelClass {
	SexyUrlLabelClass      parent_class;
};

GType             fwitux_label_get_type  (void) G_GNUC_CONST;
GtkWidget*        fwitux_label_new       (void);
void              fwitux_label_set_text  (FwituxLabel  *nav,
                                          const gchar  *text);
G_END_DECLS

#endif /* __FWITUX_LABEL_H__ */
