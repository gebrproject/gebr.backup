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

#include <glib/gstdio.h>
#include <unistd.h>
#include <string.h>
#include <gdome.h>
#include <libxml/parser.h>

#include "document.h"
#include "document_p.h"
#include "xml.h"
#include "types.h"
#include "error.h"

/* global variables */
GdomeException			exception;

/*
 * internal stuff
 */

struct geoxml_document {
	GdomeDocument*	document;
};

static GdomeDOMImplementation*	dom_implementation;
static gint			dom_implementation_ref_count = 0;

static GdomeDocument *
__geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type);

static int
__geoxml_document_validate_doc(GdomeDocument * document);

typedef GdomeDocument *
(*createDoc_func)(GdomeDOMImplementation *, char *, unsigned int, GdomeException *);

static int
__geoxml_document_load(GeoXmlDocument ** document, const gchar * src, createDoc_func func)
{
	GdomeDocument *	doc;
	int		ret;

	/* create the implementation. */
	if (!dom_implementation_ref_count) {
		dom_implementation = gdome_di_mkref();
	} else
		gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;

	/* load. */
	doc = func(dom_implementation, (gchar*)src, GDOME_LOAD_PARSING, &exception);
	if (doc == NULL) {
		ret = GEOXML_RETV_INVALID_DOCUMENT;
		goto err;
	}

	/* validade */
	ret = __geoxml_document_validate_doc(doc);
	if (ret != GEOXML_RETV_SUCCESS)
		goto err;

	*document = (GeoXmlDocument*)doc;
	return GEOXML_RETV_SUCCESS;

err:	gdome_di_unref(dom_implementation, &exception);
	--dom_implementation_ref_count;
	*document = NULL;
	return ret;
}

/*
 * private functions and variables
 */

GeoXmlDocument *
geoxml_document_new(const gchar * name, const gchar * version)
{
	GdomeDocument *	document;
	GdomeElement *	root_element;

	/* create the implementation. */
	if (!dom_implementation_ref_count)
		dom_implementation = gdome_di_mkref();
	else
		gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;

	/* doc */
	document = gdome_di_createDocument(dom_implementation,
		NULL, gdome_str_mkref(name), NULL, &exception);

	/* document (root) element */
	root_element = gdome_doc_documentElement(document, &exception);
	__geoxml_set_attr_value(root_element, "version", version);

	/* elements (as specified in DTD) */
	__geoxml_insert_new_element(root_element, "filename", NULL);
	__geoxml_insert_new_element(root_element, "title", NULL);
	__geoxml_insert_new_element(root_element, "description", NULL);
	__geoxml_insert_new_element(root_element, "help", NULL);
	__geoxml_insert_new_element(root_element, "author", NULL);
	__geoxml_insert_new_element(root_element, "email", NULL);

	return (GeoXmlDocument*)document;
}

const gchar *
geoxml_document_get_version_doc(GdomeDocument * document)
{
	return __geoxml_get_attr_value(gdome_doc_documentElement(document, &exception), "version");
}

/*
 * library functions.
 */

int
geoxml_document_load(GeoXmlDocument ** document, const gchar * path)
{
	if (g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(path, R_OK) < 0)
		return GEOXML_RETV_CANT_ACCESS_FILE;

	return __geoxml_document_load(document, path, (createDoc_func)gdome_di_createDocFromURI);
}

int
geoxml_document_load_buffer(GeoXmlDocument ** document, const gchar * xml)
{
	return __geoxml_document_load(document, xml, (createDoc_func)gdome_di_createDocFromMemory);
}

void
geoxml_document_free(GeoXmlDocument * document)
{
	if (document == NULL)
		return;

	gdome_doc_unref((GdomeDocument*)document, &exception);
	gdome_di_unref(dom_implementation, &exception);
	--dom_implementation_ref_count;
}

GeoXmlDocument *
geoxml_document_clone(GeoXmlDocument * source)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *	document;

	document = g_malloc(sizeof(struct geoxml_document));
	if (document == NULL)
		return NULL;

	document = __geoxml_document_clone_doc((GdomeDocument*)source, NULL);

	/* increase reference count */
	gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;

	return (GeoXmlDocument*)document;
}

