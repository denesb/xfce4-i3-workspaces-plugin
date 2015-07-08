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
static void
destroy_workspace(i3workspace *workspace);
i3workspace *
lookup_workspace(GSList *workspaces, const gchar *name);
gint
compare_workspaces(const i3workspace *a, const i3workspace *b);

static void
init_workspaces(i3windowManager *i3wm, GError **err);
static void
subscribe_to_events(i3windowManager *i3w, GError **err);

/*
 * Workspace event handlers
 */
static void
on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3w);
static void
on_focus_workspace(i3windowManager *i3w, i3ipcCon *current, i3ipcCon *old);
static void
on_init_workspace(i3windowManager *i3w);
static void
on_empty_workspace(i3windowManager *i3w);
static void
on_urgent_workspace(i3windowManager *i3w);
static void
on_rename_workspace(i3windowManager *i3w);

static void
on_ipc_shutdown_proxy(i3ipcConnection *connection, gpointer i3w);

/*
 * Implementations of public functions
 */

/**
 * i3wm_construct:
 * @err: The error object
 *
 * Construct the i3 windowmanager delegate struct.
 */
i3windowManager * i3wm_construct(GError **err)
{
    i3windowManager *i3wm = g_new0(i3windowManager, 1);
    GError *tmp_err = NULL;

    i3wm->connection = i3ipc_connection_new(NULL, &tmp_err);
    if (tmp_err != NULL)
    {
        g_propagate_error(err, tmp_err);
        g_free(i3wm);
        return NULL;
    }

    g_signal_connect(i3wm->connection, "ipc-shutdown", G_CALLBACK(on_ipc_shutdown_proxy), i3wm);

    i3wm->wlist = NULL;

    i3wm->on_workspace_created = NULL;
    i3wm->on_workspace_destroyed = NULL;
    i3wm->on_workspace_blurred = NULL;
    i3wm->on_workspace_focused = NULL;
    i3wm->on_workspace_urgent = NULL;
    i3wm->on_ipc_shutdown = NULL;

    init_workspaces(i3wm, &tmp_err);
    if(tmp_err != NULL)
    {
        g_propagate_error(err, tmp_err);
        i3wm_destruct(i3wm);
        return NULL;
    }

    subscribe_to_events(i3wm, &tmp_err);
    if (tmp_err != NULL)
    {
        g_propagate_error(err, tmp_err);
        i3wm_destruct(i3wm);
        return NULL;
    }

    return i3wm;
}

/**
 * i3wm_destruct:
 * @i3wm: the window manager delegate struct
 * Destruct the i3 windowmanager delegate struct.
 */
void
i3wm_destruct(i3windowManager *i3wm)
{
    g_object_unref(i3wm->connection);

    g_slist_free_full(i3wm->wlist, (GDestroyNotify) destroy_workspace);

    g_free(i3wm->wlist);
    g_free(i3wm);
}

/**
 * i3wm_get_workspaces:
 * @i3wm: the window manager delegate struct
 * Returns the workspaces array.
 *
 * Returns: GSList* of i3workspace*
 */
GSList *
i3wm_get_workspaces(i3windowManager *i3wm)
{
    return i3wm->wlist;
}

/**
 * i3wm_set_workspace_created_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace created callback.
 */
void
i3wm_set_workspace_created_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
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
void
i3wm_set_workspace_destroyed_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
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
void
i3wm_set_workspace_blurred_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
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
void
i3wm_set_workspace_focused_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
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
void
i3wm_set_workspace_urgent_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
{
    i3wm->on_workspace_urgent = callback;
    i3wm->on_workspace_urgent_data = data;
}

/**
 * i3wm_set_workspace_renamed_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace renamed callback.
 */
void
i3wm_set_workspace_renamed_callback(i3windowManager *i3wm, i3wmEventCallback callback, gpointer data)
{
    i3wm->on_workspace_renamed = callback;
    i3wm->on_workspace_renamed_data = data;
}

/**
 * i3wm_set_ipch_shutdown_callback:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the ipc shutdown event callback.
 */
void
i3wm_set_ipch_shutdown_callback(i3windowManager *i3wm,
        i3wmIpcShutdownCallback callback, gpointer data)
{
    i3wm->on_ipc_shutdown = callback;
    i3wm->on_ipc_shutdown_data = data;
}

