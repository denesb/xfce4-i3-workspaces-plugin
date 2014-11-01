xfce4-i3-workspaces-plugin
==========================

A workspace switcher plugin for xfce4-panel which can be used for the i3 window 
manager.

At the moment the plugin compiles (on my machine :) ) and it shows the currently
active workspaces.

Roadmap
-------

0.2 - the currently active workspaces are shown

0.4 - the workspace buttons show the actually selected (focused) workspace

0.8 - when clicking on a workspace button the window manager jumps to the 
      workspace

1.0 - urgent workspaces implemented

If you found a bug please report it in the [issue tracker](https://github.com/denesb/xfce4-i3-workspaces-plugin/issues "Bugs")

If you want to contact me drop me a line at dns.botond at gmail dot com

Building
--------

git clone https://github.com/denesb/xfce4-i3-workspaces-plugin.git

cd xfce4-i3-workspaces-plugin

./autogen.sh --prefix=/usr
make
sudo make install

*Note:*
The --prefix=/usr is needed because if installed in /usr/local prefix the
plugin is not discovered by xfce-panel. Maybe there is a way to tell xfce-panel
to look in other places too, but I haven't found it just yet.

Dependencies
------------

* C toolchain - obviously
* xfce4 (not sure what exactly is required from here)
* gtk
* [i3ipc-glib](https://github.com/acrisci/i3ipc-glib "i3ipc-glib")

*Note:*
1. On binary distros you may have to install the -dev version of the required
packages
2. For the compilation to work out of the box I had to install i3ipc-glib in
the /usr prefix too.

Have fun!
