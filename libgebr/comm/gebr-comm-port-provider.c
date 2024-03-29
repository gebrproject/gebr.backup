/*
 * gebr-comm-port-provider.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-comm-port-provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "gebr-comm-ssh.h"
#include <libgebr/utils.h>
#include "gebr-comm-terminalprocess.h"

#include "gebr-comm-listensocket.h"
#include "gebr-comm-process.h"
#include "../marshalers.h"
#include "gebr-comm-utils.h"

struct _GebrCommPortForward {
	GebrCommSsh *ssh;
};

struct _GebrCommPortProviderPriv {
	guint port;
	GebrCommPortType type;
	gchar *address;
	gchar *sftp_address;
	guint display;
	GebrCommSsh *ssh_forward;
	GebrCommPortForward *forward;
};

static gchar *get_local_forward_command(GebrCommPortProvider *self,
					guint *port,
					const gchar *addr,
					guint remote_port);

static gboolean get_port_from_command_output(GebrCommPortProvider *self,
                                             const gchar *buffer,
                                             guint *port);

GQuark
gebr_comm_port_provider_error_quark(void)
{
	return g_quark_from_static_string("gebr-comm-port-provider-error-quark");
}

/* GObject stuff {{{ */
G_DEFINE_TYPE(GebrCommPortProvider, gebr_comm_port_provider, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_TYPE,
	PROP_ADDRESS,
};

enum {
	PORT_DEFINED,
	ERROR,
	PASSWORD,
	QUESTION,
	ACCEPTS_KEY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
gebr_comm_port_provider_get(GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);

	switch (prop_id)
	{
	case PROP_TYPE:
		g_value_set_int(value, self->priv->type);
		break;
	case PROP_ADDRESS:
		g_value_set_string(value, self->priv->address);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_comm_port_provider_set(GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);

	switch (prop_id)
	{
	case PROP_TYPE:
		self->priv->type = g_value_get_int(value);
		break;
	case PROP_ADDRESS:
		if (self->priv->address)
			g_free(self->priv->address);
		self->priv->address = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_comm_port_provider_finalize(GObject *object)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);
	g_free(self->priv->address);
	g_free(self->priv->sftp_address);
	G_OBJECT_CLASS(gebr_comm_port_provider_parent_class)->finalize(object);
}

static void
gebr_comm_port_provider_class_init(GebrCommPortProviderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_comm_port_provider_get;
	object_class->set_property = gebr_comm_port_provider_set;
	object_class->finalize = gebr_comm_port_provider_finalize;

	/**
	 * GebrCommPortProvider::port-define:
	 *
	 * This signal is emitted after gebr_comm_port_provider_start() is
	 * executed and if no error happens in the process. It provides the
	 * port that can be used to communicate with the specified type in the
	 * constructor gebr_comm_port_provider_new().
	 */
	signals[PORT_DEFINED] =
		g_signal_new("port-defined",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, port_defined),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__INT,
			     G_TYPE_NONE, 1,
			     G_TYPE_INT);

	/**
	 * GebrCommPortProvider::error:
	 *
	 * This signal is emitted after gebr_comm_port_provider_start() is
	 * executed and an error has occured during the process. See
	 * #GEBR_COMM_PORT_PROVIDER_ERROR for more information.
	 */
	signals[ERROR] =
		g_signal_new("error",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, error),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__POINTER,
			     G_TYPE_NONE, 1,
			     G_TYPE_POINTER);

	/**
	 * GebrCommPortProvider::password:
	 */
	signals[PASSWORD] =
		g_signal_new("password",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, password),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__OBJECT_BOOLEAN,
			     G_TYPE_NONE, 2,
			     G_TYPE_OBJECT, G_TYPE_BOOLEAN);

	/**
	 * GebrCommPortProvider::question:
	 */
	signals[QUESTION] =
		g_signal_new("question",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, question),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__OBJECT_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_OBJECT, G_TYPE_STRING);

	/**
	 * GebrCommPortProvider::accepts-key
	 */
	signals[ACCEPTS_KEY] =
		g_signal_new("accepts-key",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, accepts_key),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__BOOLEAN,
			     G_TYPE_NONE, 1,
			     G_TYPE_BOOLEAN);

	g_object_class_install_property(object_class,
					PROP_TYPE,
					g_param_spec_int("type",
							 "Type",
							 "Type",
							 0, 100, 0,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(GebrCommPortProviderPriv));
}

static void
gebr_comm_port_provider_init(GebrCommPortProvider *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_COMM_TYPE_PORT_PROVIDER,
						 GebrCommPortProviderPriv);
}
/* }}} */

static gboolean
is_local_address(const gchar *addr)
{
	return gebr_comm_is_local_address(addr);
}

