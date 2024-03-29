/*
 * gebr-maestro-settings.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Core team (www.gebrproject.com)
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


#include "gebr-maestro-settings.h"
#include <unistd.h>

#include <glib/gstdio.h>
#include <glib/gkeyfile.h>
#include <glib/gi18n.h>

#include "utils.h"

struct _GebrMaestroSettingsPriv {
	gchar *path;
	GKeyFile *maestro_key;
};

GebrMaestroSettings *
gebr_maestro_settings_new(const gchar *path)
{
	GebrMaestroSettings *ms = g_new(GebrMaestroSettings, 1);
	ms->priv = g_new0(GebrMaestroSettingsPriv, 1);

	ms->priv->maestro_key = g_key_file_new();
	ms->priv->path = g_strdup(path);

	/* For the sake of backwards compatibility, load deprecated gengetopt file format ... */
	g_key_file_load_from_file(ms->priv->maestro_key, path, G_KEY_FILE_NONE, NULL);

	return ms;
}

void
gebr_maestro_settings_update(GebrMaestroSettings *ms)
{
	g_return_if_fail(ms != NULL);

	g_key_file_free(ms->priv->maestro_key);
	ms->priv->maestro_key = g_key_file_new();
	g_key_file_load_from_file(ms->priv->maestro_key, ms->priv->path, G_KEY_FILE_NONE, NULL);
}

void
gebr_maestro_settings_save(GebrMaestroSettings *ms)
{
	gsize length;
	gchar *string;
	FILE *configfp;

	string = g_key_file_to_data(ms->priv->maestro_key, &length, NULL);

	configfp = fopen(ms->priv->path, "w");
	if (configfp == NULL) {
		g_debug(_("Could not save configuration."));
		goto out;
	}

	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);

out:
	g_free(string);
	return;
}

void
gebr_maestro_settings_free(GebrMaestroSettings *ms)
{
	g_free(ms->priv->path);
	g_key_file_free(ms->priv->maestro_key);
	g_free(ms);
}

void
gebr_maestro_settings_set_domain(GebrMaestroSettings *ms,
				 const gchar *domain,
				 const gchar *label,
				 const gchar *addr,
				 const gchar *node)
{
	g_key_file_set_string(ms->priv->maestro_key, domain, "label", label);

	gebr_maestro_settings_append_address(ms, domain, addr);
	gebr_maestro_settings_add_node(ms, domain, node);
}

void
gebr_maestro_settings_prepend_address(GebrMaestroSettings *ms,
				      const gchar *domain,
				      const gchar *addr)
{
	g_return_if_fail(ms != NULL);
	g_return_if_fail(domain != NULL);
	g_return_if_fail(addr != NULL);

	if (!*addr)
		return;

	GString *maestro, *buf;
	gchar **list;

	buf = g_string_new(addr);
	maestro = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");
	list = g_strsplit(maestro->str, ",", -1);

	if (list) {
		for (gint i = 0; list[i]; i++) {
			if (g_strcmp0(list[i], addr) != 0) {
				g_string_append_c(buf, ',');
				g_string_append(buf, list[i]);
			}
		}
	}

	g_key_file_set_string(ms->priv->maestro_key, domain, "maestro", buf->str);
	gebr_maestro_settings_save(ms);

	g_string_free(buf, TRUE);
	g_string_free(maestro, TRUE);
	g_strfreev(list);
}

void
gebr_maestro_settings_append_address(GebrMaestroSettings *ms,
				     const gchar *domain,
				     const gchar *addr)
{
	g_return_if_fail(ms != NULL);
	g_return_if_fail(domain != NULL);
	g_return_if_fail(addr != NULL);

	if (!*addr)
		return;

	GString *maestro, *buf;
	gchar **list;
	gboolean has_addr = FALSE;

	maestro = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");
	buf = g_string_new(maestro->str);
	list = g_strsplit(maestro->str, ",", -1);

	if (list) {
		for (gint i = 0; list[i]; i++) {
			if (g_strcmp0(list[i], addr) == 0)
				has_addr = TRUE;
		}
	}

	if (!has_addr) {
		if (buf->len > 1)
			g_string_append_c(buf, ',');
		g_string_append(buf, addr);
	}

	g_key_file_set_string(ms->priv->maestro_key, domain, "maestro", buf->str);
	gebr_maestro_settings_save(ms);

	g_string_free(buf, TRUE);
	g_string_free(maestro, TRUE);
	g_strfreev(list);
}

