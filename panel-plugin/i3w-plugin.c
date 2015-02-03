/*  $Id$
 *
 *  Copyright (C) 2014 Dénes Botond <dns.botond@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4panel/xfce-hvbox.h>
#include <glib/gprintf.h>

#include "i3w-plugin.h"

/* prototypes */

static void
connect_callbacks(i3WorkspacesPlugin *i3_workspaces);

static i3WorkspacesPlugin *
construct_workspaces(XfcePanelPlugin *plugin);

static void
construct(XfcePanelPlugin *plugin);

static void
destruct(XfcePanelPlugin *plugin, i3WorkspacesPlugin *i3_workspaces);

static gboolean
size_changed(XfcePanelPlugin *plugin, gint size, i3WorkspacesPlugin *i3_workspaces);

static void
orientation_changed(XfcePanelPlugin *plugin,
        GtkOrientation orientation, i3WorkspacesPlugin *i3_workspaces);

static void
configure_plugin(XfcePanelPlugin *plugin, i3WorkspacesPlugin *i3_workspaces);

static void
config_changed(gpointer cb_data);

static void
add_workspaces(i3WorkspacesPlugin *i3_workspaces);
static void
remove_workspaces(i3WorkspacesPlugin *i3_workspaces);

static void
set_button_label(GtkWidget *button, gchar *name, gboolean focused,
        gboolean urgent, i3WorkspacesConfig *config);

static void
on_workspace_clicked(GtkWidget *button, gpointer data);

static void
on_workspace_created(i3workspace *workspace, gpointer data);
static void
on_workspace_destroyed(i3workspace *workspace, gpointer data);
static void
on_workspace_blurred(i3workspace *workspace, gpointer data);
static void
on_workspace_focused(i3workspace *workspace, gpointer data);
static void
on_workspace_urgent(i3workspace *workspace, gpointer data);

static void
on_ipc_shutdown(gpointer i3_w);

static void
recover_from_disconnect(i3WorkspacesPlugin *i3_workspaces);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(construct);

/* Implementations */

/**
 * connect_callbacks:
 * @i3_workspaces: the i3 workspaces plugin
 *
 * Connects all callbacks to the i3wm delegate
 */
static void
connect_callbacks(i3WorkspacesPlugin *i3_workspaces)
{
    i3wm_set_workspace_created_callback(i3_workspaces->i3wm,
            on_workspace_created, i3_workspaces);
    i3wm_set_workspace_destroyed_callback(i3_workspaces->i3wm,
            on_workspace_destroyed, i3_workspaces);
    i3wm_set_workspace_blurred_callback(i3_workspaces->i3wm,
            on_workspace_blurred, i3_workspaces);
    i3wm_set_workspace_focused_callback(i3_workspaces->i3wm,
            on_workspace_focused, i3_workspaces);
    i3wm_set_workspace_urgent_callback(i3_workspaces->i3wm,
            on_workspace_urgent, i3_workspaces);
    i3wm_set_ipch_shutdown_callback(i3_workspaces->i3wm,
            on_ipc_shutdown, i3_workspaces);
}

/**
 * construct_workspaces:
 * @plugin: the xfce plugin object
 *
 * Create the plugin.
 *
 * Returns: the plugin
 */
static i3WorkspacesPlugin *
construct_workspaces(XfcePanelPlugin *plugin)
{
    i3WorkspacesPlugin *i3_workspaces;
    GtkOrientation orientation;
    GtkWidget *label;

    /* allocate memory for the plugin structure */
    i3_workspaces = panel_slice_new0(i3WorkspacesPlugin);

    /* pointer to plugin */
    i3_workspaces->plugin = plugin;

    /* plugin configuration */
    i3_workspaces->config = i3_workspaces_config_new();
    i3_workspaces_config_load(i3_workspaces->config, plugin);

    /* get the current orientation */
    orientation = xfce_panel_plugin_get_orientation (plugin);

    /* create some panel widgets */
    i3_workspaces->ebox = gtk_event_box_new();
    gtk_widget_show(i3_workspaces->ebox);

    i3_workspaces->hvbox = xfce_hvbox_new(orientation, FALSE, 2);
    gtk_widget_show(i3_workspaces->hvbox);
    gtk_container_add(GTK_CONTAINER(i3_workspaces->ebox), i3_workspaces->hvbox);

    i3_workspaces->buttons = (GtkWidget **) g_malloc0(sizeof(GtkWidget *) * I3_WORKSPACE_N);

    GError *err = NULL;
    i3_workspaces->i3wm = i3wm_construct(&err);
    if (NULL != err)
    {
        //TODO: error_state_on();
        recover_from_disconnect(i3_workspaces);
        //TODO: error_state_off();
    }

    connect_callbacks(i3_workspaces);

    add_workspaces(i3_workspaces);

    return i3_workspaces;
}

