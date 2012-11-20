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

#include "clipboard.h"
#include "xml.h"
#include "types.h"

extern GdomeDocument *clipboard_document;

void gebr_geoxml_clipboard_clear(void)
{
	GdomeElement *document_element;
	GdomeElement *element;

	document_element = gdome_doc_documentElement(clipboard_document, &exception);
	while ((element = __gebr_geoxml_get_first_element(document_element, "*")))
		gdome_el_removeChild(document_element, (GdomeNode *) element, &exception);
}

void gebr_geoxml_clipboard_copy(GebrGeoXmlObject * object)
{
	GdomeNode * imported_node;
	GdomeElement * document_element;

	g_return_if_fail(object != NULL);

	document_element = gdome_doc_documentElement(clipboard_document, &exception);
	imported_node = gdome_doc_importNode(clipboard_document, (GdomeNode*)object, TRUE, &exception);

	gdome_el_appendChild(document_element, imported_node, &exception);
}

GebrGeoXmlObject *gebr_geoxml_clipboard_paste(GebrGeoXmlObject * object)
{
	if (object == NULL)
		return NULL;

	static const gchar *child_parent[][3] = {
		{"program", "flow", ""},
		{"parameter", "program", "parameters"},
		{NULL, NULL, NULL}
	};
	GdomeElement *paste_element;
	GdomeElement *container_element;
	GebrGeoXmlObject *first_paste = NULL;

	container_element = gdome_n_nodeType((GdomeNode *) object, &exception) == GDOME_DOCUMENT_NODE
	    ? gdome_doc_documentElement((GdomeDocument *) object, &exception) : (GdomeElement *) object;
	paste_element = __gebr_geoxml_get_first_element(gdome_doc_documentElement(clipboard_document, &exception), "*");
	for (; paste_element != NULL; paste_element = __gebr_geoxml_next_element(paste_element)) {
		for (int i = 0; child_parent[i][0] != NULL; ++i) {
			if (!strcmp(gdome_el_tagName(paste_element, &exception)->str, child_parent[i][0]) &&
			    !strcmp(gdome_el_tagName(container_element, &exception)->str, child_parent[i][1])) {
				GdomeNode *imported;
				GdomeDocument *document;

				document = gdome_el_ownerDocument(container_element, &exception);
				imported = gdome_doc_importNode(document, (GdomeNode*)paste_element, TRUE, &exception);
				if (!strlen(child_parent[i][2]))
					gdome_el_appendChild(container_element, imported, &exception);
				else
					gdome_el_appendChild(__gebr_geoxml_get_first_element
							     (container_element, child_parent[i][2]), imported,
							     &exception);
				if (first_paste == NULL)
					first_paste = GEBR_GEOXML_OBJECT(imported);
				break;
			}
		}
	}

	return first_paste;
}