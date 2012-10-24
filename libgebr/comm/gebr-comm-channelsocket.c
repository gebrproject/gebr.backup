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

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "gebr-comm-channelsocket.h"
#include "gebr-comm-socketprivate.h"
#include "gebr-comm-streamsocketprivate.h"
#include "gebr-comm-socketaddressprivate.h"

/*
 * prototypes
 */

static void __g_channel_socket_new_connection(GebrCommChannelSocket * channel_socket);

static int x11_open_helper(Buffer *b);

/*
 * gobject stuff
 */

enum {
	LAST_PROPERTY
};

enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];

static void gebr_comm_channel_socket_class_init(GebrCommChannelSocketClass * class)
{
	/* virtual */
	class->parent.new_connection = (typeof(class->parent.new_connection)) __g_channel_socket_new_connection;
}

static void gebr_comm_channel_socket_init(GebrCommChannelSocket * channel_socket)
{
	channel_socket->parent.state = GEBR_COMM_SOCKET_STATE_NOTLISTENING;
}

G_DEFINE_TYPE(GebrCommChannelSocket, gebr_comm_channel_socket, GEBR_COMM_SOCKET_TYPE)

/*
 * internal functions
 */
static void __g_channel_socket_source_disconnected(GebrCommStreamSocket * source_socket, GebrCommStreamSocket * forward_socket)
{
	gebr_comm_socket_close(GEBR_COMM_SOCKET(source_socket));
	gebr_comm_socket_close(GEBR_COMM_SOCKET(forward_socket));
}

static void __g_channel_socket_source_read(GebrCommStreamSocket * source_socket, GebrCommStreamSocket * forward_socket)
{
	GByteArray *byte_array;

	byte_array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(source_socket));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(forward_socket), byte_array);

	g_byte_array_free(byte_array, TRUE);
}

static void
__g_channel_socket_source_error(GebrCommStreamSocket * source_socket, enum GebrCommSocketError error,
				GebrCommStreamSocket * forward_socket)
{
	__g_channel_socket_source_disconnected(source_socket, forward_socket);
}

static void __g_channel_socket_forward_disconnected(GebrCommStreamSocket * forward_socket, GebrCommStreamSocket * source_socket)
{
	__g_channel_socket_source_disconnected(source_socket, forward_socket);
}

static void __g_channel_socket_forward_read(GebrCommStreamSocket * forward_socket, GebrCommStreamSocket * source_socket)
{
	__g_channel_socket_source_read(forward_socket, source_socket);
}

static void
__g_channel_socket_forward_error(GebrCommStreamSocket * forward_socket, enum GebrCommSocketError error,
				 GebrCommStreamSocket * source_socket)
{
	__g_channel_socket_source_error(source_socket, error, forward_socket);
}

static void __g_channel_socket_new_connection(GebrCommChannelSocket * channel_socket)
{
	GebrCommSocketAddress peer_address;
	int client_sockfd, sockfd;

	sockfd = _gebr_comm_socket_get_fd(&channel_socket->parent);
	while ((client_sockfd = _gebr_comm_socket_address_accept(&peer_address,
								 channel_socket->parent.address_type, sockfd)) != -1) {
		GebrCommStreamSocket *source_socket;
		GebrCommStreamSocket *forward_socket;

		source_socket =
		    _gebr_comm_stream_socket_new_connected(client_sockfd, channel_socket->parent.address_type);
		forward_socket = gebr_comm_stream_socket_new();
		gebr_comm_stream_socket_connect(forward_socket, &channel_socket->forward_address, FALSE);

		g_signal_connect(source_socket, "disconnected",
				 G_CALLBACK(__g_channel_socket_source_disconnected), forward_socket);
		g_signal_connect(source_socket, "ready-read",
				 G_CALLBACK(__g_channel_socket_source_read), forward_socket);
		g_signal_connect(source_socket, "error",
				 G_CALLBACK(__g_channel_socket_source_error), forward_socket);

		g_signal_connect(forward_socket, "disconnected",
				 G_CALLBACK(__g_channel_socket_forward_disconnected), source_socket);
		g_signal_connect(forward_socket, "ready-read",
				 G_CALLBACK(__g_channel_socket_forward_read), source_socket);
		g_signal_connect(forward_socket, "error",
				 G_CALLBACK(__g_channel_socket_forward_error), source_socket);
	}
}

/*
 * user functions
 */

GebrCommChannelSocket *gebr_comm_channel_socket_new(void)
{
	return (GebrCommChannelSocket *) g_object_new(GEBR_COMM_CHANNEL_SOCKET_TYPE, NULL);
}

