/*  $Id$
 *
 *  Copyright (C) 2014 Dénes Botond <dns.botond@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "i3w-plugin.h"

/* prototypes */
static void i3_workspaces_construct(XfcePanelPlugin *plugin);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(i3_workspaces_construct);

static void i3_on_workspace_created (i3workspace *workspace, gpointer data);
static void i3_on_workspace_destroyed (i3workspace *workspace, gpointer data);
static void i3_on_workspace_blurred (i3workspace *workspace, gpointer data);
static void i3_on_workspace_focused (i3workspace *workspace, gpointer data);
static void i3_on_workspace_urgent (i3workspace *workspace, gpointer data);

static void i3_add_workspaces(i3WorkspacesPlugin *i3_workspaces);
static void i3_remove_workspaces(i3WorkspacesPlugin *i3_workspaces);

static void i3_set_button_label(GtkWidget *button, gchar *name, gboolean focused, gboolean urgent);

static void i3_on_workspace_clicked (GtkWidget *button, gpointer data);

/* Implementations */

/**
 * i3_workspaces_new:
 * @plugin: the xfce plugin object
 *
 * Create the plugin.
 *
 * Returns: the plugin
 */
static i3WorkspacesPlugin * i3_workspaces_new (XfcePanelPlugin *plugin)
{
    i3WorkspacesPlugin   *i3_workspaces;
    GtkOrientation  orientation;
    GtkWidget      *label;

    /* allocate memory for the plugin structure */
    i3_workspaces = panel_slice_new0 (i3WorkspacesPlugin);

    /* pointer to plugin */
    i3_workspaces->plugin = plugin;

    /* get the current orientation */
    orientation = xfce_panel_plugin_get_orientation (plugin);

    /* create some panel widgets */
    i3_workspaces->ebox = gtk_event_box_new ();
    gtk_widget_show (i3_workspaces->ebox);

    i3_workspaces->hvbox = xfce_hvbox_new (orientation, FALSE, 2);
    gtk_widget_show (i3_workspaces->hvbox);
    gtk_container_add (GTK_CONTAINER (i3_workspaces->ebox), i3_workspaces->hvbox);

    i3_workspaces->i3wm = i3wm_construct();

    i3_workspaces->buttons = (GtkWidget **) g_malloc0(sizeof(GtkWidget *) * I3_WORKSPACE_N);

    i3wm_set_workspace_created_callback(i3_workspaces->i3wm, i3_on_workspace_created, i3_workspaces);
    i3wm_set_workspace_destroyed_callback(i3_workspaces->i3wm, i3_on_workspace_destroyed, i3_workspaces);
    i3wm_set_workspace_blurred_callback(i3_workspaces->i3wm, i3_on_workspace_blurred, i3_workspaces);
    i3wm_set_workspace_focused_callback(i3_workspaces->i3wm, i3_on_workspace_focused, i3_workspaces);
    i3wm_set_workspace_urgent_callback(i3_workspaces->i3wm, i3_on_workspace_urgent, i3_workspaces);

    i3_add_workspaces(i3_workspaces);

    return i3_workspaces;
}

/**
 * i3_workspaces_free:
 * @plugin: the xfce plugin
 * @i3_workspaces: the i3 workspaces plugin
 *
 * Free all related resources.
 */
static void i3_workspaces_free (XfcePanelPlugin *plugin,
                                i3WorkspacesPlugin *i3_workspaces)
{
    /* destroy the panel widgets */
    gtk_widget_destroy (i3_workspaces->hvbox);

    /* free the plugin structure */
    panel_slice_free (i3WorkspacesPlugin, i3_workspaces);
}

/**
 * i3_workspaces_orientation_changed:
 * @plugin: the xfce plugin
 * @orientation: the orientation
 * @i3_workspaces: the workspaces plugin
 *
 * Handle the plugin orientation change.
 */
static void i3_workspaces_orientation_changed (XfcePanelPlugin       *plugin,
                                               GtkOrientation        orientation,
                                               i3WorkspacesPlugin    *i3_workspaces)
{
    /* change the orienation of the box */
    xfce_hvbox_set_orientation (XFCE_HVBOX (i3_workspaces->hvbox), orientation);
}

/**
 * i3_workspaces_size_changed:
 * @plugin: the xfce plugin
 * @size: the size
 * @i3_workspaces: the workspaces plugin
 *
 * Handle the plugin size change.
 *
 * Returns: gboolean
 */
static gboolean i3_workspaces_size_changed (XfcePanelPlugin       *plugin,
                                            gint                  size,
                                            i3WorkspacesPlugin    *i3_workspaces)
{
    GtkOrientation orientation;

    /* get the orientation of the plugin */
    orientation = xfce_panel_plugin_get_orientation (plugin);

    /* set the widget size */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

    /* we handled the orientation */
    return TRUE;
}

/**
 * i3_workspaces_construct:
 * @plugin: the xfce plugin
 *
 * Constructs the xfce plugin.
 */
