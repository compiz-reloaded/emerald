AC_PREREQ(2.57)

AC_INIT()
AM_CONFIG_HEADER([config.h])

PACKAGE=emerald
source $srcdir/VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
#, [livinglatexkali@gmail.com])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_LIBTOOL
AC_HEADER_STDC
IT_PROG_INTLTOOL([0.35.0])
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=emerald
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AC_SUBST(GETTEXT_PACKAGE)
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

enginesdir=$libdir/emerald/engines
AC_SUBST(enginesdir)

AC_C_BIGENDIAN
EMERALD_REQUIRES=" xrender >= 0.8.4  \
		    gtk+-2.0 >= 2.8.0 \
		    libwnck-1.0	      \
            beryl \
		    pangocairo "
            #dbus-1 \
            #dbus-glib-1 \
            #gthread-2.0 \
PKG_CHECK_MODULES(EMERALD, $EMERALD_REQUIRES)
AC_SUBST(EMERALD_REQUIRES)
CFLAGS="$CFLAGS $EMERALD_CFLAGS"
LIBS="$LIBS $EMERALD_LIBS"
AC_CHECK_FUNCS(wnck_window_has_name)

AC_ARG_ENABLE(mime-update,
[  --enable-mime-update],
[case "${enableval}" in
  yes) mime_update=true ;;
  no)  mime_update=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-mime-update) ;;
esac],[mime_update=true])
AM_CONDITIONAL(MIMEUPDATE, test x$mime_update = xtrue)

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database)
AC_PATH_PROG(UPDATE_ICON_CACHE, gtk-update-icon-cache)

AC_OUTPUT([
libengine/emeraldengine.pc
Makefile
src/Makefile
include/Makefile
engines/Makefile
themer/Makefile
pixmaps/Makefile
libengine/Makefile
misc/Makefile
doc/Makefile
po/Makefile.in
defaults/Makefile
defaults/theme/Makefile
])
