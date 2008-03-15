/*   G�BR ME - G�BR Menu Editor
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

#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml.h>
#include <gui/parameter.h>
#include <gui/valuesequenceedit.h>

#include "enumoptionedit.h"

struct parameters_data;
struct parameter_data;
struct parameter_ui_data;

GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, struct parameters_data * parameters_data, gboolean hidden);

void
parameter_add(GtkButton * button, struct parameters_data * parameters_data);

#endif //__PARAMETER_H
