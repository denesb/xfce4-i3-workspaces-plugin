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

#include <libxfce4panel/libxfce4panel.h>
#include <glib/gprintf.h>

#include "i3w-plugin.h"

/* prototypes */

static void
connect_callbacks(i3WorkspacesPlugin *i3_workspaces);

static void
init_css(i3WorkspacesPlugin *i3_workspaces);

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
set_button_label(GtkWidget *button, i3workspace *workspace,
        i3WorkspacesConfig *config);

static gchar *
strip_workspace_numbers(const gchar *name, int num);

static void
on_workspace_clicked(GtkWidget *button, gpointer data);
static gboolean
on_workspace_scrolled(GtkWidget *ebox, GdkEventScroll *ev, gpointer data);

static void
on_workspace_created(gpointer data);
static void
on_workspace_destroyed(gpointer data);
static void
on_workspace_changed(gpointer data);

static void
on_mode_changed(gchar *mode, gpointer data);

static void
on_output_changed(gchar *mode, gpointer data);

static void
on_ipc_shutdown(gpointer i3_w);

static void
reconnect_i3wm_timer(i3WorkspacesPlugin *i3_workspaces);

static void
handle_change_output(i3WorkspacesPlugin* i3_workspaces);

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
    i3wm_set_on_workspace_created(i3_workspaces->i3wm,
            on_workspace_changed, i3_workspaces);
    i3wm_set_on_workspace_destroyed(i3_workspaces->i3wm,
            on_workspace_changed, i3_workspaces);
    i3wm_set_on_workspace_blurred(i3_workspaces->i3wm,
            on_workspace_changed, i3_workspaces);
    i3wm_set_on_workspace_focused(i3_workspaces->i3wm,
            on_workspace_changed, i3_workspaces);
    i3wm_set_on_workspace_urgent(i3_workspaces->i3wm,
            on_workspace_changed, i3_workspaces);
    i3wm_set_on_mode_changed(i3_workspaces->i3wm,
            on_mode_changed, i3_workspaces);
    i3wm_set_on_output_changed(i3_workspaces->i3wm,
            on_output_changed, i3_workspaces);
    i3wm_set_on_ipc_shutdown(i3_workspaces->i3wm,
            on_ipc_shutdown, i3_workspaces);
}

/**
 * init_css:
 * @i3_workspaces: the workspaces plugin
 *
 * Set the CSS for the application
 */
