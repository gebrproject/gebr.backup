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

#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "document.h"
#include "gebr.h"
#include "line.h"
#include "flow.h"
#include "menu.h"


/**
 * Create a new document with _type_
 *
 * Create a new document in the user's data diretory
 * with _type_ and set its filename.
 */
GebrGeoXmlDocument *document_new(enum GEBR_GEOXML_DOCUMENT_TYPE type)
{
	gchar *extension;
	GString *filename;

	GebrGeoXmlDocument *document;
	GebrGeoXmlDocument *(*new_func) ();

	switch (type) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		extension = "flw";
		new_func = (typeof(new_func)) gebr_geoxml_flow_new;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		extension = "lne";
		new_func = (typeof(new_func)) gebr_geoxml_line_new;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		extension = "prj";
		new_func = (typeof(new_func)) gebr_geoxml_project_new;
		break;
	default:
		return NULL;
	}

	/* finaly create it and... */
	document = new_func();

	/* then set filename */
	filename = document_assembly_filename(extension);
	gebr_geoxml_document_set_filename(document, filename->str);
	g_string_free(filename, TRUE);

	/* get today's date */
	gebr_geoxml_document_set_date_created(document, gebr_iso_date());

	return document;
}

/**
 * Load a document (flow, line or project) from its filename, handling errors
 */
GebrGeoXmlDocument *document_load(const gchar * filename)
{
	GebrGeoXmlDocument *document;
	GString *path;

	path = document_get_path(filename);
	document = document_load_path(path->str);
	document_save(document);
	g_string_free(path, TRUE);

	return document;
}

/**
 * Load a document (flow, line or project) from its filename, handling errors
 */
GebrGeoXmlDocument *document_load_at(const gchar * filename, const gchar * directory)
{
	GebrGeoXmlDocument *document;
	GString *path;

	path = g_string_new("");
	g_string_printf(path, "%s/%s", directory, filename);
	document = document_load_path(path->str);
	g_string_free(path, TRUE);

	return document;
}
/**
 * Callback to remove menu reference from program to maintain backward compatibility
 */
static void  __document_discard_menu_ref_callback(GebrGeoXmlProgram * program, const gchar * filename, gint index)
{
	GebrGeoXmlFlow *menu;

	GebrGeoXmlSequence *menu_program;

	menu = menu_load_ancient(filename);
	if (menu == NULL){
		return;
	}	

	/* go to menu's program index specified in flow */
	gebr_geoxml_flow_get_program(menu, &menu_program, index);
	gebr_geoxml_program_set_help(program, gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(menu_program)));


	gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));
		
}	
/**
 * Load a document from its path, handling errors
 */
GebrGeoXmlDocument *document_load_path(const gchar * path)
{
	GebrGeoXmlDocument *document;
	int ret;

	if ((ret = gebr_geoxml_document_load(&document, path, 
	g_str_has_suffix(path, ".flw") ? __document_discard_menu_ref_callback : NULL)) < 0)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load document at %s: %s."), path,
			     gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));

	return document;
}

/**
 * Save _document_ using its filename field at data directory.
 */
void document_save(GebrGeoXmlDocument * document)
{
	GString *path;

	/* get today's date */
	gebr_geoxml_document_set_date_modified(document, gebr_iso_date());

	/* TODO: check save */
	path = document_get_path(gebr_geoxml_document_get_filename(document));
	gebr_geoxml_document_save(document, path->str);

	g_string_free(path, TRUE);
}

/**
 * Import _document_ into data directory, saving it with a new filename.
 */
void document_import(GebrGeoXmlDocument * document)
{
	GString *path;
	GString *new_filename;
	const gchar *extension;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		extension = "flw";
		flow_set_paths_to(GEBR_GEOXML_FLOW(document), FALSE);
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		extension = "lne";
		line_set_paths_to(GEBR_GEOXML_LINE(document), FALSE);
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		extension = "prj";
		break;
	default:
		return;
	}
	new_filename = document_assembly_filename(extension);

	/* TODO: check save */
	path = document_get_path(new_filename->str);
	gebr_geoxml_document_set_filename(document, new_filename->str);
	gebr_geoxml_document_save(document, path->str);

	g_string_free(path, TRUE);
	g_string_free(new_filename, TRUE);
}

/**
 * Creates a filename for a document
 *
 * Creates a filename for a document using the current date and a random
 * generated string and _extension_, ensuring that it is unique in user's data directory.
 */
GString *document_assembly_filename(const gchar * extension)
{
	time_t t;
	struct tm *lt;
	gchar date[21];

	GString *filename;
	GString *path;
	GString *aux;
	gchar *basename;

	/* initialization */
	filename = g_string_new(NULL);
	aux = g_string_new(NULL);

	/* get today's date */
	time(&t);
	lt = localtime(&t);
	strftime(date, 20, "%Y_%m_%d", lt);

	/* create it */
	g_string_printf(aux, "%s/%s_XXXXXX.%s", gebr.config.data->str, date, extension);
	path = gebr_make_unique_filename(aux->str);

	/* get only what matters: the filename */
	basename = g_path_get_basename(path->str);
	g_string_assign(filename, basename);

	/* frees */
	g_string_free(path, TRUE);
	g_string_free(aux, TRUE);
	g_free(basename);

	return filename;
}

/**
 * Prefix filename with data diretory path
 */
GString *document_get_path(const gchar * filename)
{
	GString *path;

	path = g_string_new(NULL);
	g_string_printf(path, "%s/%s", gebr.config.data->str, filename);

	return path;
}

/**
 * Delete document with _filename_ from data directory.
 */
void document_delete(const gchar * filename)
{
	GString *path;

	path = document_get_path(filename);
	g_unlink(path->str);

	g_string_free(path, TRUE);
}
