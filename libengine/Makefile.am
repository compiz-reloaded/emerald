## Process this file with automake to produce Makefile.in
AM_CPPFLAGS = @EMERALD_CFLAGS@ -I$(top_srcdir)/include\
		   -DENGINE_DIR=\"$(enginesdir)\" \
		   -DDEFTHEMEDIR='"@datadir@/emerald/theme/"' \
		   -DDEFSETTINGSFILE='"@datadir@/emerald/settings.ini"'
libemeraldengine_la_LIBADD = @EMERALD_LIBS@
libemeraldengine_la_SOURCES = emerald.c themer.c
lib_LTLIBRARIES = libemeraldengine.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = emeraldengine.pc
