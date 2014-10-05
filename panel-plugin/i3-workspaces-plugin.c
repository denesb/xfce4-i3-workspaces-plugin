/*  $Id$
 *
 *  Copyright (C) 2012 John Doo <john@foo.org>
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

#include "i3_workspaces.h"
#include "i3_workspaces-dialogs.h"

/* default settings */
#define DEFAULT_SETTING1 NULL
#define DEFAULT_SETTING2 1
#define DEFAULT_SETTING3 FALSE



/* prototypes */
static void
i3_workspaces_construct (XfcePanelPlugin *plugin);


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (i3_workspaces_construct);



void
i3_workspaces_save (XfcePanelPlugin *plugin,
             SamplePlugin    *i3_workspaces)
{
  XfceRc *rc;
  gchar  *file;

  /* get the config file location */
  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
       DBG ("Failed to open config file");
       return;
    }

  /* open the config file, read/write */
  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  if (G_LIKELY (rc != NULL))
    {
      /* save the settings */
      DBG(".");
      if (i3_workspaces->setting1)
        xfce_rc_write_entry    (rc, "setting1", i3_workspaces->setting1);

      xfce_rc_write_int_entry  (rc, "setting2", i3_workspaces->setting2);
      xfce_rc_write_bool_entry (rc, "setting3", i3_workspaces->setting3);

      /* close the rc file */
      xfce_rc_close (rc);
    }
}



static void
i3_workspaces_read (SamplePlugin *i3_workspaces)
{
  XfceRc      *rc;
  gchar       *file;
  const gchar *value;

  /* get the plugin config file location */
  file = xfce_panel_plugin_save_location (i3_workspaces->plugin, TRUE);

  if (G_LIKELY (file != NULL))
    {
      /* open the config file, readonly */
      rc = xfce_rc_simple_open (file, TRUE);

      /* cleanup */
      g_free (file);

      if (G_LIKELY (rc != NULL))
        {
          /* read the settings */
          value = xfce_rc_read_entry (rc, "setting1", DEFAULT_SETTING1);
          i3_workspaces->setting1 = g_strdup (value);

          i3_workspaces->setting2 = xfce_rc_read_int_entry (rc, "setting2", DEFAULT_SETTING2);
          i3_workspaces->setting3 = xfce_rc_read_bool_entry (rc, "setting3", DEFAULT_SETTING3);

          /* cleanup */
          xfce_rc_close (rc);

          /* leave the function, everything went well */
          return;
        }
    }

  /* something went wrong, apply default values */
  DBG ("Applying default settings");

  i3_workspaces->setting1 = g_strdup (DEFAULT_SETTING1);
  i3_workspaces->setting2 = DEFAULT_SETTING2;
  i3_workspaces->setting3 = DEFAULT_SETTING3;
}



static SamplePlugin *
i3_workspaces_new (XfcePanelPlugin *plugin)
{
  SamplePlugin   *i3_workspaces;
  GtkOrientation  orientation;
  GtkWidget      *label;

  /* allocate memory for the plugin structure */
  i3_workspaces = panel_slice_new0 (SamplePlugin);

  /* pointer to plugin */
  i3_workspaces->plugin = plugin;

  /* read the user settings */
  i3_workspaces_read (i3_workspaces);

  /* get the current orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* create some panel widgets */
  i3_workspaces->ebox = gtk_event_box_new ();
  gtk_widget_show (i3_workspaces->ebox);

  i3_workspaces->hvbox = xfce_hvbox_new (orientation, FALSE, 2);
  gtk_widget_show (i3_workspaces->hvbox);
  gtk_container_add (GTK_CONTAINER (i3_workspaces->ebox), i3_workspaces->hvbox);

  /* some i3_workspaces widgets */
  label = gtk_label_new (_("Sample"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (i3_workspaces->hvbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (_("Plugin"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (i3_workspaces->hvbox), label, FALSE, FALSE, 0);

  return i3_workspaces;
}



static void
i3_workspaces_free (XfcePanelPlugin *plugin,
             SamplePlugin    *i3_workspaces)
{
  GtkWidget *dialog;

  /* check if the dialog is still open. if so, destroy it */
  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  /* destroy the panel widgets */
  gtk_widget_destroy (i3_workspaces->hvbox);

  /* cleanup the settings */
  if (G_LIKELY (i3_workspaces->setting1 != NULL))
    g_free (i3_workspaces->setting1);

  /* free the plugin structure */
  panel_slice_free (SamplePlugin, i3_workspaces);
}



static void
i3_workspaces_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            SamplePlugin    *i3_workspaces)
{
  /* change the orienation of the box */
  xfce_hvbox_set_orientation (XFCE_HVBOX (i3_workspaces->hvbox), orientation);
}



static gboolean
i3_workspaces_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     SamplePlugin    *i3_workspaces)
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



static void
i3_workspaces_construct (XfcePanelPlugin *plugin)
{
  SamplePlugin *i3_workspaces;

  /* setup transation domain */
  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

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

  /* show the configure menu item and connect signal */
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                    G_CALLBACK (i3_workspaces_configure), i3_workspaces);

  /* show the about menu item and connect signal */
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about",
                    G_CALLBACK (i3_workspaces_about), NULL);
}
