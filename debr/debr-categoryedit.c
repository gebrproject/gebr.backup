/*   DeBR - GeBR Designer
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

#include <glib/gi18n.h>
#include <libgebr/validate.h>
#include <libgebr/gui/gebr-gui-enhanced-entry.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "debr-categoryedit.h"
#include "debr.h"

/*
 * Prototypes
 */

static gboolean __category_edit_check_text(const gchar *text);
static void __category_edit_validate_iter(CategoryEdit * category_edit, GtkTreeIter *iter);
static void __category_edit_add(CategoryEdit * category_edit, GebrGeoXmlSequence * category);
static void __category_edit_remove(CategoryEdit * category_edit, GtkTreeIter * iter);
static void __category_edit_move(CategoryEdit * category_edit, GtkTreeIter * iter, GtkTreeIter * position,
				 GtkTreeViewDropPosition drop_position);
static void __category_edit_move_top(CategoryEdit * category_edit, GtkTreeIter * iter);
static void __category_edit_move_bottom(CategoryEdit * category_edit, GtkTreeIter * iter);
static GtkWidget *__category_edit_create_tree_view(CategoryEdit * category_edit);
static void category_edit_add_request(CategoryEdit * category_edit, GtkWidget *combo);
static gboolean check_duplicate (GebrGuiSequenceEdit * sequence_edit, const gchar * category);

/*
 * gobject stuff
 */

/**
 * \internal
 * Properties enumeration
 */
enum {
	CATEGORY = 1
};

/**
 * \internal
 */
	static void
category_edit_set_property(CategoryEdit * category_edit, guint property_id, const GValue * value,
			   GParamSpec * param_spec)
{
	switch (property_id) {
	case CATEGORY: {
		GebrGeoXmlSequence *category;

		gtk_list_store_clear(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store);
		category_edit->category = g_value_get_pointer(value);
		category = (GebrGeoXmlSequence *) category_edit->category;
		for (; category != NULL; gebr_geoxml_sequence_next(&category))
			__category_edit_add(category_edit, category);

		break;
	} default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(category_edit, property_id, param_spec);
		break;
	}
}

/**
 * \internal
 */
	static void
category_edit_get_property(CategoryEdit * category_edit, guint property_id, GValue * value,
			   GParamSpec * param_spec)
{
	switch (property_id) {
	case CATEGORY:
		g_value_set_pointer(value, category_edit->category);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(category_edit, property_id, param_spec);
		break;
	}
}

/**
 * \internal
 */
static void category_edit_class_init(CategoryEditClass * klass)
{
	GebrGuiSequenceEditClass *category_edit_class;
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	/* virtual */
	category_edit_class = GEBR_GUI_SEQUENCE_EDIT_CLASS(klass);
	category_edit_class->remove = (typeof(category_edit_class->remove)) __category_edit_remove;
	category_edit_class->move = (typeof(category_edit_class->move)) __category_edit_move;
	category_edit_class->move_top = (typeof(category_edit_class->move_top)) __category_edit_move_top;
	category_edit_class->move_bottom =
		(typeof(category_edit_class->move_bottom)) __category_edit_move_bottom;
	category_edit_class->create_tree_view =
		(typeof(category_edit_class->create_tree_view)) __category_edit_create_tree_view;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) category_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) category_edit_get_property;

	param_spec = g_param_spec_pointer("category",
					  "", "", (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, CATEGORY, param_spec);
}

/**
 * \internal
 */
static void category_edit_init(CategoryEdit * category_edit)
{
}

G_DEFINE_TYPE(CategoryEdit, category_edit, GEBR_GUI_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void on_combo_box_entry_activate (GtkWidget *entry, CategoryEdit *self)
{
	category_edit_add_request(self, gtk_widget_get_parent (entry));
}

/**
 * \internal
 */
static void category_edit_add_request(CategoryEdit * category_edit, GtkWidget *combo)
{
	gchar *name;
	GtkWidget *entry;

	entry = gtk_bin_get_child (GTK_BIN (category_edit->categories_combo));
	gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	gtk_widget_grab_focus (entry);
	name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
	if (check_duplicate (GEBR_GUI_SEQUENCE_EDIT(category_edit), name) ||
	    __category_edit_check_text(name)) {
		g_free(name);
		return;
	}

	debr_has_category(name, TRUE);
	__category_edit_add(category_edit, GEBR_GEOXML_SEQUENCE(gebr_geoxml_flow_append_category(category_edit->menu, name)));

	validate_image_set_check_category_list(category_edit->validate_image, category_edit->menu);
	g_signal_emit_by_name(category_edit, "changed");

	g_free(name);
}

/**
 * \internal
 * \p cell and \p path_string may be NULL
 */
static void
__category_edit_on_value_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
				GebrGuiSequenceEdit * sequence_edit)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *old_text;

	GebrGeoXmlCategory *category;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter, 0, &old_text, -1);
	gebr_gui_sequence_edit_set_keypresses(sequence_edit, TRUE);

	if (!strcmp (old_text, new_text))
		return;

	if (check_duplicate (sequence_edit, new_text) ||
	    __category_edit_check_text(new_text))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter, 2, &category, -1);

	gtk_list_store_set(sequence_edit->list_store, &iter, 0, new_text, -1);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(category), new_text);
	__category_edit_validate_iter(CATEGORY_EDIT(sequence_edit), &iter);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__category_editing_started (GtkCellRenderer *renderer,
                            GtkCellEditable *editable,
                            gchar *path,
                            GebrGuiSequenceEdit * sequence_edit)
{
	gebr_gui_sequence_edit_set_keypresses(sequence_edit, FALSE);
}
/**
 * \internal
 * Check for emptyness.
 */