void
gebr_maestro_settings_change_label(GebrMaestroSettings *ms,
                                   const gchar *domain,
                                   const gchar *label)
{
	if (g_key_file_has_group(ms->priv->maestro_key, domain))
		g_key_file_set_string(ms->priv->maestro_key, domain, "label", label);
}

gchar *
gebr_maestro_settings_get_addrs(GebrMaestroSettings *ms,
				const gchar *domain)
{
	GString *maestros;

	maestros = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");

	return g_string_free(maestros, FALSE);
}

gchar *
gebr_maestro_settings_get_addr_for_domain(GebrMaestroSettings *ms,
					  const gchar *domain,
					  gint index)
{
	gchar *maestro;
	GString *maestros;

	maestros = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");

	const gchar *list = g_string_free(maestros, FALSE);

	gchar **addrs = g_strsplit(list, ",", -1);

	if (addrs)
		maestro = g_strdup(addrs[index]);

	g_strfreev(addrs);

	return maestro;
}

gchar *
gebr_maestro_settings_get_label_for_domain(GebrMaestroSettings *ms,
                                           const gchar *domain,
                                           gboolean use_default)
{
	gchar *label_default;
	GString *label;

	if (use_default)
		label_default = gebr_maestro_settings_generate_nfs_label(ms, domain);
	else
		label_default = g_strdup("");

	label = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "label", label_default);

	g_free(label_default);

	return g_string_free(label, FALSE);
}

GKeyFile *
gebr_maestro_settings_get_key_file(GebrMaestroSettings *ms)
{
	return ms->priv->maestro_key;
}

gchar **
gebr_maestro_settings_get_ids(GebrMaestroSettings *ms)
{
	gsize len;
	gchar **ids;

	ids = g_key_file_get_groups(ms->priv->maestro_key, &len);

	return ids;
}

gchar *
gebr_maestro_settings_get_nodes(GebrMaestroSettings *ms, const gchar *domain)
{
	GString *nodes = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "nodes", "");

	return g_string_free(nodes, FALSE);
}

void
gebr_maestro_settings_add_node(GebrMaestroSettings *ms, const gchar *domain, const gchar *node)
{
	g_return_if_fail(ms != NULL);
	g_return_if_fail(domain != NULL);
	g_return_if_fail(node != NULL);

	if (!*node)
		return;

	GError *err = NULL;
	gchar *nodes_old = g_key_file_get_value(ms->priv->maestro_key, domain, "nodes", &err);

	GString *nodes = g_string_new(nodes_old);
	gchar **list = g_strsplit(nodes_old, ",", -1);
	gboolean has_addr = FALSE;

	if (list) {
		for (gint i = 0; list[i]; i++) {
			if (g_strcmp0(list[i], node) == 0)
				has_addr = TRUE;
		}
	}

	if (!has_addr) {
		if (nodes->len > 1)
			g_string_append_c(nodes, ',');
		g_string_append(nodes, node);
	}

	g_key_file_set_value(ms->priv->maestro_key, domain, "nodes", nodes->str);
	gebr_maestro_settings_save(ms);

	g_string_free(nodes, TRUE);
	g_free(nodes_old);
	g_strfreev(list);

	return;
}

gchar *
gebr_maestro_settings_generate_nfs_label(GebrMaestroSettings *ms,
                                         const gchar *nfsid)
{
	gint pos = 1;
	gsize length;
	gchar **nfss;

	nfss = g_key_file_get_groups(ms->priv->maestro_key, &length);

	if (nfss && nfsid && *nfsid) {
		for (gint i = 0; i < length; i++) {
			pos += 1;
			if (g_strcmp0(nfss[i], nfsid) == 0)
				break;
		}
	}

	gchar *label = g_strdup_printf("GêBR Domain %d", pos);

	g_strfreev(nfss);

	return label;
}
