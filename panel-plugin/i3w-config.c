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
#include "i3w-plugin.h"

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
strip_workspace_numbers_changed(GtkWidget *button, i3WorkspacesConfig *config);
void
output_changed(GtkWidget *entry, i3WorkspacesConfig *config);

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param);

/* Function Implementations */

guint32
serialize_gdkcolor(GdkColor *gdkcolor)
{
    guint32 color = 0;

    // truncate the GdkColor components to 8 bit
    guint16 red_component = gdkcolor->red >> 8;
    guint16 green_component = gdkcolor->green >> 8;
    guint16 blue_component = gdkcolor->blue >> 8;

    // shift and add the color components
    color += ((guint32) red_component) << 16;
    color += ((guint32) green_component) << 8;
    color += ((guint32) blue_component);

    return color;
}

GdkColor *
unserialize_gdkcolor(guint32 color)
{
    GdkColor *gdkcolor = g_new0(GdkColor, 1);

    // Mask and shift the color components
    guint16 red_component = (guint16) ((color & 0xff0000) >> 16);
    guint16 green_component = (guint16) ((color & 0x00ff00) >> 8);
    guint16 blue_component = (guint16) ((color & 0x0000ff));

    // expand the GdkColor components to 16 bit
    gdkcolor->red = red_component << 8;
    gdkcolor->green = green_component << 8;
    gdkcolor->blue = blue_component << 8;

    return gdkcolor;
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
    config->strip_workspace_numbers = xfce_rc_read_bool_entry(rc,
            "strip_workspace_numbers", FALSE);
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
    xfce_rc_write_bool_entry(rc, "strip_workspace_numbers",
            config->strip_workspace_numbers);
    xfce_rc_write_entry(rc, "output", config->output);

    xfce_rc_close(rc);

    return TRUE;
}

void
i3_workspaces_config_show(i3WorkspacesConfig *config, XfcePanelPlugin *plugin,
        ConfigChangedCallback cb, gpointer cb_data)
{
    GtkWidget *dialog, *dialog_vbox, *hbox, *button, *label;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_buttons(_("i3 Workspaces Plugin"),
        NULL, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
    xfce_titled_dialog_set_subtitle(XFCE_TITLED_DIALOG(dialog), _("Configuration"));

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_stick(GTK_WINDOW(dialog));

    dialog_vbox = GTK_DIALOG(dialog)->vbox;

    /* normal color */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Normal Workspace Color:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_color(unserialize_gdkcolor(
                config->normal_color));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(normal_color_changed), config);

    /* focused color */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Focused Workspace Color:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_color(unserialize_gdkcolor(
                config->focused_color));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(focused_color_changed), config);

    /* urgent color */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Urgent Workspace Color:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_color(unserialize_gdkcolor(
                config->urgent_color));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(urgent_color_changed), config);

    /* strip workspace numbers */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    button = gtk_check_button_new_with_mnemonic(_("Strip Workspace Numbers"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), config->strip_workspace_numbers == TRUE);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(strip_workspace_numbers_changed), config);

    /* output */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
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
output_changed(GtkWidget *entry, i3WorkspacesConfig *config)
{
    config->output = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

void
normal_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkColor color;
    gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
    config->normal_color = serialize_gdkcolor(&color);
}

void
focused_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkColor color;
    gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
    config->focused_color = serialize_gdkcolor(&color);
}

void
urgent_color_changed(GtkWidget *button, i3WorkspacesConfig *config)
{
    GdkColor color;
    gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
    config->urgent_color = serialize_gdkcolor(&color);
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