static gboolean __category_edit_check_text(const gchar *text)
{
	gboolean empty = FALSE;

	if (!strlen(text))
		empty = TRUE;
	else {
		gchar **categories = g_strsplit(text, "|", 0);
		for (int i = 0; categories[i] != NULL; ++i) {
			if (!strlen(categories[i])) {
				empty = TRUE;
				break;
			}
		}
		g_strfreev(categories);
	}
	const gchar *title = _("Empty category");
	if (empty)
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
					title, title,
					_("You can't add an empty category name (remember to take care with the '|' hierarchy separator)."));

	return empty;
}

/**
 * \internal
 */
static void __category_edit_validate_iter(CategoryEdit * category_edit, GtkTreeIter *iter)
{
	const gchar * value;
	GebrGeoXmlSequence *category;
	GebrValidateCase *validate_case;
	const gchar *stock;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &category, -1);

	value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category));

	gchar ** individual_categories = g_strsplit_set (value, "|", -1);
	gint i = 0;
	gchar * new_value = g_strdup("");

	for (i = 0; individual_categories[i]; i++)
	{
		gchar * cmp = g_strstrip(individual_categories[i]);
		if (!g_str_equal(cmp,""))
		{
			if (!g_str_equal(new_value, ""))
			{
				gchar * tmp = g_strdup(new_value);
				g_free(new_value);
				new_value = g_strconcat(tmp, "|", cmp, NULL);
				g_free(tmp);
			}
			else
			{
				new_value = g_strdup(cmp);
			}				
		}
		g_free(cmp);
	}


	if (g_str_equal(new_value, ""))
	{
		__category_edit_remove(category_edit, iter);
		return;
	}


	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY);
	if (gebr_validate_case_check_value(validate_case, new_value, NULL))
		stock = GTK_STOCK_DIALOG_WARNING;
	else
		stock = NULL;

	g_free(new_value);

	gtk_list_store_set(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter,
			   1, stock, -1);
}

/**
 * \internal
 */
gboolean __category_edit_on_query_tooltip(GtkTreeView * tree_view, GtkTooltip * tooltip,
					  GtkTreeIter * iter, GtkTreeViewColumn * column,
					  CategoryEdit * category_edit)
{
	gchar *stock;
	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   1, &stock, -1);
	if (stock == NULL)
		return FALSE;
	if (gtk_tree_view_column_get_sort_column_id(column) != 1)
		return FALSE;

	gchar *name;
	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   0, &name, -1);
	GebrValidateCase *validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY);
	GString *tooltip_string = g_string_new("");
	gboolean can_fix;
	gchar *fixes = gebr_validate_case_automatic_fixes_msg(validate_case, name, &can_fix);
	g_string_printf(tooltip_string, "%s\n\n%s", gebr_validate_case_get_message (validate_case), fixes);
	g_free(fixes);
	gtk_tooltip_set_markup(tooltip, tooltip_string->str);
	g_string_free(tooltip_string, TRUE);

	g_free(stock);
	g_free(name);

	return TRUE;
}

/**
 * \internal
 */
static void __category_edit_add(CategoryEdit * category_edit, GebrGeoXmlSequence * category)
{
	GtkTreeIter iter;

	gtk_list_store_append(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, &iter);
	gebr_geoxml_object_ref(category);
	gtk_list_store_set(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, &iter,
			   0, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)),
			   1, NULL, 2, category, -1);
	__category_edit_validate_iter(category_edit, &iter);
}

/**
 * \internal
 */