static void
init_css(i3WorkspacesPlugin *i3_workspaces) {
    i3WorkspacesConfig *config = i3_workspaces->config;

    if (config->use_css) {
        gtk_css_provider_load_from_data(
            i3_workspaces->css_provider, i3_workspaces->config->css, -1, NULL);
    }
    else {
        gchar *css = g_strdup_printf(
            ".workspace {\n"
            "  color: %s;\n"
            "}\n"
            ".workspace.visible {\n"
            "  color: %s;\n"
            "}\n"
            ".workspace.focused {\n"
            "  font-weight: bold;\n"
            "  color: %s;\n"
            "}\n"
            ".workspace.urgent {\n"
            "  color: %s;\n"
            "}\n"
            ".binding-mode {\n"
            "  color: %s;\n"
            "}\n",
            gdk_rgba_to_string(&config->normal_color),
            gdk_rgba_to_string(&config->visible_color),
            gdk_rgba_to_string(&config->focused_color),
            gdk_rgba_to_string(&config->urgent_color),
            gdk_rgba_to_string(&config->mode_color));

        gtk_css_provider_load_from_data(i3_workspaces->css_provider, css, -1, NULL);
        g_free(css);
    }
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
    fprintf(stderr, "xfce4_i3_workspaces: construct_workspace, plugin=%p\n",
            plugin);

    /* allocate memory for the plugin structure */
    i3_workspaces = g_slice_new0(i3WorkspacesPlugin);

    /* pointer to plugin */
    i3_workspaces->plugin = plugin;

    /* plugin configuration */
    i3_workspaces->config = i3_workspaces_config_new();
    i3_workspaces_config_load(i3_workspaces->config, plugin);

    /* get the current orientation */
    orientation = xfce_panel_plugin_get_orientation (plugin);

    /* set up css */
    i3WorkspacesConfig *config = i3_workspaces->config;
    GdkScreen *screen = gdk_screen_get_default();
    i3_workspaces->css_provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(i3_workspaces->css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    init_css(i3_workspaces);

    /* create some panel widgets */
    i3_workspaces->ebox = gtk_event_box_new();
    gtk_widget_show(i3_workspaces->ebox);

    /* listen for scroll events */
    gtk_widget_add_events(i3_workspaces->ebox, GDK_SCROLL_MASK);
    g_signal_connect(G_OBJECT(i3_workspaces->ebox), "scroll-event",
            G_CALLBACK(on_workspace_scrolled), i3_workspaces);

    i3_workspaces->hvbox = gtk_box_new(orientation, 2);
    gtk_widget_show(i3_workspaces->hvbox);
    gtk_container_add(GTK_CONTAINER(i3_workspaces->ebox), i3_workspaces->hvbox);

    i3_workspaces->workspace_buttons = g_hash_table_new(g_direct_hash, g_direct_equal);

    /* Add a label for the binding mode */
    i3_workspaces->mode_label = gtk_label_new(NULL);
    GtkStyleContext *mode_label_context = gtk_widget_get_style_context(
        GTK_WIDGET(i3_workspaces->mode_label));
    gtk_style_context_add_class(mode_label_context, "binding-mode");
    gtk_box_pack_end(GTK_BOX(i3_workspaces->hvbox), i3_workspaces->mode_label, FALSE, FALSE, 0);
    gtk_widget_show(i3_workspaces->mode_label);

    /* Setup a timer to connect to the i3wm until success. */
    reconnect_i3wm_timer(i3_workspaces);

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

    /* Auto-detect output configuration */
    handle_change_output(i3_workspaces);
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

    /* cancel the timer */
    if (i3_workspaces->timeout) {
        g_source_remove(i3_workspaces->timeout);
        i3_workspaces->timeout = 0;
    }

    /* destroy the i3wm delegate */
    if (i3_workspaces->i3wm) {
        i3wm_destruct(i3_workspaces->i3wm);
        i3_workspaces->i3wm = NULL;
    }

    /* free the plugin structure */
    g_slice_free(i3WorkspacesPlugin, i3_workspaces);
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
    gtk_orientable_set_orientation(GTK_ORIENTABLE(i3_workspaces->hvbox), orientation);
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

    init_css(i3_workspaces);
    handle_change_output(i3_workspaces);
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
    g_assert(i3_workspaces->i3wm);
    GSList *wlist = i3wm_get_workspaces(i3_workspaces->i3wm);

    GSList *witem;
    for (witem = wlist; witem != NULL; witem = witem->next)
    {
        i3workspace *workspace = (i3workspace *) witem->data;
        if (workspace &&
            (i3_workspaces->config->output[0] == 0 ||
             g_strcmp0(i3_workspaces->config->output, workspace->output) == 0))
        {
            GtkWidget * button;
            button = xfce_panel_create_button();
            GtkStyleContext *context = gtk_widget_get_style_context(button);
            gtk_style_context_add_class(context, "workspace");

            gtk_button_set_label(GTK_BUTTON(button), workspace->name);

            set_button_label(button, workspace, i3_workspaces->config);

            g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(on_workspace_clicked), i3_workspaces);

            /* show the panel's right-click menu on this button */
            xfce_panel_plugin_add_action_widget(i3_workspaces->plugin, button);

            /* avoid acceleration key interference */
            gtk_button_set_use_underline(GTK_BUTTON(button), FALSE);
            gtk_box_pack_end(GTK_BOX(i3_workspaces->hvbox), button, FALSE, FALSE, 0);
            gtk_widget_show(button);

            g_hash_table_insert(i3_workspaces->workspace_buttons, workspace, button);
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
    GList *wlist = g_hash_table_get_values(i3_workspaces->workspace_buttons);
    g_hash_table_remove_all(i3_workspaces->workspace_buttons);
    g_list_free_full(wlist, (GDestroyNotify) gtk_widget_destroy);
}

/**
 * on_workspace_changed:
 * @workspace: the workspace
 * @data: the workspaces plugin
 *
 * Workspace changed event handler.
 */
static void
on_workspace_changed(gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;

    remove_workspaces(i3_workspaces);
    add_workspaces(i3_workspaces);
}

/**
 * on_mode_changed:
 * @mode: the mode
 * @data: the workspaces plugin
 *
 * Binging mode changed event handler.
 */
static void
on_mode_changed(gchar *mode, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;
	if (!strncmp(mode, "default", 7)) {
		gtk_label_set_text((GtkLabel *) i3_workspaces->mode_label, "");
    }
	else {
		gtk_label_set_text((GtkLabel *) i3_workspaces->mode_label, mode);
	}
}

/**
 * handle_change_output:
 * @i3_workspaces: the workspaces plugin
 *
 * Recomputes the panel's output based on XRandR's current data.
 * Does not run if auto_detect_outputs is set to false.
 */
static void
handle_change_output (i3WorkspacesPlugin* i3_workspaces)
{

    if(!i3_workspaces->config->auto_detect_outputs) return;

    // Re-query X Server for monitor information, since it may have changed
    i3_workspaces_outputs_t outputs = get_outputs();

    // Get the plugin's widget window and its location in root window (i.e: screen) coordinates
    int x, y;
    GdkWindow* window = gtk_widget_get_window(i3_workspaces->ebox);
    gdk_window_get_root_origin(window, &x, &y);

    // Get the monitor name for the window location and set the config value
    char* output_name = get_monitor_name_at(outputs, x, y);

    i3_workspaces->config->output = output_name;
    remove_workspaces(i3_workspaces);
    add_workspaces(i3_workspaces);

    free_outputs(outputs);
}

/**
 * on_output_changed:
 * @change: the change field, always "unspecified" for now
 * @data: the workspaces plugin
 *
 * Output changed event handler.
 */
static void
on_output_changed(gchar *mode, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;
    handle_change_output(i3_workspaces);
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
set_button_label(GtkWidget *button, i3workspace *workspace,
        i3WorkspacesConfig *config)
{
    GtkStyleContext *context = gtk_widget_get_style_context(button);

    // Set button class based on workspace state
    if (workspace->urgent) gtk_style_context_add_class(context, "urgent");
    else gtk_style_context_remove_class(context, "urgent");
    if (workspace->focused) gtk_style_context_add_class(context, "focused");
    else gtk_style_context_remove_class(context, "focused");
    if (workspace->visible) gtk_style_context_add_class(context, "visible");
    else gtk_style_context_remove_class(context, "visible");

    gchar *name = (config->strip_workspace_numbers && workspace->num > 0) ?
        strip_workspace_numbers(workspace->name, workspace->num) :
        workspace->name;

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_text(GTK_LABEL(label), name);
}

/**
 * strip_workspace_numbers:
 * @workspaceName - the name of the workspace
 * @workspaceNum - the number of the workspace
 *
 * Strips the workspace name of the workspace number.
 * Returns: an i3workspace*
 */
static gchar *
strip_workspace_numbers(const gchar *name, int num)
{
    size_t offset = 0;
    offset += (num < 10) ? 1 : 2;
    if (name[offset] == ':') offset++;

    int len = strlen(name);
    gchar *strippedName = NULL;

    if (offset < len)
    {
        int strippedLen = len - offset + 1;
        strippedName = (gchar *) malloc(strippedLen);
        strippedName = memcpy(strippedName, name + offset, strippedLen);
    }
    else
    {
        strippedName = strdup(name);
    }

    return strippedName;
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
    GSList *wlist, *witem;
    i3workspace *workspace = NULL;

    if (!i3_workspaces->i3wm) {
        return;
    }

    wlist = i3wm_get_workspaces(i3_workspaces->i3wm);
    for (witem = wlist; witem != NULL; witem = witem->next)
    {
        workspace = (i3workspace *) witem->data;
        GtkWidget *w = (GtkWidget *) g_hash_table_lookup(i3_workspaces->workspace_buttons, workspace);

        if (w == button)
            break;
    }

    GError *err = NULL;
    i3wm_goto_workspace(i3_workspaces->i3wm, workspace, &err);
    if (err != NULL)
    {
        fprintf(stderr, "%s", err->message);
    }
}

/**
 * on_workspace_scrolled:
 * @ebox: the plugin's event box
 * @ev: the event data
 * @data: the workspace plugin
 *
 * Workspace scroll event handler.
 *
 * Returns: TRUE to stop event propogation
 */
static gboolean
on_workspace_scrolled(GtkWidget *ebox, GdkEventScroll *ev, gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *)data;

    if (!i3_workspaces->i3wm) {
        /* Hasn't connected, so stop the event. Once connected the workspace
         * is ensured to be updated. */
        return TRUE;
    }

    GList *wlist = g_hash_table_get_keys(i3_workspaces->workspace_buttons);
    wlist = g_list_sort(wlist, (GCompareFunc) i3wm_workspace_cmp);

    /* Find the focused workspace */
    i3workspace *workspace = NULL;
    GList *witem = NULL;
    for (witem = wlist; witem != NULL; witem = witem->next)
    {
        workspace = (i3workspace *) witem->data;
        if (workspace->focused)
            break;
    }

    if (witem == NULL)
        return FALSE;

    if (ev->direction == GDK_SCROLL_UP)
        witem = witem->next;
    else if (ev->direction == GDK_SCROLL_DOWN)
        witem = witem->prev;
    else
        return FALSE;

    if (witem == NULL)
        return FALSE;

    workspace = (i3workspace *) witem->data;

    GError *err = NULL;
    i3wm_goto_workspace(i3_workspaces->i3wm, workspace, &err);
    if (err != NULL)
    {
        fprintf(stderr, "%s", err->message);
    }
    return TRUE;
}

static void
on_ipc_shutdown(gpointer i3_w)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) i3_w;
    fprintf(stderr, "xfce4_i3_workspaces: on_ipc_shutdown, i3_w=%p\n", i3_w);

    /* Remove previous resources */
    remove_workspaces(i3_workspaces);
    if (i3_workspaces->i3wm) {
        i3wm_destruct(i3_workspaces->i3wm);
        i3_workspaces->i3wm = NULL;
    }

    /* When shutdown, wake up the timer try to reconnect */
    reconnect_i3wm_timer(i3_workspaces);
}