void gebr_comm_channel_socket_free(GebrCommChannelSocket * channel_socket)
{
	g_return_if_fail(GEBR_COMM_IS_CHANNEL_SOCKET(channel_socket));

	gebr_comm_socket_close(&channel_socket->parent);
	g_object_unref(channel_socket);
}

gboolean
gebr_comm_channel_socket_start(GebrCommChannelSocket * channel_socket, GebrCommSocketAddress * listen_address,
			       GebrCommSocketAddress * forward_address)
{
	g_return_val_if_fail(GEBR_COMM_IS_CHANNEL_SOCKET(channel_socket), FALSE);

	if (!gebr_comm_socket_address_get_is_valid(listen_address) ||
	    !gebr_comm_socket_address_get_is_valid(forward_address))
		return FALSE;

	int sockfd;
	struct sockaddr *sockaddr;
	gsize sockaddr_size;
	GError *error;

	/* initialization */
	error = NULL;
	sockfd = socket(_gebr_comm_socket_address_get_family(listen_address), SOCK_STREAM, 0);
	_gebr_comm_socket_init(&channel_socket->parent, sockfd, listen_address->type);
	channel_socket->parent.state = GEBR_COMM_SOCKET_STATE_NOTLISTENING;
	g_io_channel_set_flags(channel_socket->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
	_gebr_comm_socket_enable_read_watch(&channel_socket->parent);

	/* bind and listen */
	_gebr_comm_socket_address_get_sockaddr(listen_address, &sockaddr, &sockaddr_size);
	if (bind(sockfd, sockaddr, sockaddr_size))
		return FALSE;
	if (listen(sockfd, 1024)) {
		channel_socket->parent.state = GEBR_COMM_SOCKET_STATE_NOTLISTENING;
		return FALSE;
	}
	channel_socket->parent.state = GEBR_COMM_SOCKET_STATE_LISTENING;
	channel_socket->forward_address = *forward_address;

	return TRUE;
}

GebrCommSocketAddress gebr_comm_channel_socket_get_forward_address(GebrCommChannelSocket * channel_socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_CHANNEL_SOCKET(channel_socket), _gebr_comm_socket_address_unknown());

	return channel_socket->forward_address;
}

/*
 * This is a special state for X11 authentication spoofing.  An opened X11
 * connection (when authentication spoofing is being done) remains in this
 * state until the first packet has been completely read.  The authentication
 * data in that packet is then substituted by the real data if it matches the
 * fake data, and the channel is put into normal mode.
 * XXX All this happens at the client side.
 * Returns: 0 = need more data, -1 = wrong cookie, 1 = ok
 */
static int
x11_open_helper(Buffer *b)
{
	u_char *ucp;
	u_int proto_len, data_len;

	/* Check if the fixed size part of the packet is in buffer. */
	if (buffer_len(b) < 12)
		return 0;

	/* Parse the lengths of variable-length fields. */
	ucp = buffer_ptr(b);
	if (ucp[0] == 0x42) {	/* Byte order MSB first. */
		proto_len = 256 * ucp[6] + ucp[7];
		data_len = 256 * ucp[8] + ucp[9];
	} else if (ucp[0] == 0x6c) {	/* Byte order LSB first. */
		proto_len = ucp[6] + 256 * ucp[7];
		data_len = ucp[8] + 256 * ucp[9];
	} else {
		debug2("Initial X11 packet contains bad byte order byte: 0x%x",
		    ucp[0]);
		return -1;
	}

	/* Check if the whole packet is in buffer. */
	if (buffer_len(b) <
	    12 + ((proto_len + 3) & ~3) + ((data_len + 3) & ~3))
		return 0;

	/* Check if authentication protocol matches. */
	if (proto_len != strlen(x11_saved_proto) ||
	    memcmp(ucp + 12, x11_saved_proto, proto_len) != 0) {
		debug2("X11 connection uses different authentication protocol.");
		return -1;
	}
	/* Check if authentication data matches our fake data. */
	if (data_len != x11_fake_data_len ||
	    memcmp(ucp + 12 + ((proto_len + 3) & ~3),
		x11_fake_data, x11_fake_data_len) != 0) {
		debug2("X11 auth data does not match fake data.");
		return -1;
	}
	/* Check fake data length */
	if (x11_fake_data_len != x11_saved_data_len) {
		error("X11 fake_data_len %d != saved_data_len %d",
		    x11_fake_data_len, x11_saved_data_len);
		return -1;
	}
	/*
	 * Received authentication protocol and data match
	 * our fake data. Substitute the fake data with real
	 * data.
	 */
	memcpy(ucp + 12 + ((proto_len + 3) & ~3),
	    x11_saved_data, x11_saved_data_len);
	return 1;
}
