AC_PREREQ(2.57)

m4_define([emerald_version], [esyscmd(. ./VERSION;echo -n $VERSION)])
AC_INIT([emerald],[emerald_version], [livinglatexkali@gmail.com])

AM_INIT_AUTOMAKE([1.9 dist-bzip2])
AC_CONFIG_HEADER([config.h])

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
		    libdecoration     \
		    pangocairo"

dnl ============================================================
dnl Check for the pkg-config path.
if test x"$PKG_CONFIG_PATH" = x; then
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig
        else
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${PKG_CONFIG_PATH}
fi
export PKG_CONFIG_PATH
AC_SUBST(PKG_CONFIG_PATH)
AC_MSG_NOTICE([Using PKG_CONFIG_PATH=$PKG_CONFIG_PATH])

PKG_CHECK_MODULES(EMERALD, $EMERALD_REQUIRES)
PKG_CHECK_EXISTS(libwnck-1.0 >= 2.19.3,
		 [have_libwnck_2_19_3=yes], [have_libwnck_2_19_3=no])

if test "$have_libwnck_2_19_3" = yes; then
  AC_DEFINE(HAVE_LIBWNCK_2_19_3, 1,
      [Define to 1 if libwnck version >= 2_19_3])
fi

AC_SUBST(EMERALD_REQUIRES)
CFLAGS="$CFLAGS $EMERALD_CFLAGS"
LIBS="$LIBS $EMERALD_LIBS"
AC_CHECK_FUNCS(wnck_window_has_name)

PKG_CHECK_EXISTS(libwnck-1.0 >= 2.19.4,
		     [have_libwnck_2_19_4=yes], [have_libwnck_2_19_4=no])
if test "$have_libwnck_2_19_4" = yes; then
    AC_DEFINE(HAVE_LIBWNCK_2_19_4, 1,
	      [Define to 1 if libwnck version >= 2_19_4])
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
pixmaps/Makefile
libengine/Makefile
misc/Makefile
doc/Makefile
po/Makefile.in
defaults/Makefile
defaults/theme/Makefile
])
