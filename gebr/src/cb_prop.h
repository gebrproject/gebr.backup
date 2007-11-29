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

#ifndef _GEBR_CB_PROP_H_
#define _GEBR_CB_PROP_H_

#include <gtk/gtk.h>

void
properties_action		(GtkDialog *dialog,
				 gint       arg1,
				 gpointer   user_data);

void
properties_set_to_default		();

void
properties_parameters_set_to_flow	(void);

void
properties_toogle_file_browse      (GtkToggleButton *togglebutton,
				    gpointer         user_data);

#endif //_GEBR_CB_PROP_H_
