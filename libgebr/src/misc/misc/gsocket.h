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
 *
 *   Inspired on Qt 4.3 version of QAbstractSocket, by Trolltech
 */

#ifndef __GSOCKET_H
#define __GSOCKET_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

G_BEGIN_DECLS

GType
g_socket_get_type(void);

#define G_SOCKET_TYPE			(g_socket_get_type())
#define G_SOCKET(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_SOCKET_TYPE, GSocket))
#define G_SOCKET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), G_SOCKET_TYPE, GSocketClass))
#define G_IS_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_SOCKET_TYPE))
#define G_IS_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_SOCKET_TYPE))
#define G_SOCKET_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), G_SOCKET_TYPE, GSocketClass))

typedef struct _GSocket	GSocket;
typedef struct _GSocketClass	GSocketClass;

enum GSocketError {
	G_SOCKET_ERROR_CONNECTION_REFUSED,
	G_SOCKET_ERROR_LOOKUP,
};

enum GSocketState {
	G_SOCKET_STATE_NONE,
	/* tcp socket */
	G_SOCKET_STATE_UNCONNECTED,
	G_SOCKET_STATE_LOOKINGUP,
	G_SOCKET_STATE_CONNECTING,
	G_SOCKET_STATE_CONNECTED,
	/* server */
	G_SOCKET_STATE_NOTLISTENING,
	G_SOCKET_STATE_LISTENING,
};

struct _GSocket {
	GObject			parent;

	GIOChannel *		io_channel;
	struct sockaddr_in	sockaddr_in;
	enum GSocketState	state;
	GByteArray *		queue_write_bytes;
};
struct _GSocketClass {
	GObjectClass		parent;

	/* virtual */
	void			(*connected)(GSocket * self);
	void			(*disconnected)(GSocket * self);
	void			(*new_connection)(GSocket * self);
	/* signals */
	void			(*ready_read)(GSocket * self);
	void			(*ready_write)(GSocket * self);
	void			(*error)(GSocket * self, enum GSocketError error);
};

/*
 * user functions
 */

void
g_socket_close(GSocket *);

enum GSocketState
g_socket_get_state(GSocket *);

gulong
g_socket_bytes_available(GSocket *);

GByteArray *
g_socket_read(GSocket *, gsize);

GString *
g_socket_read_string(GSocket *, gsize);

GByteArray *
g_socket_read_all(GSocket *);

GString *
g_socket_read_string_all(GSocket *);

gsize
g_socket_write(GSocket *, GByteArray *);

gsize
g_socket_write_string(GSocket *, GString *);

G_END_DECLS

#endif //__GSOCKET_H