static void i3_workspaces_construct (XfcePanelPlugin *plugin)
{
    i3WorkspacesPlugin *i3_workspaces;

    /* create the plugin */
    i3_workspaces = i3_workspaces_new (plugin);

    /* add the ebox to the panel */
    gtk_container_add (GTK_CONTAINER (plugin), i3_workspaces->ebox);

    /* show the panel's right-click menu on this ebox */
    xfce_panel_plugin_add_action_widget (plugin, i3_workspaces->ebox);

    /* connect plugin signals */
    g_signal_connect (G_OBJECT (plugin), "free-data",
            G_CALLBACK (i3_workspaces_free), i3_workspaces);

    g_signal_connect (G_OBJECT (plugin), "size-changed",
            G_CALLBACK (i3_workspaces_size_changed), i3_workspaces);

    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
            G_CALLBACK (i3_workspaces_orientation_changed), i3_workspaces);
}

/**
 * i3_remove_workspaces:
 * @i3_workspaces: the workspaces plugin
 *
 * Remove all workspaces.
 */
static void i3_remove_workspaces(i3WorkspacesPlugin *i3_workspaces)
{
    gint i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        GtkWidget * button = i3_workspaces->buttons[i];

        if (button)
        {
            gtk_widget_destroy(button);
            i3_workspaces->buttons[i] = NULL;
        }
    }
}

/**
 * i3_add_workspaces:
 * @i3_workspaces: the workspaces plugin
 *
 * Add the workspaces.
 */
static void i3_add_workspaces(i3WorkspacesPlugin *i3_workspaces)
{
    i3workspace **workspaces = i3wm_get_workspaces(i3_workspaces->i3wm);
    gint i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        i3workspace *workspace = workspaces[i];
        if (workspace)
        {
            GtkWidget * button;
            button = gtk_button_new_with_label(workspace->name);

            i3_set_button_label(button, workspace->name, workspace->focused, workspace->urgent);

            g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(i3_on_workspace_clicked), i3_workspaces);

            gtk_button_set_use_underline(GTK_BUTTON(button), FALSE); /* avoid acceleration key interference */
            gtk_box_pack_start(GTK_BOX(i3_workspaces->hvbox), button, FALSE, FALSE, 0);
            gtk_widget_show(button);
            i3_workspaces->buttons[workspace->num] = button;
        }
    }
}

/**
 * i3_on_workspace_created:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace created event handler.
 */
static void i3_on_workspace_created (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    i3_remove_workspaces(i3_workspaces);
    i3_add_workspaces(i3_workspaces);
}

/**
 * i3_on_workspace_destroyed:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace destroyed event handler.
 */
static void i3_on_workspace_destroyed (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    gtk_widget_destroy(i3_workspaces->buttons[workspace->num]);
    i3_workspaces->buttons[workspace->num] = NULL;
}

/**
 * i3_on_workspace_blurred:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace blurred event handler.
 */
static void i3_on_workspace_blurred (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    i3_set_button_label(button, workspace->name, workspace->focused, workspace->urgent);
}

/**
 * i3_on_workspace_focused:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace focused event handler.
 */
static void i3_on_workspace_focused (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    workspace->urgent = FALSE; // reset the urgent flag
    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    i3_set_button_label(button, workspace->name, workspace->focused, workspace->urgent);
}

/**
 * i3_on_workspace_urgent:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace urgent event handler.
 */
static void i3_on_workspace_urgent (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    i3_set_button_label(button, workspace->name, workspace->focused, workspace->urgent);
}

/**
 * i3_set_button_label:
 * @button: the button
 * @name: the workspace name
 * @focused: is the workspace focused?
 * @urgent: is the workspace urgent?
 *
 * Generate the label for the workspace button.
 */
static void i3_set_button_label(GtkWidget *button, gchar *name, gboolean focused, gboolean urgent)
{
    static gchar * template = "<span foreground=\"%s\" weight=\"%s\">%s</span>";
    static gchar * focused_weight = "bold";
    static gchar * blurred_weight = "normal";
    static gchar * urgent_color = "red";
    static gchar * normal_color = "black";

    // allocate space for the maximum possible size of the label
    gulong maxlen = strlen(name) + 50;
    gchar *label_str = (gchar *) calloc(maxlen, sizeof(gchar));

    g_snprintf(label_str, maxlen, template,
            urgent ? urgent_color : normal_color,
            focused ? focused_weight : blurred_weight,
            name);

    GtkWidget * label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_markup(GTK_LABEL(label), label_str);

    free(label_str);
}

/**
 * i3_on_workspace_clicked:
 * @button: the clicked button
 * @data: the workspace plugin
 *
 * Workspace button click event handler.
 */
static void i3_on_workspace_clicked (GtkWidget *button, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *)data;

    gint button_index = 0;
    gint i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        if (i3_workspaces->buttons[i] == button)
        {
            button_index = i;
            break;
        }
    }
    i3wm_goto_workspace(i3_workspaces->i3wm, button_index);
}
