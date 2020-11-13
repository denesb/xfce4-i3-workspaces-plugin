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

Installing
----------

### Fedora/CentOS/openSUSE

The package is available as RPM for Fedora 30+, openSUSE 15.1+ and CentOS 7. It's provided on a COPR
repository maintained by [@fepitre](https://github.com/fepitre): https://copr.fedorainfracloud.org/coprs/fepitre/xfce4-i3/.

#### Fedora and CentOS

Please have a look on the [official documentation](https://docs.pagure.org/copr.copr/how_to_enable_repo.html#how-to-enable-repo) on how to enable COPR repository. Then, for Fedora,

```
dnf install xfce4-i3-workspaces-plugin
```

and for CentOS,

```
yum install xfce4-i3-workspaces-plugin
```

#### openSUSE

Add the repo file by adjusting openSUSE release version. For example, for 15.1

```
zypper addrepo -f https://copr.fedorainfracloud.org/coprs/fepitre/xfce4-i3/repo/opensuse-leap-15.1/fepitre-xfce4-i3-opensuse-leap-15.1.repo fepitre-xfce4-i3
```

and for Tumbleweed

```
zypper addrepo -f https://copr.fedorainfracloud.org/coprs/fepitre/xfce4-i3/repo/opensuse-tumbleweed/fepitre-xfce4-i3-opensuse-tumbleweed.repo fepitre-xfce4-i3
```

Then, install the package

```
zypper install xfce4-i3-workspaces-plugin
```

####  Gentoo

Add the repository

```
eselect repository add knotteye git https://pond.waldn.net/git/knotteye/overlay.git
```

Sync it

```
emaint sync -r knotteye
```

And install the package

```
emerge --ask xfce-extra/xfce4-i3-workspaces-plugin
```

If you get errors about package masking, add the following lines to `/etc/portage/package.accept_keywords`
```
>=x11-misc/i3ipc-glib-1.0.1 ~amd64
>=xfce-extra/xfce4-i3-workspaces-plugin-1.3.2 ~amd64
```

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
the `/usr` prefix too.

### Building from git (needs autotools)
```
git clone https://github.com/denesb/xfce4-i3-workspaces-plugin.git
cd xfce4-i3-workspaces-plugin
./autogen.sh --prefix=/usr
make
sudo make install
```

### Building from release tarball
* Download & extract tarball

```
cd xfce4-i3-workspaces-plugin
./configure --prefix=/usr
make
sudo make install
```

*Note:*
The `--prefix=/usr` is needed because if installed in `/usr/local` prefix the
plugin is not discovered by xfce-panel. Maybe there is a way to tell xfce-panel
to look in other places too, but I haven't found it just yet.

Setup
-----

A good, detailed guide of how you can use i3wm together with xfce4 can be found [here](http://feeblenerd.blogspot.ro/2015/11/pretty-i3-with-xfce.html).

Have fun!
