ACLOCAL_AMFLAGS = -I build-aux

EXTRA_DIST = \
	build-aux/ax_pthread.m4 \
	build-aux/glibtests.m4 \
	build-aux/glib-tap.mk \
	build-aux/introspection.m4 \
	build-aux/tap-driver.sh \
	build-aux/git-version-gen \
	build-aux/tap-test

SUBDIRS = src doc

if BUILD_TESTS
# conditionally recurse only if we build the test suite
SUBDIRS += tests
endif

# see git-version-gen
dist-hook: gen-ChangeLog
	echo $(VERSION) > $(distdir)/.tarball-version

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

BUILT_SOURCES = $(top_srcdir)/.version
EXTRA_DIST += $(top_srcdir)/.version

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

gen-ChangeLog:
	@if test -d "$(srcdir)/.git"; then \
	  echo Generating ChangeLog... ; \
	  ( cd "$(srcdir)" \
	    && $(top_srcdir)/build-aux/missing --run git log --stat ) > ChangeLog.tmp \
	  && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
	  || ( rm -f ChangeLog.tmp; \
	       echo Failed to generate ChangeLog >&2 ); \
	else \
	  echo A git checkout is required to generate a ChangeLog >&2; \
	fi

if ENABLE_GCOV
# use recursive makes in order to ignore errors during check/perf
lcov:
	-$(MAKE) $(AM_MAKEFLAGS) check
	$(MAKE) $(AM_MAKEFLAGS) genlcov

# we have to massage the lcov.info file slightly to hide the effect of libtool
# placing the objects files in the .libs/ directory separate from the *.c
genlcov:
	$(LTP) --directory $(top_builddir) --capture --output-file gcoroutine-lcov.info --test-name GCOROUTINE_TEST --no-checksum
	$(SED) -e 's#.libs/##' < gcoroutine-lcov.info > gcoroutine-lcov.info.tmp
	LANG=C $(LTP_GENHTML) --prefix $(top_builddir) --output-directory gcoroutine-lcov --title "GCoroutine Code Coverage" --show-details gcoroutine-lcov.info.tmp
	rm -f gcoroutine-lcov.info.tmp

lcov-clean:
	-$(LTP) --directory $(top_builddir) -z
	-$(RM) -rf gcoroutine-lcov.info gcoroutine-lcov
else
lcov genlcov lcov-clean:
	@echo You need to configure GCoroutine with --enable-code-coverage
endif

.PHONY: lcov genlcov lcov-clean

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL)

-include $(top_srcdir)/git.mk
