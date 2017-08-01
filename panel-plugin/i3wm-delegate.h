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

#include <i3ipc-glib/i3ipc-glib.h>

typedef struct _i3workspace
{
    gint num;
    gchar *name;
    gboolean focused;
    gboolean urgent;
    gboolean visible;
    gchar *output;
} i3workspace;

typedef void (*i3wmWorkspaceCallback) (gpointer data);
typedef void (*i3wmModeCallback_fun) (gchar *mode, gpointer data);
typedef void (*i3wmOutputCallback_fun) (gchar *mode, gpointer data);
typedef void (*i3wmIpcShutdownCallback) (gpointer data);

typedef struct _i3wm_callback
{
    i3wmWorkspaceCallback function;
    gpointer data;
} i3wmCallback;

typedef struct _i3wm_mode_callback
{
    i3wmModeCallback_fun function;
    gpointer data;
} i3wmModeCallback;

typedef struct _i3wm_output_callback
{
    i3wmOutputCallback_fun function;
    gpointer data;
} i3wmOutputCallback;

typedef struct _i3windowManager
{
    i3ipcConnection *connection;
    GSList *wlist;

    i3wmCallback on_workspace_created;
    i3wmCallback on_workspace_destroyed;
    i3wmCallback on_workspace_blurred;
    i3wmCallback on_workspace_focused;
    i3wmCallback on_workspace_urgent;
    i3wmCallback on_workspace_renamed;
    i3wmModeCallback on_mode_changed;
    i3wmOutputCallback on_output_changed;
    i3wmIpcShutdownCallback on_ipc_shutdown;
    gpointer on_ipc_shutdown_data;
}
i3windowManager;


i3windowManager *
i3wm_construct(GError **err);

void
i3wm_destruct(i3windowManager *i3wm);

GSList *
i3wm_get_workspaces(i3windowManager *i3wm);

gint
i3wm_workspace_cmp(const i3workspace *a, const i3workspace *b);

void
i3wm_set_on_workspace_created(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_workspace_destroyed(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_workspace_blurred(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_workspace_focused(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_workspace_urgent(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_workspace_renamed(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data);

void
i3wm_set_on_mode_changed(i3windowManager *i3wm, i3wmModeCallback_fun callback, gpointer data);

void
i3wm_set_on_output_changed(i3windowManager *i3wm, i3wmOutputCallback_fun callback, gpointer data);

void
i3wm_set_on_ipc_shutdown(i3windowManager *i3wm, i3wmIpcShutdownCallback callback, gpointer data);

void
i3wm_goto_workspace(i3windowManager *i3wm, i3workspace *workspace, GError **err);

#endif /* !__I3W_DELEGATE_H__ */
