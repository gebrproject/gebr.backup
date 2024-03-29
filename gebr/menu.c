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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
#include "gebr.h"
#include "document.h"
#include "callbacks.h"
#include "gebr-menu-view.h"

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

static const gchar * directory_list_demos[] = {
	GEBR_SYS_DEMOS_DIR,
	"/usr/share/gebr/demos",
	"/usr/local/share/gebr/demos",
	NULL
};

/*
 * Prototypes
 */

static void menu_scan_directory(const gchar * directory, GKeyFile * menu_key_file, GKeyFile * category_key_file);

static gboolean menu_compare_times(const gchar * directory, time_t index_time, gboolean recursive);

void __menu_list_populate(const gchar *path, const gchar *index_menu, const gchar *index_category, GHashTable *categories_hash);
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

static gboolean
menu_path_has_index(const gchar *path,
		    gchar **index_menu,
		    gchar **index_category)
{
	gchar *filename_menu = g_build_filename(path, "menus.idx2", NULL);
	gchar *filename_cat = g_build_filename(path, "categories.idx2", NULL);

	if (g_file_test(filename_menu, G_FILE_TEST_EXISTS) &&
	    g_file_test(filename_cat, G_FILE_TEST_EXISTS)) {
		if (index_menu)
			*index_menu = g_strdup(filename_menu);
		if (index_category)
			*index_category = g_strdup(filename_cat);
		return TRUE;
	}

	g_free(filename_menu);
	g_free(filename_cat);
	return FALSE;
}

static gboolean
menu_path_internal_index_is_valid(const gchar *path,
				  const gchar *local_folder,
				  gchar **index_menu,
				  gchar **index_category)
{
	gboolean need = FALSE;
	GString *filename_menu = g_string_new(path);
	gchar *local_filename_menu;
	gchar *local_filename_cat;

	gebr_g_string_replace(filename_menu, "/", "_");
	if (filename_menu->str[filename_menu->len] != '_')
		g_string_append_c(filename_menu, '_');

	GString *filename_cat = g_string_new(filename_menu->str);

	filename_menu = g_string_append(filename_menu, "menus.idx2");
	filename_cat = g_string_append(filename_cat, "categories.idx2");

	local_filename_menu = g_build_filename(local_folder, filename_menu->str, NULL);
	local_filename_cat = g_build_filename(local_folder, filename_cat->str, NULL);

	struct stat menu_info, categ_info;

	g_stat(local_filename_menu, &menu_info);
	g_stat(local_filename_cat, &categ_info);

	gboolean file_exists = g_file_test(local_filename_menu, G_FILE_TEST_EXISTS) &&
		g_file_test(local_filename_cat, G_FILE_TEST_EXISTS);
	gboolean need_update = menu_compare_times(path, menu_info.st_mtime, TRUE) || menu_compare_times(path, categ_info.st_mtime, TRUE);

	if (!file_exists || (file_exists && need_update))
			need = TRUE;

	g_string_free(filename_menu, TRUE);
	g_string_free(filename_cat, TRUE);

	if (!need) {
		if (index_menu)
			*index_menu = local_filename_menu;
		if (index_category)
			*index_category = local_filename_cat;
	}

	return need;
}

typedef struct {
	gchar *path;
	gchar *index_menu;
	gchar *index_category;
} MenuInfo;

static void
free_menu_info(MenuInfo *info)
{
	g_free(info->path);
	g_free(info->index_category);
	g_free(info->index_menu);
	g_free(info);
}

