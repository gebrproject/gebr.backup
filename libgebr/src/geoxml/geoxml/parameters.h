/*   libgebr - G�BR Library
 *   Copyright (C) 2007  Br�ulio Barros de Oliveira (brauliobo@gmail.com)
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

#ifndef __LIBGEBR_GEOXML_PARAMETERS_H
#define __LIBGEBR_GEOXML_PARAMETERS_H

/**
 * \struct GeoXmlParameters parameters.h geoxml/parameters.h
 * \brief
 * Represents a list of parameters.
 * \dot
 * digraph parameters {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlSequence" -> "GeoXmlParameter";
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameters" -> "GeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> "GeoXmlProgram";
 * 	"GeoXmlParameters" -> "GeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GeoXmlProgram" -> "GeoXmlParameters";
 * }
 * \enddot
 * \see parameters.h
 */

/**
 * Get the base parameters class of \p group, which is
 * a GeoXmlParameterGroup instance.
 */
#define GEOXML_PARAMETERS(group) ((GeoXmlParameters*)(group))

/**
 * The GeoXmlParameters struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameters GeoXmlParameters;

#include <glib.h>

#include "parameter.h"
#include "sequence.h"

/**
 * Create a new parameter.
 * Use geoxml_sequence_prepend or geoxml_sequence_append to add it to the
 * list of parameters.
 */
GeoXmlParameter *
geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type);

/**
 * Create a new parameter and append it to \p parameters.
 * Provided for convenience.
 *
 * \see geoxml_parameters_new_parameter
 */
GeoXmlParameter *
geoxml_parameters_append_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type);

/**
 * Get the first paramater of \p program.
 *
 * \note Due to internal implementation, it is very slow to get the nieth paramater.
 * If you want so, you'll need to call geoxml_sequence_next multiple times
 */
GeoXmlSequence *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters);

/**
 * Get the number of parameters that \p parameters has.
 *
 * If \p parameters is NULL returns -1.
 */
glong
geoxml_parameters_get_number(GeoXmlParameters * parameters);

/**
 * Return TRUE if \p parameters is a instance of GeoXmlParameterGroup
 *
 * If \p parameters is NULL returns FALSE.
 */
gboolean
geoxml_parameters_get_is_group(GeoXmlParameters * parameters);

#endif //__LIBGEBR_GEOXML_PARAMETERS_H