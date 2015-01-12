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

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>
#include <stdlib.h>
#include <string.h>

#include "i3wm-delegate.h"

/*
 * Prototypes
 */
static void init_workspaces(i3windowManager *i3wm, GError **err);
static i3workspace * create_workspace(i3ipcWorkspaceReply * workspaceReply);
static void destroy_workspace(i3workspace * workspace);
static void subscribe_to_events(i3windowManager *i3w, GError **err);
static i3workspace * lookup_workspace(i3windowManager *i3w, const gchar *const workspaceName);

/*
 * Workspace event handlers
 */
static void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3w);
static void on_focus_workspace(i3windowManager *i3w, i3ipcCon *current, i3ipcCon *old);
static void on_init_workspace(i3windowManager *i3w);
static void on_empty_workspace(i3windowManager *i3w);
static void on_urgent_workspace(i3windowManager *i3w);

static void on_ipc_shutdown(i3windowManager *i3wm);

/*
 * Implementations of public functions
 */

/**
 * i3wm_construct:
 * Construct the i3 windowmanager delegate struct.
 */
i3windowManager * i3wm_construct(GError **err)
{
    i3windowManager *i3wm = g_new0(i3windowManager, 1);
    GError *connection_err = NULL;

    i3wm->connection = i3ipc_connection_new(NULL, &connection_err);
    if (NULL != connection_err)
    {
        g_propagate_error(err, connection_err);
        g_free(i3wm);
        return NULL;
    }

    g_signal_connect(i3wm->connection, "ipc-shutdown", G_CALLBACK(on_ipc_shutdown), i3wm);

    i3wm->workspaces = (i3workspace **) g_malloc0(I3_WORKSPACE_N * sizeof(i3workspace *));

    i3wm->on_workspace_created = NULL;
    i3wm->on_workspace_destroyed = NULL;
    i3wm->on_workspace_blurred = NULL;
    i3wm->on_workspace_focused = NULL;
    i3wm->on_workspace_urgent = NULL;

    GError *init_err = NULL;
    init_workspaces(i3wm, &init_err);
    if(NULL != init_err)
    {
        g_propagate_error(err, init_err);
        g_object_unref(i3wm->connection);
        g_free(i3wm->workspaces);
        g_free(i3wm);
        return NULL;
    }

    GError *subscribe_err = NULL;
    subscribe_to_events(i3wm, &subscribe_err);
    if (NULL != subscribe_err)
    {
        g_propagate_error(err, subscribe_err);
        g_object_unref(i3wm->connection);
        //TODO: free every workspace
        g_free(i3wm->workspaces);
        g_free(i3wm);
        return NULL;
    }

    return i3wm;
}

/**
 * i3wm_destruct:
 * @i3wm: the window manager delegate struct
 * Destruct the i3 windowmanager delegate struct.
 */
void i3wm_destruct(i3windowManager *i3wm)
{
    g_object_unref(i3wm->connection);

    int i;
    for (i = 0; i < I3_WORKSPACE_N; i++)
    {
        destroy_workspace(i3wm->workspaces[i]);
    }

    g_free(i3wm->workspaces);
}

/**
 * i3wm_get_workspaces:
 * @i3wm: the window manager delegate struct
 * Returns the workspaces array.
 *
 * Returns: i3workspace**
 */
i3workspace ** i3wm_get_workspaces(i3windowManager *i3wm)
{
    return i3wm->workspaces;
}

/**
 * i3wm_set_workspace_created_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace created callback.
 */
void i3wm_set_workspace_created_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data)
{
    i3wm->on_workspace_created = callback;
    i3wm->on_workspace_created_data = data;
}

/**
 * i3wm_set_workspace_destroyed_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace destroyed callback.
 */
void i3wm_set_workspace_destroyed_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data)
{
    i3wm->on_workspace_destroyed = callback;
    i3wm->on_workspace_destroyed_data = data;
}

/**
 * i3wm_set_workspace_blurred_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace blurred callback.
 */
void i3wm_set_workspace_blurred_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data)
{
    i3wm->on_workspace_blurred = callback;
    i3wm->on_workspace_focused_data = data;
}

/**
 * i3wm_set_workspace_focused_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace focused callback.
 */
void i3wm_set_workspace_focused_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data)
{
    i3wm->on_workspace_focused = callback;
    i3wm->on_workspace_blurred_data = data;
}

/**
 * i3wm_set_workspace_urgent_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace urgent callback.
 */
