#include "i3w-multi-monitor-utils.h"

XRRCrtcInfo* find_crtc (Display* dpy, XRRScreenResources* res, RRCrtc xid) {
    //assert(res);
    XRRCrtcInfo* found = NULL;
    int c;
    for (c = 0; c < res->ncrtc && !found; ++c) {
        if (xid == res->crtcs[c]) {
            found = XRRGetCrtcInfo(dpy, res, res->crtcs[c]);
        }
    }
    return found;
}

int is_connected (Connection c) {
    return c == RR_Connected;
}

i3_workspaces_outputs_t get_outputs() {
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
                char* name = output_info->name;
                int width = info->width;
                int height = info->height;
                int x = info->x;
                int y = info->y;

                char* name_dest = malloc(strlen(name)*sizeof(char));
                strcpy(name_dest, name);

                i3_workspaces_output_t* output = &outputs.outputs[num_connected_outputs];

                output->name = name_dest;
                output->width = width;
                output->height = height;
                output->x = x;
                output->y = y;

                num_connected_outputs += 1;
            }
        }

        outputs.num_outputs = num_connected_outputs;
    }

    outputs.num_outputs = num_connected_outputs;
    return outputs;

}

char* get_monitor_name_at(i3_workspaces_outputs_t outputs, int win_x, int win_y) {
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

int main () {
    i3_workspaces_outputs_t outputs = get_outputs();

    printf("%s\n", get_monitor_name_at(outputs, 2000, 10));

}
