Summary: A workspaces plugin for xfce4 and the i3 window manager
Name:    xfce4-i3-workspaces-plugin
Version: 1.3.1
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.gz
License: GPL v3
URL:     https://github.com/altdesktop/xfce4-i3-workspaces-plugin

BuildRequires: libtool
BuildRequires: gtk2-devel
BuildRequires: xfce4-dev-tools
BuildRequires: libxfce4ui-devel
BuildRequires: libxfce4util-devel
BuildRequires: xfce4-panel-devel
BuildRequires: i3ipc-glib

%if 0%{?suse_version} >= 1500
Requires: libglib-2_0-0
%else
Requires: glib2
%endif
Requires: libxfce4ui
Requires: libxfce4util
Requires: xfce4-panel
Requires: i3ipc-glib

%description
A workspaces plugin for xfce4 and the i3 window manager.

%prep
%setup -q


%build
./autogen.sh --prefix=%{_prefix} --libdir=%{_libdir}
make


%install
%make_install


%files
%doc COPYING
%doc README.md
%{_libdir}/xfce4/panel/plugins/libi3workspaces.*
%{_datadir}/xfce4/panel/plugins/i3-workspaces.desktop
%{_datadir}/icons/hicolor/48x48/apps/xfce4-sample-plugin.png
%{_datadir}/icons/hicolor/scalable/apps/xfce4-sample-plugin.svg


%changelog
