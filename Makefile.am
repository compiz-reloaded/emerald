AM_CPPFLAGS = -I$(top_srcdir)/include

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS =		\
	libengine	 \
	src		 \
	include		 \
	engines		 \
	themer		 \
	icons		 \
	misc		 \
	defaults	 \
	doc		 \
	po

EXTRA_DIST =		   \
	AUTHORS		    \
	COPYING		    \
	NEWS		    \
	README.md	    \
	autogen.sh	    \
	intltool-extract.in \
	intltool-merge.in   \
	intltool-update.in

DISTCHECK_CONFIGURE_FLAGS = --with-gtk=$(GTK_API_VERSION)

DISTCLEANFILES =		\
	po/stamp-it		 \
	intltool-extract	 \
	intltool-merge		 \
	intltool-update		 \
	po/.intltool-merge-cache \
	decor_ver.out

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog
