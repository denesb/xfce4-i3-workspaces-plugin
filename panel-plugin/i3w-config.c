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
normal_color_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
focused_color_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
urgent_color_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
visible_color_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
mode_color_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
strip_workspace_numbers_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
auto_detect_outputs_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
output_changed(GtkWidget *entry, i3WorkspacesConfig *config);

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param);

/* Function Implementations */

guint32
serialize_gdkrgba(GdkRGBA *gdkrgba)
{
    guint32 color = 0;

    // convert the GdkRGBA components to 8 bit ints
    guint8 red_component = gdkrgba->red * 255;
    guint8 green_component = gdkrgba->green * 255;
    guint8 blue_component = gdkrgba->blue * 255;

    // shift and add the color components
    color += ((guint32) red_component) << 16;
    color += ((guint32) green_component) << 8;
    color += ((guint32) blue_component);

    return color;
}

GdkRGBA *
unserialize_gdkrgba(guint32 color)
{
    GdkRGBA *gdkrgba = g_new0(GdkRGBA, 1);

    // Mask and shift the color components
    guint8 red_component = (color & 0xff0000) >> 16;
    guint8 green_component = (color & 0x00ff00) >> 8;
    guint8 blue_component = (color & 0x0000ff);

    // convert back to floats in range 0.0 to 1.0
    gdkrgba->red = red_component / 255.0;
    gdkrgba->green = green_component / 255.0;
    gdkrgba->blue = blue_component / 255.0;
    gdkrgba->alpha = 1.0;

    return gdkrgba;
}

i3WorkspacesConfig *
i3_workspaces_config_new()
{
    return g_new0(i3WorkspacesConfig, 1);
}

void
i3_workspaces_config_free(i3WorkspacesConfig *config)
{
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

    config->normal_color = xfce_rc_read_int_entry(rc, "normal_color", 0x000000);
    config->focused_color = xfce_rc_read_int_entry(rc, "focused_color", 0x000000);
    config->urgent_color = xfce_rc_read_int_entry(rc, "urgent_color", 0xff0000);
    config->mode_color = xfce_rc_read_int_entry(rc, "mode_color", 0xff0000);
    config->visible_color = xfce_rc_read_int_entry(rc, "visible_color", 0x000000);
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

    xfce_rc_write_int_entry(rc, "normal_color", config->normal_color);
    xfce_rc_write_int_entry(rc, "focused_color", config->focused_color);
    xfce_rc_write_int_entry(rc, "urgent_color", config->urgent_color);
    xfce_rc_write_int_entry(rc, "mode_color", config->mode_color);
    xfce_rc_write_int_entry(rc, "visible_color", config->visible_color);
    xfce_rc_write_bool_entry(rc, "strip_workspace_numbers",
            config->strip_workspace_numbers);
    xfce_rc_write_bool_entry(rc, "auto_detect_outputs",
                             config->auto_detect_outputs);
    xfce_rc_write_entry(rc, "output", config->output);

    xfce_rc_close(rc);

    return TRUE;
}

void add_color_picker(i3WorkspacesConfig *config, GtkWidget *dialog_vbox, char *text, guint32 color, gpointer callback) {
    GtkWidget *hbox, *button, *label;

    /* focused color */
    hbox = gtk_box_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_(text));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_rgba(unserialize_gdkrgba(color));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(callback), config);
}

void
i3_workspaces_config_show(i3WorkspacesConfig *config, XfcePanelPlugin *plugin,
        ConfigChangedCallback cb, gpointer cb_data)
{
    GtkWidget *dialog, *dialog_content, *hbox, *button, *label;

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

    add_color_picker(config, dialog_content, "Normal Workspace Color:", config->normal_color, normal_color_changed);
    add_color_picker(config, dialog_content, "Focused Workspace Color:", config->focused_color, focused_color_changed);
    add_color_picker(config, dialog_content, "Urgent Workspace Color:", config->urgent_color, urgent_color_changed);
    add_color_picker(config, dialog_content, "Unfocused Visible Workspace Color:", config->visible_color, visible_color_changed);
    add_color_picker(config, dialog_content, "Binding Mode Color:", config->mode_color, mode_color_changed);

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
normal_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    config->normal_color = serialize_gdkrgba(&color);
}

void
focused_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    config->focused_color = serialize_gdkrgba(&color);
}

void
urgent_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    config->urgent_color = serialize_gdkrgba(&color);
}

void
visible_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    config->visible_color = serialize_gdkrgba(&color);
}

void

mode_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    config->mode_color = serialize_gdkrgba(&color);
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