void i3wm_set_workspace_urgent_callback(i3windowManager *i3wm, i3wm_event_callback callback, gpointer data)
{
    i3wm->on_workspace_urgent = callback;
    i3wm->on_workspace_urgent_data = data;
}

/**
 * i3wm_goto_workspace:
 * @i3wm: the window manager delegate struct
 * @workspace_num: the index of the workspace to jump to
 *
 * Instruct the window manager to jump to the specified workspace.
 */
void i3wm_goto_workspace(i3windowManager *i3wm, gint workspace_num, GError **err)
{
    size_t cmd_size = 13 * sizeof(gchar);
    gchar *command_str = (gchar *) malloc(cmd_size);
    memset(command_str, 0, cmd_size);
    sprintf(command_str, "workspace %i", workspace_num);

    GError *ipc_err = NULL;

    gchar *reply = i3ipc_connection_message(
            i3wm->connection,
            I3IPC_MESSAGE_TYPE_COMMAND,
            command_str,
            &ipc_err);

    if (ipc_err != NULL)
    {
        g_propagate_error(err, ipc_err);
    }
    else
    {
        g_free(reply);
    }
}

/*
 * Implementations of private functions
 */

/**
 * create_workspace:
 * @workspaceReply: the i3ipcWorkspaceReply struct
 *
 * Create a i3workspace struct from the i3ipcWorkspaceReply struct
 *
 * Returns: the created workspace
 */
i3workspace * create_workspace(i3ipcWorkspaceReply * workspaceReply)
{
    i3workspace *workspace = (i3workspace *) g_malloc(sizeof(i3workspace));

    workspace->num = workspaceReply->num;
    workspace->name = g_strdup(workspaceReply->name);
    workspace->focused = workspaceReply->focused;
    workspace->urgent = workspaceReply->urgent;

    return workspace;
}

/**
 * destroy_workspace:
 * @workspace: the workspace to destroy
 *
 * Destroys the workspace
 */
void destroy_workspace(i3workspace *workspace)
{
    g_free(workspace->name);
    g_free(workspace);
    workspace = NULL;
}

/**
 * init_workspaces:
 * @i3wm: the window manager delegate struct
 *
 * Initialize the workspace list.
 */
void init_workspaces(i3windowManager *i3wm, GError **err)
{
    GError *get_err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, &get_err);
    if (NULL != get_err)
    {
        g_propagate_error(err, get_err);
        return;
    }

    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3workspace * workspace = create_workspace((i3ipcWorkspaceReply *) listItem->data);
        i3wm->workspaces[workspace->num] = workspace;
    }

    g_slist_free_full(workspacesList, (GDestroyNotify)i3ipc_workspace_reply_free);
}

/**
 * subscribe_to_events:
 * @i3wm: the window manager delegate struct
 *
 * Subscribe to the workspace events.
 */
void subscribe_to_events(i3windowManager *i3wm, GError **err)
{
    GError *ipc_err = NULL;
    i3ipcCommandReply *reply = NULL;

    reply = i3ipc_connection_subscribe(i3wm->connection, I3IPC_EVENT_WORKSPACE, &ipc_err);
    if (NULL != ipc_err)
    {
        g_propagate_error(err, ipc_err);
        return;
    }

    g_signal_connect_after(i3wm->connection, "workspace", G_CALLBACK(on_workspace_event), i3wm);

    i3ipc_command_reply_free(reply);
}

/**
 * lookup_workspace:
 * @i3wm - the window manager delegate struct
 * @workspaceName - the name of the workspace to be looked up
 *
 * Lookup a workspace struct by its name.
 * Returns: an i3workspace*
 */
i3workspace * lookup_workspace(i3windowManager *i3wm, const gchar *const workspaceName)
{
    i3workspace *the_workspace = NULL;
    int i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        i3workspace *workspace = i3wm->workspaces[i];
        if (workspace && 0 == strcmp(workspaceName, workspace->name))
        {
            the_workspace = workspace;
        }
    }

    return the_workspace;
}

/**
 * on_workspace_event:
 * @conn: the connection with the window manager
 * @e: event data
 * @i3wm: the window manager delegate struct
 *
 * The workspace event callback.
 */
