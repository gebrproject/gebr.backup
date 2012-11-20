/*   GeBR - An environment for seismic processing.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "menu.h"
#include "../defines.h"
#include "gebr.h"
#include "document.h"

/**
 * \internal
 * A NULL terminated vector of all directories to be scanned for menu files.
 * NOTE: GEBR_SYS_MENUS_DIR <em>must</em> be the first element.
 */
static const gchar * directory_list[] = {
	GEBR_SYS_MENUS_DIR,
	"/usr/share/gebr/menus",
	"/usr/local/share/gebr/menus",
	NULL
};

/*
 * Prototypes
 */

static void menu_scan_directory(const gchar * directory, GKeyFile * menu_key_file, GKeyFile * category_key_file);
static gboolean menu_compare_times(const gchar * directory, time_t index_time, gboolean recursive);

/*
 * Public functions
 */

GebrGeoXmlFlow *menu_load_ancient(const gchar * filename)
{
	GebrGeoXmlFlow *menu;
	GString *path;

	path = g_string_new(NULL);

	/* system directory, for newer menus */
	if (strstr(filename, "/") != NULL) {
		g_string_printf(path, "%s/%s", GEBR_SYS_MENUS_DIR, filename);
		if (g_access(path->str, F_OK) == 0)
			goto found;
	}
	/* system directory */
	g_string_printf(path, "%s/%s", GEBR_SYS_MENUS_DIR, filename);
	if (g_access(path->str, F_OK) == 0)
		goto found;
	/* user's menus directory */
	g_string_printf(path, "%s/%s", gebr.config.usermenus->str, filename);
	if (g_access(path->str, F_OK) == 0)
		goto found;

	/* menu not found */
	menu = NULL;
	goto out;

found:	menu = menu_load_path(path->str);

out:	g_string_free(path, TRUE);

	return menu;
}

GebrGeoXmlFlow *menu_load_path(const gchar * path)
{
	GebrGeoXmlFlow *flow;
	document_load_path((GebrGeoXmlDocument**)(&flow), path);
	return flow;
}

gboolean menu_refresh_needed(void)
{
	gboolean needed;
	GString *index_path;

	gchar *home_menu_dir;
	GString *menudir_path;
	struct stat _stat;
	time_t index_time;

	needed = FALSE;
	index_path = g_string_new(NULL);
	menudir_path = g_string_new(NULL);
	home_menu_dir = g_strconcat(g_get_home_dir(), "/.gebr/gebr/menus", NULL);

	/* index path string */
	g_string_printf(index_path, "%s/.gebr/gebr/menus.idx2", g_get_home_dir());
	if (g_access(index_path->str, F_OK | R_OK) && menu_list_create_index() == FALSE)
		goto out;

	/* Time for index */
	stat(index_path->str, &_stat);
	index_time = _stat.st_mtime;

	/* Times for all menus */
	if (menu_compare_times(gebr.config.usermenus->str, index_time, FALSE)
	    || menu_compare_times(home_menu_dir, index_time, FALSE)) {
		needed = TRUE;
		goto out;
	}

	for (gint i = 0; directory_list[i]; i++) {
		if (menu_compare_times(directory_list[i], index_time, FALSE)) {
			needed = TRUE;
			goto out;
		}
	}

out:
	g_string_free(index_path, TRUE);
	g_string_free(menudir_path, TRUE);
	g_free(home_menu_dir);

	return needed;
}