static gboolean
reconnect_i3wm_callback(gpointer data)
{
    i3WorkspacesPlugin *i3_workspaces = (i3WorkspacesPlugin *) data;
    GError *err = NULL;

    if (i3_workspaces->i3wm) {
        // maybe already initialized by other timers?
        fprintf(stderr, "warn: other timer already reconnected?\n");
        return G_SOURCE_REMOVE;
    }

    fprintf(stderr, "Connecting to i3 workspace manager...\n");
    i3_workspaces->i3wm = i3wm_construct(&err);
    if (err != NULL) {
        fprintf(stderr, "Still waiting for i3 window manager: %s\n",
                err->message);
        g_error_free(err);
        return G_SOURCE_CONTINUE;
    }

    // connected, doing the init things; todo: is memory barrier needed?
    //   don't know whether the timer is sync or not
    connect_callbacks(i3_workspaces);
    add_workspaces(i3_workspaces);

    i3_workspaces->timeout = 0;
    return G_SOURCE_REMOVE;
}

static void
reconnect_i3wm_timer(i3WorkspacesPlugin *i3_workspaces)
{
    guint id = g_timeout_add_seconds(1, reconnect_i3wm_callback, i3_workspaces);
    if (id == 0) {
        fprintf(stderr, "Timer broken! Plugin will no longer works.\n");
    }
    i3_workspaces->timeout = id;
}