/**
 * construct:
 * @plugin: the xfce plugin
 *
 * Constructs the xfce plugin.
 */
static void
construct(XfcePanelPlugin *plugin)
{
    i3WorkspacesPlugin *i3_workspaces = construct_workspaces(plugin);

    /* add the ebox to the panel */
    gtk_container_add (GTK_CONTAINER(plugin), i3_workspaces->ebox);

    /* show the panel's right-click menu on this ebox */
    xfce_panel_plugin_add_action_widget(plugin, i3_workspaces->ebox);

    /* connect plugin signals */
    g_signal_connect(G_OBJECT(plugin), "free-data",
            G_CALLBACK(destruct), i3_workspaces);

    g_signal_connect(G_OBJECT(plugin), "size-changed",
            G_CALLBACK(size_changed), i3_workspaces);

    g_signal_connect(G_OBJECT(plugin), "orientation-changed",
            G_CALLBACK (orientation_changed), i3_workspaces);

    g_signal_connect(G_OBJECT(plugin), "configure-plugin",
            G_CALLBACK (configure_plugin), i3_workspaces);

    /* show the configure menu item */
    xfce_panel_plugin_menu_show_configure(plugin);
}

/**
 * destruct:
 * @plugin: the xfce plugin
 * @i3_workspaces: the i3 workspaces plugin
 *
 * Free all related resources.
 */
static void
destruct(XfcePanelPlugin *plugin, i3WorkspacesPlugin *i3_workspaces)
{
    /* save configuration */
    i3_workspaces_config_save(i3_workspaces->config, plugin);
    i3_workspaces_config_free(i3_workspaces->config);

    /* destroy the panel widgets */
    gtk_widget_destroy(i3_workspaces->hvbox);

    /* destroy the i3wm delegate */
    i3wm_destruct(i3_workspaces->i3wm);

    /* free the plugin structure */
    panel_slice_free(i3WorkspacesPlugin, i3_workspaces);
}

/**
 * size_changed:
 * @plugin: the xfce plugin
 * @size: the size
 * @i3_workspaces: the workspaces plugin
 *
 * Handle the plugin size change.
 *
 * Returns: gboolean
 */
static gboolean
size_changed(XfcePanelPlugin *plugin, gint size, i3WorkspacesPlugin *i3_workspaces)
{
    GtkOrientation orientation;

    /* get the orientation of the plugin */
    orientation = xfce_panel_plugin_get_orientation(plugin);

    /* set the widget size */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
    else
        gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);

    /* we handled the orientation */
    return TRUE;
}

/**
 * orientation_changed:
 * @plugin: the xfce plugin
 * @orientation: the orientation
 * @i3_workspaces: the workspaces plugin
 *
 * Handle the plugin orientation change.
 */
static void
orientation_changed(XfcePanelPlugin *plugin,
        GtkOrientation orientation, i3WorkspacesPlugin *i3_workspaces)
{
    /* change the orienation of the box */
    xfce_hvbox_set_orientation(XFCE_HVBOX(i3_workspaces->hvbox), orientation);
}


/**
 * configure_plugin
 * @orientation: the orientation
 * @i3_workspaces: the workspaces plugin
 *
 * Handle the plugin configure menu item click event.
 */
static void
configure_plugin(XfcePanelPlugin *plugin, i3WorkspacesPlugin *i3_workspaces)
{
    i3_workspaces_config_show(i3_workspaces->config, plugin,
            config_changed, (gpointer)i3_workspaces);
}

/**
 * config_changed:
 * @gpointer cb_data: the callback data
 *
 * Callback funtion which is called when the configuration is updated
 */
static void
config_changed(gpointer cb_data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) cb_data;
    remove_workspaces(i3_workspaces);
    add_workspaces(i3_workspaces);
}

/**
 * add_workspaces:
 * @i3_workspaces: the workspaces plugin
 *
 * Add the workspaces.
 */
