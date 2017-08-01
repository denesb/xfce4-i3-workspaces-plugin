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

Development
-----------

If you found a bug please report it in the [issue tracker](https://github.com/denesb/xfce4-i3-workspaces-plugin/issues "Bugs")

Patches and pull requests are welcome!

Feel free to contact me at: dns.botond at gmail dot com.

Building
--------

### Build Dependencies

* C toolchain (I only tested with gcc).
* pkg-config
* xfce4-dev-tools

### Runtime Dependencies
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

### Building from git (needs autotools)
* git clone https://github.com/denesb/xfce4-i3-workspaces-plugin.git
* cd xfce4-i3-workspaces-plugin
* ./autogen.sh --prefix=/usr
* make
* sudo make install

### Building from release tarball
* Download & extract tarball
* cd xfce4-i3-workspaces-plugin
* ./configure --prefix=/usr
* make
* sudo make install

*Note:*
The --prefix=/usr is needed because if installed in /usr/local prefix the
plugin is not discovered by xfce-panel. Maybe there is a way to tell xfce-panel
to look in other places too, but I haven't found it just yet.

Setup
-----

A good, detailed guide of how you can use i3wm together with xfce4 can be found [here](http://feeblenerd.blogspot.ro/2015/11/pretty-i3-with-xfce.html).

Have fun!
