/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gdome.h>
#include <gdome-xpath.h>
#include <glib.h>

#include "xml.h"
#include "types.h"

/*
 * Internal internal functions
 */

static gchar *
__geoxml_remove_line_break(const gchar * tag_value)
{
	gchar *		linebreak;
	GString *	value;
	gchar *		new_tag_value;

	if ((linebreak = strchr(tag_value, 0x0A)) == NULL)
		return (gchar*)tag_value;

	value = g_string_new(tag_value);
	while ((linebreak = strchr(value->str, 0x0A)) != NULL)
		g_string_erase(value, (linebreak - value->str), 1);

	new_tag_value = value->str;
	g_string_free(value, FALSE);
	return new_tag_value;
}

/*
 * Auxiliary library functions
 */

void
__geoxml_create_CDATASection(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *	node;

	node = (GdomeNode*)gdome_doc_createCDATASection(gdome_el_ownerDocument(parent_element, &exception),
			gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

void
__geoxml_create_TextNode(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *	node;

	node = (GdomeNode*)gdome_doc_createTextNode(gdome_el_ownerDocument(parent_element, &exception),
			gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

GdomeElement *
__geoxml_new_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeElement *	element;

	element = gdome_doc_createElement(gdome_el_ownerDocument(parent_element, &exception),
		gdome_str_mkref(tag_name), &exception);

	return element;
}

GdomeElement *
__geoxml_insert_new_element(GdomeElement * parent_element, const gchar * tag_name, GdomeElement * before_element)
{
	GdomeElement *	element;

	element = __geoxml_new_element(parent_element, tag_name);
	gdome_el_insertBefore(parent_element, (GdomeNode*)element, (GdomeNode*)before_element, &exception);

	return element;
}

GdomeElement *
__geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeNode *		node;
	GdomeNodeList *		node_list;
	GdomeDOMString *	string;

	string = gdome_str_mkref(tag_name);

	/* get the list of elements with this tag_name. */
	node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
	node = gdome_nl_item(node_list, 0, &exception);

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index, gboolean recursive)
{
	if (recursive == FALSE) {
		GString *		expression;
		GdomeElement *		child;
		GdomeXPathResult *	xpath_result;

		expression = g_string_new(NULL);

		g_string_printf(expression, "child::%s[%lu]", tag_name, index);
		xpath_result = __geoxml_xpath_evaluate(parent_element, expression->str);
		child = (GdomeElement*)gdome_xpresult_singleNodeValue(xpath_result, &exception);

		g_string_free(expression, TRUE);
		gdome_xpresult_unref(xpath_result, &exception);

		return child;
	} else {
		GdomeElement *		element;
		GdomeNodeList *		node_list;
		GdomeDOMString *	string;

		string = gdome_str_mkref(tag_name);

		node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
		if (index >= gdome_nl_length(node_list, &exception))
			return NULL;
		element = (GdomeElement*)gdome_nl_item(node_list, index, &exception);

		gdome_str_unref(string);
		gdome_nl_unref(node_list, &exception);

		return element;
	}
}

GdomeElement *
__geoxml_get_element_by_id(GdomeElement * base, const gchar * id)
{
	GdomeDOMString *	string;
	GdomeElement *		element;

	string = gdome_str_mkref(id);
	element = gdome_doc_getElementById(gdome_el_ownerDocument(base, &exception), string, &exception);
	gdome_str_unref(string);

	return element;
}

GdomeXPathResult *
__geoxml_get_elements_by_idref(GdomeElement * base, const gchar * idref)
{
	GString *		expression;
	GdomeXPathResult *	xpath_result;
	GdomeElement *		document_element;

	expression = g_string_new(NULL);
	document_element = gdome_doc_documentElement(gdome_el_ownerDocument(base, &exception), &exception);

	g_string_printf(expression, "idref(('%s'))", idref);
	xpath_result = __geoxml_xpath_evaluate(document_element, expression->str);

	/* the result may contain document's element because of lastid attribute
	 * if so, ignore it */
	if ((GdomeElement*)gdome_xpresult_singleNodeValue(xpath_result, &exception) == document_element) {
		puts("idref at document element");
		gdome_xpresult_iterateNext(xpath_result, &exception);
	}

	g_string_free(expression, TRUE);

	return xpath_result;
}

glong
__geoxml_get_element_index(GdomeElement * element)
{
	glong		index;
	GdomeElement *	i;

	/* TODO: try XPath position() */

	/* root element */
	if (gdome_el_parentNode(element, &exception) == NULL)
		return 0;

	index = 0;
	i = __geoxml_get_element_at((GdomeElement*)gdome_el_parentNode(element, &exception), "*", 0, FALSE);
	do {
		if (i == element)
			return index;
		++index;
	} while ((i = __geoxml_next_element(i)) != NULL);

	return -1;
}

gulong
__geoxml_get_elements_number(GdomeElement * parent_element, const gchar * tag_name)
{
	GString *		expression;
	GdomeElement *		child;
	gulong			elements_number;

	expression = g_string_new(NULL);
	elements_number = 0;

	g_string_printf(expression, "child::%s", tag_name);
	/* TODO: use an expression instead */
	__geoxml_foreach_xpath_result(child, __geoxml_xpath_evaluate(parent_element, expression->str))
		elements_number++;

	g_string_free(expression, TRUE);

	return elements_number;
}

const gchar *
__geoxml_get_element_value(GdomeElement * element)
{
	GdomeDOMString *	string;
	GdomeNode *		child;

	child = gdome_el_firstChild(element, &exception);
	string = gdome_n_nodeValue(child, &exception);
	if (string == NULL)
		return "";
	if (gdome_n_nodeType(child, &exception) == GDOME_TEXT_NODE) {
		gchar *		protected_str;

		protected_str = __geoxml_remove_line_break(string->str);
		if (protected_str != string->str)
			gdome_n_set_nodeValue(child, gdome_str_mkref(protected_str), &exception);

		return protected_str;
	}

	return string->str;
}

void
__geoxml_set_element_value(GdomeElement * element, const gchar * tag_value,
	createValueNode_function create_func)
{
	GdomeNode *		value_node;
	const gchar *		value_str;

	/* delete all childs elements */
	while ((value_node = gdome_el_firstChild(element, &exception)) != NULL)
		gdome_el_removeChild(element, value_node, &exception);

	/* Protection agains line-breaks inside text nodes */
	value_str = (create_func == __geoxml_create_TextNode)
		? __geoxml_remove_line_break(tag_value) : tag_value;

	value_node = gdome_el_firstChild(element, &exception);
	if (value_node == NULL)
		create_func(element, value_str);
	else
		gdome_n_set_nodeValue(value_node, gdome_str_mkref_dup(value_str), &exception);
}

const gchar *
__geoxml_get_tag_value(GdomeElement * parent_element, const gchar * tag_name)
{
	return __geoxml_get_element_value(__geoxml_get_first_element(parent_element, tag_name));
}

void
__geoxml_set_tag_value(GdomeElement * parent_element, const gchar * tag_name, const gchar * tag_value,
	createValueNode_function create_func)
{
	__geoxml_set_element_value(__geoxml_get_first_element(parent_element, tag_name), tag_value, create_func);
}

void
__geoxml_set_attr_value(GdomeElement * element, const gchar * name, const gchar * value)
{
	GdomeDOMString *	string;

	string = gdome_str_mkref(name);
	gdome_el_setAttribute(element, string, gdome_str_mkref_dup(value), &exception);
	gdome_str_unref(string);
}

const gchar *
__geoxml_get_attr_value(GdomeElement * element, const gchar * name)
{
	GdomeDOMString *	string;
	GdomeDOMString *	attr_value;

	string = gdome_str_mkref(name);
	attr_value = gdome_el_getAttribute(element, string, &exception);
	gdome_str_unref(string);

	return attr_value->str;
}

void
__geoxml_remove_attr(GdomeElement * element, const gchar * name)
{
	GdomeDOMString *	string;

	string = gdome_str_mkref(name);
	gdome_el_removeAttribute(element, string, &exception);
	gdome_str_unref(string);
}

GdomeElement *
__geoxml_previous_element(GdomeElement * element)
{
	GdomeNode *	node;

	node = (GdomeNode*)element;
	do
		node = gdome_n_previousSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_next_element(GdomeElement * element)
{
	GdomeNode *	node;

	node = (GdomeNode*)element;
	do
		node = gdome_n_nextSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_previous_same_element(GdomeElement * element)
{
	GdomeElement *	previous_element;

	previous_element = __geoxml_previous_element(element);
	if (previous_element != NULL &&
	gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(previous_element, &exception)))
		return previous_element;

	return NULL;
}

GdomeElement *
__geoxml_next_same_element(GdomeElement * element)
{
	GdomeElement *	next_element;

	next_element = __geoxml_next_element(element);
	if (next_element != NULL &&
	gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(next_element, &exception)))
		return next_element;

	return NULL;
}

void
__geoxml_element_assign_new_id(GdomeElement * element)
{
	GdomeElement *	document_element;
	gulong		lastid;
	gchar *		lastid_str;
	GdomeElement *	reference_element;

	document_element = gdome_doc_documentElement(gdome_el_ownerDocument(element, &exception), &exception);
        lastid_str = (gchar*)__geoxml_get_attr_value(document_element, "lastid");
	if (strlen(lastid_str))
		sscanf(lastid_str, "n%lu", &lastid);
	else
		lastid = 0;
	lastid++;
	lastid_str = g_strdup_printf("n%lu", lastid);

	/* change referenced elements */
	__geoxml_foreach_xpath_result(reference_element,
	__geoxml_get_elements_by_idref(element, __geoxml_get_attr_value(element, "id")))
		__geoxml_set_attr_value(reference_element, "id", lastid_str);

	__geoxml_set_attr_value(document_element, "lastid", lastid_str);
	__geoxml_set_attr_value(element, "id", lastid_str);

	g_free(lastid_str);
}

void
__geoxml_element_assign_reference_id(GdomeElement * element, GdomeElement * reference)
{
	__geoxml_set_attr_value(element, "id",
		__geoxml_get_attr_value(reference, "id"));
}

GdomeXPathResult *
__geoxml_xpath_evaluate(GdomeElement * context, const gchar * expression)
{
	GdomeDOMString *	string;
	GdomeXPathEvaluator *	xpath_evaluator;
	GdomeXPathResult *	xpath_result;

	string = gdome_str_mkref(expression);
	xpath_evaluator = gdome_xpeval_mkref();
	xpath_result = gdome_xpeval_evaluate(xpath_evaluator, string, (GdomeNode*)context,
		NULL, 0, NULL, &exception);

	/* frees */
	gdome_str_unref(string);
	gdome_xpeval_unref(xpath_evaluator, &exception);

	return xpath_result;
}
