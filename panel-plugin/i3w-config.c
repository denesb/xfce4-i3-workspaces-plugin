/*  xfce4-netspeed-plugin
 *
 *  Copyright (c) 2011 Calin Crisan <ccrisan@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "i3w-config.h"

typedef struct {
    i3WorkspacesConfig *config;
    XfcePanelPlugin *plugin;
    ConfigChangedCallback cb;
    gpointer cb_data;
} ConfigDialogClosedParam;

void
use_css_changed(GtkStack *stack, GParamSpec *pspec, i3WorkspacesConfig *config);
void
color_changed(GtkWidget *button, GdkRGBA *color_setting);
void
css_changed(GtkTextBuffer *buffer, i3WorkspacesConfig *config);
void
strip_workspace_numbers_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
auto_detect_outputs_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
output_changed(GtkWidget *entry, i3WorkspacesConfig *config);

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param);

/* Function Implementations */

i3WorkspacesConfig *
i3_workspaces_config_new()
{
    return g_new0(i3WorkspacesConfig, 1);
}

void
i3_workspaces_config_free(i3WorkspacesConfig *config)
{
    g_free(config->css);
    g_free(config->output);
    g_free(config);
}

gboolean
i3_workspaces_config_load(i3WorkspacesConfig *config, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, FALSE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    gdk_rgba_parse(&config->normal_color,
                   xfce_rc_read_entry(rc, "normal_color", "#000000"));
    gdk_rgba_parse(&config->focused_color,
                   xfce_rc_read_entry(rc, "focused_color", "#000000"));
    gdk_rgba_parse(&config->urgent_color,
                   xfce_rc_read_entry(rc, "urgent_color", "#ff0000"));
    gdk_rgba_parse(&config->mode_color,
                   xfce_rc_read_entry(rc, "mode_color", "#ff0000"));
    gdk_rgba_parse(&config->visible_color,
                   xfce_rc_read_entry(rc, "visible_color", "#000000"));

    const gchar default_css[] =
        ".workspace { }\n"
        ".workspace.visible { }\n"
        ".workspace.focused { font-weight: bold; }\n"
        ".workspace.urgent { color: red; }\n"
        ".binding-mode { }\n";

    config->css = g_strdup(xfce_rc_read_entry(rc, "css", default_css));
    config->use_css = xfce_rc_read_bool_entry(rc, "use_css", FALSE);

    config->strip_workspace_numbers = xfce_rc_read_bool_entry(rc,
            "strip_workspace_numbers", FALSE);
    config->auto_detect_outputs = xfce_rc_read_bool_entry(rc,
            "auto_detect_outputs", FALSE);
    config->output = g_strdup(xfce_rc_read_entry(rc, "output", ""));

    xfce_rc_close(rc);

    return TRUE;
}

gboolean
i3_workspaces_config_save(i3WorkspacesConfig *config, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, TRUE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    xfce_rc_write_bool_entry(rc, "use_css", config->use_css);
    xfce_rc_write_entry(rc, "normal_color", gdk_rgba_to_string(&config->normal_color));
    xfce_rc_write_entry(rc, "focused_color", gdk_rgba_to_string(&config->focused_color));
    xfce_rc_write_entry(rc, "urgent_color", gdk_rgba_to_string(&config->urgent_color));
    xfce_rc_write_entry(rc, "mode_color", gdk_rgba_to_string(&config->mode_color));
    xfce_rc_write_entry(rc, "visible_color", gdk_rgba_to_string(&config->visible_color));
    xfce_rc_write_entry(rc, "css", config->css);
    xfce_rc_write_bool_entry(rc, "strip_workspace_numbers",
            config->strip_workspace_numbers);
    xfce_rc_write_bool_entry(rc, "auto_detect_outputs",
                             config->auto_detect_outputs);
    xfce_rc_write_entry(rc, "output", config->output);

    xfce_rc_close(rc);

    return TRUE;
}

void
add_color_picker(i3WorkspacesConfig *config, GtkWidget *vbox, char *text, GdkRGBA *color_setting) {
    GtkWidget *hbox, *button, *label;

    /* focused color */
    hbox = gtk_box_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_(text));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_rgba(color_setting);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(color_changed), color_setting);
}

