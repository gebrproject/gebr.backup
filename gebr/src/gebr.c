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

/*
 * File: gebr.c
 */
#include "gebr.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "interface.h"
#include "cmdline.h"
#include "callbacks.h"
#include "interface.h"
#include "cb_proj.h"
#include "server.h"
#include "menus.h"

/*
 * Function: gebr_init
 * Take initial measures
 */
void
gebr_init(void)
{
	gchar	fname[STRMAX];
	gchar *	home;

	/* initialization */
	gebr.server_store = NULL;

	/* assembly user's gebr directory */
	home = getenv("HOME");
	sprintf(fname, "%s/.gebr", home);

	/* log file */
	strcat(fname, "/gebr.log");
	gebr.log_fp = fopen(fname, "a+");

	/* allocating list of temporary files */
	gebr.tmpfiles = g_slist_alloc();

	/* list of servers */
	protocol_init();

	/* icons */
	{
		GError *	error;

		error = NULL;
		gebr.pixmaps.unconfigured_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_unconfigured.png", &error);
		gebr.pixmaps.configured_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_configured.png", &error);
		gebr.pixmaps.disabled_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_disabled.png", &error);
		gebr.pixmaps.running_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_running.png", &error);
	}

	log_message(START, "GêBR Initiating...", TRUE);
}

/*
 * Function: gebr_init
 * Free memory, remove temporaries files and quit
 */
gboolean
gebr_quit(void)
{
	g_slist_foreach(gebr.tmpfiles, (GFunc) unlink, NULL);
	g_slist_foreach(gebr.tmpfiles, (GFunc) free, NULL);

	g_slist_free(gebr.tmpfiles);

	g_object_unref(gebr.pixmaps.unconfigured_icon);
	g_object_unref(gebr.pixmaps.configured_icon);
	g_object_unref(gebr.pixmaps.disabled_icon);

	gtk_main_quit();

	return FALSE;
}

/*
 * Function: gebr_config_load
 * Initialize configuration for G�BR
 */
int
gebr_config_load(int argc, char ** argv)
{
	gchar	fname[STRMAX];
	gchar *	home = getenv("HOME");

	/* assembly config. directory */
	sprintf(fname, "%s/.gebr", home);
	/* create it if necessary */
	if (access(fname, F_OK)) {
		struct stat	home_stat;

		stat(home, &home_stat);
		if (mkdir(fname, home_stat.st_mode))
			exit(EXIT_FAILURE);
	}

	/* Init server list store */
	gebr.server_store = gtk_list_store_new (SERVER_N_COLUMN,
					G_TYPE_STRING,
					G_TYPE_POINTER);
	strcat(fname,"/gebr.conf");

	gebr.pref.username_value = g_string_new(NULL);
	gebr.pref.email_value = g_string_new(NULL);
	gebr.pref.editor_value = g_string_new(NULL);
	gebr.pref.usermenus_value = g_string_new(NULL);
	gebr.pref.data_value = g_string_new(NULL);
	gebr.pref.browser_value = g_string_new(NULL);

	if (access (fname, F_OK) == 0) {
		int i;
		/* Initialize GeBR with option in gebr.conf */
		if(cmdline_parser_configfile (fname, &gebr.config, 1, 1, 0) != 0) {
			fprintf(stderr,"%s: try '--help' option\n", argv[0]);
			exit(EXIT_FAILURE);
		}

		/* Filling our structure in */
		g_string_append(gebr.pref.username_value, gebr.config.name_arg);
		g_string_append(gebr.pref.email_value, gebr.config.email_arg);
		g_string_append(gebr.pref.editor_value, gebr.config.editor_arg);
		g_string_append(gebr.pref.usermenus_value, gebr.config.usermenus_arg);
		g_string_append(gebr.pref.data_value, gebr.config.data_arg);
		g_string_append(gebr.pref.browser_value, gebr.config.browser_arg);

		if (!(gebr.config.usermenus_given &&
			gebr.config.data_given &&
			gebr.config.editor_given &&
			gebr.config.browser_given
			)) {
			assembly_preference_win();
			gtk_widget_show_all (gebr.pref.win);
		} else {
			menus_populate ();
			projects_refresh();
		}

		if (!gebr.config.server_given){
		   GtkTreeIter	iter;
		   gchar hostname[100];

		   gethostname(hostname, 100);
		   gtk_list_store_append (gebr.server_store, &iter);
		   gtk_list_store_set (gebr.server_store, &iter,
				       SERVER_ADDRESS, hostname,
				       SERVER_POINTER, server_new(hostname),
				       -1);
		}
		else{
		   for (i=0; i <gebr.config.server_given; i++) {
		      GtkTreeIter	iter;

		      /* TODO: free servers structs on exit */
		      gtk_list_store_append (gebr.server_store, &iter);
		      gtk_list_store_set (gebr.server_store, &iter,
					  SERVER_ADDRESS, gebr.config.server_arg[i],
					  SERVER_POINTER, server_new(gebr.config.server_arg[i]),
					  -1);
		   }
		}
	}
	else {
		assembly_preference_win();
		gtk_widget_show_all (gebr.pref.win);
	}

	return EXIT_SUCCESS;
}

