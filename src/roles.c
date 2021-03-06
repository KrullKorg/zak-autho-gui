/* 
 * Copyright (C) 2011-2012 Andrea Zagli <azagli@libero.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <string.h>

#include <sql-parser/gda-sql-parser.h>

#include "roles.h"
#include "role.h"

static void autoz_gui_roles_class_init (AutozGuiRolesClass *klass);
static void autoz_gui_roles_init (AutozGuiRoles *roles);

static void autoz_gui_roles_load (AutozGuiRoles *roles);
static void autoz_gui_roles_edit (AutozGuiRoles *roles);

static void autoz_gui_roles_selected (AutozGuiRoles *roles);
static void autoz_gui_roles_on_role_updated (gpointer instance, gpointer user_data);

static void autoz_gui_roles_set_property (GObject *object,
                                     guint property_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void autoz_gui_roles_get_property (GObject *object,
                                     guint property_id,
                                     GValue *value,
                                     GParamSpec *pspec);

static void autoz_gui_roles_on_btn_new_clicked (GtkButton *button,
                      gpointer user_data);
static void autoz_gui_roles_on_btn_edit_clicked (GtkButton *button,
                      gpointer user_data);
static void autoz_gui_roles_on_btn_delete_clicked (GtkButton *button,
                        gpointer user_data);
static void autoz_gui_roles_on_trv_autoz_gui_roles_row_activated (GtkTreeView *tree_view,
                                             GtkTreePath *tree_path,
                                             GtkTreeViewColumn *column,
                                             gpointer user_data);

static void autoz_gui_roles_on_btn_cancel_clicked (GtkButton *button,
                      gpointer user_data);
static void autoz_gui_roles_on_btn_ok_clicked (GtkButton *button,
                      gpointer user_data);

#define AUTOZ_GUI_ROLES_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_AUTOZ_GUI_ROLES, AutozGuiRolesPrivate))

enum
{
	COL_ID,
	COL_NAME
};

typedef struct _AutozGuiRolesPrivate AutozGuiRolesPrivate;
struct _AutozGuiRolesPrivate
	{
		AutozGuiCommons *commons;

		GtkWidget *widget;

		GtkTreeSelection *sel_selection;
		GtkListStore *lstore_roles;

		gboolean selection;
	};

G_DEFINE_TYPE (AutozGuiRoles, autoz_gui_roles, G_TYPE_OBJECT)

static void
autoz_gui_roles_class_init (AutozGuiRolesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (AutozGuiRolesPrivate));

	object_class->set_property = autoz_gui_roles_set_property;
	object_class->get_property = autoz_gui_roles_get_property;

	/**
	 * Roles::selected:
	 * @roles:
	 *
	 */
	klass->selected_signal_id = g_signal_new ("selected",
	                                               G_TYPE_FROM_CLASS (object_class),
	                                               G_SIGNAL_RUN_LAST,
	                                               0,
	                                               NULL,
	                                               NULL,
	                                               g_cclosure_marshal_VOID__UINT,
	                                               G_TYPE_NONE,
	                                               1, G_TYPE_UINT);
}

static void
autoz_gui_roles_init (AutozGuiRoles *roles)
{
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);
}

/**
 * autoz_gui_roles_new:
 * @commons:
 * @selection:
 *
 * Returns: the newly created #AutozGuiRoles object.
 */
AutozGuiRoles
*autoz_gui_roles_new (AutozGuiCommons *commons, gboolean selection)
{
	GError *error;

	AutozGuiRoles *a = AUTOZ_GUI_ROLES (g_object_new (autoz_gui_roles_get_type (), NULL));

	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (a);

	priv->commons = commons;
	priv->selection = selection;

	error = NULL;
	gtk_builder_add_objects_from_file (priv->commons->gtkbuilder, priv->commons->guifile,
	                                   g_strsplit (g_strconcat ("lstore_roles",
	                                                            (selection ? "|w_roles" : "|vbox5"),
	                                                            NULL),
	                                               "|", -1),
	                                   &error);
	if (error != NULL)
		{
			g_warning ("Error: %s.", error->message);
			return NULL;
		}

	priv->widget = GTK_WIDGET (gtk_builder_get_object (priv->commons->gtkbuilder, (selection ? "w_roles" : "vbox5")));
	priv->sel_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (priv->commons->gtkbuilder, "treeview2")));
	priv->lstore_roles = GTK_LIST_STORE (gtk_builder_get_object (priv->commons->gtkbuilder, "lstore_roles"));

	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "button7"),
	                  "clicked", G_CALLBACK (autoz_gui_roles_on_btn_new_clicked), (gpointer)a);
	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "button9"),
	                  "clicked", G_CALLBACK (autoz_gui_roles_on_btn_edit_clicked), (gpointer)a);
	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "button8"),
	                  "clicked", G_CALLBACK (autoz_gui_roles_on_btn_delete_clicked), (gpointer)a);
	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "treeview2"),
	                  "row-activated", G_CALLBACK (autoz_gui_roles_on_trv_autoz_gui_roles_row_activated), (gpointer)a);

	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "button10"),
	                  "clicked", G_CALLBACK (autoz_gui_roles_on_btn_cancel_clicked), (gpointer)a);
	g_signal_connect (gtk_builder_get_object (priv->commons->gtkbuilder, "button13"),
	                  "clicked", G_CALLBACK (autoz_gui_roles_on_btn_ok_clicked), (gpointer)a);

	if (!selection)
		{
			gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (priv->commons->gtkbuilder, "button10")));
			gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (priv->commons->gtkbuilder, "button13")));
		}

	autoz_gui_roles_load (a);

	return a;
}

