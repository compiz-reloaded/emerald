## Process this file with automake to produce Makefile.in
#intentionally blank
INCLUDES = -I$(top_srcdir)/include @EMERALD_CFLAGS@

ENGINELIBS = $(top_builddir)/libengine/libemeraldengine.la

liblegacy_la_LDFLAGS = -module -avoid-version -no-undefined
liblegacy_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
liblegacy_la_SOURCES = legacy.c

libzootreeves_la_LDFLAGS = -module -avoid-version -no-undefined
libzootreeves_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libzootreeves_la_SOURCES = zootreeves.c

libvrunner_la_LDFLAGS = -module -avoid-version -no-undefined
libvrunner_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libvrunner_la_SOURCES = vrunner.c

libtruglass_la_LDFLAGS = -module -avoid-version -no-undefined
libtruglass_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libtruglass_la_SOURCES = truglass.c

liboxygen_la_LDFLAGS = -module -avoid-version -no-undefined
liboxygen_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
liboxygen_la_SOURCES = oxygen.c

libpixmap_la_LDFLAGS = -module -avoid-version -no-undefined
libpixmap_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libpixmap_la_SOURCES = pixmap.c

enginedir = $(enginesdir)

engine_LTLIBRARIES = \
					 liblegacy.la \
					 libvrunner.la \
					 libtruglass.la \
					 liboxygen.la \
					 libpixmap.la \
					 libzootreeves.la
