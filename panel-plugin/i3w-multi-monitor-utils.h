#ifndef __MULTIMONITORUTILS_H_
#define __MULTIMONITORUTILS_H_

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>	/* we share subpixel information */
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>
#include <glib.h>

typedef struct {
    int x;
    int y;
    int width;
    int height;
    char* name;
} i3_workspaces_output_t;

typedef struct {
    int num_outputs; 
    i3_workspaces_output_t* outputs;
} i3_workspaces_outputs_t;

i3_workspaces_outputs_t get_outputs();

void free_outputs();

const char* get_monitor_name_at(i3_workspaces_outputs_t outputs, int win_x, int win_y);

#endif 
