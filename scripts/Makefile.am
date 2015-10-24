NULL =

bin_SCRIPTS = \
	compiz-decorator-emerald \
	compiz-lxde-emerald \
	compiz-mate-emerald \
	compiz-xfce-emerald

desktopfilesdir	      = $(DESTDIR)$(datadir)/applications
desktopfiles_in_files = \
	compiz-lxde-emerald.desktop.in \
	compiz-mate-emerald.desktop.in \
	compiz-xfce-emerald.desktop.in \
	emerald-decorator.desktop.in
desktopfiles_files    = $(desktopfiles_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

install-data-local:
	if mkdir -p $(desktopfilesdir) && test -w $(desktopfilesdir); then ( \
		$(INSTALL) -m 644 $(desktopfiles_files) \
		$(desktopfilesdir) \
		); \
	fi

uninstall-local:
	if test -w $(desktopfilesdir); then ( \
		rm -f $(desktopfilesdir)/*.desktop \
		); \
	fi

noinst_DATA =		      \
	$(desktopfiles_files)

EXTRA_DIST =			 \
	$(desktopfiles_in_files) \
	compiz-decorator-emerald \
	compiz-lxde-emerald \
	compiz-mate-emerald \
	compiz-xfce-emerald

DISTCLEANFILES = \
	$(noinst_DATA)