/*
 * emit_signals:
 *
 * This method will emit the correct signal given the @port and @error. If
 * @error is non-%NULL, then an GebrCommPortProvider::error signal is emitted,
 * otherwise the GebrCommPortProvider::port-defined signal is emitted.
 *
 * Note: the @error domain must be GEBR_COMM_PORT_PROVIDER_ERROR, see
 * #GebrCommPortProviderError enumeration for a list of possible errors.
 */
static void
emit_signals(GebrCommPortProvider *self, guint port, GError *error)
{
	if (!error)
		g_signal_emit(self, signals[PORT_DEFINED], 0, port);
	else
		g_signal_emit(self, signals[ERROR], 0, error);
}

/*
 * PortProviderVirtualMethods:
 *
 * This structure provides some virtual methods that must be implemented by
 * both local and remote logics. Each method must calculate the port and call
 * emit_signals() when done or upon error. See emit_signals() documentation for
 * more info.
 */
struct PortProviderVirtualMethods {
	void (*get_maestro_port) (GebrCommPortProvider *self);
	void (*get_daemon_port)  (GebrCommPortProvider *self);
	void (*get_x11_port)     (GebrCommPortProvider *self);
	void (*get_sftp_port)    (GebrCommPortProvider *self);
};

void local_get_maestro_port (GebrCommPortProvider *self);
void local_get_daemon_port  (GebrCommPortProvider *self);
void local_get_x11_port     (GebrCommPortProvider *self);
void local_get_sftp_port    (GebrCommPortProvider *self);

void remote_get_maestro_port (GebrCommPortProvider *self);
void remote_get_daemon_port  (GebrCommPortProvider *self);
void remote_get_x11_port     (GebrCommPortProvider *self);
void remote_get_sftp_port    (GebrCommPortProvider *self);

struct PortProviderVirtualMethods local_methods = {
	.get_maestro_port = local_get_maestro_port,
	.get_daemon_port  = local_get_daemon_port,
	.get_x11_port     = local_get_x11_port,
	.get_sftp_port    = local_get_sftp_port,
};

struct PortProviderVirtualMethods remote_methods = {
	.get_maestro_port = remote_get_maestro_port,
	.get_daemon_port  = remote_get_daemon_port,
	.get_x11_port     = remote_get_x11_port,
	.get_sftp_port    = remote_get_sftp_port,
};

static void
start(GebrCommPortProvider *self, struct PortProviderVirtualMethods *vmethods)
{
	GError *error = NULL;

	switch (self->priv->type)
	{
	case GEBR_COMM_PORT_TYPE_MAESTRO:
		vmethods->get_maestro_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_DAEMON:
		vmethods->get_daemon_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_X11:
		vmethods->get_x11_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_SFTP:
		vmethods->get_sftp_port(self);
		break;
	default:
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_UNKNOWN_TYPE,
			    "Unknown port type");
		emit_signals(self, 0, error);
		break;
	}
}

/* Set our own error*/
static void
transform_spawn_sync_error(GError *error, GError **local_error)
{
	if (!error)
		return;

	GebrCommPortProviderError error_type;
	const gchar *error_msg = NULL;

	error_type = GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN;
	error_msg = "Error in the process execution";

	g_set_error(local_error,
		    GEBR_COMM_PORT_PROVIDER_ERROR,
		    error_type,
		    "%s",
		    error_msg);
}

static void
set_forward(GebrCommPortProvider *self, GebrCommSsh *ssh)
{
	g_return_if_fail(self->priv->forward == NULL);
	self->priv->forward = g_new0(GebrCommPortForward, 1);
	self->priv->forward->ssh = ssh;
}

static guint
get_port(GebrCommPortProvider *self)
{
	if (self->priv->port != 0)
		return self->priv->port;

	if (self->priv->type == GEBR_COMM_PORT_TYPE_X11)
		self->priv->port = gebr_comm_get_available_port(6000);
	else
		self->priv->port = gebr_comm_get_available_port(2000);

	return self->priv->port;
}

/* Local port provider implementation {{{ */
static void
local_get_port(GebrCommPortProvider *self, gboolean maestro)
{
	const gchar *binary = maestro ? "gebrm":"gebrd";
	gchar *output = NULL;
	gchar *err = NULL;
	gint status;
	GError *error = NULL;

	g_spawn_command_line_sync(binary, &output, &err, &status, &error);

	GError *local_error = NULL;

	guint port;

	if (error || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
		transform_spawn_sync_error(error, &local_error);
		g_clear_error(&error);
	} else if (output) {
		if (!get_port_from_command_output(self, output, &port))
			return;
	} else {
		g_warn_if_reached();
	}

	emit_signals(self, port, error);

	g_free(output);
}

