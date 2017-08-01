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

long
ws_name_to_number(const char *name);
static gint
workspace_name_cmp(const gchar *a, const gchar *b);
static gint
workspace_reply_cmp(const i3ipcWorkspaceReply *a, const i3ipcWorkspaceReply *b);
static gint
workspace_reply_str_cmp(const i3ipcWorkspaceReply *w, const gchar *s);
static gint
workspace_str_cmp(const i3workspace *w, const gchar *s);

static void
init_workspaces(i3windowManager *i3wm, GError **err);
static void
subscribe_to_events(i3windowManager *i3w, GError **err);

static void
invoke_callback(const i3wmCallback callback);

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
on_move_workspace(i3windowManager *i3w);

/*
 * Mode event handler
 */
static void
on_mode_event(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer i3w);

/*
 * Output event handler
 */
static void
on_output_event(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer i3w);

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
i3windowManager *
i3wm_construct(GError **err)
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

    i3wm->on_workspace_created.function = NULL;
    i3wm->on_workspace_destroyed.function = NULL;
    i3wm->on_workspace_blurred.function = NULL;
    i3wm->on_workspace_focused.function = NULL;
    i3wm->on_workspace_urgent.function = NULL;
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

/*
 * i3wm_workspace_cmp:
 * @a - i3workspace *
 * @b - i3workspace *
 *
 * Compare the two workspaces: return -1 if a < b, 1 if a > b, 0 if a = b
 * Returns: gint
 */
gint
i3wm_workspace_cmp(const i3workspace *a, const i3workspace *b)
{
    return workspace_name_cmp(a->name, b->name);
}

/**
 * i3wm_set_on_workspace_created:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace created callback.
 */
void
i3wm_set_on_workspace_created(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_created.function = callback;
    i3wm->on_workspace_created.data = data;
}

/**
 * i3wm_set_on_workspace_destroyed:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace destroyed callback.
 */
void
i3wm_set_on_workspace_destroyed(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_destroyed.function = callback;
    i3wm->on_workspace_destroyed.data = data;
}

/**
 * i3wm_set_on_workspace_blurred:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace blurred callback.
 */
void
i3wm_set_on_workspace_blurred(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_blurred.function = callback;
    i3wm->on_workspace_focused.data = data;
}

/**
 * i3wm_set_on_workspace_focused:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace focused callback.
 */
void
i3wm_set_on_workspace_focused(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_focused.function = callback;
    i3wm->on_workspace_blurred.data = data;
}

/**
 * i3wm_set_on_workspace_urgent:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace urgent callback.
 */
void
i3wm_set_on_workspace_urgent(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_urgent.function = callback;
    i3wm->on_workspace_urgent.data = data;
}

/**
 * i3wm_set_on_workspace_renamed:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the workspace renamed callback.
 */
void
i3wm_set_on_workspace_renamed(i3windowManager *i3wm, i3wmWorkspaceCallback callback, gpointer data)
{
    i3wm->on_workspace_renamed.function = callback;
    i3wm->on_workspace_renamed.data = data;
}

/**
 * i3wm_set_on_mode_changed:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the binding mode changed callback.
 */
void
i3wm_set_on_mode_changed(i3windowManager *i3wm, i3wmModeCallback_fun callback, gpointer data)
{
    i3wm->on_mode_changed.function = callback;
    i3wm->on_mode_changed.data = data;
}

/**
 * i3wm_set_on_output_changed
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the binding output changed callback.
 */
void
i3wm_set_on_output_changed(i3windowManager *i3wm, i3wmOutputCallback_fun callback, gpointer data)
{
    i3wm->on_output_changed.function = callback;
    i3wm->on_output_changed.data = data;
}

/**
 * i3wm_set_ipc_shutdown:
 * @i3wm: the window manager delegate struct
 * @callback: the callback
 * @data: the data to be passed to the callback function
 *
 * Set the ipc shutdown event callback.
 */
