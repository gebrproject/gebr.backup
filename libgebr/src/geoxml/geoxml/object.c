/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <string.h>

#include <gdome.h>

#include "object.h"
#include "types.h"
#include "xml.h"
#include "document_p.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_object {
	GdomeElement *element;
};

/*
 * library functions.
 */

GebrGeoXmlObjectType gebr_geoxml_object_get_type(GebrGeoXmlObject * object)
{
	GdomeElement * element;

	static const gchar *tag_map[] = { "",
		"project", "line", "flow",
		"program", "parameters", "parameter",
		"option"
	};
	guint i;

	g_return_val_if_fail(object != NULL, GEBR_GEOXML_OBJECT_TYPE_UNKNOWN);

	if (gdome_n_nodeType((GdomeNode*)object, &exception) == GDOME_DOCUMENT_NODE)
		element = gdome_doc_documentElement((GdomeDocument*)object, &exception);
	else
		element = (GdomeElement*)object;

	for (i = 1; i <= 7; ++i)
		if (!strcmp(gdome_el_tagName(element, &exception)->str, tag_map[i]))
			return (GebrGeoXmlObjectType)i;

	return GEBR_GEOXML_OBJECT_TYPE_UNKNOWN;
}

void gebr_geoxml_object_set_user_data(GebrGeoXmlObject * object, gpointer user_data)
{
	g_return_if_fail(object != NULL);

	GebrGeoXmlObjectType type = gebr_geoxml_object_get_type(object);
	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW || type == GEBR_GEOXML_OBJECT_TYPE_LINE || type == GEBR_GEOXML_OBJECT_TYPE_PROJECT)
		_gebr_geoxml_document_get_data(object)->user_data = user_data;
	else
		((GdomeNode *) object)->user_data = user_data;
}

gpointer gebr_geoxml_object_get_user_data(GebrGeoXmlObject * object)
{
	GebrGeoXmlObjectType type;
	
	type = gebr_geoxml_object_get_type(object);

	g_return_val_if_fail(object != NULL, NULL);

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW || type == GEBR_GEOXML_OBJECT_TYPE_LINE || type == GEBR_GEOXML_OBJECT_TYPE_PROJECT)
		return _gebr_geoxml_document_get_data(object)->user_data;
	else
		return ((GdomeNode *) object)->user_data;
}

GebrGeoXmlDocument *gebr_geoxml_object_get_owner_document(GebrGeoXmlObject * object)
{
	g_return_val_if_fail(object != NULL, NULL);

	return (GebrGeoXmlDocument *) gdome_el_ownerDocument((GdomeElement *) object, &exception);
}

GebrGeoXmlObject *gebr_geoxml_object_copy(GebrGeoXmlObject * object)
{
	g_return_val_if_fail(object != NULL, NULL);

	return (GebrGeoXmlObject *) gdome_el_cloneNode((GdomeElement *) object, TRUE, &exception);
}