/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2010 GeBR core team (http://www.gebrproject.com/)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
char *strptime(const char *s, const char *formato, struct tm *tm);

#include <glib.h>

#include <geoxml.h>
#include "../../utils.h"
#include "../../date.h"

gboolean fixfname = FALSE;
gchar *setfname = NULL;
gchar *title = NULL;
gchar *desc = NULL;
gchar *author = NULL;
gchar *email = NULL;
gchar *date = NULL;
gboolean helpdel = FALSE;
gchar *fnhelp = NULL;
gint iprog = 0;
gchar *binary = NULL;
gchar *version = NULL;
gchar *url = NULL;
gchar **menu = NULL;
gboolean capt = FALSE;

/* Command-line parameters definition */
static GOptionEntry entries[] = {
	{"iprog", 'i', 0, G_OPTION_ARG_INT, &iprog, "index of program to edit", NULL},
	{"fixfname", 'f', 0, G_OPTION_ARG_NONE, &fixfname, "fix filename", NULL},
	{"setfname", 'F', 0, G_OPTION_ARG_FILENAME, &setfname, "set filename", NULL},
	{"title", 't', 0, G_OPTION_ARG_STRING, &title, "set title", ""},
	{"desc", 'd', 0, G_OPTION_ARG_STRING, &desc, "set description", "one line description"},
	{"author", 'A', 0, G_OPTION_ARG_STRING, &author, "set authors", "name"},
	{"email", 'e', 0, G_OPTION_ARG_STRING, &email, "set email", "email address"},
	{"created", 'D', 0, G_OPTION_ARG_STRING, &date, "set created date (see notes below)", NULL},
	{"helpdel", 0, 0, G_OPTION_ARG_NONE, &helpdel, "delete help text", NULL},
	{"sethelp", 'H', 0, G_OPTION_ARG_FILENAME, &fnhelp, "replace help by an HTML file", NULL},
	{"binary", 'b', 0, G_OPTION_ARG_STRING, &binary, "Binary command (without path)", "program/script"},
        {"version", 'V', 0, G_OPTION_ARG_STRING, &version, "Binary's version", NULL},
	{"url", 'u', 0, G_OPTION_ARG_STRING, &url, "URL for a program", "http://www.../"},
	//{ "capitalize", 'c', 0, G_OPTION_ARG_NONE, &capt, "capitalize descriptions and labels", NULL},
	{G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &menu, "",
	 "menu1.mnu menu2.mnu ..."},
	{NULL}
};

GString *help_load(const gchar * fname);

int main(int argc, char **argv)
{

	int nmenu;
	int imenu;

	GError *error = NULL;
	GOptionContext *context;

	/* Summary */
	context = g_option_context_new(NULL);
	g_option_context_set_summary(context,
				     "Edit tags of menu files for GeBR. Many menu files can\n"
				     "be edited at once, but using the same tag values to all menus.");

	/* Description */
	g_option_context_set_description(context,
					 "Parameter --created set menu's creation date. It accepts \"now\" or\n"
					 "a full qualified UTC date, like \"2008-09-23 21:12\".\n\n"
					 "If iprog is 0, then title and description options refers to menu's\n"
					 "title and description. If iprog > 0, then ith program is edited.\n"
					 "Copyright (C) 2008-2010 Ricardo Biloti <biloti@gebrproject.com>");

	g_option_context_add_main_entries(context, entries, NULL);

	/* Complain about unknown options */
	g_option_context_set_ignore_unknown_options(context, FALSE);

	/* Parse command line */
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		return EXIT_FAILURE;
	}
	g_option_context_free(context);

	/* End of command line parse */
	if (menu == NULL)
		return 0;
	nmenu = 0;
	while (menu[++nmenu] != NULL) ;

	for (imenu = 0; imenu < nmenu; imenu++) {
		GebrGeoXmlDocument *doc;
		GebrGeoXmlFlow *flow;
		GebrGeoXmlSequence *seq;
		GebrGeoXmlProgram *prog;
		gint nprog;

		if (gebr_geoxml_document_load((GebrGeoXmlDocument **) (&flow), menu[imenu], TRUE, NULL) !=
		    GEBR_GEOXML_RETV_SUCCESS) {
			fprintf(stderr, "Unable to load %s\n", menu[imenu]);
			break;
		}
		doc = GEBR_GEOXML_DOC(flow);
		nprog = gebr_geoxml_flow_get_programs_number(flow);
		if (fixfname)
			gebr_geoxml_document_set_filename(doc, menu[imenu]);
		if (setfname != NULL)
			gebr_geoxml_document_set_filename(doc, setfname);
		if (author != NULL)
			gebr_geoxml_document_set_author(doc, author);
		if (email != NULL)
			gebr_geoxml_document_set_email(doc, email);
		if (date != NULL) {
			if (strcmp(date, "now") == 0) {
				gebr_geoxml_document_set_date_created(doc, gebr_iso_date());
			} else {
				static gchar datestr[100];
				struct tm tm;
				if (strptime(date, "%Y-%m-%d%H:%M", &tm) == NULL) {
					printf("Date parse error. See help for accepted formats.\n");
					break;
				}
				strftime(datestr, 100, "%Y-%m-%dT%H:%M:00Z", &tm);
				gebr_geoxml_document_set_date_created(doc, datestr);
			}
		}

		if (iprog == 0) {
			if (title != NULL)
				gebr_geoxml_document_set_title(doc, title);
			if (desc != NULL)
				gebr_geoxml_document_set_description(doc, desc);
			if (url || binary || version)
				printf("To set URL, binary, or binary's version you must specify iprog\n");
			if (helpdel)
				gebr_geoxml_document_set_help(doc, "");
			if (fnhelp) {
				GString *html_content;

				html_content = help_load(fnhelp);
				if (html_content == NULL) {
					return EXIT_FAILURE;
				}

				gebr_geoxml_document_set_help(doc, html_content->str);

				g_string_free(html_content, TRUE);
			}
		} else {
			if (iprog > nprog) {
				printf("Invalid program index for menu %s\n", menu[imenu]);
				goto out;
			}

			gebr_geoxml_flow_get_program(flow, &seq, iprog - 1);
			prog = GEBR_GEOXML_PROGRAM(seq);

			if (title != NULL)
				gebr_geoxml_program_set_title(prog, title);
			if (desc != NULL)
				gebr_geoxml_program_set_description(prog, desc);
			if (binary != NULL)
				gebr_geoxml_program_set_binary(prog, binary);
                        if (version != NULL)
				gebr_geoxml_program_set_version(prog, version);
			if (url != NULL)
				gebr_geoxml_program_set_url(prog, url);
			if (helpdel)
				gebr_geoxml_program_set_help(prog, "");

			if (fnhelp) {
				GString *html_content;

				html_content = help_load(fnhelp);
				if (html_content == NULL) {
					return EXIT_FAILURE;
				}

				gebr_geoxml_program_set_help(prog, html_content->str);

				g_string_free(html_content, TRUE);
			}
		}

 out:		gebr_geoxml_document_set_date_modified(doc, gebr_iso_date());
		if (gebr_geoxml_document_save(doc, menu[imenu]) != GEBR_GEOXML_RETV_SUCCESS)
			fprintf(stderr, "Unable to save %s\n", menu[imenu]);
		gebr_geoxml_document_free(doc);
	}

	return 0;
}

GString *help_load(const gchar * fname)
{

	GString *html_content;
	FILE *fp;
	gchar buffer[1000];

	if ((fp = fopen(fnhelp, "r")) == NULL) {
		fprintf(stderr, "Unable to open %s\n", fname);
		return NULL;
	}

	html_content = g_string_new(NULL);
	g_string_assign(html_content, "");
	while (fgets(buffer, sizeof(buffer), fp))
		g_string_append(html_content, buffer);
	fclose(fp);

	/* ensure UTF-8 encoding */
	if (g_utf8_validate(html_content->str, -1, NULL) == FALSE) {
		gchar *converted;

		/* TODO: what else should be tried? */
		converted = gebr_locale_to_utf8(html_content->str);
		if (converted == NULL) {
			g_free(converted);
			fprintf(stderr, "Please change the help encoding to UTF-8");
			g_string_free(html_content, TRUE);
			return NULL;
		}

		g_string_assign(html_content, converted);
		g_free(converted);
	}

	return html_content;
}