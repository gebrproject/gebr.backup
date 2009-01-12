/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBGEBR_MISC_UTILS_H
#define __LIBGEBR_MISC_UTILS_H

#include <glib.h>

void
append_filename_extension(GString * filename, const gchar * extension);

GString *
make_unique_filename(const gchar * template);

GString *
make_temp_filename(const gchar * template);

int
home_mode(void);

gboolean
gebr_create_config_dirs(void);

gchar *
g_simple_locale_to_utf8(const gchar * string);

gboolean
g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key);

GString *
g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key, const gchar * default_value);

gboolean
g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key, gboolean default_value);

int
g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value);

#endif //__LIBGEBR_MISC_UTILS_H
