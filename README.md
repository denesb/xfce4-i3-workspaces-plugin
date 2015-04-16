xfce4-i3-workspaces-plugin
==========================

A workspace switcher plugin for xfce4-panel which can be used for the i3 window manager.

For available releases see the [releases](https://github.com/denesb/xfce4-i3-workspaces-plugin/releases) page.

Features
--------

Present a list of buttons, one for each workspace, labeled with the workspace name.
The focused workspace is marked with a bold label. Urgent workspaces are marked with red labels.
Different colors can be configured for the label in focused/non-focused states.
Support for strip workspace numbers configuration.
Clicking on a workspace button will navigate you to the respective workspace.

Multiple monitors are not supported at the moment. To be more precice I haven't tested it with
multiple monitors. It might work though.

Development
-----------

If you found a bug please report it in the [issue tracker](https://github.com/denesb/xfce4-i3-workspaces-plugin/issues "Bugs")

If you want to contact me drop me a line at dns.botond at gmail dot com

Patches and pull requests are welcome!

Building
--------

Prerequisites: C toolchain (I only tested with gcc).

###Dependencies

* glib2
* gtk+2
* libxfce4ui-4.8
* libxfce4util-4.8
* xfce4-panel-4.8
* [i3ipc-glib](https://github.com/acrisci/i3ipc-glib "i3ipc-glib")

*Note:*
+ On binary distros you may have to install the -dev version of the required
packages
+ For the compilation to work out of the box I had to install i3ipc-glib in
the /usr prefix too.

###Building from git (needs autotools)
* git clone https://github.com/denesb/xfce4-i3-workspaces-plugin.git
* cd xfce4-i3-workspaces-plugin
* ./autogen.sh --prefix=/usr
* make
* sudo make install

###Building from release tarball
* Download & extract tarball
* cd xfce4-i3-workspaces-plugin
* ./configure --prefix=/usr
* make
* sudo make install

*Note:*
The --prefix=/usr is needed because if installed in /usr/local prefix the
plugin is not discovered by xfce-panel. Maybe there is a way to tell xfce-panel
to look in other places too, but I haven't found it just yet.

Have fun!
