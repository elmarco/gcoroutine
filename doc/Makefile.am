DOC_MODULE = gcoroutine
DOC_MAIN_SGML_FILE = gcoroutine-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/src

SCAN_OPTIONS = \
	--rebuild-types \
	--deprecated-guards="GCOROUTINE_DISABLE_DEPRECATED" \
	--ignore-decorators="GCOROUTINE_FUNC"
SCANGOBJ_OPTIONS =
MKDB_OPTIONS = --output-format=xml --name-space=gcoroutine
FIXXREF_OPTIONS =

HFILE_GLOB = $(top_srcdir)/src/*.h $(top_builddir)/src/*.h
CFILE_GLOB = $(top_srcdir)/src/*.c

IGNORE_HFILES = \
	config.h \
	valgrind.h \
	gcoroutineprivate.h

EXTRA_HFILES =

HTML_IMAGES =

content_files =
expand_content_files =

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(GLIB_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/src/libgcoroutine-1.0.la

include $(top_srcdir)/gtk-doc.make

-include $(top_srcdir)/git.mk
