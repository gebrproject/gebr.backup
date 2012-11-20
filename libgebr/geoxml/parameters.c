/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include "error.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameter_p.h"
#include "parameters.h"
#include "program-parameter.h"
#include "program_parameter_p.h"
#include "sequence.h"
#include "types.h"
#include "value_sequence.h"
#include "xml.h"

/*
 * internal stuff
 */

struct gebr_geoxml_parameters {
	GdomeElement *element;
};

GebrGeoXmlParameters *__gebr_geoxml_parameters_append_new(GdomeElement * parent)
{
	GebrGeoXmlParameters *parameters;

	parameters = (GebrGeoXmlParameters *) __gebr_geoxml_insert_new_element(parent, "parameters", NULL);
	gebr_geoxml_parameters_set_default_selection(parameters, NULL);

	return parameters;
}

gboolean __gebr_geoxml_parameters_group_check(GebrGeoXmlParameters * parameters)
{
	GebrGeoXmlParameterGroup *group = gebr_geoxml_parameters_get_group(parameters);
	if (group == NULL)
		return TRUE; 

	return (gebr_geoxml_parameter_group_get_template(group) == parameters)
	       	? TRUE : FALSE;
}

void
__gebr_geoxml_parameters_do_insert_in_group_stuff(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter)
{
	glong index;
	gboolean exclusive;
	GdomeElement *parent;
	GebrGeoXmlParameterGroup *parameter_group;
	GebrGeoXmlSequence *instance;

	/* The structure is group/template-instance/parameters, that is why we need the grandparent. */
	parent = (GdomeElement *) gdome_el_parentNode((GdomeElement *) parameters, &exception);
	parent = (GdomeElement *) gdome_el_parentNode((GdomeElement *) parent, &exception);
	if (strcmp(gdome_el_tagName(parent, &exception)->str, "group"))
		return;

	index = __gebr_geoxml_get_element_index((GdomeElement *) parameter);
	parameter_group = GEBR_GEOXML_PARAMETER_GROUP(gdome_el_parentNode(parent, &exception));
	exclusive = gebr_geoxml_parameter_group_is_exclusive(parameter_group);

	gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	for (; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
		GdomeElement *reference;
		GebrGeoXmlSequence *position;

		reference = (GdomeElement *) gdome_el_cloneNode((GdomeElement *) parameter, TRUE, &exception);
		gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &position, index);
		gdome_el_insertBefore_protected((GdomeElement *) instance, (GdomeNode *) reference,
						(GdomeNode *) position, &exception);
		__gebr_geoxml_parameter_set_be_reference((GebrGeoXmlParameter *) reference);

		// If template is exclusive and this is the first parameter being inserted
		// into the instance, we must set this <parameters> tag to be exclusive too.
		if (exclusive && index == 0)
			gebr_geoxml_parameters_set_default_selection(GEBR_GEOXML_PARAMETERS(instance),
								     GEBR_GEOXML_PARAMETER(reference));
	}
}

/*
 * library functions.
 */

GebrGeoXmlParameter *gebr_geoxml_parameters_append_parameter(GebrGeoXmlParameters * parameters,
							     GebrGeoXmlParameterType type)
{
	if (parameters == NULL)
		return NULL;
	if (type == GEBR_GEOXML_PARAMETER_TYPE_REFERENCE)
		return NULL;
	if (__gebr_geoxml_parameters_group_check(parameters) == FALSE)
		return NULL;

	GdomeElement *element;

	element = __gebr_geoxml_insert_new_element((GdomeElement *) parameters, "parameter", NULL);
	__gebr_geoxml_insert_new_element(element, "label", NULL);
	__gebr_geoxml_parameter_insert_type((GebrGeoXmlParameter *) element, type);

	__gebr_geoxml_parameters_do_insert_in_group_stuff(parameters, (GebrGeoXmlParameter *) element);

	return (GebrGeoXmlParameter *) element;
}

void gebr_geoxml_parameters_set_default_selection(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (parameter == NULL) {
		__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "default-selection", "0");
		return;
	}

	gchar *value;

	value = g_strdup_printf("%ld", __gebr_geoxml_get_element_index((GdomeElement *) parameter) + 1);
	__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "default-selection", value);
	g_free(value);

	if (gebr_geoxml_parameters_get_selection(parameters) == NULL)
		gebr_geoxml_parameters_set_selection(parameters, parameter);
}

GebrGeoXmlParameter *gebr_geoxml_parameters_get_default_selection(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;

	gulong index;
	GebrGeoXmlSequence *parameter;

	index = atol(__gebr_geoxml_get_attr_value((GdomeElement *) parameters, "default-selection"));
	if (index == 0)
		return NULL;
	index--;
	gebr_geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GebrGeoXmlParameter *) gebr_geoxml_parameters_get_first_parameter(parameters);

	return (GebrGeoXmlParameter *) parameter;
}

