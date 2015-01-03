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

#ifndef I3W_OPTIONS_H
#define I3W_OPTIONS_H

#include <glib.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
    GdkColor *normal_color;
    GdkColor *urgent_color;
    gboolean strip_workspace_number;
}
i3WorkspacesOptions;

i3WorkspacesOptions * i3_workspaces_options_new();
void                  i3_workspaces_options_free(i3WorkspacesOptions *options);
gboolean              i3_workspaces_options_load(i3WorkspacesOptions *options, XfcePanelPlugin *plugin);
gboolean              i3_workspaces_options_save(i3WorkspacesOptions *options, XfcePanelPlugin *plugin);

#endif /* I3W_OPTIONS_H */
