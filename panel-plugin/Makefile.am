INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-i3-workspaces-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

#
# i3 workspaces plugin
#
plugin_LTLIBRARIES = \
	libi3workspaces.la

plugindir = \
	$(libdir)/xfce4/panel/plugins

libi3workspaces_la_SOURCES = \
	i3w-multi-monitor-utils.c \
	i3wm-delegate.c \
	i3w-config.c \
	i3w-plugin.c \
	i3w-multi-monitor-utils.h \
	i3wm-delegate.h \
	i3w-config.h \
	i3w-plugin.h

libi3workspaces_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(PLATFORM_CFLAGS)

libi3workspaces_la_LDFLAGS = \
       -avoid-version \
       -module \
       -no-undefined \
       -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
       $(LIBX11_CFLAGS)
       $(LIBXRANDR_CFLAGS)
       $(PLATFORM_LDFLAGS)

libi3workspaces_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(LIBI3IPCGLIB_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel/plugins

desktop_DATA =								\
	i3-workspaces.desktop

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	i3-workspaces.desktop.in

CLEANFILES =								\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