/**
 * autoz_gui_roles_get_widget:
 * @roles:
 *
 */
GtkWidget
*autoz_gui_roles_get_widget (AutozGuiRoles *roles)
{
	AutozGuiRolesPrivate *priv;

	g_return_val_if_fail (IS_AUTOZ_GUI_ROLES (roles), NULL);

	priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	return priv->widget;
}

/* PRIVATE */
static void
autoz_gui_roles_load (AutozGuiRoles *roles)
{
	GtkTreeIter iter;

	gchar *sql;

	GdaStatement *stmt;
	GError *error;
	GdaDataModel *dm;

	gint rows;
	gint row;

	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	gtk_list_store_clear (priv->lstore_roles);

	sql = g_strdup_printf ("SELECT id, role_id"
	                       " FROM %sroles"
	                       " ORDER BY role_id",
	                       priv->commons->prefix);
	stmt = gda_sql_parser_parse_string (priv->commons->gdaparser, sql, NULL, NULL);
	g_free (sql);
	dm = gda_connection_statement_execute_select (priv->commons->gdacon, stmt, NULL, &error);
	g_object_unref (stmt);
	if (dm != NULL)
		{
			rows = gda_data_model_get_n_rows (dm);
			for (row = 0; row < rows; row++)
				{
					gtk_list_store_append (priv->lstore_roles, &iter);
					gtk_list_store_set (priv->lstore_roles, &iter,
					                    COL_ID, g_value_get_int (gda_data_model_get_value_at (dm, 0, row, NULL)),
					                    COL_NAME, gda_value_stringify (gda_data_model_get_value_at (dm, 1, row, NULL)),
					                    -1);
				}

			g_object_unref (dm);
		}
	else
		{
			/* TODO */
		}
}

static void
autoz_gui_roles_edit (AutozGuiRoles *roles)
{
	GtkTreeIter iter;
	guint id;

	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	if (gtk_tree_selection_get_selected (priv->sel_selection, NULL, &iter))
		{
			GtkWidget *w;

			gtk_tree_model_get (GTK_TREE_MODEL (priv->lstore_roles), &iter,
			                    COL_ID, &id,
			                    -1);

			AutozGuiRole *c = autoz_gui_role_new (priv->commons, id);

			g_signal_connect (G_OBJECT (c), "updated",
			                  G_CALLBACK (autoz_gui_roles_on_role_updated), (gpointer)roles);

			w = autoz_gui_role_get_widget (c);
			gtk_window_set_transient_for (GTK_WINDOW (w), priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")));
			gtk_widget_show_all (w);
		}
	else
		{
			GtkWidget *dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
			                                            GTK_DIALOG_DESTROY_WITH_PARENT,
			                                            GTK_MESSAGE_WARNING,
			                                            GTK_BUTTONS_OK,
			                                            "Select a role.");
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
}

static void
autoz_gui_roles_on_role_updated (gpointer instance, gpointer user_data)
{
	autoz_gui_roles_load ((AutozGuiRoles *)user_data);
}

static void
autoz_gui_roles_selected (AutozGuiRoles *roles)
{
	GtkTreeIter iter;
	guint id;

	AutozGuiRolesClass *klass = AUTOZ_GUI_ROLES_GET_CLASS (roles);
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	if (gtk_tree_selection_get_selected (priv->sel_selection, NULL, &iter))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (priv->lstore_roles), &iter,
			                    COL_ID, &id,
			                    -1);

			g_signal_emit (G_OBJECT (roles), klass->selected_signal_id, 0, id);

			gtk_widget_destroy (priv->widget);
			g_object_unref (G_OBJECT (roles));
		}
	else
		{
			GtkWidget *dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
			                                            GTK_DIALOG_DESTROY_WITH_PARENT,
			                                            GTK_MESSAGE_WARNING,
			                                            GTK_BUTTONS_OK,
			                                            "Select a role.");
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
}

static void
autoz_gui_roles_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	AutozGuiRoles *roles = AUTOZ_GUI_ROLES (object);
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	switch (property_id)
		{
			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
				break;
		}
}

