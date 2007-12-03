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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <misc/utils.h>

#include "ui_help.h"
#include "gebr.h"
#include "support.h"

#define BUFFER_SIZE 1024

gchar * unable_to_write_help_error = _("Unable to write help in temporary file");

/*
 * Function: help_show
 * Open user's browser with _help_
 */
void
help_show(gchar * help, gchar * title, gchar * fname)
{
	FILE *		html_fp;
	GString *	html_path;

	GString *	ghelp;
	GString *	url;
	GString *	cmd_line;

	if (!strlen(help))
		return;
	if (!gebr.config.browser->len) {
		gebr_message(ERROR, TRUE, FALSE, _("No editor defined. Choose one at Configure/Preferences"));
		return;
	}

	/* initialization */
	html_path = g_string_new(NULL);
	ghelp = g_string_new(NULL);
	url = g_string_new(NULL);
	cmd_line = g_string_new(NULL);

	/* Temporary file */
	g_string_printf(html_path, "/tmp/gebr_%s.html", make_temp_filename());

	/* Gambiarra */
	{
		gchar *	gebrcsspos;
		int	pos;

		g_string_assign(ghelp, help);

		if ((gebrcsspos = strstr(ghelp->str, "gebr.css")) != NULL) {
			pos = (gebrcsspos - ghelp->str)/sizeof(char);
			g_string_erase(ghelp, pos, 8);
			g_string_insert(ghelp, pos, "file://" DATA_DIR "/gebr.css");
		}
	}

	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(ERROR, TRUE, TRUE, unable_to_write_help_error);
		goto out;
	}
	fwrite(ghelp->str, sizeof(char), strlen(ghelp->str), html_fp);
	fclose(html_fp);

	/* Add file to list of files to be removed */
	gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_path->str);

	/* Launch an external browser */
	g_string_printf(url, "file://%s", html_path->str);
	g_string_printf(cmd_line, "%s %s &", gebr.config.browser->str, url->str);
	system(cmd_line->str);

	/* frees */
out:	g_string_free(html_path, FALSE);
	g_string_free(ghelp, TRUE);
	g_string_free(url, TRUE);
	g_string_free(cmd_line, TRUE);
}

/* Function: ui_help_edit
 * Edit help in editor.
 * Edit help in editor as reponse to button clicks.
 */
void
help_edit(GtkButton * button, GeoXmlDocument * document)
{
	FILE *		html_fp;
	GString *	html_path;

	gchar		buffer[BUFFER_SIZE];
	GString *	help;
	GString *	cmd_line;

	/* Call an editor */
	if (!gebr.config.editor->len) {
		gebr_message(ERROR, TRUE, FALSE, _("No editor defined. Choose one at Configure/Preferences"));
		return;
	}

	/* initialization */
	html_path = g_string_new(NULL);
	cmd_line = g_string_new(NULL);
	help = g_string_new(NULL);

	/* Temporary file */
	g_string_printf(html_path, "/tmp/gebr_%s.html", make_temp_filename());

	/* Write current help to temporary file */
	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(ERROR, TRUE, TRUE, unable_to_write_help_error);
		goto out;
	}
	fputs(geoxml_document_get_help(document), html_fp);
	fclose(html_fp);

	/* Run editor and wait for user... */
	g_string_printf(cmd_line, "%s %s", gebr.config.editor->str, html_path->str);
	system(cmd_line->str);

	/* Read back the help from file */
	html_fp = fopen(html_path->str, "r");
	while (fgets(buffer, BUFFER_SIZE, html_fp) != NULL)
		g_string_append(help, buffer);
	fclose(html_fp);
	unlink(html_path->str);

	/* Finally, the edited help back to the document */
	geoxml_document_set_help(document, help->str);

	/* frees */
out:	g_string_free(html_path, TRUE);
	g_string_free(cmd_line, TRUE);
	g_string_free(help, TRUE);
}