void
i3_workspaces_config_show(i3WorkspacesConfig *config, XfcePanelPlugin *plugin,
        ConfigChangedCallback cb, gpointer cb_data)
{
    GtkWidget *dialog, *dialog_content, *hbox, *vbox, *view, *button, *label, *stack, *stack_switcher;
    GtkTextBuffer *buffer;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_mixed_buttons(_("i3 Workspaces Plugin"),
      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      "window-close", "_Close",
      GTK_RESPONSE_OK,
      NULL);
    xfce_titled_dialog_set_subtitle(XFCE_TITLED_DIALOG(dialog), _("Configuration"));

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_stick(GTK_WINDOW(dialog));

    dialog_content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    /* color buttons or CSS */
    stack = gtk_stack_new();

    /* color buttons */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    add_color_picker(config, vbox, "Normal Workspace Color:", &config->normal_color);
    add_color_picker(config, vbox, "Focused Workspace Color:", &config->focused_color);
    add_color_picker(config, vbox, "Urgent Workspace Color:", &config->urgent_color);
    add_color_picker(config, vbox, "Unfocused Visible Workspace Color:", &config->visible_color);
    add_color_picker(config, vbox, "Binding Mode Color:", &config->mode_color);
    gtk_stack_add_titled(GTK_STACK(stack), vbox, "buttons", "Color Pickers");
    gtk_widget_set_visible(vbox, TRUE);

    /* CSS */
    hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (view));
    gtk_text_buffer_set_text(buffer, config->css, -1);
    gtk_box_pack_start(GTK_BOX(hbox), view, FALSE, FALSE, 0);
    gtk_stack_add_titled(GTK_STACK(stack), hbox, "css", "Raw CSS");
    g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(css_changed), config);

    stack_switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));
    gtk_box_pack_start(GTK_BOX(dialog_content), stack_switcher, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dialog_content), stack, FALSE, FALSE, 0);
    gtk_widget_set_visible(hbox, TRUE);
    gtk_stack_set_visible_child_name(GTK_STACK(stack), config->use_css ? "css" : "buttons");
    g_signal_connect(G_OBJECT(stack), "notify::visible-child", G_CALLBACK(use_css_changed), config);


    /* strip workspace numbers */
    hbox = gtk_box_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_content), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    button = gtk_check_button_new_with_mnemonic(_("Strip Workspace Numbers"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), config->strip_workspace_numbers == TRUE);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(strip_workspace_numbers_changed), config);

    /* auto detect output */
    hbox = gtk_box_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_content), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    button = gtk_check_button_new_with_mnemonic(_("Auto detect outputs"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), config->auto_detect_outputs == TRUE);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(auto_detect_outputs_changed), config);

    /* output */
    hbox = gtk_box_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_content), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Output:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_entry_set_text(GTK_ENTRY(button), config->output);
    g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(output_changed), config);


    /* close event */
    ConfigDialogClosedParam *param = g_new(ConfigDialogClosedParam, 1);
    param->plugin = plugin;
    param->config = config;
    param->cb = cb;
    param->cb_data = cb_data;
    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(config_dialog_closed), param);

    gtk_widget_show_all(dialog);
}

void
use_css_changed(GtkStack *stack, GParamSpec *pspec, i3WorkspacesConfig *config) {
    const gchar *visible_child = gtk_stack_get_visible_child_name(stack);
    config->use_css = !g_strcmp0(visible_child, "css");
}

void
css_changed(GtkTextBuffer *buffer, i3WorkspacesConfig *config)
{
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    config->css = g_strdup(gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
}

void
strip_workspace_numbers_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    config->strip_workspace_numbers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

void
auto_detect_outputs_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    config->auto_detect_outputs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

void
output_changed(GtkWidget *entry, i3WorkspacesConfig *config)
{
    config->output = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

void
color_changed(GtkWidget *button, GdkRGBA *color_setting)
{
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), color_setting);
}

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param)
{
    xfce_panel_plugin_unblock_menu(param->plugin);

    gtk_widget_destroy(dialog);

    i3_workspaces_config_save(param->config, param->plugin);

    if (param->cb) param->cb(param->cb_data);

    g_free(param);
}
