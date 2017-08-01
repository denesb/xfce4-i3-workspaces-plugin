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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "i3wm-delegate.h"
#include "i3w-multi-monitor-utils.h"
#include "i3w-config.h"

G_BEGIN_DECLS

/* plugin structure */
typedef struct
{
    XfcePanelPlugin *plugin;

    /* panel widgets */
    GtkWidget       *ebox;
    GtkWidget       *hvbox;

    // hash table of i3workspace * => GtkButton *
    GHashTable      *workspace_buttons;

	// binding mode label
	GtkWidget       *mode_label;

    i3WorkspacesConfig *config;

    i3windowManager *i3wm;
}
i3WorkspacesPlugin;

G_END_DECLS

#endif /* !__PLUGIN_H__ */