void gebr_geoxml_parameters_set_selection(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (gebr_geoxml_parameters_get_default_selection(parameters) == NULL)
		return;
	if (parameter == NULL) {
		__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "selection", "0");
		return;
	}

	gchar *value;

	value = g_strdup_printf("%ld", __gebr_geoxml_get_element_index((GdomeElement *) parameter) + 1);
	__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "selection", value);
	g_free(value);
}

GebrGeoXmlParameter *gebr_geoxml_parameters_get_selection(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	if (gebr_geoxml_parameters_get_default_selection(parameters) == NULL)
		return NULL;

	gulong index;
	GebrGeoXmlSequence *parameter;

	index = atol(__gebr_geoxml_get_attr_value((GdomeElement *) parameters, "selection"));
	if (index == 0)
		return NULL;
	index--;
	gebr_geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GebrGeoXmlParameter *)gebr_geoxml_parameters_get_first_parameter(parameters);

	return (GebrGeoXmlParameter *)parameter;
}

GebrGeoXmlSequence *gebr_geoxml_parameters_get_first_parameter(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GebrGeoXmlSequence *) __gebr_geoxml_get_first_element((GdomeElement *) parameters, "parameter");
}

int
gebr_geoxml_parameters_get_parameter(GebrGeoXmlParameters * parameters, GebrGeoXmlSequence ** parameter, gulong index)
{
	if (parameters == NULL) {
		*parameter = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*parameter = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at((GdomeElement *) parameters,
									 "parameter", index, FALSE);

	return (*parameter == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_parameters_get_number(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number((GdomeElement *) parameters, "parameter");
}

gboolean gebr_geoxml_parameters_get_is_in_group(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;

	GdomeElement *parent;

	parent = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters, &exception);
	if (strcmp(gdome_el_tagName(parent, &exception)->str, "template-instance") == 0)
		parent = (GdomeElement*)gdome_el_parentNode(parent, &exception);

	return !strcmp(gdome_el_tagName(parent, &exception)->str, "group") ? TRUE : FALSE;
}

GebrGeoXmlParameterGroup *gebr_geoxml_parameters_get_group(GebrGeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	GdomeElement *parent;
	parent = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters, &exception);

	if (strcmp(gdome_el_tagName(parent, &exception)->str, "template-instance") == 0)
		parent = (GdomeElement*)gdome_el_parentNode(parent, &exception);

	return !strcmp(gdome_el_tagName(parent, &exception)->str, "group")
	    ? (GebrGeoXmlParameterGroup*)gdome_el_parentNode(parent, &exception)
	    : NULL;
}

gboolean gebr_geoxml_parameters_is_var_used (GebrGeoXmlParameters *self,
					     const gchar *var_name)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameter *p;

	gebr_geoxml_parameters_get_parameter (self, &seq, 0);

	while (seq) {
		p = GEBR_GEOXML_PARAMETER (seq);
		if (gebr_geoxml_parameter_get_is_program_parameter (p)) {
			if(gebr_geoxml_program_parameter_is_var_used (GEBR_GEOXML_PROGRAM_PARAMETER (seq), var_name))
				return TRUE;
		} else {
			GebrGeoXmlSequence *params;

			gebr_geoxml_parameter_group_get_instance (GEBR_GEOXML_PARAMETER_GROUP (seq),
								  &params, 0);
			while (params) {
				if (gebr_geoxml_parameters_is_var_used (GEBR_GEOXML_PARAMETERS (params), var_name))
					return TRUE;
				gebr_geoxml_sequence_next (&params);
			}
		}

		gebr_geoxml_sequence_next (&seq);
	}

	return FALSE;
}

void gebr_geoxml_parameters_reset_to_default(GebrGeoXmlParameters * parameters)
{
	GebrGeoXmlSequence *parameter;

	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
		if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter)) ==
		    GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			GebrGeoXmlSequence *instance;

			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
				gebr_geoxml_parameters_reset_to_default(GEBR_GEOXML_PARAMETERS(instance));
				gebr_geoxml_parameters_set_selection(GEBR_GEOXML_PARAMETERS(instance),
								     gebr_geoxml_parameters_get_default_selection
								     (GEBR_GEOXML_PARAMETERS(instance)));
			}

			continue;
		}

		GebrGeoXmlSequence *value;
		GebrGeoXmlSequence *default_value;

		gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, &value, 0);
		gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
							TRUE, &default_value, 0);
		for (; default_value != NULL;
		     gebr_geoxml_sequence_next(&default_value), gebr_geoxml_sequence_next(&value)) {
			if (value == NULL)
				value =
				    GEBR_GEOXML_SEQUENCE(gebr_geoxml_program_parameter_append_value
							 (GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE));
			gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value),
						       gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE
										      (default_value)));
		}

		/* remove extras values */
		while (value != NULL) {
			GebrGeoXmlSequence *tmp;

			tmp = value;
			gebr_geoxml_sequence_next(&tmp);
			gebr_geoxml_sequence_remove(value);
			value = tmp;
		}
	}
}