GdomeDocument *
__geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *	document;
	GdomeElement *	root_element;
	GdomeElement *	source_root_element;
	GdomeNode *	node;

	source_root_element = gdome_doc_documentElement(source, &exception);
	document = gdome_di_createDocument(dom_implementation,
		NULL, gdome_el_nodeName(source_root_element, &exception), document_type, &exception);
	if (document == NULL)
		return NULL;

	/* copy version attribute */
	root_element = gdome_doc_documentElement(document, &exception);
	__geoxml_set_attr_value(root_element, "version",
		__geoxml_get_attr_value(source_root_element, "version"));

	node = gdome_el_firstChild(source_root_element, &exception);
	do {
		GdomeNode* new_node = gdome_doc_importNode(document, node, TRUE, &exception);
		gdome_el_appendChild(root_element, new_node, &exception);

		node = gdome_n_nextSibling(node, &exception);
	} while (node != NULL);

	return document;
}

enum GEOXML_DOCUMENT_TYPE
geoxml_document_get_type(GeoXmlDocument * document)
{
	if (document == NULL)
		return GEOXML_DOCUMENT_TYPE_FLOW;

	GdomeElement *	root_element;

	root_element = gdome_doc_documentElement((GdomeDocument*)document, &exception);

	if (g_ascii_strcasecmp("flow", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEOXML_DOCUMENT_TYPE_FLOW;
	else if (g_ascii_strcasecmp("line", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEOXML_DOCUMENT_TYPE_LINE;
	else if (g_ascii_strcasecmp("project", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEOXML_DOCUMENT_TYPE_PROJECT;

	return GEOXML_DOCUMENT_TYPE_FLOW;
}

const gchar *
geoxml_document_get_version(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return geoxml_document_get_version_doc((GdomeDocument*)document);
}

int
geoxml_document_validate(const gchar * filename)
{
	if (g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(filename, R_OK) < 0)
		return GEOXML_RETV_CANT_ACCESS_FILE;

	GeoXmlDocument *	document;
	int			ret;

	ret = geoxml_document_load(&document, filename);
	geoxml_document_free(document);

	return ret;
}

int
__geoxml_document_validate_doc(GdomeDocument * document)
{
	/* Based on code by Daniel Veillard
	 * References:
	 *   http://xmlsoft.org/examples/index.html#parse2.c
	 *   http://xmlsoft.org/examples/parse2.c
	 */

	xmlParserCtxtPtr	ctxt;
	xmlDocPtr		doc;
	gchar *			xml;

	GString *		dtd_filename;

	GdomeDocument *		tmp_doc;
	GdomeDocumentType *	tmp_document_type;

	gchar *			version;
	int			ret;

	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL)
		return GEOXML_RETV_NO_MEMORY;
	if (gdome_doc_doctype(document, &exception) != NULL) {
		ret = GEOXML_RETV_DTD_SPECIFIED;
		goto out2;
	}

	/* initialization */
	dtd_filename = g_string_new(NULL);

	/* find DTD */
	g_string_printf(dtd_filename, "%s/%s-%s.dtd", GEOXML_DTDDIR,
		gdome_el_nodeName(geoxml_document_root_element(document), &exception)->str,
		geoxml_document_get_version((GeoXmlDocument*)document));
	if (g_file_test(dtd_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(dtd_filename->str, R_OK) < 0) {
		ret = GEOXML_RETV_CANT_ACCESS_DTD;
		goto out;
	}

	/* include DTD with full path */
	tmp_document_type = gdome_di_createDocumentType(dom_implementation,
		gdome_el_nodeName(geoxml_document_root_element(document), &exception), NULL,
		gdome_str_mkref_own(dtd_filename->str), &exception);
	tmp_doc = __geoxml_document_clone_doc(document, tmp_document_type);
	if (tmp_document_type == NULL || tmp_doc == NULL) {
		ret = GEOXML_RETV_NO_MEMORY;
		goto out;
	}

	gdome_di_saveDocToMemoryEnc(dom_implementation, tmp_doc,
		&xml, ENCODING, GDOME_SAVE_STANDARD, &exception);
	doc = xmlCtxtReadMemory(ctxt, xml, strlen(xml), (char *)g_get_current_dir(), NULL,
		XML_PARSE_NOBLANKS |
		XML_PARSE_DTDATTR |  /* default DTD attributes */
		XML_PARSE_NOENT |    /* substitute entities */
		XML_PARSE_DTDVALID);

	/* frees memory before we quit. */
	gdome_doc_unref(tmp_doc, &exception);
	g_free(xml);

	if (doc == NULL) {
		xmlFreeParserCtxt(ctxt);
		ret = GEOXML_RETV_INVALID_DOCUMENT;
		goto out;
	} else {
		xmlFreeDoc(doc);
		if (ctxt->valid == 0) {
			xmlFreeParserCtxt(ctxt);
			ret = GEOXML_RETV_INVALID_DOCUMENT;
			goto out;
		}
	}

	/* success, now change to last version */
	version = (gchar*)geoxml_document_get_version_doc(document);
	/* nothing to do for version 0.1.0 */
	__geoxml_set_attr_value(geoxml_document_root_element(document), "version", "0.1.1");
	/* TODO: port to 0.2.0 from 0.1.x */

	ret = GEOXML_RETV_SUCCESS;
out:	g_string_free(dtd_filename, TRUE);
out2:	xmlFreeParserCtxt(ctxt);

	return ret;
}

int
geoxml_document_save(GeoXmlDocument * document, const gchar * path)
{
	if (document == NULL)
		return FALSE;
	if (g_access(path, F_OK) < 0) {
		FILE *	fp;

		fp = fopen(path, "w");
		if (fp == NULL)
			return GEOXML_RETV_CANT_ACCESS_FILE;
		fclose(fp);
	}

	return gdome_di_saveDocToFileEnc(dom_implementation, (GdomeDocument*)document,
		path, ENCODING, GDOME_SAVE_LIBXML_INDENT, &exception)
			? GEOXML_RETV_SUCCESS
			: GEOXML_RETV_CANT_ACCESS_FILE;
}

int
geoxml_document_to_string(GeoXmlDocument * document, gchar ** xml_string)
{
	if (document == NULL)
		return FALSE;

	return gdome_di_saveDocToMemoryEnc(dom_implementation, (GdomeDocument*)document,
		xml_string, ENCODING, GDOME_SAVE_LIBXML_INDENT, &exception)
			? GEOXML_RETV_SUCCESS
			: GEOXML_RETV_NO_MEMORY;
}

void
geoxml_document_set_filename(GeoXmlDocument * document, const gchar * filename)
{
	if (document == NULL || filename == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"filename", filename, __geoxml_create_TextNode);
}

void
geoxml_document_set_title(GeoXmlDocument * document, const gchar * title)
{
	if (document == NULL || title == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"title", title, __geoxml_create_TextNode);
}

void
geoxml_document_set_author(GeoXmlDocument * document, const gchar * author)
{
	if (document == NULL || author == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"author", author, __geoxml_create_TextNode);
}

void
geoxml_document_set_email(GeoXmlDocument * document, const gchar * email)
{
	if (document == NULL || email == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"email", email, __geoxml_create_TextNode);
}

void
geoxml_document_set_date_created(GeoXmlDocument * document, const gchar * created)
{
	if (document == NULL || created == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(document), "date"),
		"created", created, __geoxml_create_TextNode);
}

void
geoxml_document_set_date_modified(GeoXmlDocument * document, const gchar * modified)
{
	if (document == NULL || modified == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(document), "date"),
		"modified", modified, __geoxml_create_TextNode);
}

void
geoxml_document_set_description(GeoXmlDocument * document, const gchar * description)
{
	if (document == NULL || description == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"description", description, __geoxml_create_TextNode);
}

void
geoxml_document_set_help(GeoXmlDocument * document, const gchar * help)
{
	if (document == NULL || help == NULL)
		return;
	__geoxml_set_tag_value(geoxml_document_root_element(document),
		"help", help, __geoxml_create_CDATASection);
}

const gchar *
geoxml_document_get_filename(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "filename");
}

const gchar *
geoxml_document_get_title(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "title");
}

const gchar *
geoxml_document_get_author(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "author");
}

const gchar *
geoxml_document_get_email(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "email");
}

const gchar *
geoxml_document_get_date_created(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(document), "date"), "created");
}

const gchar *
geoxml_document_get_date_modified(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(document), "date"), "modified");
}

const gchar *
geoxml_document_get_description(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "description");
}

const gchar *
geoxml_document_get_help(GeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __geoxml_get_tag_value(geoxml_document_root_element(document), "help");
}