void
local_get_maestro_port(GebrCommPortProvider *self)
{
	local_get_port(self, TRUE);
}

void
local_get_daemon_port(GebrCommPortProvider *self)
{
	local_get_port(self, FALSE);
}

void
local_get_x11_port(GebrCommPortProvider *self)
{
	g_return_if_fail(self->priv->display != 0);
	emit_signals(self, self->priv->display, NULL);
}

void
local_get_sftp_port(GebrCommPortProvider *self)
{
	GError *error = NULL;
	g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
		    GEBR_COMM_PORT_PROVIDER_ERROR_SFTP_NOT_REQUIRED,
		    "SFTP not required in local configuration");
	emit_signals(self, 0, error);
}
/* }}} */

/* Remote port provider implementation {{{ */
static void
on_ssh_password(GebrCommSsh *ssh, gboolean retry, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[PASSWORD], 0, ssh, retry);
}

static void
on_ssh_question(GebrCommSsh *ssh, const gchar *question, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[QUESTION], 0, ssh, question);
}

static void
on_ssh_error(GebrCommSsh *ssh, const gchar *msg, GebrCommPortProvider *self)
{
	GError *error = NULL;
	g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
		    GEBR_COMM_PORT_PROVIDER_ERROR_SSH,
		    "%s", msg);
	emit_signals(self, 0, error);
}

static void
on_ssh_key(GebrCommSsh *ssh, gboolean accepts_key, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[ACCEPTS_KEY], 0, accepts_key);
}

struct TunnelPollData {
	GebrCommPortProvider *self;
	guint port;
};

static gboolean
tunnel_poll_port(gpointer user_data)
{
	struct TunnelPollData *data = user_data;

	if (gebr_comm_listen_socket_is_local_port_available(data->port))
		return TRUE;

	emit_signals(data->self, data->port, NULL);
	g_free(data);
	return FALSE;
}

static gboolean
get_port_from_command_output(GebrCommPortProvider *self,
                             const gchar *buffer,
                             guint *port)
{
	gchar *redirect_addr = g_strrstr(buffer, GEBR_ADDR_PREFIX);

	if (redirect_addr) {
		redirect_addr += strlen(GEBR_ADDR_PREFIX);
		gchar *nl = strchr(redirect_addr, '\n');
		gchar *addr;
		if (nl)
			addr = g_strndup(redirect_addr, (nl - redirect_addr)/sizeof(gchar));
		else
			addr = g_strdup(redirect_addr);
		g_strstrip(addr);

		if (gebr_comm_is_address_equal(self->priv->address, addr)) {
			*port = atoi(buffer + strlen(GEBR_PORT_PREFIX));
			g_free(addr);
		} else {
			GError *err = NULL;
			g_set_error(&err, GEBR_COMM_PORT_PROVIDER_ERROR, GEBR_COMM_PORT_PROVIDER_ERROR_REDIRECT, "%s", addr);
			emit_signals(self, 0, err);
			g_free(addr);
			return FALSE;
		}
	} else
		*port = atoi(buffer + strlen(GEBR_PORT_PREFIX));

	return TRUE;
}

static void
on_ssh_stdout(GebrCommSsh *_ssh, const GString *buffer, GebrCommPortProvider *self)
{
	guint port = 2125;
	guint remote_port;

	if (!get_port_from_command_output(self, buffer->str, &remote_port))
		return;

	gchar *command = get_local_forward_command(self, &port, "127.0.0.1", remote_port);

	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	gebr_comm_ssh_set_command(ssh, command);
	set_forward(self, ssh);
	gebr_comm_ssh_run(ssh);
	g_free(command);

	struct TunnelPollData *data = g_new(struct TunnelPollData, 1);
	data->self = self;
	data->port = port;
	g_timeout_add(200, tunnel_poll_port, data);
}

static gchar *
get_launch_command(GebrCommPortProvider *self, gboolean is_maestro)
{
	const gchar *binary = is_maestro ? "gebrm" : "gebrd";

	gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key();

	GString *cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s -v -x %s \"bash -l -c '%s >&3' 3>&1 >/dev/null 2>&1\"",
	                ssh_cmd, self->priv->address, binary);
	gchar *cmd = g_shell_quote(cmd_line->str);

	g_string_printf(cmd_line, "bash -c %s", cmd);

	g_free(cmd);
	g_free(ssh_cmd);

	return g_string_free(cmd_line, FALSE);
}

void
remote_get_port(GebrCommPortProvider *self, gboolean is_maestro)
{
	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	g_signal_connect(ssh, "ssh-stdout", G_CALLBACK(on_ssh_stdout), self);
	g_signal_connect(ssh, "ssh-key", G_CALLBACK(on_ssh_key), self);
	gchar *command = get_launch_command(self, is_maestro);
	gebr_comm_ssh_set_command(ssh, command);
	gebr_comm_ssh_run(ssh);
	g_free(command);
}