/*
 * Function: gebr_config_reload
 * Reload configuration for G�BR
 */
int
gebr_config_reload(void)
{
	menus_create_index();
	menus_populate ();
	projects_refresh();

	return EXIT_SUCCESS;
}

/*
 * Function: gebr_config_save
 * Save G�BR config to file.
 *
 * Write .gebr.conf file.
 */
int
gebr_config_save(void)
{
	FILE *		fp;
	char 		fname[STRMAX];

	strcpy (fname, getenv ("HOME"));
	strcat (fname, "/.gebr/gebr.conf");

	fp = fopen (fname, "w");
	if (fp == NULL) {
		log_message(ERROR, "Unable to write configuration", TRUE);
		return EXIT_FAILURE;
	}

	if (gebr.pref.username_value->str != NULL)
		fprintf(fp, "name = \"%s\"\n", gebr.pref.username_value->str);
	if (gebr.pref.email_value->str != NULL)
		fprintf(fp, "email = \"%s\"\n", gebr.pref.email_value->str);
	if (gebr.pref.usermenus_value->str != NULL)
		fprintf(fp, "usermenus = \"%s\"\n", gebr.pref.usermenus_value->str);
	if (gebr.pref.data_value->str != NULL)
		fprintf(fp, "data = \"%s\"\n",  gebr.pref.data_value->str);
	if (gebr.pref.editor_value->str != NULL)
		fprintf(fp, "editor = \"%s\"\n", gebr.pref.editor_value->str);
	if (gebr.pref.browser_value->str != NULL)
		fprintf(fp, "browser = \"%s\"\n", gebr.pref.browser_value->str);

	{
		GtkTreeIter	iter;
		gboolean	valid;

		/* check if it is already open */
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.server_store), &iter);
		while (valid) {
			gchar *	server;

			gtk_tree_model_get (GTK_TREE_MODEL(gebr.server_store), &iter,
					SERVER_ADDRESS, &server,	-1);

			fprintf(fp, "server = \"%s\"\n", server);

			g_free(server);
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.server_store), &iter);
		}
	}

	fclose(fp);
	log_message (ACTION, "Configuration saved", TRUE);

	return EXIT_SUCCESS;
}

/*
 * Function: log_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 *
 */
void
log_message(enum msg_type type, const gchar * message, gboolean in_statusbar)
{
	gchar *		ident_str;
	gchar		time_str[STRMAX];
	time_t		t;
	struct tm *	lt;

	switch (type) {
	case START:
		ident_str = "[STR]";
		break;
	case END:
		ident_str = "[END]";
		break;
	case ACTION:
		ident_str = "[ACT]";
		break;
	case ERROR:
		ident_str = "[ERR]";
		break;
	case WARNING:
		ident_str = "[WARN]";
		break;
	case INTERFACE:
		ident_str = "[IFAC]";
		break;
	case SERVER:
		ident_str = "[SERV]";
		break;
	default:
		ident_str = "[UNK]";
	}

	/* TODO: check verbosity before write */
	time (&t);
	lt = localtime (&t);
	strftime (time_str, STRMAX, "%F %T", lt);
	fprintf(gebr.log_fp, "%s %s %s\n", ident_str, time_str, message);

	if (in_statusbar)
		gtk_statusbar_push (GTK_STATUSBAR (gebr.statusbar), 0, message);
}
