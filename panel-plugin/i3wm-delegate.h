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

#ifndef __I3W_DELEGATE_H__
#define __I3W_DELEGATE_H__

#define I3_WORKSPACE_N 11

typedef struct _i3workspace
{
    gint num;
    gchar *name;
    gboolean visible;
    gboolean focused;
    gboolean urgent;
    gchar *output;
} i3workspace;

typedef void (*i3wm_event_callback) (i3workspace *workspace);

typedef struct _i3windowManager
{
    i3ipcConnection *connection;
    i3workspace **workspaces;

    gboolean workspaceInitialized;
    i3workspace *lastBlurredWorkspace;

    i3wm_event_callback on_workspace_created;
    i3wm_event_callback on_workspace_destroyed;
    i3wm_event_callback on_workspace_blurred;
    i3wm_event_callback on_workspace_focused;
}
i3windowManager;


i3windowManager * i3wm_construct();

void i3wm_destruct();

i3workspace ** i3wm_get_workspaces(i3windowManager *i3wm);

void i3wm_set_workspace_created_callback(i3windowManager *i3wm, i3wm_event_callback callback);

void i3wm_set_workspace_destroyed_callback(i3windowManager *i3wm, i3wm_event_callback callback);

void i3wm_set_workspace_blurred_callback(i3windowManager *i3wm, i3wm_event_callback callback);

void i3wm_set_workspace_focused_callback(i3windowManager *i3wm, i3wm_event_callback callback);

#endif /* !__I3W_DELEGATE_H__ */
