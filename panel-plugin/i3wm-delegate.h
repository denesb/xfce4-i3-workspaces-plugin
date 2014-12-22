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

#ifndef __I3W_DELEGATE_H__
#define __I3W_DELEGATE_H__

#define I3_WORKSPACE_N 11

#include <i3ipc-glib/i3ipc-glib.h>

typedef struct _i3workspace
{
    gint num;
    gchar *name;
    gboolean focused;
    gboolean urgent;
} i3workspace;

typedef void (*i3wm_event_callback) (i3workspace *workspace, gpointer data);

typedef struct _i3windowManager
{
    i3ipcConnection *connection;
    i3workspace **workspaces;

    i3wm_event_callback on_workspace_created;
    i3wm_event_callback on_workspace_destroyed;
    i3wm_event_callback on_workspace_blurred;
    i3wm_event_callback on_workspace_focused;
    i3wm_event_callback on_workspace_urgent;

    gpointer on_workspace_created_data;
    gpointer on_workspace_destroyed_data;
    gpointer on_workspace_blurred_data;
    gpointer on_workspace_focused_data;
    gpointer on_workspace_urgent_data;
}
i3windowManager;


i3windowManager * i3wm_construct();

void i3wm_destruct();

i3workspace ** i3wm_get_workspaces(i3windowManager *i3wm);

void i3wm_set_workspace_created_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data);

void i3wm_set_workspace_destroyed_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data);

void i3wm_set_workspace_blurred_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data);

void i3wm_set_workspace_focused_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data);

void i3wm_set_workspace_urgent_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data);

void i3wm_goto_workspace(i3windowManager *i3wm, gint workspace_num);

#endif /* !__I3W_DELEGATE_H__ */
