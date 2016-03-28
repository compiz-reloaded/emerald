AC_PREREQ(2.57)

AC_INIT([emerald],[0.8.12.3], [chat-to-me@raveit.de])

AM_INIT_AUTOMAKE([1.9 foreign tar-ustar dist-xz no-dist-gzip check-news])
AC_CONFIG_HEADER([config.h])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
LT_INIT
AC_HEADER_STDC
IT_PROG_INTLTOOL([0.35.0])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=emerald
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AC_SUBST(GETTEXT_PACKAGE)
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

enginesdir=$libdir/emerald/engines
AC_SUBST(enginesdir)

AC_C_BIGENDIAN

dnl ============================================================
dnl Check for the pkg-config path.
if test x"$PKG_CONFIG_PATH" = x; then
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${prefix}/share/pkgconfig
        else
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${prefix}/share/pkgconfig:${PKG_CONFIG_PATH}
fi
export PKG_CONFIG_PATH
AC_SUBST(PKG_CONFIG_PATH)
AC_MSG_NOTICE([Using PKG_CONFIG_PATH=$PKG_CONFIG_PATH])

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  AC_HELP_STRING([--with-gtk=2.0|3.0],
                 [Which gtk+ version to compile against (default: 3.0)]),
  [case "$with_gtk" in
      2.0|3.0) ;;
      *) AC_MSG_ERROR([invalid gtk+ version specified]);;
  esac],
  [with_gtk=3.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_MIN_VERSION=2.10.0
       WNCK_API_VERSION=1.0
       WNCK_MIN_VERSION=2.22.0
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_MIN_VERSION=3.0.0
       WNCK_API_VERSION=3.0
       WNCK_MIN_VERSION=3.0.0
       ;;
esac

EMERALD_REQUIRES=" xrender >= 0.8.4				  \
		    gtk+-$GTK_API_VERSION >= $GTK_MIN_VERSION	   \
		    libwnck-$WNCK_API_VERSION >= $WNCK_MIN_VERSION \
		    libdecoration				   \
		    pangocairo"

PKG_CHECK_MODULES(EMERALD, $EMERALD_REQUIRES)

AC_SUBST(EMERALD_REQUIRES)
CFLAGS="$CFLAGS $EMERALD_CFLAGS"
LIBS="$LIBS $EMERALD_LIBS -lm -ldl"
AC_CHECK_FUNCS(wnck_window_has_name)

AC_ARG_ENABLE(decor09,
  [  --enable-decor09        Use libdecoration 0.9.x support],
  [use_decor09=$enableval], [use_decor09=no])

AM_CONDITIONAL(DECOR_INTERFACE_VERSION, test "x$use_decor09" = "xyes")
if test "$use_decor09" = yes; then
  AC_DEFINE(DECOR_INTERFACE_VERSION, 20110504, [Build with libdecoration 0.9.x support])
fi

AC_ARG_ENABLE(mime-update,
[  --enable-mime-update],
[case "${enableval}" in
  yes) mime_update=true ;;
  no)  mime_update=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-mime-update) ;;
esac],[mime_update=false])
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
icons/Makefile
libengine/Makefile
misc/Makefile
doc/Makefile
po/Makefile.in
defaults/Makefile
defaults/theme/Makefile
])
