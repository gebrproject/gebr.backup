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

#ifndef __GEBR_UTILS_H
#define __GEBR_UTILS_H

#include <dirent.h>
#include <fnmatch.h>

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	TIME_NONE,
	TIME_MOMENTS_AGO,
	TIME_HOURS_AGO,
	TIME_DAYS_AGO,
	TIME_WEEKS_AGO,
	TIME_MONTHS_AGO,
	TIME_YEARS_AGO,
	TIME_N_TYPES,
} TimesType;

/**
 * gebr_g_string_remove_accents:
 * @title:the text with accents to be removed
 *
 * Replace accented characters on a string to non accented characters.
 * If character is non ASCII, it is replaced to ""
 */
gchar *
gebr_g_string_remove_accents(gchar *title);

void
/**
 * gebr_g_string_replace:
 * @string: The string to with the text.
 * @oldtext: The text to be replaced.
 * @newtext: The text to replace oldtext.
 *
 * Replace each reference of oldtext in string
 * with newtext. If newtext if NULL, then each reference of oldtext found is removed.
 */
gebr_g_string_replace(GString * string,
		      const gchar * oldtext,
		      const gchar * newtext);

void gebr_g_string_replace_first_ref(GString * string, const gchar * oldtext, const gchar * newtext);
gboolean gebr_g_string_starts_with(GString * string, const gchar * val);
gboolean gebr_g_string_ends_with(GString * string, const gchar * val);

/**
 * Appends \p extension in \p filename if it is not present.
 * \return TRUE if append was made, FALSE otherwise.
 */
gboolean gebr_append_filename_extension(GString * filename, const gchar * extension);

gboolean gebr_path_is_at_home(const gchar * path);
gboolean gebr_path_use_home_variable(GString * path);
gboolean gebr_path_resolve_home_variable(GString * path);
void gebr_path_set_to(GString * path, gboolean relative);

GString *gebr_temp_directory_create(void);
void gebr_temp_directory_destroy(GString * path);

GString *gebr_make_unique_filename(const gchar * template);
GString *gebr_make_temp_filename(const gchar * template);

gint gebr_system(const gchar *cmd, ...);
gboolean gebr_dir_has_files(const gchar *dir_path);

int gebr_home_mode(void);

gboolean gebr_create_config_dirs(void);

gchar *gebr_locale_to_utf8(const gchar * string);

gboolean g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key);
GString *gebr_g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key,
					 const gchar * default_value);
gboolean gebr_g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key,
					  gboolean default_value);
int gebr_g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value);
gboolean gebr_g_key_file_has_key(GKeyFile * key_file, const gchar * group, const gchar * key);
gboolean gebr_g_key_file_remove_key(GKeyFile * key_file, const gchar * group, const gchar * key);