void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3wm)
{
    if (0 == strncmp(e->change, "focus", 5)) on_focus_workspace((i3windowManager *) i3wm, e->current, e->old);
    else if (0 == strncmp(e->change, "init", 5)) on_init_workspace((i3windowManager *) i3wm);
    else if (0 == strncmp(e->change, "empty", 5)) on_empty_workspace((i3windowManager *) i3wm);
    else if (0 == strncmp(e->change, "urgent", 6)) on_urgent_workspace((i3windowManager *) i3wm);
    else g_printf("Unknown event: %s\n", e->change);
}

/**
 * on-focus_workspace:
 * @i3wm: the window manager delegate struct
 * @current: the currently focused workspace
 * @old: the previously focused workspace
 *
 * Focus workspace event handler.
 */
void on_focus_workspace(i3windowManager *i3wm, i3ipcCon *current, i3ipcCon *old)
{
    const gchar *const blurredName = i3ipc_con_get_name(old);
    const gchar *const focusedName = i3ipc_con_get_name(current);

    i3workspace * blurredWorkspace = lookup_workspace(i3wm, blurredName);
    if (blurredWorkspace) // this will be NULL in case of the scratch workspace
    {
        blurredWorkspace->focused = FALSE;
        if (i3wm->on_workspace_blurred)
            i3wm->on_workspace_blurred(blurredWorkspace, i3wm->on_workspace_blurred_data);
    }

    i3workspace * focusedWorkspace = lookup_workspace(i3wm, focusedName);
    focusedWorkspace->focused = TRUE;
    if (i3wm->on_workspace_focused)
        i3wm->on_workspace_focused(focusedWorkspace, i3wm->on_workspace_focused_data);
}

/**
 * on_init_workspace:
 * @i3wm - the window manager delegate struct
 *
 * Init workspace event handler
 */
void on_init_workspace(i3windowManager *i3wm)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
        if (NULL == i3wm->workspaces[workspaceReply->num])
        {
            i3workspace *workspace = create_workspace(workspaceReply);
            i3wm->workspaces[workspace->num] = workspace;

            if (i3wm->on_workspace_created)
                i3wm->on_workspace_created(workspace, i3wm->on_workspace_created_data);
        }
    }

    g_slist_free_full(workspacesList, (GDestroyNotify)i3ipc_workspace_reply_free);
}

/**
 * on_empty_workspace:
 * @i3wm - the window manager delegate struct
 *
 * Empty workspace event handler
 */
void on_empty_workspace(i3windowManager *i3wm)
{
    i3workspace *workspace = NULL;

    i3ipcWorkspaceReply *workspaceReplies[I3_WORKSPACE_N];
    memset(workspaceReplies, 0, I3_WORKSPACE_N * sizeof(i3ipcWorkspaceReply *));

    // get the updated worksace list from the window manager
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
        workspaceReplies[workspaceReply->num] = workspaceReply;
    }

    // find the removed workspace
    int i;
    for (i = 1; i < I3_WORKSPACE_N; i++)
    {
        if (i3wm->workspaces[i] && NULL == workspaceReplies[i])
        {
            if (i3wm->on_workspace_destroyed)
                i3wm->on_workspace_destroyed(i3wm->workspaces[i], i3wm->on_workspace_destroyed_data);

            destroy_workspace(i3wm->workspaces[i]);
            i3wm->workspaces[i] = NULL; // remove the workspace
            break;
        }
    }

    g_slist_free_full(workspacesList, (GDestroyNotify)i3ipc_workspace_reply_free);
}

/**
 * on_urgent_workspace:
 * @i3wm: the window manager delegate struct
 *
 * Urgent workspace event handler.
 */
void on_urgent_workspace(i3windowManager *i3wm)
{
    // get the updated worksace list from the window manager
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
        if (workspaceReply->urgent)
        {
            i3workspace *workspace = i3wm->workspaces[workspaceReply->num];
            workspace->urgent = TRUE;
            if (i3wm->on_workspace_urgent)
                i3wm->on_workspace_urgent(workspace, i3wm->on_workspace_urgent_data);
        }
    }

    g_slist_free_full(workspacesList, (GDestroyNotify)i3ipc_workspace_reply_free);
}

static void on_ipc_shutdown(i3windowManager *i3wm)
{
    g_object_unref(i3wm->connection);

    gboolean connected= FALSE;

    while (!connected)
    {
        GError *err = NULL;
        i3wm->connection = i3ipc_connection_new(NULL, &err);

        if (NULL != err)
        {
            connected = FALSE;
            fprintf(stderr, "%s", err->message);
            g_error_free(err);
        }
        else
        {
            connected = TRUE;
        }
    }
}
