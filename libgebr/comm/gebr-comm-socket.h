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
 *   Inspired on Qt 4.3 version of QAbstractSocket, by Trolltech
 */

#ifndef __GEBR_COMM_SOCKET_H
#define __GEBR_COMM_SOCKET_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

#include "gebr-comm-socketaddress.h"

G_BEGIN_DECLS

GType gebr_comm_socket_get_type(void);

#define GEBR_COMM_SOCKET_TYPE			(gebr_comm_socket_get_type())
#define GEBR_COMM_SOCKET(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_SOCKET_TYPE, GebrCommSocket))
#define GEBR_COMM_SOCKET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_SOCKET_TYPE, GebrCommSocketClass))
#define GEBR_COMM_IS_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_SOCKET_TYPE))
#define GEBR_COMM_IS_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_SOCKET_TYPE))
#define GEBR_COMM_SOCKET_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_SOCKET_TYPE, GebrCommSocketClass))

typedef struct _GebrCommSocket GebrCommSocket;
typedef struct _GebrCommSocketClass GebrCommSocketClass;

enum GebrCommSocketError {
	GEBR_COMM_SOCKET_ERROR_NONE,
	GEBR_COMM_SOCKET_ERROR_CONNECTION_REFUSED,
	GEBR_COMM_SOCKET_ERROR_SERVER_TIMED_OUT,
	GEBR_COMM_SOCKET_ERROR_LOOKUP,
	GEBR_COMM_SOCKET_ERROR_UNKNOWN,
};

enum GebrCommSocketState {
	GEBR_COMM_SOCKET_STATE_NONE,
	GEBR_COMM_SOCKET_STATE_UNCONNECTED,
	GEBR_COMM_SOCKET_STATE_NOTLISTENING,
	GEBR_COMM_SOCKET_STATE_LOOKINGUP,
	GEBR_COMM_SOCKET_STATE_CONNECTING,
	GEBR_COMM_SOCKET_STATE_CONNECTED,
	GEBR_COMM_SOCKET_STATE_LISTENING,
};

struct _GebrCommSocket {
	GObject parent;

	GIOChannel *io_channel;
	guint write_watch_id;
	guint read_watch_id;
	GByteArray *queue_write_bytes;

	enum GebrCommSocketAddressType address_type;
	enum GebrCommSocketState state;
	enum GebrCommSocketError last_error;
};
struct _GebrCommSocketClass {
	GObjectClass parent;

	/* virtual */
	void (*connected) (GebrCommSocket * self);
	void (*disconnected) (GebrCommSocket * self);
	void (*new_connection) (GebrCommSocket * self);
	/* signals */
	void (*ready_read) (GebrCommSocket * self);
	void (*ready_write) (GebrCommSocket * self);
	void (*error) (GebrCommSocket * self, enum GebrCommSocketError error);
};

/*
 * user functions
 */

void gebr_comm_socket_close(GebrCommSocket *);

void gebr_comm_socket_flush(GebrCommSocket *);

void gebr_comm_socket_set_blocking(GebrCommSocket *, gboolean);

enum GebrCommSocketState gebr_comm_socket_get_state(GebrCommSocket *);

enum GebrCommSocketError gebr_comm_socket_get_last_error(GebrCommSocket *);

GebrCommSocketAddress gebr_comm_socket_get_address(GebrCommSocket *);

gulong gebr_comm_socket_bytes_available(GebrCommSocket *);

gulong gebr_comm_socket_bytes_to_write(GebrCommSocket *);

GByteArray *gebr_comm_socket_read(GebrCommSocket *, gsize);

GString *gebr_comm_socket_read_string(GebrCommSocket *, gsize);

GByteArray *gebr_comm_socket_read_all(GebrCommSocket *);

GString *gebr_comm_socket_read_string_all(GebrCommSocket *);

void gebr_comm_socket_write(GebrCommSocket *, GByteArray *);

void gebr_comm_socket_write_string(GebrCommSocket *, GString *);

void gebr_comm_socket_write_immediately(GebrCommSocket *, GByteArray *);

void gebr_comm_socket_write_string_immediately(GebrCommSocket *, GString *);

G_END_DECLS
#endif				//__GEBR_COMM_SOCKET_H
