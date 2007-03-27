AC_PREREQ(2.57)

AC_INIT([emerald],esyscmd(. ./VERSION;echo -n $VERSION), [livinglatexkali@gmail.com])

AC_DEFINE_UNQUOTED(VERSION,"$VERSION",[Version])

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

AC_ARG_WITH([cm],
  [AS_HELP_STRING([--with-cm=NAME],[composite manager to use. beryl, compiz @<:@default=beryl@:>@])],
  [case "${withval}" in
   beryl|compiz) cm_manager="${withval}" ;;
   *) AC_MSG_ERROR([Wrong argument ${withval} for --with-cm, allowed values are beryl and compiz]) ;;
   esac], [cm_manager="beryl"])
AC_MSG_NOTICE([Using $cm_manager])

EMERALD_REQUIRES=" xrender >= 0.8.4  \
		    gtk+-2.0 >= 2.8.0 \
		    libwnck-1.0	      \
		    pangocairo"

case "$cm_manager" in
  beryl) EMERALD_REQUIRES="$EMERALD_REQUIRES \
			   libberyldecoration"
	 AC_DEFINE([HAVE_BERYL], [], [Compile for Beryl])
  ;;
  compiz) EMERALD_REQUIRES="$EMERALD_REQUIRES \
			   libdecoration"
	AC_DEFINE([HAVE_COMPIZ], [], [Compile for Compiz])
  ;;
esac

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