void
remote_get_maestro_port(GebrCommPortProvider *self)
{
	remote_get_port(self, TRUE);
}

void
remote_get_daemon_port(GebrCommPortProvider *self)
{
	remote_get_port(self, FALSE);
}

static gchar *
get_x11_command(GebrCommPortProvider *self, guint *x11_port)
{
	gchar *ssh_cmd;
	guint16 display_number = 0;
	GString *cmd_line;
	GString *display_host = g_string_new(NULL);

	gchar *display = getenv("DISPLAY");

	// GeBR has display and Maestro don't
	if (display && strlen(display)) {
		g_string_append_len(display_host, display, (strchr(display, ':')-display)/sizeof(gchar));

		GString *tmp = g_string_new(strchr(display, ':'));
		if (sscanf(tmp->str, ":%hu.", &display_number) != 1)
			display_number = 0;
	}

	if (!display_host->len)
		g_string_assign(display_host, "127.0.0.1");

	if (!display_number)
		*x11_port = get_port(self);
	else
		*x11_port = display_number + 6000;

	ssh_cmd = gebr_comm_get_ssh_command_with_key();
	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s -x -R %d:%s:%d %s -N", ssh_cmd, self->priv->display,
			display_host->str, *x11_port, self->priv->address);

	g_free(ssh_cmd);

	return g_string_free(cmd_line, FALSE);
}

void
remote_get_x11_port(GebrCommPortProvider *self)
{
	static guint x11_port = 6010;
	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	gchar *command = get_x11_command(self, &x11_port);
	gebr_comm_ssh_set_command(ssh, command);
	set_forward(self, ssh);
	gebr_comm_ssh_run(ssh);
	g_free(command);
	emit_signals(self, x11_port, NULL);
}

static gchar *
get_local_forward_command(GebrCommPortProvider *self,
			  guint *port,
			  const gchar *addr,
			  guint remote_port)
{
	gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key();
	GString *string = g_string_new(NULL);

	*port = get_port(self);

	g_string_printf(string, "%s -x -L %d:%s:%d %s -N", ssh_cmd, *port,
			addr, remote_port, self->priv->address);

	g_free(ssh_cmd);

	return g_string_free(string, FALSE);
}

void
remote_get_sftp_port(GebrCommPortProvider *self)
{
	guint port = 2000;
	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	gchar *command = get_local_forward_command(self, &port, self->priv->sftp_address, 22);
	gebr_comm_ssh_set_command(ssh, command);
	set_forward(self, ssh);
	gebr_comm_ssh_run(ssh);
	g_free(command);

	struct TunnelPollData *data = g_new(struct TunnelPollData, 1);
	data->self = self;
	data->port = port;
	g_timeout_add(200, tunnel_poll_port, data);
}

/* Authentication keys methods*/
/* }}} */

/* Public API {{{ */
GebrCommPortProvider *
gebr_comm_port_provider_new(GebrCommPortType type,
			    const gchar *address)
{
	return g_object_new(GEBR_COMM_TYPE_PORT_PROVIDER,
			    "type", type,
			    "address", address,
			    NULL);
}

void
gebr_comm_port_provider_set_display(GebrCommPortProvider *self,
				    guint display)
{
	self->priv->display = display;
}

void
gebr_comm_port_provider_set_sftp_address(GebrCommPortProvider *self,
					 const gchar *address)
{
	self->priv->sftp_address = g_strdup(address);
}

void
gebr_comm_port_provider_start(GebrCommPortProvider *self)
{
	gebr_comm_port_provider_start_with_port(self, 0);
}

void
gebr_comm_port_provider_start_with_port(GebrCommPortProvider *self, guint port)
{
	self->priv->port = port;

	struct PortProviderVirtualMethods *vmethods;

	if (is_local_address(self->priv->address))
		vmethods = &local_methods;
	else
		vmethods = &remote_methods;

	start(self, vmethods);
}

GebrCommPortForward *
gebr_comm_port_provider_get_forward(GebrCommPortProvider *self)
{
	return self->priv->forward;
}

/* GebrCommPortForward methods */

void
gebr_comm_port_forward_close(GebrCommPortForward *port_forward)
{
	g_return_if_fail(port_forward != NULL);

	if (port_forward->ssh) {
		gebr_comm_ssh_kill(port_forward->ssh);
		g_object_unref(port_forward->ssh);
		port_forward->ssh = NULL;
	}
}

void
gebr_comm_port_forward_free(GebrCommPortForward *port_forward)
{
	g_free(port_forward);
}
/* }}} */
