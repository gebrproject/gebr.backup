/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#ifndef __GEBRD_H
#define __GEBRD_H

#include <misc/gtcpserver.h>
#include <misc/gtcpsocket.h>

extern struct gebrd	gebrd;

struct gebrd {
	GTcpServer *	tcp_server;
	GList *		clients;
	GList *		jobs;

	GMainLoop *	main_loop;
};

void
gebrd_init(void);

void
gebrd_quit(void);

#endif //__GEBRD_H