static void __category_edit_remove(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_remove(sequence);
	gtk_list_store_remove(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter);

	validate_image_set_check_category_list(category_edit->validate_image, category_edit->menu);
	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
	static void
__category_edit_move(CategoryEdit * category_edit, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position)
{
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlSequence *position_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), position,
			   2, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter, position);
	} else {
		gebr_geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter, position);
	}

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static void __category_edit_move_top(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_move_after(sequence, NULL);
	gtk_list_store_move_after(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static void __category_edit_move_bottom(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_move_before(sequence, NULL);
	gtk_list_store_move_before(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(category_edit, "changed");
}

static gboolean
tree_view_on_button_pressed(GtkTreeView * tree_view, GdkEventButton * event,
			    CategoryEdit * category_edit)
{
	if (event->button != 1)
		return FALSE;

	GtkTreePath *path;
	GtkTreeViewColumn *column;
	/* Get tree path for row that was clicked */
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view),
					  (gint) event->x, (gint) event->y, &path, &column, NULL, NULL)) {
		if (gtk_tree_view_column_get_sort_column_id(column) == 1) {
			GtkTreeIter iter;
			gchar *stock;
			GebrGeoXmlValueSequence *category;

			gtk_tree_model_get_iter(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store),
						&iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store), &iter,
					   1, &stock,
					   2, &category,
					   -1);
			if (stock) {
				GebrValidateCase *validate_case =
					gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY);
				gchar *fix = gebr_validate_case_fix(validate_case,
								    gebr_geoxml_value_sequence_get(category));

				gtk_list_store_set(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store, &iter,
						   0, fix,
						   1, stock,
						   -1);

				gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(category), fix);
				__category_edit_validate_iter(category_edit, &iter);

				g_signal_emit_by_name(category_edit, "changed");

				g_free(fix);
				g_free(stock);
			}
		}
		gtk_tree_path_free(path);
	}

	return FALSE;
}

/**
 * \internal
 */
static GtkWidget *__category_edit_create_tree_view(CategoryEdit * category_edit)
{

	/**
	 * \internal
	 * Handle validation warning clicks, triggering automatic fixes.
	 */
	GtkWidget *tree_view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	tree_view =
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(GEBR_GUI_SEQUENCE_EDIT(category_edit)->list_store));
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(tree_view),
						    (GebrGuiGtkTreeViewTooltipCallback)__category_edit_on_query_tooltip,
						    category_edit);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(tree_view_on_button_pressed), category_edit);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(__category_edit_on_value_edited), category_edit);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(__category_editing_started), category_edit);
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_column_set_sort_column_id(col, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_expand(col, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", 1);
	gtk_tree_view_column_set_sort_column_id(col, 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

GtkWidget *category_edit_new(GebrGeoXmlFlow * menu, gboolean new_menu)
{
	CategoryEdit *category_edit;
	GtkListStore *list_store;
	GtkWidget *hbox;
	GtkWidget *categories_combo;
	GtkWidget *validate_image;

	list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, -1);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	categories_combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(debr.categories_model), CATEGORY_NAME);
	gtk_widget_show(categories_combo);
	gtk_box_pack_start(GTK_BOX(hbox), categories_combo, TRUE, TRUE, 0);
	validate_image = validate_image_warning_new();
	gtk_box_pack_start(GTK_BOX(hbox), validate_image, FALSE, FALSE, 0);

	GebrGeoXmlSequence *category;
	gebr_geoxml_flow_get_category(menu, &category, 0);
	category_edit = g_object_new(TYPE_CATEGORY_EDIT,
				     "value-widget", hbox,
				     "list-store", list_store,
				     "category", category,
				     NULL);

	category_edit->categories_combo = categories_combo;
	category_edit->validate_image = validate_image;
	category_edit->menu = menu;
	if (!new_menu)
		validate_image_set_check_category_list(category_edit->validate_image, category_edit->menu);

	g_signal_connect (gtk_bin_get_child (GTK_BIN (categories_combo)), "activate",
			  G_CALLBACK (on_combo_box_entry_activate), category_edit);
	g_signal_connect (category_edit, "add-request",
			  G_CALLBACK (category_edit_add_request), categories_combo);

	return (GtkWidget *) category_edit;
}

static gboolean check_duplicate (GebrGuiSequenceEdit * sequence_edit, const gchar * category)
{
	gchar *i_categ;
	gboolean retval = FALSE;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GebrValidateCase *validate_case;

	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY);
	g_object_get(G_OBJECT(sequence_edit), "list-store", &model, NULL);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gchar *i_fix;
		gchar *fix;
		gchar *i_lower;
		gchar *lower;

		gtk_tree_model_get(model, &iter, 0, &i_categ, -1);
		i_fix = gebr_validate_case_fix (validate_case, i_categ);
		fix = gebr_validate_case_fix (validate_case, category);

		if (fix == NULL || i_fix == NULL)
			continue;

		i_lower = g_utf8_strdown (i_fix, -1);
		lower = g_utf8_strdown (fix, -1);

		if (strcmp (i_lower, lower) == 0) {
			gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(sequence_edit->tree_view), &iter);
			retval = TRUE;
			break;
		}

		g_free(i_fix);
		g_free(fix);
		g_free(i_lower);
		g_free(lower);
		g_free(i_categ);
	}

	if (retval) {
		const gchar *title = _("Category already exists");
		gchar *str_mrk;
		str_mrk = g_strdup_printf("The category <i>%s</i> already exists in the list, the operation will be cancelled.", i_categ);
		gebr_gui_message_dialog (GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
					 title, title,
					 str_mrk);
		g_free(i_categ);
	}

	return retval;
}