/**
 * i3wm_goto_workspace:
 * @i3wm: the window manager delegate struct
 * @workspace: the workspace to jump to
 *
 * Instruct the window manager to jump to the specified workspace.
 */
void
i3wm_goto_workspace(i3windowManager *i3wm, i3workspace *workspace, GError **err)
{
    size_t cmd_size = (13 + strlen(workspace->name)) * sizeof(gchar);
    gchar *command_str = (gchar *) malloc(cmd_size);
    memset(command_str, 0, cmd_size);
    sprintf(command_str, "workspace %s", workspace->name);

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
i3workspace *
create_workspace(i3ipcWorkspaceReply * workspaceReply)
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
void
destroy_workspace(i3workspace *workspace)
{
    g_free(workspace->name);
    g_free(workspace);
}

/*
 * lookup_workspace:
 * @list - GSList of i3workspace *
 * @name - the name of the workspace to be looked up
 *
 * Look up a workspace by its name.
 * Returns: i3workspace *
 */
i3workspace *
lookup_workspace(GSList *list, const gchar *name)
{
    i3workspace *the_workspace = NULL;

    for (; list != NULL; list = list->next)
    {
        i3workspace *workspace = (i3workspace *) list->data;
        if (strcmp(name, workspace->name) == 0)
        {
            the_workspace = workspace;
            break;
        }
    }

    return the_workspace;
}

/*
 * compare_workspaces:
 * @a - i3workspace *
 * @b - i3workspace *
 *
 * Compare the two workspaces: return -1 if a < b, 1 if a > b, 0 if a = b
 * Returns: gint
 */
gint
compare_workspaces(const i3workspace *a, const i3workspace *b)
{
    if (a->num < b->num)
        return -1;
    else if (a->num > b->num)
        return 1;
    else
        return strcmp(a->name, b->name);
}

/**
 * init_workspaces:
 * @i3wm: the window manager delegate struct
 * @err: the error object
 *
 * Initialize the workspace list.
 */
void
init_workspaces(i3windowManager *i3wm, GError **err)
{
    GError *get_err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, &get_err);

    if (get_err != NULL)
    {
        g_propagate_error(err, get_err);
        return;
    }

    GSList *list_item;
    for (list_item = workspacesList; list_item != NULL; list_item = list_item->next)
    {
        i3workspace *workspace = create_workspace((i3ipcWorkspaceReply *) list_item->data);
        i3wm->wlist = g_slist_prepend(i3wm->wlist, workspace);
    }

    i3wm->wlist = g_slist_reverse(i3wm->wlist);

    g_slist_free_full(workspacesList, (GDestroyNotify) i3ipc_workspace_reply_free);
}

/**
 * subscribe_to_events:
 * @i3wm: the window manager delegate struct
 * @err: the error object
 *
 * Subscribe to the workspace events.
 */
void
subscribe_to_events(i3windowManager *i3wm, GError **err)
{
    GError *ipc_err = NULL;
    i3ipcCommandReply *reply = NULL;

    reply = i3ipc_connection_subscribe(i3wm->connection, I3IPC_EVENT_WORKSPACE, &ipc_err);
    if (ipc_err != NULL)
    {
        g_propagate_error(err, ipc_err);
        return;
    }

    g_signal_connect_after(i3wm->connection, "workspace", G_CALLBACK(on_workspace_event), i3wm);

    i3ipc_command_reply_free(reply);
}

/**
 * on_workspace_event:
 * @conn: the connection with the window manager
 * @e: event data
 * @i3wm: the window manager delegate struct
 *
 * The workspace event callback.
 */
void
on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer i3wm)
{
    if (strncmp(e->change, "focus", 5) == 0) on_focus_workspace((i3windowManager *) i3wm, e->current, e->old);
    else if (strncmp(e->change, "init", 5) == 0) on_init_workspace((i3windowManager *) i3wm);
    else if (strncmp(e->change, "empty", 5) == 0) on_empty_workspace((i3windowManager *) i3wm);
    else if (strncmp(e->change, "urgent", 6) == 0) on_urgent_workspace((i3windowManager *) i3wm);
    else if (strncmp(e->change, "rename", 6) == 0) on_rename_workspace((i3windowManager *) i3wm);
    else g_printf("Unknown event: %s\n", e->change);
}

/**
 * on_focus_workspace:
 * @i3wm: the window manager delegate struct
 * @current: the currently focused workspace
 * @old: the previously focused workspace
 *
 * Focus workspace event handler.
 */
