/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __PROJECT_H
#define __PROJECT_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

/**
 * Create a new project.
 */
void project_new(void);
/**
 * Delete the selected project.
 *
 * TODO:
 * * If a project is not empty, the user user should be
 *   warned. Besides, it should be asked about erasing
 *   all project's lines.
 * * Project's line files should be deleted as well.
 */
gboolean project_delete(gboolean confirm);

/**
 * Add \p project to the list of projects.
 */
GtkTreeIter project_append_iter(GebrGeoXmlProject * project);

/**
 * Add \p line to \p project_iter.
 */
GtkTreeIter project_append_line_iter(GtkTreeIter * project_iter, GebrGeoXmlLine * line);

/**
 * Reload the projets from the data directory.
 */
void project_list_populate(void);

/**
 * Moves \p src_line to \p dest_project before or after \p position, depending on \p before flag.
 */
void project_line_move(GebrGeoXmlProjectLine * src_line, GebrGeoXmlProject * dest_project,
		       GebrGeoXmlProjectLine * position, gboolean before);


#endif				//__PROJECT_H
