AC_PREREQ(2.57)

AC_INIT()
AM_CONFIG_HEADER([config.h])

PACKAGE=emerald
source $srcdir/VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
#, [livinglatexkali@gmail.com])
AC_PREFIX_DEFAULT(/usr)

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

dnl
dnl APR
dnl

APR_CONFIGS="apr-config /usr/local/apr/bin/apr-config"
AC_ARG_WITH(apr-config,
[[  --with-apr-config=FILE    Use the given path to apr-config when determining
                            APR configuration; defaults to "apr-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        APR_CONFIGS=$withval
    fi
])
AC_MSG_CHECKING([for APR])
APR_CONFIG=""
for VALUE in $APR_CONFIGS ; do
	if $VALUE --cflags >/dev/null 2>&1; then
		APR_CONFIG=$VALUE
		break
	fi
done
if test $APR_CONFIG ; then
    AC_MSG_RESULT([found])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([APR is required. Try --with-apr-config.])
fi
	
CPPFLAGS="$CPPFLAGS  `$APR_CONFIG --cppflags` `$APR_CONFIG --includes`"
APR_LIBS="`$APR_CONFIG --link-ld --libs`"

dnl
dnl APR util
dnl

APU_CONFIGS="apu-config /usr/local/apr/bin/apu-config"
AC_ARG_WITH(apu-config,
[[  --with-apu-config=FILE    Use the given path to apu-config when determining
                            APR util configuration; defaults to "apu-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        APU_CONFIGS=$withval
    fi
])
AC_MSG_CHECKING([for APR util])
APU_CONFIG=""
for VALUE in $APU_CONFIGS ; do
	if $VALUE --includes >/dev/null 2>&1; then
		APU_CONFIG=$VALUE
		break
	fi
done
if test $APU_CONFIG ; then
    AC_MSG_RESULT([found])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([APR util is required. Try --with-apu-config.])
fi
CPPFLAGS="$CPPFLAGS `$APU_CONFIG --includes`"
APR_LIBS="$APR_LIBS `$APU_CONFIG --link-ld --libs`"


dnl
dnl Neon
dnl
SEARCH="neon-config"
AC_ARG_WITH(neon-config,
[[  --with-neon-config=FILE     Use the given path to neon-config when determining
                            Neon configuration; defaults to "neon-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        SEARCH=$withval
    fi
])
AC_MSG_CHECKING([Neon])
NEON_CONFIG=""
for VALUE in $SEARCH ; do
	if CHECK=`$VALUE --version 2>&1` ; then
		NEON_CONFIG=$VALUE
		break
	fi
done
if test $NEON_CONFIG ; then	
	AC_MSG_RESULT([found])
else
	AC_MSG_RESULT([not found])
	AC_MSG_ERROR([Neon is required. Try --with-neon-config.])
fi
#CPPFLAGS="$CPPFLAGS `$NEON_CONFIG --cflags`"
#NEON_LIBS="`$NEON_CONFIG --libs`"


dnl
dnl Subversion
dnl

SVN_INCLUDES="/usr/local/include /usr/include"
AC_ARG_WITH(svn-include,
[[  --with-svn-include=DIR   Use the given path to the subversion headers.]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        SVN_INCLUDES=$withval
    fi
])
AC_MSG_CHECKING([for Subversion headers])
SVN_INCLUDE=""
for VALUE in $SVN_INCLUDES ; do
    if test -f $VALUE/subversion-1/svn_types.h ; then
        SVN_INCLUDE=$VALUE
        break
    fi
done
if test $SVN_INCLUDE ; then
    AC_MSG_RESULT([found])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Subversion headers are required. Try --with-svn-include.])
fi
CPPFLAGS="$CPPFLAGS -I$SVN_INCLUDE/subversion-1"

SVN_LIBS="/usr/local/lib /usr/lib"
AC_ARG_WITH(svn-lib,
[[  --with-svn-lib=DIR  Use the given path to the subversion libraries.]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        SVN_LIBS=$withval
    fi
])
AC_MSG_CHECKING([for Subversion libraries])
SVN_LIB=""
for VALUE in $SVN_LIBS ; do
	if ls $VALUE/libsvn_client-1.* >/dev/null 2>&1; then
		SVN_LIB=$VALUE
		break
	fi
done
if test $SVN_LIB ; then
    AC_MSG_RESULT([found])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Subversion libraries are required. Try --with-svn-lib.])
fi
SVN_LIBS="-L$SVN_LIB -lsvn_client-1 -lsvn_wc-1 -lsvn_ra-1 -lsvn_delta-1 -lsvn_subr-1"

AC_SUBST(APR_LIBS)
AC_SUBST(SVN_LIBS)
AC_SUBST(NEON_LIBS)


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
