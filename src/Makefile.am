# preamble
NULL =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
DISTCLEANFILES =
lib_LTLIBRARIES =

# source
source_h = \
	gcoroutine-version-macros.h \
	gcoroutine-macros.h \
	$(NULL)
source_c = \
	gcoroutine.c \
	$(NULL)

if COROUTINE_UCONTEXT
source_c += gcoroutine-ucontext.c
endif

if COROUTINE_WINFIBER
source_c += gcoroutine-winfiber.c
endif

if COROUTINE_GTHREAD
source_c += gcoroutine-gthread.c
endif

source_h_priv = \
	gcoroutineprivate.h \
	$(NULL)

# version header
DISTCLEANFILES += gcoroutine-version.h
EXTRA_DIST += \
	valgrind.h \
	gcoroutine-version.h.in

shared_cflags = $(GLIB_CFLAGS) $(GCOV_CFLAGS)
shared_libadd = $(GLIB_LIBS) $(GCOV_LDADD)

# main library
libgcoroutine_1_0_la_CPPFLAGS = \
	-DGCOROUTINE_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"GCoroutine\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

libgcoroutine_1_0_la_CFLAGS = $(shared_cflags) $(GCOROUTINE_VISIBILITY_CFLAGS)
libgcoroutine_1_0_la_LIBADD = $(shared_libadd)
libgcoroutine_1_0_la_LDFLAGS = -no-undefined
libgcoroutine_1_0_la_SOURCES = $(source_h) $(source_h_priv) $(source_c) $(source_c_priv) $(BUILT_SOURCES)

lib_LTLIBRARIES += libgcoroutine-1.0.la

# headers
gcoroutineincludedir = $(includedir)/gcoroutine-1.0
gcoroutineinclude_HEADERS = $(source_h) gcoroutine.h
nodist_gcoroutineinclude_HEADERS = \
	$(top_builddir)/src/gcoroutine-version.h \
	$(NULL)

# pkg-config files
gcoroutine-1.0.pc: gcoroutine.pc
	$(AM_V_GEN)cp -f $< $@

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = gcoroutine-1.0.pc
CLEANFILES += gcoroutine-1.0.pc
EXTRA_DIST += gcoroutine.pc.in

-include $(top_srcdir)/git.mk
