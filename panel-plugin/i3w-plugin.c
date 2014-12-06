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

static void i3_add_workspaces(i3WorkspacesPlugin *i3_workspaces);
static void i3_remove_workspaces(i3WorkspacesPlugin *i3_workspaces);


void i3_workspaces_save(XfcePanelPlugin *xfcePlugin, i3WorkspacesPlugin *i3workspacesPlugin)
{
}

static void i3_workspaces_read (i3WorkspacesPlugin *i3_workspaces)
{
}

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
    i3wm_set_workspace_focused_callback(i3_workspaces->i3wm, i3_on_workspace_focused, i3_workspaces);
    i3wm_set_workspace_blurred_callback(i3_workspaces->i3wm, i3_on_workspace_blurred, i3_workspaces);

    i3_add_workspaces(i3_workspaces);

    return i3_workspaces;
}

static void i3_workspaces_free (XfcePanelPlugin *plugin,
                                i3WorkspacesPlugin *i3_workspaces)
{
    /* destroy the panel widgets */
    gtk_widget_destroy (i3_workspaces->hvbox);

    /* free the plugin structure */
    panel_slice_free (i3WorkspacesPlugin, i3_workspaces);
}

static void i3_workspaces_orientation_changed (XfcePanelPlugin       *plugin,
                                               GtkOrientation        orientation,
                                               i3WorkspacesPlugin    *i3_workspaces)
{
    /* change the orienation of the box */
    xfce_hvbox_set_orientation (XFCE_HVBOX (i3_workspaces->hvbox), orientation);
}

    static gboolean
i3_workspaces_size_changed (XfcePanelPlugin *plugin,
        gint             size,
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

    g_signal_connect (G_OBJECT (plugin), "save",
            G_CALLBACK (i3_workspaces_save), i3_workspaces);

    g_signal_connect (G_OBJECT (plugin), "size-changed",
            G_CALLBACK (i3_workspaces_size_changed), i3_workspaces);

    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
            G_CALLBACK (i3_workspaces_orientation_changed), i3_workspaces);
}

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

            /* if focused, bold the text on the button; otherwise use regular boldness */
            if (workspace->focused)
            {
                gchar * label_str;
                GList * list;

                label_str = (gchar *) calloc(strlen(workspace->name) + 8, sizeof(gchar));
                strcpy(label_str, "<b>");
                strcat(label_str, workspace->name);
                strcat(label_str, "</b>");

                list = gtk_container_get_children(GTK_CONTAINER(button));
                gtk_label_set_markup(GTK_LABEL(list->data), label_str);

                free(label_str);
            }

            gtk_button_set_use_underline(GTK_BUTTON(button), FALSE); /* avoid acceleration key interference */
            gtk_box_pack_start(GTK_BOX(i3_workspaces->hvbox), button, FALSE, FALSE, 0);
            gtk_widget_show(button);
            i3_workspaces->buttons[workspace->num] = button;
        }
    }
}

static void i3_on_workspace_created (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    i3_remove_workspaces(i3_workspaces);
    i3_add_workspaces(i3_workspaces);
}

static void i3_on_workspace_destroyed (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    gtk_widget_destroy(i3_workspaces->buttons[workspace->num]);
    i3_workspaces->buttons[workspace->num] = NULL;
}

static void i3_on_workspace_blurred (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;
}

static void i3_on_workspace_focused (i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    i3_remove_workspaces(i3_workspaces);
    i3_add_workspaces(i3_workspaces);
}

