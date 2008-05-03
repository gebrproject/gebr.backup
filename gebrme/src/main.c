/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <locale.h>
/* TODO: Check for libintl on configure */
#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "gebrme.h"

int
main(int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init(&argc, &argv);

	/* temporary: necessary for representing fractional numbers only with comma */
	setlocale(LC_NUMERIC, "C");

	gebrme_create_window();
	gtk_widget_show(gebrme.window);
	gtk_main();

	return 0;
}
