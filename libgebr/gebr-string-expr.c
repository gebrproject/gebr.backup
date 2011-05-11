/*   libgebr - GeBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <glib/gi18n.h>

#include "gebr-string-expr.h"
#include "gebr-iexpr.h"

/* Prototypes & Private {{{1 */
struct _GebrStringExprPriv {
	GHashTable *vars;
};

/* Prototypes {{{2 */
static void gebr_string_expr_interface_init(GebrIExprInterface *iface);

static void gebr_string_expr_finalize(GObject *object);

G_DEFINE_TYPE_WITH_CODE(GebrStringExpr, gebr_string_expr, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_IEXPR,
					      gebr_string_expr_interface_init));

/* Class & Instance initialize/finalize {{{1 */
static void gebr_string_expr_class_init(GebrStringExprClass *klass)
{
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = gebr_string_expr_finalize;

	g_type_class_add_private(klass, sizeof(GebrStringExprPriv));
}

static void gebr_string_expr_init(GebrStringExpr *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_STRING_EXPR,
						 GebrStringExprPriv);

	self->priv->vars = g_hash_table_new_full(g_str_hash,
						 g_str_equal,
						 g_free,
						 NULL);
}

static void gebr_string_expr_finalize(GObject *object)
{
	GebrStringExpr *self = GEBR_STRING_EXPR(object);

	g_hash_table_unref(self->priv->vars);
}

/* GebrIExpr interface methods implementation {{{1 */
static gboolean gebr_string_expr_set_var(GebrIExpr              *iface,
					 const gchar            *name,
					 GebrGeoXmlParameterType type,
					 const gchar            *value,
					 GError                **err)
{
	GError *error = NULL;
	GebrStringExpr *self = GEBR_STRING_EXPR(iface);

	switch (type)
	{
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		break;
	default:
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_TYPE,
			    _("Invalid variable type"));
		return FALSE;
	}

	gebr_iexpr_is_valid(iface, value, &error);

	if (error) {
		g_propagate_error(err, error);
		return FALSE;
	}

	g_hash_table_insert(self->priv->vars,
			    g_strdup(name),
			    GINT_TO_POINTER(1));

	return TRUE;
}

static gboolean gebr_string_expr_is_valid(GebrIExpr   *iface,
					  const gchar *expr,
					  GError     **err)
{
	return gebr_string_expr_eval(GEBR_STRING_EXPR(iface), expr, NULL, err);
}

static void gebr_string_expr_reset(GebrIExpr *iface)
{
	GebrStringExpr *self = GEBR_STRING_EXPR(iface);
	g_hash_table_remove_all(self->priv->vars);
}

static void gebr_string_expr_interface_init(GebrIExprInterface *iface)
{
	iface->set_var = gebr_string_expr_set_var;
	iface->is_valid = gebr_string_expr_is_valid;
	iface->reset = gebr_string_expr_reset;
}

/* Public functions {{{1 */
GebrStringExpr *gebr_string_expr_new(void)
{
	return g_object_new(GEBR_TYPE_STRING_EXPR, NULL);
}

gboolean gebr_string_expr_eval(GebrStringExpr   *self,
			       const gchar      *expr,
			       gchar           **result,
			       GError          **err)
{
	gint j;
	gint i = 0;
	gchar *name;
	gboolean retval = TRUE;
	gboolean fetch_var = FALSE;
	GString *decoded = g_string_new(NULL);

	name = g_new(gchar, strlen(expr));

	while (expr[i])
	{
		if (expr[i] == '[' && expr[i+1] == '[') {
			i += 2;
			g_string_append_c(decoded, '[');
			continue;
		}

		if (expr[i] == '[') {
			fetch_var = TRUE;
			i++;
			j = 0;
			continue;
		}

		if (fetch_var) {
			if (expr[i] == ']') {
				fetch_var = FALSE;
				name[j] = '\0';
				if (g_hash_table_lookup(self->priv->vars, name))
					g_string_append(decoded, name);
				else {
					retval = FALSE;
					g_set_error(err, GEBR_IEXPR_ERROR,
						    GEBR_IEXPR_ERROR_UNDEF_VAR,
						    _("Variable %s is undefined"),
						    name);
					goto exception;
				}
				i++;
				continue;
			} else {
				name[j++] = expr[i++];
				continue;
			}
		}

		if (expr[i] == ']') {
		       if (expr[i+1] != ']') {
			       retval = FALSE;
			       g_set_error(err, GEBR_IEXPR_ERROR,
					   GEBR_IEXPR_ERROR_SYNTAX,
					   _("Syntax error: unmatched closing bracket"));
			       goto exception;
		       } else {
			       g_string_append_c(decoded, ']');
			       i += 2;
		       }
		       continue;
		}

		// Escape double-quotes and backslashes
		if (expr[i] == '"' || expr[i] == '\\')
			g_string_append_c(decoded, '\\');
		g_string_append_c(decoded, expr[i++]);
	}

	if (fetch_var) {
		retval = FALSE;
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_SYNTAX,
			    _("Syntax error: unmatched opening bracket"));
		goto exception;
	}

	if (result)
		*result = g_strdup(decoded->str);

exception:
	g_string_free(decoded, TRUE);
	g_free(name);

	return retval;
}