void
menu_list_populate(void)
{
	gchar *index_menu = NULL;
	gchar *index_category = NULL;
	gchar *filename;
	gchar *gebr_home = g_build_filename(g_get_home_dir(), ".gebr", "gebr", NULL);

	GList *insert_menus = NULL;

	static gboolean first_time = TRUE;
	gboolean need_update = FALSE;
	gboolean clear_model = FALSE;
	GHashTable *categories_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)gtk_tree_iter_free);

	for (gint i = 0; directory_list[i]; i++) {
		if (i > 0 && gebr_realpath_equal (directory_list[i], directory_list[0]))
			continue;

		const gchar *directory = directory_list[i];
		gebr_directory_foreach_file(filename, directory) {
			gchar *path = g_build_filename(directory, filename, NULL);
			if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
				if (!menu_path_has_index(path, &index_menu, &index_category)) {
					if (menu_path_internal_index_is_valid(path, gebr_home, &index_menu, &index_category)) {
						need_update = TRUE;
						if (!menu_list_create_index(path, &index_menu, &index_category, FALSE)) {
							g_warning("Could not create index for %s", path);
							continue;
						}
					}
				}
				if (need_update || first_time) {
					if (!clear_model) {
						gebr_menu_view_clear_model(gebr.menu_view);
						clear_model = TRUE;
					}
				}

				MenuInfo *info = g_new(MenuInfo, 1);
				info->path = g_strdup(path);
				info->index_menu = g_strdup(index_menu);
				info->index_category = g_strdup(index_category);
				insert_menus = g_list_prepend(insert_menus, info);

				g_free(index_menu);
				g_free(index_category);
			}
			g_free(path);
		}
	}


	/* Scan debr installed menus and create index */
	need_update = FALSE;
	GString *path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/gebr/menus", g_get_home_dir());

	if (menu_path_internal_index_is_valid(path->str, gebr_home, &index_menu, &index_category)) {
		need_update = TRUE;
		menu_list_create_index(path->str, &index_menu, &index_category, FALSE);
	}
	if (need_update || first_time) {
		if (!clear_model) {
			gebr_menu_view_clear_model(gebr.menu_view);
			clear_model = TRUE;
		}
	}
	MenuInfo *info = g_new(MenuInfo, 1);
	info->path = NULL;
	info->index_menu = g_strdup(index_menu);
	info->index_category = g_strdup(index_category);
	insert_menus = g_list_prepend(insert_menus, info);

	g_free(index_menu);
	g_free(index_category);
	g_string_free(path, TRUE);

	/* Scan user's folder and create index */

	need_update = FALSE;
	if (menu_path_internal_index_is_valid(gebr.config.usermenus->str, gebr_home, &index_menu, &index_category)) {
		need_update = TRUE;
		menu_list_create_index(gebr.config.usermenus->str, &index_menu, &index_category, FALSE);
	}
	if (need_update || first_time || gebr.update_usermenus) {
		if (!clear_model) {
			gebr_menu_view_clear_model(gebr.menu_view);
			clear_model = TRUE;
		}
		gebr.update_usermenus = FALSE;
	}
	info = g_new(MenuInfo, 1);
	info->path = NULL;
	info->index_menu = g_strdup(index_menu);
	info->index_category = g_strdup(index_category);
	insert_menus = g_list_prepend(insert_menus, info);

	for (GList *i = insert_menus; i && clear_model; i = i->next) {
		MenuInfo *info = i->data;
		__menu_list_populate(info->path, info->index_menu, info->index_category, categories_hash);
	}

	first_time = FALSE;

	g_list_foreach(insert_menus, (GFunc)free_menu_info, NULL);
	g_list_free(insert_menus);

	g_free(index_menu);
	g_free(index_category);

	g_free(gebr_home);
	g_hash_table_unref(categories_hash);
}

void __menu_list_populate(const gchar *path,
                          const gchar *index_menu,
                          const gchar *index_category,
                          GHashTable *categories_hash)
{
	GKeyFile *menu_key_file;
	GKeyFile *category_key_file;
	gchar **category_list;
	gsize category_list_length;
	GtkTreeIter iter;

	menu_key_file = g_key_file_new();
	category_key_file = g_key_file_new();

	if (index_category && !g_key_file_load_from_file(category_key_file, index_category , G_KEY_FILE_NONE, NULL))
		goto out;

	if (index_menu && !g_key_file_load_from_file(menu_key_file, index_menu, G_KEY_FILE_NONE, NULL))
		goto out;

	category_list = g_key_file_get_groups(category_key_file, &category_list_length);
	for (int i = 0; category_list[i]; i++) {
		gchar ** menus_list;
		gsize menus_list_length;
		menus_list = g_key_file_get_string_list(category_key_file, category_list[i], "menus", &menus_list_length, NULL);
		iter = gebr_menu_view_find_or_add_category(category_list[i], categories_hash, gebr.menu_view);
		for (int j = 0; menus_list[j]; j++) {
			gchar *title;
			gchar *desc;
			gchar *file;
			if (*menus_list[j] == '.') {
				file = g_strconcat(path, (menus_list[j] + 1), NULL);
			} else {
				file = g_strdup(menus_list[j]);
			}
			title = g_key_file_get_string(menu_key_file, menus_list[j], "title", NULL);
			desc = g_key_file_get_string(menu_key_file, menus_list[j], "description", NULL);

			gebr_menu_view_add_menu(&iter, title, desc, file, gebr.menu_view);

			g_free(title);
			g_free(desc);
			g_free(file);
		}
		g_strfreev(menus_list);
	}
	g_strfreev(category_list);

out:	g_key_file_free(menu_key_file);
	g_key_file_free(category_key_file);
}