void menu_list_populate(void)
{
	GKeyFile *menu_key_file;
	GKeyFile *category_key_file;
	GString *categories_path;
	GString *menus_path;
	gchar **category_list;
	gsize category_list_length;
	GtkTreeIter iter;
	GtkTreeIter child;

	categories_path = g_string_new(NULL);
	menus_path = g_string_new(NULL);
	menu_key_file = g_key_file_new();
	category_key_file = g_key_file_new();

	gtk_tree_store_clear(gebr.ui_flow_edition->menu_store);

	g_string_printf(categories_path, "%s/.gebr/gebr/categories.idx2", g_get_home_dir());
	g_string_printf(menus_path, "%s/.gebr/gebr/menus.idx2", g_get_home_dir());
	if (!g_file_test(categories_path->str, G_FILE_TEST_EXISTS) ||
	    !g_file_test(menus_path->str, G_FILE_TEST_EXISTS)) {
		menu_list_create_index();
	}

	if (!g_key_file_load_from_file(category_key_file, categories_path->str, G_KEY_FILE_NONE, NULL))
		goto out;

	if (!g_key_file_load_from_file(menu_key_file, menus_path->str, G_KEY_FILE_NONE, NULL))
		goto out;

	GHashTable *categories_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)gtk_tree_iter_free);
	/**
	 * \internal
	 */
	GtkTreeIter find_or_add_category(const gchar *title)
	{
		GtkTreeIter parent;
		GtkTreeIter *pparent = NULL;
		GtkTreeIter iter;

		gchar **category_tree = g_strsplit(title, "|", 0);
		GString *category_name = g_string_new("");
		for (int i = 0; category_tree[i] != NULL; ++i) {
			GString *bold = g_string_new(NULL);
			gchar *escaped_title = g_markup_escape_text(category_tree[i], -1);
			g_string_printf(bold, "<b>%s</b>", escaped_title);
			g_free(escaped_title);

			g_string_append_printf(category_name, "|%s", category_tree[i]);
			GtkTreeIter * category_iter = g_hash_table_lookup(categories_hash, category_name->str);
			if (category_iter == NULL) {
				gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &iter, i > 0 ? &parent : NULL);
				gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &iter,
						   MENU_TITLE_COLUMN, bold->str, -1);
				g_hash_table_insert(categories_hash, g_strdup(category_name->str), gtk_tree_iter_copy(&iter));
			} else
				iter = *category_iter;

			parent = iter;
			pparent = &parent;
			g_string_free(bold, TRUE);
		}
		g_string_free(category_name, TRUE);
		g_strfreev(category_tree);

		return iter;
	}

	category_list = g_key_file_get_groups(category_key_file, &category_list_length);
	for (int i = 0; category_list[i]; i++) {
		gchar ** menus_list;
		gsize menus_list_length;
		menus_list = g_key_file_get_string_list(category_key_file, category_list[i], "menus", &menus_list_length, NULL);
		iter = find_or_add_category(category_list[i]);
		for (int j = 0; menus_list[j]; j++) {
			gchar *title;
			gchar *desc;
			title = g_key_file_get_string(menu_key_file, menus_list[j], "title", NULL);
			gchar *escaped_title = g_markup_escape_text(title, -1);
			desc = g_key_file_get_string(menu_key_file, menus_list[j], "description", NULL);
			gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &child, &iter);
			gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &child,
					   MENU_TITLE_COLUMN, escaped_title,
					   MENU_DESC_COLUMN, desc,
					   MENU_FILEPATH_COLUMN, menus_list[j],
					   -1);
			g_free(escaped_title);
			g_free(title);
			g_free(desc);
		}
		g_strfreev(menus_list);
	}
	g_strfreev(category_list);
	g_hash_table_unref(categories_hash);

out:	g_key_file_free(menu_key_file);
	g_key_file_free(category_key_file);
	g_string_free(categories_path, TRUE);
	g_string_free(menus_path, TRUE);
}

gboolean menu_list_create_index(void)
{
	GString *path;
	GKeyFile *menu_key_file;
	GKeyFile *category_key_file;
	FILE *menu_fp;
	FILE *category_fp;
	gchar *key_file_data;
	gboolean ret;
	gsize key_file_length;

	ret = TRUE;
	path = g_string_new(NULL);
	menu_key_file = g_key_file_new();
	category_key_file = g_key_file_new();

	// Verify duplicates in directory_list
	g_string_printf(path, "%s/.gebr/gebr/menus", g_get_home_dir());
	menu_scan_directory(gebr.config.usermenus->str, menu_key_file, category_key_file);
	menu_scan_directory(path->str, menu_key_file, category_key_file);
	menu_scan_directory(directory_list[0], menu_key_file, category_key_file);
	for (int i = 1; directory_list[i]; i++)
		if (!gebr_paths_equal (directory_list[i], directory_list[0]))
			menu_scan_directory(directory_list[i], menu_key_file, category_key_file);

	g_string_printf(path, "%s/.gebr/gebr/menus.idx2", g_get_home_dir());
	if ((menu_fp = fopen(path->str, "w")) == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Unable to write menus' index."));
		ret = FALSE;
		goto out;
	}

	key_file_data = g_key_file_to_data(menu_key_file, &key_file_length, NULL);
	fwrite(key_file_data, sizeof(gchar), key_file_length, menu_fp);
	g_free(key_file_data);

	fclose(menu_fp);
	
	g_string_printf(path, "%s/.gebr/gebr/categories.idx2", g_get_home_dir());
	if ((category_fp = fopen(path->str, "w")) == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Unable to write menus' index."));
		ret = FALSE;
		goto out;
	}
	
	key_file_data = g_key_file_to_data(category_key_file, &key_file_length, NULL);
	fwrite(key_file_data, sizeof(gchar), key_file_length, category_fp);
	g_free(key_file_data);

	fclose(category_fp);

 out:	g_string_free(path, TRUE);
	g_key_file_free(menu_key_file);
	g_key_file_free(category_key_file);

	return ret;
}