void
i3wm_set_on_ipc_shutdown(i3windowManager *i3wm,
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
create_workspace(i3ipcWorkspaceReply * wreply)
{
    i3workspace *workspace = (i3workspace *) g_malloc(sizeof(i3workspace));

    workspace->num = wreply->num;
    workspace->name = g_strdup(wreply->name);
    workspace->focused = wreply->focused;
    workspace->visible = wreply->visible;
    workspace->urgent = wreply->urgent;
    workspace->output = g_strdup(wreply->output);

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
    g_free(workspace->output);
    g_free(workspace);
}

/*
 * ws_name_to_number:
 * @name - char *
 *
 * Parses the workspace name as a number. Returns -1 if the workspace should be
 * interpreted as a "named workspace".
 * This function was copied verbatim from the i3 source tree, from src/util.c
 *
 * Returns: long
 */
long
ws_name_to_number(const char *name) {
    /* positive integers and zero are interpreted as numbers */
    char *endptr = NULL;
    long parsed_num = strtol(name, &endptr, 10);
    if (parsed_num == LONG_MIN ||
        parsed_num == LONG_MAX ||
        parsed_num < 0 ||
        endptr == name) {
        parsed_num = -1;
    }

    return parsed_num;
}

/*
 * workspace_name_cmp:
 * @a - gchar *
 * @b - gchar *
 *
 * Compare two workspace names.
 * Return -x if a > b, x if a < b and 0 if a == b, where x is an arbitrary
 * natural number.
 * Returns: gint
 */
static gint
workspace_name_cmp(const gchar *a, const gchar *b)
{
    gint result;
    long na, nb;

    na = ws_name_to_number(a);
    nb = ws_name_to_number(b);

    if (na == -1 || nb == -1)
    {
        if (na == -1 && nb == -1)
            result = strcmp(b, a);
        else
            result = na == -1 ? -1 : 1;
    }
    else
    {
        result = nb - na;
    }

    return result;
}

/*
 * workspace_reply_cmp:
 * @a - i3ipcWorkspaceReply *
 * @b - i3ipcWorkspaceReply *
 *
 * Compare the two workspaces replies by name
 * Returns: gint
 */
static gint
workspace_reply_cmp(const i3ipcWorkspaceReply *a, const i3ipcWorkspaceReply *b)
{
    return workspace_name_cmp(a->name, b->name);
}

/*
 * workspace_reply_str_cmp:
 * @w - i3ipcWorkspaceReply *
 * @s - gchar *
 *
 * Compare workspace reply's name with the string
 * Returns: gint
 */
static gint
workspace_reply_str_cmp(const i3ipcWorkspaceReply *w, const gchar *s)
{
    return workspace_name_cmp(w->name, s);
}

/*
 * workspace_str_cmp:
 * @w - i3workspace *
 * @s - gchar *
 *
 * Compare workspace's name with the string
 * Returns: gint
 */
static gint
workspace_str_cmp(const i3workspace *w, const gchar *s)
{
    return workspace_name_cmp(w->name, s);
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
    if (i3wm->wlist) {
        g_slist_free_full(i3wm->wlist, (GDestroyNotify) destroy_workspace);
    }

    i3wm->wlist = NULL;

    GError *get_err = NULL;
    GSList *wlist = i3ipc_connection_get_workspaces(i3wm->connection, &get_err);

    if (get_err != NULL)
    {
        g_propagate_error(err, get_err);
        return;
    }

    GSList *witem;
    for (witem = wlist; witem != NULL; witem = witem->next)
    {
        i3workspace *workspace = create_workspace((i3ipcWorkspaceReply *) witem->data);
        i3wm->wlist = g_slist_prepend(i3wm->wlist, workspace);
    }

    i3wm->wlist = g_slist_reverse(i3wm->wlist);
    i3wm->wlist = g_slist_sort(i3wm->wlist, (GCompareFunc) i3wm_workspace_cmp);

    g_slist_free_full(wlist, (GDestroyNotify) i3ipc_workspace_reply_free);
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

	// subscribe to workspace changes
    reply = i3ipc_connection_subscribe(i3wm->connection, I3IPC_EVENT_WORKSPACE, &ipc_err);
    if (ipc_err != NULL)
    {
        g_propagate_error(err, ipc_err);
        return;
    }

	// subscribe to mode changes
    reply = i3ipc_connection_subscribe(i3wm->connection, I3IPC_EVENT_MODE, &ipc_err);
    if (ipc_err != NULL)
    {
        g_propagate_error(err, ipc_err);
        return;
    }

	// subscribe to output changes
    reply = i3ipc_connection_subscribe(i3wm->connection, I3IPC_EVENT_OUTPUT, &ipc_err);
    if (ipc_err != NULL)
    {
        g_propagate_error(err, ipc_err);
        return;
    }

    g_signal_connect_after(i3wm->connection, "workspace", G_CALLBACK(on_workspace_event), i3wm);
    g_signal_connect_after(i3wm->connection, "mode", G_CALLBACK(on_mode_event), i3wm);
    g_signal_connect_after(i3wm->connection, "output", G_CALLBACK(on_output_event), i3wm);

    i3ipc_command_reply_free(reply);
}

/**
 * invoke_callback
 * @callback: the i3wmCallback structure
 * @workspace: the workspace
 *
 * Invokes the specified callback with the workspace as parameter.
 */
static void
invoke_callback(const i3wmCallback callback)
{
    if (callback.function)
    {
        callback.function(callback.data);
    }
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
    else if (strncmp(e->change, "move", 4) == 0) on_move_workspace((i3windowManager *) i3wm);
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
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  invoke_callback(i3wm->on_workspace_focused);
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
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  invoke_callback(i3wm->on_workspace_created);
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
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  invoke_callback(i3wm->on_workspace_destroyed);
}

/**
 * on_urgent_workspace:
 * @i3wm: the window manager delegate struct
 *
 * Urgent workspace event handler.
 * This can mean two thigs: either a workspace became urgent or it was urgent and
 * now it isn't.
 */
void
on_urgent_workspace(i3windowManager *i3wm)
{
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  invoke_callback(i3wm->on_workspace_urgent);
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
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  // Since created already removes/adds all the worskpaces,
  // we don't need to also call the "destroyed" callback
  invoke_callback(i3wm->on_workspace_created);
}

/**
 * on_move_workspace:
 * @i3wm: the window manager delegate struct
 *
 * Moved workspace event handler.
 */
void
on_move_workspace(i3windowManager *i3wm)
{
  GError *tmp_err = NULL;
  init_workspaces(i3wm, &tmp_err);
  // Since created already removes/adds all the worskpaces,
  // we don't need to also call the "destroyed" callback
  invoke_callback(i3wm->on_workspace_created);
}

/**
 * on_mode_event:
 * @conn: the connection with the window manager
 * @e: event data
 * @i3w: the window manager delegate struct
 *
 * The binding mode event callback.
 */
void
on_mode_event(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer i3w) {
    i3windowManager *i3wm = (i3windowManager *) i3w;
    i3wm->on_mode_changed.function(e->change, i3wm->on_mode_changed.data);
}

/**
 * on_output_event:
 * @conn: the connection with the window manager
 * @e: event data
 * @i3w: the window manager delegate struct
 *
 * The output callback.
 */
void 
on_output_event(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer i3w) {
    i3windowManager *i3wm = (i3windowManager *) i3w;
    GError *tmp_err = NULL;
    init_workspaces(i3wm, &tmp_err);
    invoke_callback(i3wm->on_workspace_created);
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
