include $(top_srcdir)/build-aux/glib-tap.mk

AM_CFLAGS = -g $(GLIB_CFLAGS)
AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"GCoroutine\" \
	-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_30 \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src
LDADD = $(top_builddir)/src/libgcoroutine-1.0.la $(GLIB_LIBS)

test_programs = coroutine

-include $(top_srcdir)/git.mk
