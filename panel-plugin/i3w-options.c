/*  xfce4-netspeed-plugin
 *
 *  Copyright (c) 2011 Calin Crisan <ccrisan@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "options.h"

guint32 serialize_gdkcolor(GdkColor *gdkcolor);
GdkColor *unserialize_gdkcolor(guint32 color);

/* Function Implementations */

guint32 serialize_gdkcolor(GdkColor *gdkcolor)
{
    guint32 color = 0;

    // Shift and add the color components
    color += ((guint32) gdkcolor->red) << 4;
    color += ((guint32) gdkcolor->green) << 2;
    color += ((guint32) gdkcolor->blue);

    return color;
}

GdkColor *unserialize_gdkcolor(guint32 color)
{
    GdkColor *gdkcolor = g_new0(GdkColor, 1);

    // Mask and shift the color components
    gdkcolor->red = (guint16) ((color & 0xff0000) >> 4);
    gdkcolor->green = (guint16) ((color & 0x00ff00) >> 2);
    gdkcolor->blue = (guint16) (color & 0x0000ff);

    return gdkcolor;
}

i3WorkspacesOptions *i3_workspaces_options_new()
{
    i3WorkspacesOptions *options = g_new0(i3WorkspacesOptions, 1);
    
    return options;
}

void i3_workspaces_options_free(i3WorkspacesOptions *options)
{
    if (options->normal_color) g_free(options->normal_color);
    if (options->urgent_color) g_free(options->urgent_color);
    
    g_free(options);
}

gboolean i3_workspaces_options_load(i3WorkspacesOptions *options, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, FALSE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    guint32 normal_color = xfce_rc_read_int_entry(rc, "normal_color", 0x000000);
    options->normal_color = unserialize_gdkcolor(normal_color);

    guint32 urgent_color = xfce_rc_read_int_entry(rc, "urgent_color", 0xff0000);
    options->urgent_color = unserialize_gdkcolor(urgent_color);

    options->strip_workspace_numbers = xfce_rc_read_bool_entry(rc, "strip_workspace_numbers", FALSE);

    xfce_rc_close(rc);
    
    return TRUE;
}

gboolean i3_workspaces_options_save(i3WorkspacesOptions *options, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, TRUE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    xfce_rc_write_int_entry(rc, "normal_color", serialize_gdkcolor(options->normal_color));
    xfce_rc_write_int_entry(rc, "urgent_color", serialize_gdkcolor(options->urgent_color));
    xfce_rc_write_bool_entry(rc, "strip_workspace_numbers", options->strip_workspace_numbers);

    xfce_rc_close(rc);
    
    return TRUE;
}
