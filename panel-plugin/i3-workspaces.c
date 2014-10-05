#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>
#include <string.h>

#define WORKSPACE_N 11

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

typedef struct _i3workspacesPlugin
{
    GtkWidget *hbox;

    i3ipcConnection *conn;
    i3workspace *workspaces[WORKSPACE_N];

    gboolean workspaceInitialized;
    i3workspace *lastBlurredWorkspace;
}
i3workspacesPlugin;

static void init_plugin(i3workspacesPlugin *plugin);
static void init_workspaces(i3workspacesPlugin *plugin);
static i3workspace * create_workspace(i3ipcWorkspaceReply * workspaceReply);
static void destroy_workspace(i3workspace * workspace);
static void subscribe_to_events(i3workspacesPlugin *plugin);
static void add_workspace(i3workspacesPlugin *plugin, const gchar *const workspaceName);
static i3workspace * lookup_workspace(i3workspacesPlugin *plugin, const gchar *const workspaceName);

/*
 * Workspace event handlers
 */
static void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer plugin);
static void on_focus_workspace(i3workspacesPlugin *plugin, i3ipcCon *current, i3ipcCon *old);
static void on_init_workspace(i3workspacesPlugin *plugin);
static void on_empty_workspace(i3workspacesPlugin *plugin);
static void on_urgent_workspace(i3workspacesPlugin *plugin);

/*
 * GUI related functions
 */
static void gui_focus_workspace(i3workspacesPlugin *plugin, i3workspace *workspace);
static void gui_add_workspace(i3workspacesPlugin *plugin, i3workspace *workspace);
static void gui_remove_workspace(i3workspacesPlugin *plugin, i3workspace *workspace);

int main(int argc, char **argv)
{
    gtk_init (&argc, &argv);

    GtkWidget *window;
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    i3workspacesPlugin *plugin;
    plugin = (i3workspacesPlugin *) g_malloc(sizeof(i3workspacesPlugin));

    init_plugin(plugin);

    gtk_container_add(GTK_CONTAINER (window), plugin->hbox);
    gtk_widget_show(window);

    gtk_main();

    return 0;
}

static void init_plugin(i3workspacesPlugin *plugin)
{
    plugin->hbox = gtk_hbox_new(FALSE, 0);

    plugin->conn = i3ipc_connection_new(NULL, NULL);

    int i;
    for (i = 0; i < WORKSPACE_N; i++)
    {
        plugin->workspaces[i] = NULL;
    }

    init_workspaces(plugin);
    subscribe_to_events(plugin);

    for (i = 1; i < WORKSPACE_N; i++)
    {
        i3workspace *workspace = plugin->workspaces[i];
        if (workspace) gui_add_workspace(plugin, workspace);
    }

    gtk_widget_show(plugin->hbox);
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

void  init_workspaces(i3workspacesPlugin *plugin)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(plugin->conn, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
        i3workspace * workspace = create_workspace((i3ipcWorkspaceReply *) listItem->data);
        plugin->workspaces[workspace->num] = workspace;
    }
}

void subscribe_to_events(i3workspacesPlugin *plugin)
{
    GError **err = NULL;
    i3ipcCommandReply *reply = NULL;

    reply = i3ipc_connection_subscribe(plugin->conn, I3IPC_EVENT_WORKSPACE, err);

    g_signal_connect_after(plugin->conn, "workspace", G_CALLBACK(on_workspace_event), plugin);
}

void add_workspace(i3workspacesPlugin *plugin, const gchar *const workspaceName)
{
    GError **err = NULL;
    GSList *workspacesList = i3ipc_connection_get_workspaces(plugin->conn, err);
    GSList *listItem;
    for (listItem = workspacesList; listItem != NULL; listItem = listItem->next)
    {
         i3ipcWorkspaceReply * workspaceReply = (i3ipcWorkspaceReply *) listItem->data;
         if (NULL == plugin->workspaces[workspaceReply->num])
         {
             i3workspace *workspace = create_workspace(workspaceReply);
             g_printf("Adding workspace #%i %s\n", workspace->num, workspace->name);
             plugin->workspaces[workspace->num] = workspace;
             gui_add_workspace(plugin, workspace);
         }
    }
}

i3workspace * lookup_workspace(i3workspacesPlugin *plugin, const gchar *const workspaceName)
{
    i3workspace *the_workspace = NULL;
    int i;
    for (i = 1; i < WORKSPACE_N; i++)
    {
        i3workspace *workspace = plugin->workspaces[i];
        if (workspace && 0 == strcmp(workspaceName, workspace->name))
        {
            the_workspace = workspace;
        }
    }

    return the_workspace;
}

void on_workspace_event(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer plugin)
{
    if (0 == strncmp(e->change, "focus", 5)) on_focus_workspace((i3workspacesPlugin *) plugin, e->current, e->old);
    else if (0 == strncmp(e->change, "init", 5)) on_init_workspace((i3workspacesPlugin *) plugin);
    else if (0 == strncmp(e->change, "empty", 5)) on_empty_workspace((i3workspacesPlugin *) plugin);
    else if (0 == strncmp(e->change, "urgent", 6)) on_urgent_workspace((i3workspacesPlugin *) plugin);
    else g_printf("Unknown event: %s\n", e->change);
}

void on_focus_workspace(i3workspacesPlugin *plugin, i3ipcCon *current, i3ipcCon *old)
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

    plugin->lastBlurredWorkspace = lookup_workspace(plugin, i3ipc_con_get_name(old));
    const gchar *const name = i3ipc_con_get_name(current);

    if (plugin->workspaceInitialized)
    {

        g_printf("add works\n");
        add_workspace(plugin, name);
        plugin->workspaceInitialized = FALSE;
    }

    i3workspace *workspace = lookup_workspace(plugin, name);

    gui_focus_workspace(plugin, workspace);
}

void on_init_workspace(i3workspacesPlugin *plugin)
{
    g_printf("init\n");
    plugin->workspaceInitialized = TRUE;
}

void on_empty_workspace(i3workspacesPlugin *plugin)
{
    g_printf("empty\n");
    i3workspace *workspace = plugin->lastBlurredWorkspace;

    plugin->workspaces[workspace->num] = NULL; 
    gui_remove_workspace(plugin, workspace);
    destroy_workspace(workspace);
    plugin->lastBlurredWorkspace = NULL;
}

void on_urgent_workspace(i3workspacesPlugin *plugin)
{
    //TODO
}

void gui_focus_workspace(i3workspacesPlugin *plugin, i3workspace *workspace)
{
}

void gui_add_workspace(i3workspacesPlugin *plugin, i3workspace *workspace)
{
    gtk_box_pack_start(GTK_BOX(plugin->hbox), workspace->button, FALSE, FALSE, 10);
    gtk_widget_show(workspace->button);
}

void gui_remove_workspace(i3workspacesPlugin *plugin, i3workspace *workspace)
{
    gtk_widget_hide(workspace->button);
    gtk_widget_destroy(workspace->button);
    workspace->button = NULL;
}
