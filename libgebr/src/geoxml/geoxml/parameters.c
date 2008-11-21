/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
 *
 *   This parameters is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This parameters is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this parameters.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <gdome.h>

#include "parameters.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "program_parameter.h"
#include "program_parameter_p.h"
#include "parameter_p.h"
#include "parameter_group.h"

/*
 * internal stuff
 */

struct geoxml_parameters {
	GdomeElement * element;
};

gboolean
__geoxml_parameters_group_check(GeoXmlParameters * parameters)
{
	GdomeElement *	parent_element;

	/* if this is not in a group then there is no problem */
	parent_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters, &exception);
	if (strcmp(gdome_el_nodeName(parent_element, &exception)->str, "group") != 0)
		return TRUE;

	return (gboolean)(__geoxml_get_element_index((GdomeElement*)parameters) == 1);
}

/*
 * library functions.
 */

GeoXmlParameter *
geoxml_parameters_append_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;
	if (__geoxml_parameters_group_check(parameters) == FALSE)
		return NULL;

	GdomeElement *	element;

	element = __geoxml_insert_new_element((GdomeElement*)parameters, "parameter", NULL);
	__geoxml_insert_new_element(element, "label", NULL);
	__geoxml_parameter_insert_type((GeoXmlParameter*)element, type);

	return (GeoXmlParameter*)element;
}

GeoXmlParameter *
geoxml_parameters_append_reference_parameter(GeoXmlParameters * parameters, GeoXmlParameter * reference)
{
	if (parameters == NULL)
		return NULL;
	if (__geoxml_parameters_group_check(parameters) == FALSE)
		return NULL;

	GeoXmlParameter *	parameter;

	parameter = (GeoXmlParameter*)gdome_el_cloneNode((GdomeElement*)reference, TRUE, &exception);
	__geoxml_parameter_set_be_reference(parameter, reference, TRUE);
	gdome_el_insertBefore((GdomeElement*)parameters, (GdomeNode*)parameter, NULL, &exception);

	return parameter;
}

void
geoxml_parameters_set_exclusive(GeoXmlParameters * parameters, GeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (parameter == NULL) {
		__geoxml_set_attr_value((GdomeElement*)parameters, "exclusive", "0");
		return;
	}

	gchar *	value;

	value = g_strdup_printf("%ld", __geoxml_get_element_index((GdomeElement*)parameter)+1);
	__geoxml_set_attr_value((GdomeElement*)parameters, "exclusive", value);
	g_free(value);

	if (geoxml_parameters_get_selected(parameters) == NULL)
		geoxml_parameters_set_selected(parameters, parameter);
}

GeoXmlParameter *
geoxml_parameters_get_exclusive(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;

	gulong			index;
	GeoXmlSequence *	parameter;

	index = atol(__geoxml_get_attr_value((GdomeElement*)parameters, "exclusive"));
	if (index == 0)
		return NULL;
	index--;
	geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GeoXmlParameter*)geoxml_parameters_get_first_parameter(parameters);

	return (GeoXmlParameter*)parameter;
}

void
geoxml_parameters_set_selected(GeoXmlParameters * parameters, GeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (geoxml_parameters_get_exclusive(parameters) == NULL)
		return;
	if (parameter == NULL) {
		__geoxml_set_attr_value((GdomeElement*)parameters, "selected", "0");
		return;
	}

	gchar *	value;

	value = g_strdup_printf("%ld", __geoxml_get_element_index((GdomeElement*)parameter)+1);
	__geoxml_set_attr_value((GdomeElement*)parameters, "selected", value);
	g_free(value);
}

GeoXmlParameter *
geoxml_parameters_get_selected(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	if (geoxml_parameters_get_exclusive(parameters) == NULL)
		return NULL;

	gulong			index;
	GeoXmlSequence *	parameter;

	index = atol(__geoxml_get_attr_value((GdomeElement*)parameters, "selected"));
	if (index == 0)
		return NULL;
	index--;
	geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GeoXmlParameter*)geoxml_parameters_get_first_parameter(parameters);

	return (GeoXmlParameter *)parameter;
}

GeoXmlSequence *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
}

int
geoxml_parameters_get_parameter(GeoXmlParameters * parameters, GeoXmlSequence ** _parameter, gulong index)
{
	if (parameters == NULL) {
		*_parameter = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	gulong			i;
	GeoXmlSequence *	parameter;

	parameter = (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
	for (i = 0; i < index; ++i) {
		parameter = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)parameter);
		if (parameter == NULL) {
			*_parameter = parameter;
			return GEOXML_RETV_INVALID_INDEX;
		}
	}
	*_parameter = parameter;

	return GEOXML_RETV_SUCCESS;
}

glong
geoxml_parameters_get_number(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;

	gint			parameters_number = 0;
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (parameters_number = 0; parameter != NULL; ++parameters_number)
		geoxml_sequence_next(&parameter);

	return parameters_number;
}

gboolean
geoxml_parameters_get_is_in_group(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	return !strcmp(
		gdome_el_tagName((GdomeElement*)gdome_el_parentNode(
			(GdomeElement*)parameters, &exception), &exception)->str,
		"group") ? TRUE : FALSE;
}

void
geoxml_parameters_reset(GeoXmlParameters * parameters, gboolean recursive)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter))
		geoxml_parameter_reset(GEOXML_PARAMETER(parameter), recursive);
}