#define gebr_foreach_gslist_hyg(element, list, hygid) \
	GSList * __list##hygid = list, * __i##hygid = list; \
	if (__i##hygid != NULL || (g_slist_free(__list##hygid), 0)) \
		for (element = (typeof(element))__i##hygid->data; \
		(__i##hygid != NULL && (element = (typeof(element))__i##hygid->data, 1)) || (g_slist_free(__list##hygid), 0); \
		__i##hygid = g_slist_next(__i##hygid))
#define gebr_foreach_gslist(element, list) \
	gebr_foreach_gslist_hyg(element, list, nohyg)

#define gebr_glist_foreach_hyg(element, list, hygid) \
	GList * __list##hygid = list, * __i##hygid = list; \
	if (g_list_next(__i##hygid) != NULL || (g_list_free(__list##hygid), 0)) \
		for (element = (typeof(element))g_list_first(__i##hygid)->data; \
		(__i##hygid != NULL && (element = (typeof(element))__i##hygid->data, 1)) || (g_list_free(__list##hygid), 0); \
		__i##hygid = g_list_next(__i##hygid))
#define gebr_glist_foreach(element, list) \
	libgebr_gslist_foreach_hyg(element, list, nohyg)

#define gebr_directory_foreach_file_hyg(filename, directory, hygid) \
	DIR *		dir##hygid; \
	struct dirent *	file##hygid; \
	if ((dir##hygid = opendir(directory)) != NULL) \
		while (((file##hygid = readdir(dir##hygid)) != NULL && \
		(filename = file##hygid->d_name, 1)) || \
		(closedir(dir##hygid), 0)) \
			if (strcmp(filename, ".") && strcmp(filename, ".."))
#define gebr_directory_foreach_file(filename, directory) \
	gebr_directory_foreach_file_hyg(filename, directory, nohyg)

const gchar *gebr_validate_int(const gchar * text_value, const gchar * min, const gchar * max);
const gchar *gebr_validate_float(const gchar * text_value, const gchar * min, const gchar * max);

#if !GLIB_CHECK_VERSION(2,16,0)
int g_strcmp0(const char * str1, const char * str2);
#endif

/**
 * gebr_realpath_equal:
 * @path1: the first path
 * @path2: the second path
 *
 * Compares if path1 and path2 resolves to the same file.
 * The paths compared for equality by calling g_stat() on both of them
 * and comparing their inode parameters are the same.
 *
 * Returns: %TRUE if @path1 points to the same file/directory as @path2.
 * %FALSE otherwise, including if one or both of then does not exists in the file system.
 */
gboolean gebr_realpath_equal (const gchar *path1, const gchar *path2);

/**
 * gebr_str_escape:
 * @str: the string to be escaped
 *
 * Escapes all backslashes and quotes in a string. It is based on glib's g_strescape.
 *
 * Returns: a newly allocated string.
 */
gchar *gebr_str_escape (const gchar *str);

/**
 * gebr_get_english_date:
 * @format: the date format, as seen in g_date_strftime()
 * @locale: a string representing the locale, such as "C"
 *
 * Returns: a newly allocated string containing the date in english.
 */
gchar *gebr_date_get_localized (const gchar *format, const gchar *locale);

/*
 * Create a random id compose of printable characters
 * Returns: a newly allocated string containing the id
 */
gchar *gebr_id_random_create(gssize bytes);

/*
 * Open a file for writing. If the file already exists, its contents is returned.
 * If not \p new_lock_content is written to the file. 
 * A newly allocated string with the contents of \p pathname is returned.
 */
gchar * gebr_lock_file(const gchar *pathname, const gchar *new_lock_content, gboolean symlink);

/**
 * gebr_str_ascii_word_at:
 * @str:
 * @pos: (in-out): Position to search the word
 */
gchar *gebr_str_word_before_pos(const gchar *str, gint *pos);

/**
 * gebr_str_remove_trailing_zeros:
 *
 * Modifies @str inplace by removing trailing zeros.
 *
 * Returns: The same @str, but modified.
 */
gchar *gebr_str_remove_trailing_zeros(gchar *str);

/**
 * gebr_str_canonical_var_name:
 * @keyword: The keyword to be canonized. (not modified)
 * @new_value: Returns a new alocated string with the canonized value of
 * keyword.
 * @error: A GError (not used until now)
 *
 * Returns: TRUE if everthing is ok. False otherwise.
 */
gboolean gebr_str_canonical_var_name(const gchar * keyword,
				     gchar ** new_value,
				     GError ** error);

/**
 * gebr_calculate_relative_time:
 * @time1: The first date to compare 
 * @time2: The second date to compare 
 * @type:
 * @delta:
 *
 * Returns: A message (string) of the relative time and NULL if time2 is older
 * than time1.
 */
gchar *gebr_calculate_relative_time (GTimeVal *time1,
                                     GTimeVal *time2,
                                     TimesType *type,
                                     gdouble *delta);

/**
 * gebr_get_lower_bound_for_type:
 * @type:
 *
 * Returns:
 */
gdouble gebr_get_lower_bound_for_type(TimesType type);


gint gebr_get_number_of_time_controls();

/**
 * gebr_get_control_text_for_type:
 * @type:
 *
 * Return:
 */
gchar *gebr_get_control_text_for_type(TimesType type);

/**
 * gebr_calculate_relative_time:
 * @time1: The first date to compare 
 * @time2: The second date to compare 
 *
 * Returns: A message (string) of the relative time and NULL if time2 is older
 * than time1.
 */
gchar *gebr_calculate_detailed_relative_time(GTimeVal *time1, GTimeVal *time2);

gchar *gebr_compute_diff_iso_times(const gchar *iso_time1, const gchar *iso_time2);

gint gebr_compute_diff_clock_to_me(const gchar *iso_time1);

/**
 * gebr_utf8_is_asc_alnum:
 * @str: The string to check
 *
 * Returns: %TRUE if @str is ASC and alpha-numerical.
 */
gboolean gebr_utf8_is_asc_alnum(const gchar *str);

/**
 * gebr_utf8_strstr:
 *
 * Searches for @needle in @haystack and returns a pointer to the begging of
 * the substring or %NULL if it was not found.
 */
gchar *gebr_utf8_strstr(const gchar *haystack,
			const gchar *needle);

/**
 * gebr_calculate_number_of_processors:
 * @total_nprocs: Number of processors of a server
 * @aggressive: Aggressive percentage (slider)
 *
 * Calculate real number of processors will be used
 */
gint gebr_calculate_number_of_processors(gint total_nprocs,
                                         gdouble aggressive);

/**
 * gebr_pairstrfreev:
 *
 * Frees a vector of pairs of strings.
 */
void gebr_pairstrfreev(gchar ***strv);

/**
 * gebr_remove_path_prefix:
 *
 * Remove @prefix from @path
 */
gchar *gebr_remove_path_prefix(const gchar *prefix, const gchar *path);

/**
 * gebr_relativise_old_home_path:
 *
 * Change $HOME of @path_string to &lt;HOME&gt;
 */
gchar *gebr_relativise_old_home_path(const gchar *path_string);

/**
 * gebr_relativise_home_path:
 *
 * Substitute @home with the prefix HOME
 * @pvector is a vector of pair of strings, the first entry is an
 * alias and the second is an absolute path.
 */
gchar *gebr_relativise_home_path(const gchar *path_string,
                                 const gchar *mount_point,
                                 const gchar *home);

/**
 * gebr_relativise_path:
 *
 * Substitute @path with the longest prefix in @pvector.
 * @pvector is a vector of pair of strings, the first entry is an
 * alias and the second is an absolute path.
 */
gchar *gebr_relativise_path(const gchar *path_string,
                            const gchar *mount_point,
                            gchar ***pvector);
/**
 * gebr_resolve_relative_path:
 *
 * Transfors a relative path in absolute path
 */

gchar *gebr_resolve_relative_path(const char *path,
				gchar ***pvector);

gchar ***gebr_generate_paths_with_home(const gchar *home);

/**
 * gebr_gtk_bookmarks_add_paths:
 *
 * Add bookmarks on @filename file with @uri_prefix and base paths set on vector @paths,
 * and include suffix (GeBR) on name to identify
 */
void gebr_gtk_bookmarks_add_paths(const gchar *filename,
                                  const gchar *uri_prefix,
                                  gchar ***paths);

/**
 * gebr_gtk_bookmarks_remove_paths:
 *
 * Remove bookmarks according @paths for @filename
 */
void gebr_gtk_bookmarks_remove_paths(const gchar *filename,
                                     gchar ***paths);

gboolean gebr_verify_starting_slash (const gchar *string);

gboolean gebr_validate_path(const gchar *path,
			    gchar ***paths,
			    gchar **err_msg);

gint gebr_strv_indexof(const gchar **strv, const gchar *value);

/**
 * gebr_comm_server_generate_key:
 */
gboolean gebr_generate_key();

/**
 * gebr_add_ssh_key:
 */
gboolean gebr_add_remove_ssh_key(gboolean remove);

/**
 * gebr_get_address_without_user:
 *
 * Get substring after the first @ and before the first space
 * In case there's no @, get the substring before the first space
 *
 */
gchar *gebr_get_address_without_user(const gchar *address);

/*
 *verify_address_without_username
 *
 *Verify if the given address has a @ or a space
 */
gboolean gebr_verify_address_without_username(const gchar *address);

gboolean gebr_kill_by_port(gint port);

gchar * gebr_key_filename(gboolean public);

gchar *gebr_create_id_with_current_time();

/*
 * Transform a list of lists in a single list
 */
GList *gebr_double_list_to_list(GList *double_list);

/**
 * Get contents of compressed gz file
 *
 * gebr_gzfile_get_contents:
 * @filename: The name of gzipped file
 * @contents: a GString to fill the contents of file
 * @error: an error message or NULL
 *
 * Returns: %FALSE if have some error, %TRUE otherwise
 */
gboolean gebr_gzfile_get_contents(const gchar *filename,
                                  GString *contents,
                                  gchar **error);

/**
 * gebr_callback_true:
 *
 * Returns %TRUE.
 */
gboolean gebr_callback_true(void);

/**
 * gebr_string_freeall:
 *
 * Equivalent to calling g_string_free(string, TRUE).
 */
void gebr_string_freeall(GString *string);

/*
 * gebr_add_pattern_on_address:
 *
 * Apply pattern on @addr, converting localhost or
 * IP 127.0.0.1 to hostname
 */
const gchar *gebr_apply_pattern_on_address(const gchar *addr);

/*
 * gebr_gqueue_push_tail_avoiding_duplicates:
 *
 * Push @data on tail of the @queue the in case @data is not in the @queue.
 */
GQueue *gebr_gqueue_push_tail_avoiding_duplicates(GQueue *queue, const gchar *data);

G_END_DECLS

#endif				//__GEBR_UTILS_H