static void
add_workspaces(i3WorkspacesPlugin *i3_workspaces)
{
    i3workspace **workspaces = i3wm_get_workspaces(i3_workspaces->i3wm);
    gint i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        i3workspace *workspace = workspaces[i];
        if (workspace)
        {
            GtkWidget * button;
            button = xfce_panel_create_button();
            gtk_button_set_label(GTK_BUTTON(button), workspace->name);

            set_button_label(button, workspace->name, workspace->focused,
                    workspace->urgent, i3_workspaces->config);

            g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(on_workspace_clicked), i3_workspaces);

            /* avoid acceleration key interference */
            gtk_button_set_use_underline(GTK_BUTTON(button), FALSE);
            gtk_box_pack_start(GTK_BOX(i3_workspaces->hvbox), button, FALSE, FALSE, 0);
            gtk_widget_show(button);
            i3_workspaces->buttons[workspace->num] = button;
        }
    }
}

/**
 * remove_workspaces:
 * @i3_workspaces: the workspaces plugin
 *
 * Remove all workspaces.
 */
static void
remove_workspaces(i3WorkspacesPlugin *i3_workspaces)
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
 * on_workspace_created:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace created event handler.
 */
static void
on_workspace_created(i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    remove_workspaces(i3_workspaces);
    add_workspaces(i3_workspaces);
}

/**
 * on_workspace_destroyed:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace destroyed event handler.
 */
static void
on_workspace_destroyed(i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    gtk_widget_destroy(i3_workspaces->buttons[workspace->num]);
    i3_workspaces->buttons[workspace->num] = NULL;
}

/**
 * on_workspace_blurred:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace blurred event handler.
 */
static void
on_workspace_blurred(i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    set_button_label(button, workspace->name, workspace->focused,
            workspace->urgent, i3_workspaces->config);
}

/**
 * on_workspace_focused:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace focused event handler.
 */
static void
on_workspace_focused(i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    workspace->urgent = FALSE; // reset the urgent flag
    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    set_button_label(button, workspace->name, workspace->focused,
            workspace->urgent, i3_workspaces->config);
}

/**
 * on_workspace_urgent:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace urgent event handler.
 */
static void
on_workspace_urgent(i3workspace *workspace, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    GtkWidget *button = i3_workspaces->buttons[workspace->num];
    set_button_label(button, workspace->name, workspace->focused,
            workspace->urgent, i3_workspaces->config);
}

/**
 * set_button_label:
 * @button: the button
 * @name: the workspace name
 * @focused: is the workspace focused?
 * @urgent: is the workspace urgent?
 *
 * Generate the label for the workspace button.
 */
static void
set_button_label(GtkWidget *button, gchar *name, gboolean focused, gboolean urgent, 
        i3WorkspacesConfig *config)
{
    static gchar * template = "<span foreground=\"#%06X\" weight=\"%s\">%s</span>";
    static gchar * focused_weight = "bold";
    static gchar * blurred_weight = "normal";

    // allocate space for the maximum possible size of the label
    gulong maxlen = strlen(name) + 51;
    gchar *label_str = (gchar *) calloc(maxlen, sizeof(gchar));

    g_snprintf(label_str, maxlen, template,
            urgent ? config->urgent_color :
               focused ? config->focused_color : config->normal_color,
            focused ? focused_weight : blurred_weight,
            name);

    GtkWidget * label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_markup(GTK_LABEL(label), label_str);

    free(label_str);
}

/**
 * on_workspace_clicked:
 * @button: the clicked button
 * @data: the workspace plugin
 *
 * Workspace button click event handler.
 */
static void
on_workspace_clicked(GtkWidget *button, gpointer data)
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

    GError *err = NULL;
    i3wm_goto_workspace(i3_workspaces->i3wm, button_index, &err);
    if (err != NULL)
    {
        fprintf(stderr, "%s", err->message);
    }
}

static void
on_ipc_shutdown(gpointer i3_w)
{
    g_printf("on_ipc_shutdown\n");
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) i3_w;

    remove_workspaces(i3_workspaces);
    i3wm_destruct(i3_workspaces->i3wm);
    i3_workspaces->i3wm = NULL;

    //TODO: error_state_on();
    //recover_from_disconnect(i3_workspaces);
    //TODO: error_state_off();

    //connect_callbacks(i3_workspaces);

    //add_workspaces(i3_workspaces);
}

static void
recover_from_disconnect(i3WorkspacesPlugin *i3_workspaces)
{
    GError *err = NULL;

    i3_workspaces->i3wm = i3wm_construct(&err);
    while (NULL != err)
    {
        //fprintf(stderr, "Cannot connect to the i3 window manager: %s\n",
         //       err->message);
        g_error_free(err);
        err = NULL;
        i3_workspaces->i3wm = i3wm_construct(&err);
    }
}
