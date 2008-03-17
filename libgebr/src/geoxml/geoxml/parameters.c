/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#include <gdome.h>

#include "parameters.h"
#include "xml.h"
#include "types.h"
#include "parameter_group.h"
#include "program_parameter.h"

/*
 * internal stuff
 */

struct geoxml_parameters {
	GdomeElement * element;
};

gboolean
__geoxml_parameters_adjust_group_npar(GeoXmlParameters * parameters, gint adjust)
{
	GdomeElement *	parent_element;
	gchar *		value;

	parent_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters, &exception);
	if (g_ascii_strcasecmp(gdome_el_nodeName(parent_element, &exception)->str, "group") != 0)
		return TRUE;
	if (g_ascii_strcasecmp(__geoxml_get_attr_value(parent_element, "instances"), "1") != 0)
		return FALSE;

	value = g_strdup_printf("%d",
		atoi(__geoxml_get_attr_value(parent_element, "npar")) + adjust);
	__geoxml_set_attr_value(parent_element, "npar", value);

	return TRUE;
}

GeoXmlParameter *
__geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	GdomeElement *	parameter_element;

	/* increases the npar counter if it is a group */
	if (__geoxml_parameters_adjust_group_npar(parameters, +1) == FALSE)
		return NULL;

	parameter_element = __geoxml_new_element((GdomeElement*)parameters, parameter_type_to_str[type]);
	if (type != GEOXML_PARAMETERTYPE_GROUP) {
		__geoxml_insert_new_element(parameter_element, "keyword", NULL);
		__geoxml_insert_new_element(parameter_element, "label", NULL);
		__geoxml_insert_new_element(parameter_element, (type != GEOXML_PARAMETERTYPE_FLAG) ? "value" : "state", NULL);

		if (type != GEOXML_PARAMETERTYPE_FLAG)
			geoxml_program_parameter_set_required((GeoXmlProgramParameter*)parameter_element, FALSE);
		else
			geoxml_program_parameter_set_flag_default((GeoXmlProgramParameter*)parameter_element, FALSE);

		switch (type) {
		case GEOXML_PARAMETERTYPE_FILE:
			geoxml_program_parameter_set_file_be_directory((GeoXmlProgramParameter*)parameter_element, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			geoxml_program_parameter_set_range_properties((GeoXmlProgramParameter*)parameter_element, "", "", "", "");
			break;
		default:
			break;
		}
	} else {
		__geoxml_insert_new_element(parameter_element, "label", NULL);
		__geoxml_insert_new_element(parameter_element, "parameters", NULL);
		/* attributes */
		__geoxml_set_attr_value(parameter_element, "exclusive", "no");
		__geoxml_set_attr_value(parameter_element, "expand", "yes");
		__geoxml_set_attr_value(parameter_element, "multiple", "yes");
		__geoxml_set_attr_value(parameter_element, "npar", "0");
		__geoxml_set_attr_value(parameter_element, "instances", "1");
	}

	return (GeoXmlParameter*)parameter_element;
}

void
__geoxml_parameters_reset(GeoXmlParameters * parameters, gboolean recursive)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	while (parameter != NULL) {
		if (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter)) == GEOXML_PARAMETERTYPE_GROUP) {
			if (recursive == FALSE) {
				geoxml_sequence_next(&parameter);
				continue;
			}
			__geoxml_parameters_reset(
				geoxml_parameter_group_get_parameters(GEOXML_PARAMETER_GROUP(parameter)),
				recursive);
		} else {
			geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter), "");
			geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(parameter), "");
		}

		geoxml_sequence_next(&parameter);
	}
}

/*
 * library functions.
 */

GeoXmlParameter *
geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;
	return __geoxml_parameters_new_parameter(parameters, type);
}

GeoXmlParameter *
geoxml_parameters_append_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;

	GdomeElement *	element;

	element = (GdomeElement*)__geoxml_parameters_new_parameter(parameters, type);
	if (element == NULL)
		return NULL;
	gdome_el_insertBefore((GdomeElement*)parameters, (GdomeNode*)element, NULL, &exception);

	return (GeoXmlParameter*)element;
}

GeoXmlSequence *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
}

glong
geoxml_parameters_get_number(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;

	gint		i;
	gint		parameters_number = 0;

	for (i = 0; i < parameter_type_to_str_len; ++i)
		parameters_number += __geoxml_get_elements_number((GdomeElement*)parameters, parameter_type_to_str[i]);

	return parameters_number;
}

gboolean
geoxml_parameters_get_is_group(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	return !g_ascii_strcasecmp(gdome_el_nodeName((GdomeElement*)parameters, &exception)->str, "group")
		? TRUE : FALSE;
}
