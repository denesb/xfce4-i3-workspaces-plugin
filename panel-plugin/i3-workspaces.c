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

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>
#include <string.h>

#include "i3-workspaces.h"

typedef struct _i3workspace
{
    gint num;
    gchar *name;
    gboolean visible;
    gboolean focused;
    gboolean urgent;
    gchar *output;
    GtkWidget *button;
} i3workspace;

typedef struct _i3workspaces
{
    GtkWidget *parentBox;

    i3ipcConnection *connection;
    i3workspace *workspaces[WORKSPACE_N];

    gboolean workspaceInitialized;
    i3workspace *lastBlurredWorkspace;
}
i3workspaces;

/*
 * Prototypes
 */
static void init_workspaces(i3workspaces *i3w);
static i3workspace * create_workspace(i3ipcWorkspaceReply * workspaceReply);
static void destroy_workspace(i3workspace * workspace);
static void subscribe_to_events(i3workspaces *i3w);
static void add_workspace(i3workspaces *i3w, const gchar *const workspaceName);
static i3workspace * lookup_workspace(i3workspaces *i3w, const gchar *const workspaceName);

/*
 * Workspace event handlers
 */
static void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3w);
static void on_focus_workspace(i3workspaces *i3w, i3ipcCon *current, i3ipcCon *old);
static void on_init_workspace(i3workspaces *i3w);
static void on_empty_workspace(i3workspaces *i3w);
static void on_urgent_workspace(i3workspaces *i3w);

/*
 * GUI related functions
 */
static void gui_focus_workspace(i3workspaces *i3w, i3workspace *workspace);
static void gui_add_workspace(i3workspaces *i3w, i3workspace *workspace);
static void gui_remove_workspace(i3workspaces *i3w, i3workspace *workspace);

/*
 * Implementations
 */

void i3workspaces_init(GtkWidget *parent);
{
    i3workspaces *i3w;
    i3w = (i3workspaces *) g_malloc(sizeof(i3workspaces));

    i3w->parentBox = parent;

    i3w->connection = i3ipc_connection_new(NULL, NULL);

    int i;
    for (i = 0; i < WORKSPACE_N; i++)
    {
        i3w->workspaces[i] = NULL;
    }

    init_workspaces(i3w);
    subscribe_to_events(i3w);

    for (i = 1; i < WORKSPACE_N; i++)
    {
        i3workspace *workspace = i3w->workspaces[i];
        if (workspace) gui_add_workspace(i3w, workspace);
    }
}

i3workspace * create_workspace(i3ipcWorkspaceReply * workspaceReply)
{
    i3workspace *workspace = (i3workspace *) g_malloc(sizeof(i3workspace));

    workspace->num = workspaceReply->num;
    workspace->name = g_strdup(workspaceReply->name);
    workspace->visible = workspaceReply->visible;
    workspace->focused = workspaceReply->focused;
    workspace->urgent = workspaceReply->urgent;
    workspace->output = NULL; // Not supported yet
    workspace->button = gtk_button_new_with_label(workspace->name);

    gtk_button_set_use_underline(GTK_BUTTON(workspace->button), workspace->focused);

    return workspace;
}

void destroy_workspace(i3workspace *workspace)
{
    g_free(workspace->name);
    workspace->name = NULL;
    g_free(workspace);
    workspace = NULL;
}

void  init_workspaces(i3workspaces *i3w)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3w->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3workspace * workspace = create_workspace((i3ipcWorkspaceReply *) listItem->data);
        i3w->workspaces[workspace->num] = workspace;
    }
}

void subscribe_to_events(i3workspaces *i3w)
{
    GError **err = NULL;
    i3ipcCommandReply *reply = NULL;

    reply = i3ipc_connection_subscribe(i3w->connection, I3IPC_EVENT_WORKSPACE, err);

    g_signal_connect_after(i3w->connection, "workspace", G_CALLBACK(on_workspace_event), i3w);
}

void add_workspace(i3workspaces *i3w, const gchar *const workspaceName)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3w->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
         i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
         if (NULL == i3w->workspaces[workspaceReply->num])
         {
             i3workspace *workspace = create_workspace(workspaceReply);
             g_printf("Adding workspace #%i %s\n", workspace->num, workspace->name);
             i3w->workspaces[workspace->num] = workspace;
             gui_add_workspace(i3w, workspace);
         }
    }
}

i3workspace * lookup_workspace(i3workspaces *i3w, const gchar *const workspaceName)
{
    i3workspace *the_workspace = NULL;
    int i;
    for (i = 1; i < WORKSPACE_N; i++)
    {
        i3workspace *workspace = i3w->workspaces[i];
        if (workspace && 0 == strcmp(workspaceName, workspace->name))
        {
            the_workspace = workspace;
        }
    }

    return the_workspace;
}

void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3w)
{
    if (0 == strncmp(e->change, "focus", 5)) on_focus_workspace((i3workspaces *) i3w, e->current, e->old);
    else if (0 == strncmp(e->change, "init", 5)) on_init_workspace((i3workspaces *) i3w);
    else if (0 == strncmp(e->change, "empty", 5)) on_empty_workspace((i3workspaces *) i3w);
    else if (0 == strncmp(e->change, "urgent", 6)) on_urgent_workspace((i3workspaces *) i3w);
    else g_printf("Unknown event: %s\n", e->change);
}

void on_focus_workspace(i3workspaces *i3w, i3ipcCon *current, i3ipcCon *old)
{
    g_printf("focus ");
    if (old)
    {
        g_printf("%s -> ", i3ipc_con_get_name(old));
    }
    else
    {
        g_printf("NULL -> ");
    }
    g_printf("%s\n", i3ipc_con_get_name(current));

    i3w->lastBlurredWorkspace = lookup_workspace(i3w, i3ipc_con_get_name(old));
    const gchar *const name = i3ipc_con_get_name(current);

    if (i3w->workspaceInitialized)
    {

        g_printf("add works\n");
        add_workspace(i3w, name);
        i3w->workspaceInitialized = FALSE;
    }

    i3workspace *workspace = lookup_workspace(i3w, name);

    gui_focus_workspace(i3w, workspace);
}

void on_init_workspace(i3workspaces *i3w)
{
    g_printf("init\n");
    i3w->workspaceInitialized = TRUE;
}

void on_empty_workspace(i3workspaces *i3w)
{
    g_printf("empty\n");
    i3workspace *workspace = i3w->lastBlurredWorkspace;

    i3w->workspaces[workspace->num] = NULL; 
    gui_remove_workspace(i3w, workspace);
    destroy_workspace(workspace);
    i3w->lastBlurredWorkspace = NULL;
}

void on_urgent_workspace(i3workspaces *i3w)
{
    //TODO
}

void gui_focus_workspace(i3workspaces *i3w, i3workspace *workspace)
{
}

void gui_add_workspace(i3workspaces *i3w, i3workspace *workspace)
{
    gtk_box_pack_start(GTK_BOX(i3w->parentBox), workspace->button, FALSE, FALSE, 10);
    gtk_widget_show(workspace->button);
}

void gui_remove_workspace(i3workspaces *i3w, i3workspace *workspace)
{
    gtk_widget_hide(workspace->button);
    gtk_widget_destroy(workspace->button);
    workspace->button = NULL;
}
