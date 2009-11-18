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
 *
 *   Inspired on Qt 4.3 version of QSocketAddress, by Trolltech
 */

#ifndef __GEBR_COMM_SOCKET_ADDRESSPRIVATE_H
#define __GEBR_COMM_SOCKET_ADDRESSPRIVATE_H

#include <glib.h>

#include "gsocketaddress.h"

G_BEGIN_DECLS

GebrCommSocketAddress
_gebr_comm_socket_address_unknown(void);

gboolean
_gebr_comm_socket_address_get_sockaddr(GebrCommSocketAddress * socket_address, struct sockaddr ** sockaddr, gsize * size);

int
_gebr_comm_socket_address_get_family(GebrCommSocketAddress * socket_address);

int
_gebr_comm_socket_address_getsockname(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type, int sockfd);

int
_gebr_comm_socket_address_getpeername(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type, int sockfd);

int
_gebr_comm_socket_address_accept(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type, int sockfd);

G_END_DECLS

#endif // 