const gchar ** menu_get_global_directories(void)
{
	return directory_list + 1;
}

/*
 * Private functions
 */

/**
 * \internal
 * Compares the modification time of all menu files in \p directory with \p index_time and determines if the directory
 * was changed.
 * \return TRUE if refresh is needed, ie, \p directory time is greater than \p index_time.
 */
static gboolean menu_compare_times(const gchar * directory, time_t index_time, gboolean recursive)
{
	gchar *filename;
	GString *path;
	gboolean refresh_needed;
	struct stat info;

	stat(directory, &info);
	if (index_time < info.st_mtime)
		return TRUE;

	refresh_needed = FALSE;
	path = g_string_new(NULL);
	gebr_directory_foreach_file(filename, directory) {
		g_string_printf(path, "%s/%s", directory, filename);
		if (recursive && g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
			if (menu_compare_times(path->str, index_time, TRUE)) {
				refresh_needed = TRUE;
				goto out;
			}
		}
		if (fnmatch("*.mnu", filename, 1))
			continue;

		stat(path->str, &info);
		if (index_time < info.st_mtime) {
			refresh_needed = TRUE;
			goto out;
		}
	}

 out:	g_string_free(path, TRUE);
	return refresh_needed;
}

/**
 * \internal
 * Scans \p directory for menus files.
 */
static void menu_scan_directory(const gchar * directory, GKeyFile * menu_key_file, GKeyFile * category_key_file)
{
	gchar *filename;
	gchar **category_list;
	glong categories_number;
	GString *path;

	path = g_string_new(NULL);
	gebr_directory_foreach_file(filename, directory) {
		int i;
		GebrGeoXmlDocument *menu;
		GebrGeoXmlSequence *category;

		g_string_printf(path, "%s/%s", directory, filename);
		if (g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
			menu_scan_directory(path->str, menu_key_file, category_key_file);
			continue;
		}
		if (fnmatch("*.mnu", filename, 1))
			continue;

		if (document_load_path((GebrGeoXmlDocument**)(&menu), path->str))
			continue;

		gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(menu), &category, 0);

		categories_number = gebr_geoxml_flow_get_categories_number(GEBR_GEOXML_FLOW(menu));
		category_list = g_new(gchar *, categories_number+1);

		for (i = 0; category != NULL; gebr_geoxml_sequence_next(&category), i++) {
			gchar **menus_list;
			gsize menus_list_length;
			category_list[i] = g_strdup(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)));
			menus_list = g_key_file_get_string_list(category_key_file, category_list[i], "menus", &menus_list_length, NULL);

			if (menus_list) {
				gchar **menus_cpy = g_new(gchar*, menus_list_length+2);
				for (int j = 0; j < menus_list_length; j++)
					menus_cpy[j] = g_strdup(menus_list[j]);
				menus_cpy[menus_list_length] = g_strdup(path->str);
				menus_cpy[menus_list_length+1] = NULL;
				g_strfreev(menus_list);
				menus_list = menus_cpy;
				menus_list_length++;
			} else {
				menus_list = g_new(gchar *, 2);
				menus_list[0] = g_strdup(path->str);
				menus_list[1] = NULL;
				menus_list_length = 1;
			}
			g_key_file_set_string_list(category_key_file, category_list[i], "menus", (const gchar * const *)menus_list, menus_list_length);
			g_strfreev(menus_list);
		}
		category_list[i] = NULL;

		g_key_file_set_string_list(menu_key_file, path->str, "category", (const gchar * const *)category_list, categories_number);
		g_key_file_set_string(menu_key_file, path->str, "title", gebr_geoxml_document_get_title(menu));
		g_key_file_set_string(menu_key_file, path->str, "description", gebr_geoxml_document_get_description(menu));

		g_strfreev(category_list);
		document_free(menu);
	}

	g_string_free(path, TRUE);
}