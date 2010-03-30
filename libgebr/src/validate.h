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

/**
 * TRUE if str is not empty.
 */
gboolean gebr_validate_check_is_not_empty(const gchar * str);

/**
 * TRUE if str does not start with lower case letter.
 */
gboolean gebr_validate_check_no_lower_case(const gchar * sentence);

/**
 *  CHANGE \p sentence first letter to upper case.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_lower_case check
 */
gchar * gebr_validate_change_first_to_upper(const gchar * sentence);

/**
 * TRUE if str has not consecutive blanks.
 */
gboolean gebr_validate_check_no_multiple_blanks(const gchar * str);

/**
 *  CHANGE \p sentence to remove mutiple blanks.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_multiple_blanks check
 */
gchar * gebr_validate_change_multiple_blanks(const gchar * sentence);

/**
 * TRUE if str does not start or end with blanks/tabs.
 */
gboolean gebr_validate_check_no_blanks_at_boundaries(const gchar * str);

/**
 *  CHANGE \p sentence to remove blanks at boundaries.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_multiple_blanks check
 */
gchar * gebr_validate_change_no_blanks_at_boundaries(const gchar * sentence);

/**
 * TRUE if str does not end if a punctuation mark.
 */
gboolean gebr_validate_check_no_punctuation_at_end(const gchar * str);

/**
 *  CHANGE \p sentence to remove punctuation at string's end.
 *  \return  string with last punctuation changed by a NULL character 
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_punctuation_at_end check
 */
gchar * gebr_validate_change_no_punctuation_at_end(const gchar * sentence);

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

#endif				//__GEBR_VALIDATE_H