static void
autoz_gui_roles_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	AutozGuiRoles *roles = AUTOZ_GUI_ROLES (object);
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	switch (property_id)
		{
			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
				break;
		}
}

/* CALLBACK */
static void
autoz_gui_roles_on_btn_new_clicked (GtkButton *button,
                      gpointer user_data)
{
	GtkWidget *w;

	AutozGuiRoles *roles = (AutozGuiRoles *)user_data;
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	AutozGuiRole *c = autoz_gui_role_new (priv->commons, 0);

	g_signal_connect (G_OBJECT (c), "updated",
	                  G_CALLBACK (autoz_gui_roles_on_role_updated), (gpointer)roles);

	w = autoz_gui_role_get_widget (c);
	gtk_window_set_transient_for (GTK_WINDOW (w), priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")));
	gtk_widget_show_all (w);
}

static void
autoz_gui_roles_on_btn_edit_clicked (GtkButton *button,
                      gpointer user_data)
{
	autoz_gui_roles_edit ((AutozGuiRoles *)user_data);
}

static void
autoz_gui_roles_on_btn_delete_clicked (GtkButton *button,
                        gpointer user_data)
{
	GtkWidget *dialog;
	gboolean risp;

	GtkTreeIter iter;
	guint id;

	AutozGuiRoles *roles = (AutozGuiRoles *)user_data;
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE (roles);

	if (gtk_tree_selection_get_selected (priv->sel_selection, NULL, &iter))
		{
			dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
			                                 GTK_DIALOG_DESTROY_WITH_PARENT,
			                                 GTK_MESSAGE_QUESTION,
			                                 GTK_BUTTONS_YES_NO,
			                                 "Are you sure to want to delete the selected role?");
			risp = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			if (risp == GTK_RESPONSE_YES)
				{
					GError *error;
					GdaStatement *stmt;

					gtk_tree_model_get (GTK_TREE_MODEL (priv->lstore_roles), &iter,
					                    COL_ID, &id,
					                    -1);

					error = NULL;
					stmt = gda_sql_parser_parse_string (priv->commons->gdaparser,
					                                    g_strdup_printf ("DELETE FROM %sroles WHERE id = %d", priv->commons->prefix, id),
					                                    NULL, &error);

					if (stmt == NULL || error != NULL)
						{
							dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
							                                 GTK_DIALOG_DESTROY_WITH_PARENT,
							                                 GTK_MESSAGE_WARNING,
							                                 GTK_BUTTONS_OK,
							                                 "You must select a role.%s",
							                                 error != NULL && error->message != NULL ? g_strdup_printf ("\n\n%s", error->message) : "");
							gtk_dialog_run (GTK_DIALOG (dialog));
							gtk_widget_destroy (dialog);
							return;
						}

					error = NULL;
					if (gda_connection_statement_execute_non_select (priv->commons->gdacon, stmt, NULL, NULL, &error) <= 0)
						{
							dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
							                                 GTK_DIALOG_DESTROY_WITH_PARENT,
							                                 GTK_MESSAGE_WARNING,
							                                 GTK_BUTTONS_OK,
							                                 "Error on saving.\n\n%s",
							                                 (error != NULL && error->message != NULL ? error->message : "No details."));
							gtk_dialog_run (GTK_DIALOG (dialog));
							gtk_widget_destroy (dialog);
						}

					autoz_gui_roles_load (roles);
				}
		}
	else
		{
			dialog = gtk_message_dialog_new (priv->selection ? GTK_WINDOW (priv->widget) : GTK_WINDOW (gtk_builder_get_object (priv->commons->gtkbuilder, "w_main")),
			                                 GTK_DIALOG_DESTROY_WITH_PARENT,
			                                 GTK_MESSAGE_WARNING,
			                                 GTK_BUTTONS_OK,
			                                 "Select a role.");
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
}

static void
autoz_gui_roles_on_trv_autoz_gui_roles_row_activated (GtkTreeView *tree_view,
                                             GtkTreePath *tree_path,
                                             GtkTreeViewColumn *column,
                                             gpointer user_data)
{
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE ((AutozGuiRoles *)user_data);

	if (priv->selection)
		{
			autoz_gui_roles_selected ((AutozGuiRoles *)user_data);
		}
	else
		{
			autoz_gui_roles_edit ((AutozGuiRoles *)user_data);
		}
}

static void
autoz_gui_roles_on_btn_cancel_clicked (GtkButton *button,
                      gpointer user_data)
{
	AutozGuiRolesPrivate *priv = AUTOZ_GUI_ROLES_GET_PRIVATE ((AutozGuiRoles *)user_data);

	gtk_widget_destroy (priv->widget);
	g_object_unref (G_OBJECT (user_data));
}

static void
autoz_gui_roles_on_btn_ok_clicked (GtkButton *button,
                      gpointer user_data)
{
	autoz_gui_roles_selected ((AutozGuiRoles *)user_data);
}
