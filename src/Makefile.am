## Process this file with automake to produce Makefile.in
emerald_LDADD   = @EMERALD_LIBS@ $(top_builddir)/libengine/libemeraldengine.la
emerald_SOURCES = main.c \
			   engine_loader.c
emerald_program = emerald

AM_CPPFLAGS = @EMERALD_CFLAGS@ -I$(top_srcdir)/include \
		   -DLOCALEDIR="\"@datadir@/locale"\"\
		   -DENGINE_DIR=\"$(enginesdir)\"

bin_PROGRAMS = $(emerald_program)
