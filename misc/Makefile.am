## Process this file with automake to produce Makefile.in
desktopdir = $(datadir)/applications/
desktop_DATA = emerald-theme-manager.desktop

mimeicondir = $(datadir)/icons/hicolor/48x48/mimetypes
mimeicon_DATA = application-x-emerald-theme.png

mimedir = $(datadir)/mime-info
mime_DATA = emerald.mime

emeraldxmldir = $(datadir)/mime/packages/
emeraldxml_DATA = emerald.xml

EXTRA_DIST = \
        $(desktop_DATA) \
        $(mimeicon_DATA) \
        $(mime_DATA) \
        $(emeraldxml_DATA)

if MIMEUPDATE
install-data-local: install-desktopDATA
	$(UPDATE_MIME_DATABASE) $(datadir)/mime
	$(UPDATE_ICON_CACHE) $(datadir)/icons/gnome
	$(UPDATE_DESKTOP_DATABASE)
endif
