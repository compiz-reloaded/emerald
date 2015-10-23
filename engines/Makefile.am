## Process this file with automake to produce Makefile.in
#intentionally blank
AM_CPPFLAGS = -I$(top_srcdir)/include @EMERALD_CFLAGS@

ENGINELIBS = ../libengine/libemeraldengine.la

liblegacy_la_LDFLAGS = -module -avoid-version -no-undefined
liblegacy_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
liblegacy_la_SOURCES = legacy.c legacy_icon.h

libline_la_LDFLAGS = -module -avoid-version -no-undefined
libline_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libline_la_SOURCES = line.c legacy_icon.h

libzootreeves_la_LDFLAGS = -module -avoid-version -no-undefined
libzootreeves_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libzootreeves_la_SOURCES = zootreeves.c zootreeves_icon.h

libvrunner_la_LDFLAGS = -module -avoid-version -no-undefined
libvrunner_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libvrunner_la_SOURCES = vrunner.c vrunner_icon.h

libtruglass_la_LDFLAGS = -module -avoid-version -no-undefined
libtruglass_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libtruglass_la_SOURCES = truglass.c truglass_icon.h

liboxygen_la_LDFLAGS = -module -avoid-version -no-undefined
liboxygen_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
liboxygen_la_SOURCES = oxygen.c oxygen_icon.h

libpixmap_la_LDFLAGS = -module -avoid-version -no-undefined
libpixmap_la_LIBADD = @EMERALD_LIBS@ $(ENGINELIBS)
libpixmap_la_SOURCES = pixmap.c pixmap_icon.h

enginedir = $(enginesdir)

engine_LTLIBRARIES = \
					 liblegacy.la \
					 libline.la \
					 libvrunner.la \
					 libtruglass.la \
					 liboxygen.la \
					 libpixmap.la \
					 libzootreeves.la
