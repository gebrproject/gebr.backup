/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#ifndef _GEBR_MENUS_H_
#define _GEBR_MENUS_H_

#include <glib.h>

GeoXmlFlow *
menu_load(const gchar * filename);

GeoXmlFlow *
menu_load_path(const gchar * path);

GString *
menu_get_path(const gchar * filename);

int
menus_populate(void);

gboolean
menus_create_index(void);

#endif //_GEBR_MENUS_H_
