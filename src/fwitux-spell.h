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

#ifndef __FWITUX_SPELL_H__
#define __FWITUX_SPELL_H__

G_BEGIN_DECLS

gboolean     fwitux_spell_supported           (void);
const gchar *fwitux_spell_get_language_name   (const gchar *code);
GList       *fwitux_spell_get_language_codes  (void);
void         fwitux_spell_free_language_codes (GList       *codes);
gboolean     fwitux_spell_check               (const gchar *word);
GList *      fwitux_spell_get_suggestions     (const gchar *word);
void         fwitux_spell_free_suggestions    (GList       *suggestions);

G_END_DECLS

#endif /* __FWITUX_SPELL_H__ */
