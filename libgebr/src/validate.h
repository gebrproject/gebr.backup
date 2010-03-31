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

#ifndef __GEBR_VALIDATE_H
#define __GEBR_VALIDATE_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GEBR_VALIDATE_CASE_AUTHOR,
	GEBR_VALIDATE_CASE_CATEGORY,
	GEBR_VALIDATE_CASE_DATE,
	GEBR_VALIDATE_CASE_DESCRIPTION,
	GEBR_VALIDATE_CASE_FILENAME,
	GEBR_VALIDATE_CASE_HELP,
	GEBR_VALIDATE_CASE_TITLE,
	GEBR_VALIDATE_CASE_EMAIL,

	GEBR_VALIDATE_CASE_PROGRAM_BINARY,
	GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION,
	GEBR_VALIDATE_CASE_PROGRAM_TITLE,
	GEBR_VALIDATE_CASE_PROGRAM_URL,
	GEBR_VALIDATE_CASE_PROGRAM_VERSION,

	GEBR_VALIDATE_CASE_PARAMETER_KEYWORD,
	GEBR_VALIDATE_CASE_PARAMETER_LABEL,
} GebrValidateCaseName;

typedef struct _GebrValidateCase GebrValidateCase;

struct _GebrValidateCase {
	GebrValidateCaseName name;
	gint flags;
	const gchar * errmsg;
};

GebrValidateCase * gebr_validate_get_validate_case(GebrValidateCaseName name);

/**
 * TRUE if str is not empty.
 */
gboolean gebr_validate_check_is_not_empty(const gchar * str);

/**
 * TRUE if str does not start with lower case letter.
 */
gboolean gebr_validate_check_no_lower_case(const gchar * sentence);

/**
 * TRUE if str has not consecutive blanks.
 */
gboolean gebr_validate_check_no_multiple_blanks(const gchar * str);

/**
 * TRUE if str does not start or end with blanks/tabs.
 */
gboolean gebr_validate_check_no_blanks_at_boundaries(const gchar * str);

/**
 * TRUE if str does not end if a punctuation mark.
 */
gboolean gebr_validate_check_no_punctuation_at_end(const gchar * str);

/**
 * TRUE if str has not path and ends with .mnu
 */
gboolean gebr_validate_check_menu_filename(const gchar * str);
 
/**
 * TRUE if \p str is of kind <code>xxx@yyy</code>, with xxx composed by letter, digits, underscores, dots and dashes,
 * and yyy composed by at least one dot, letter digits and dashes.
 *
 * \see http://en.wikipedia.org/wiki/E-mail_address
 */
gboolean gebr_validate_check_is_email(const gchar * str);

G_END_DECLS

#endif				//__GEBR_VALIDATE_H