gboolean menu_list_create_index(const gchar *path,
                                gchar **index_menu,
                                gchar **index_category,
                                gboolean use_default)
{
	GKeyFile *menu_key_file;
	GKeyFile *category_key_file;
	FILE *menu_fp;
	FILE *category_fp;
	gchar *key_file_data;
	gboolean ret;
	gsize key_file_length;

	ret = TRUE;

	gchar *imenu, *icat;

	if (use_default) {
		imenu = g_build_filename(path, "menus.idx2", NULL);
		if (index_menu)
			*index_menu = g_strdup(imenu);

		icat = g_build_filename(path, "categories.idx2", NULL);
		if (index_category)
			*index_category = g_strdup(icat);
	} else {
		GString *menu_name = g_string_new(path);
		gebr_g_string_replace(menu_name, "/", "_");
		gchar *mname = g_strdup_printf("%s_menus.idx2", menu_name->str);
		imenu = g_build_filename(g_get_home_dir(), ".gebr", "gebr", mname, NULL);
		if (index_menu)
			*index_menu = g_strdup(imenu);
		g_free(mname);
		g_string_free(menu_name, FALSE);

		GString *cat_name = g_string_new(path);
		gebr_g_string_replace(cat_name, "/", "_");
		gchar *cname = g_strdup_printf("%s_categories.idx2", cat_name->str);
		icat = g_build_filename(g_get_home_dir(), ".gebr", "gebr", cname, NULL);
		if (index_category)
			*index_category = g_strdup(icat);
		g_free(cname);
		g_string_free(cat_name, FALSE);
	}

	menu_key_file = g_key_file_new();
	category_key_file = g_key_file_new();

	menu_scan_directory(path, menu_key_file, category_key_file);

	/* Create menu index for @path */
	if ((menu_fp = fopen(imenu, "w")) == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Unable to write Menus' index."));
		ret = FALSE;
		goto out;
	}
	g_free(imenu);

	key_file_data = g_key_file_to_data(menu_key_file, &key_file_length, NULL);
	fwrite(key_file_data, sizeof(gchar), key_file_length, menu_fp);
	g_free(key_file_data);

	fclose(menu_fp);
	
	/* Create category index for @path */
	if ((category_fp = fopen(icat, "w")) == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Unable to write Menus' index."));
		ret = FALSE;
		goto out;
	}
	g_free(icat);
	
	key_file_data = g_key_file_to_data(category_key_file, &key_file_length, NULL);
	fwrite(key_file_data, sizeof(gchar), key_file_length, category_fp);
	g_free(key_file_data);

	fclose(category_fp);

 out:
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

static gchar *canonize_name(const gchar *path)
{
	GFile *file = g_file_new_for_path(path);
	GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, 0, NULL, NULL);

	gchar *label = g_strdup(g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME));
	g_strdelimit(label, "_", ' ');

	g_object_unref(file);
	g_object_unref(info);

	return label;
}

static gboolean populate_menu_demo(const gchar *dir, GtkMenu *menu)
{
	gchar *filename;
	gchar *dirname;
	GString *path;
	gboolean has_demo = FALSE;
	path = g_string_new(NULL);

	gebr_directory_foreach_file(dirname, dir) {
		gboolean has_dir = FALSE;
		has_demo = TRUE;
		g_string_printf(path, "%s/%s", dir, dirname);
		if (!g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
			g_string_free(path, TRUE);
			continue;
		}
		gchar *label = canonize_name(path->str);
		GtkWidget *menu_item = gtk_menu_item_new_with_label(label);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		GtkWidget *submenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
		g_free(label);

		GList *list = NULL;
		gebr_directory_foreach_file_hyg(filename, path->str, dir) {
			if (!fnmatch("*.prj?", filename, 1)) {
				has_dir = TRUE;
				list = g_list_prepend(list, g_strdup(filename));
			}
		}
		list = g_list_sort(list, (GCompareFunc) g_strcmp0);
		for (GList *i = list; i; i = i->next) {
			gchar *demo_path = g_strconcat(path->str , "/", i->data, NULL);
			gchar *label = canonize_name(demo_path);
			label[strlen(label) - 5] = '\0';
			menu_item = gtk_menu_item_new_with_label(label);
			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
			g_signal_connect(menu_item, "activate", G_CALLBACK(import_demo), demo_path);
			g_object_weak_ref(G_OBJECT(menu_item), (GWeakNotify)g_free, demo_path);
			g_free(label);
		}

		g_list_foreach(list, (GFunc) g_free, NULL);
		g_list_free(list);
		if (!has_dir) {
			menu_item = gtk_menu_item_new_with_label(_("Empty"));
			gtk_widget_set_sensitive(menu_item, FALSE);
			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
		}
	}
	g_string_free (path, TRUE);

	if (!has_demo)
		return FALSE;
	return TRUE;
}

void demos_list_create(GtkMenu *menu)
{
	gboolean has_demo;
	has_demo = populate_menu_demo(directory_list_demos[0], menu);
	for (int i = 1; directory_list_demos[i]; i++)
		if (!gebr_realpath_equal (directory_list_demos[i], directory_list_demos[0])) {
			if (has_demo)
				populate_menu_demo(directory_list_demos[i], menu);
			else
				has_demo = populate_menu_demo(directory_list_demos[i], menu);
		}
	if (!has_demo) {
		GtkWidget *menu_item;
		menu_item = gtk_menu_item_new_with_label(_("Install Seismic Unix package.\n"
							 "Click here to get the script of Seismic Unix on GêBR Project website."));
		g_signal_connect(menu_item, "activate", G_CALLBACK(open_url_on_press_event), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	}
}
