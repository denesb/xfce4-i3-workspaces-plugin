#include "i3w-multi-monitor-utils.h"

/**
 * find_crtc:
 * @dpy: The X Display
 * @res: The XRandR screen resources
 * @xid: The X Id of the crtc
 *
 * Find a crtc's information given its xid
 *
 * Returns: The crtc's information struct
 */
XRRCrtcInfo*
find_crtc (Display* dpy, XRRScreenResources* res, RRCrtc xid) {
    XRRCrtcInfo* found = NULL;
    int c;
    for (c = 0; c < res->ncrtc && !found; ++c) {
        if (xid == res->crtcs[c]) {
            found = XRRGetCrtcInfo(dpy, res, res->crtcs[c]);
        }
    }
    return found;
}


/**
 * is_connected:
 * @plugin: the xfce plugin object
 *
 * Does c mean connected display?
 *
 * Returns: True iff c indicated connected display
 */
gboolean
is_connected (Connection c) {
    return c == RR_Connected;
}


/**
 * get_outputs:
 *
 * Gets the output information from XRandR. For each monitor, the
 * output name, screen position and current resolution.
 *
 * Returns: The outputs structure. Must be freed with free_outputs.
 */
i3_workspaces_outputs_t
get_outputs() {
    // Get Xlib information
    Display* dpy = XOpenDisplay (NULL); 
	int screen = DefaultScreen (dpy);
    Window root = RootWindow (dpy, screen);
	XRRScreenResources* res = XRRGetScreenResourcesCurrent (dpy, root);
    int num_outputs = res->noutput;

    // Allocate output
    i3_workspaces_outputs_t outputs;
    outputs.outputs = malloc(sizeof(i3_workspaces_output_t)*num_outputs);

    // NOTE: We will only store connected outputs, even though we allocated space
    // for all outputs. This is better than having to count the outputs first and 
    // asking for space later.
    int num_connected_outputs = 0; 
    int o;
    for (o = 0; o < num_outputs; ++o) {
        XRROutputInfo *output_info = XRRGetOutputInfo(dpy, res, res->outputs[o]);
        int connected = is_connected(output_info->connection);

        if (connected) {
            XRRCrtcInfo* info = find_crtc(dpy, res, output_info->crtc);
            if (info) {
                i3_workspaces_output_t* output = &outputs.outputs[num_connected_outputs];

                output->name = malloc((strlen(output_info->name)+1)*sizeof(char));
                strcpy(output->name, output_info->name);

                output->width = info->width;
                output->height = info->height;
                output->x = info->x;
                output->y = info->y;

                num_connected_outputs++;
            }
        }
    }

    outputs.num_outputs = num_connected_outputs;
    return outputs;

}

/**
 * free_outputs:
 * @outputs: The outputs structure 
 *
 * Frees the allocated memory for an outputs structure
 *
 */
void
free_outputs(i3_workspaces_outputs_t outputs) {
  free(outputs.outputs);
}

/**
 * get_monitor_name_at:
 * @outputs: The outputs information, as returned by get_outputs
 * @win_x: The X position of the coordinates to check
 * @win_y: The Y position of the coordinates to check
 *
 * Obtains the monitor name at a given X Screen coordinates.
 *
 * Returns: A string, representing the monitor name.
 */
const char*
get_monitor_name_at(i3_workspaces_outputs_t outputs, int win_x, int win_y) {
    int o;
    for (o = 0; o < outputs.num_outputs; ++o) {
        int width = outputs.outputs[o].width;
        int height = outputs.outputs[o].height;
        int x_offs = outputs.outputs[o].x;
        int y_offs = outputs.outputs[o].y;

        if (win_x >= x_offs && win_x < x_offs + width &&
            win_y >= y_offs && win_y < y_offs + height) {
            return outputs.outputs[o].name;
        }
    }
    return NULL;
}