void
on_focus_workspace(i3windowManager *i3wm, i3ipcCon *current, i3ipcCon *old)
{
    const gchar *const blurredName = i3ipc_con_get_name(old);
    const gchar *const focusedName = i3ipc_con_get_name(current);

    i3workspace * blurredWorkspace = lookup_workspace(i3wm->wlist, blurredName);
    if (blurredWorkspace) // this will be NULL in case of the scratch workspace
    {
        blurredWorkspace->focused = FALSE;
        if (i3wm->on_workspace_blurred)
            i3wm->on_workspace_blurred(blurredWorkspace, i3wm->on_workspace_blurred_data);
    }

    i3workspace * focusedWorkspace = lookup_workspace(i3wm->wlist, focusedName);
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
void
on_init_workspace(i3windowManager *i3wm)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
        if (NULL == lookup_workspace(i3wm->wlist, workspaceReply->name))
        {
            i3workspace *workspace = create_workspace(workspaceReply);
            i3wm->wlist = g_slist_insert_sorted(i3wm->wlist, workspace, (GCompareFunc)compare_workspaces);

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
void
on_empty_workspace(i3windowManager *i3wm)
{
    // get the updated worksace list from the window manager
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);

    GSList *old_item = i3wm->wlist;
    GSList *new_item = workspacesList;

    // find the removed workspace
    while (old_item != NULL)
    {
        i3workspace *wo = (i3workspace *)old_item->data;
        i3workspace *wn = (i3workspace *)new_item->data;

        if (wn == NULL || strcmp(wo->name, wn->name))
        {
            if (i3wm->on_workspace_destroyed)
                i3wm->on_workspace_destroyed(wo, i3wm->on_workspace_destroyed_data);

            i3wm->wlist = g_slist_remove(i3wm->wlist, old_item->data);
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
 * This can mean two thigs: either a workspace became urgent or it was urgent and
 * not it isn't.
 */
void
on_urgent_workspace(i3windowManager *i3wm)
{
    // get the updated workspace list from the window manager
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);

    GSList *their_item = workspacesList;
    GSList *our_item = i3wm->wlist;

    // find the workspace whoose urgen flag has changed
    while (their_item != NULL)
    {
        i3ipcWorkspaceReply * their_workspace = (i3ipcWorkspaceReply *) their_item->data;
        i3workspace *our_workspace = (i3workspace *) our_item->data;

        if (their_workspace->urgent != our_workspace->urgent)
        {
            our_workspace->urgent = their_workspace->urgent;
            if (i3wm->on_workspace_urgent)
                i3wm->on_workspace_urgent(our_workspace, i3wm->on_workspace_urgent_data);
        }
    }

    g_slist_free_full(workspacesList, (GDestroyNotify) i3ipc_workspace_reply_free);
}

/**
 * on_rename_workspace:
 * @i3wm: the window manager delegate struct
 *
 * Renamed workspace event handler.
 */
void
on_rename_workspace(i3windowManager *i3wm)
{
    // get the updated workspace list from the window manager
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(i3wm->connection, err);

    GSList *their_item = workspacesList;
    GSList *our_item = i3wm->wlist;

    // find the workspace whose name has changed
    while (our_item != NULL)
    {
        i3ipcWorkspaceReply *their_workspace = (i3ipcWorkspaceReply *) their_item->data;
        i3workspace *our_workspace = (i3workspace *) our_item->data;

        if (strcmp(our_workspace->name, their_workspace->name) != 0)
        {
            g_free(our_workspace->name);
            our_workspace->name = g_strdup(their_workspace->name);

            if (i3wm->on_workspace_renamed)
                i3wm->on_workspace_renamed(our_workspace, i3wm->on_workspace_renamed_data);

            break;
        }
    }

    g_slist_free_full(workspacesList, (GDestroyNotify) i3ipc_workspace_reply_free);
}

/**
 * on_ipc_shutdown_proxy:
 * @connection: the ipc connection object
 * @i3wm - pointer to the window manager delegate struct
 *
 * Passes the ipc-shutdown signal to the callback
 */
static void
on_ipc_shutdown_proxy(i3ipcConnection *connection, gpointer i3w)
{
    i3windowManager *i3wm = (i3windowManager *) i3w;
    if (i3wm->on_ipc_shutdown)
        i3wm->on_ipc_shutdown(i3wm->on_ipc_shutdown_data);
}
