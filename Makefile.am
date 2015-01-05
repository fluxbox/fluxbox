# Makefile.am for Fluxbox - http://fluxbox.org

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h

# the defaults.hh is wrote to builddir
src_incdir = $(top_srcdir)/src -I$(top_builddir)/src
fbtk_incdir = $(top_srcdir)/src/FbTk
nls_incdir = $(top_srcdir)/nls

EXTRA_DIST = \
	3rd/vim/ \
	build-aux/config.rpath \
	nls/fluxbox-nls.hh \
	nls/nlsinfo

CLEANFILES =
bin_PROGRAMS =

MAINTAINERCLEANFILES = \
	aclocal.m4 \
	config.h.in \
	configure \
	Makefile.in \
	stamp-h.in

DEFAULT_MENU=@DEFAULT_MENU@
DEFAULT_STYLE=@DEFAULT_STYLE@
DEFAULT_KEYSFILE=@DEFAULT_KEYS@
DEFAULT_APPSFILE=@DEFAULT_APPS@
DEFAULT_OVERLAY=@DEFAULT_OVERLAY@
DEFAULT_INITFILE=@DEFAULT_INIT@
DEFAULT_WINDOWMENU=@DEFAULT_WINDOWMENU@
PROGRAM_PREFIX=@program_prefix@
PROGRAM_SUFFIX=@program_suffix@

# Do not rely on configure.ac AC_CONFIG_FILES for install paths.  The
# paths should be expanded at make time rather than configure.
edit_cmd = sed \
	 -e 's|@default_style[@]|$(DEFAULT_STYLE)|g' \
	 -e 's|@default_menu[@]|$(DEFAULT_MENU)|g' \
	 -e 's|@default_keysfile[@]|$(DEFAULT_KEYSFILE)|g' \
	 -e 's|@default_appsfile[@]|$(DEFAULT_APPSFILE)|g' \
	 -e 's|@bindir[@]|$(bindir)|g' \
	 -e 's|@datadir[@]|$(datadir)|g' \
	 -e 's|@exec_prefix[@]|$(exec_prefix)|g' \
	 -e 's|@includedir[@]|$(includedir)|g' \
	 -e 's|@localstatedir[@]|$(localstatedir)|g' \
	 -e 's|@pkgbindir[@]|$(pkgbindir)|g' \
	 -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	 -e 's|@pkgprefix[@]|$(pkgprefix:NONE=)|g' \
	 -e 's|@pkgsuffix[@]|$(pkgsuffix:NONE=)|g' \
	 -e 's|@prefix[@]|$(prefix)|g' \
	 -e 's|@sbindir[@]|$(sbindir)|g' \
	 -e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	 -e 's|@usrbin_execdir[@]|$(usrbin_execdir)|g' \
	 -e 's|@usrlib_execdir[@]|$(usrlib_execdir)|g' \
	 -e 's|@usrsbin_execdir[@]|$(usrsbin_execdir)|g' \
	 -e 's|@EXEEXT[@]|$(EXEEXT)|g' \
	 -e 's|@VERSION[@]|$(VERSION)|g'

PATHFILES =
CLEANFILES += $(PATHFILES)
EXTRA_DIST += $(PATHFILES:=.in)

$(PATHFILES): Makefile
	$(MKDIR_P) $(dir $@)
	@ rm -f $@ $@.tmp
	$(AM_V_GEN) srcdir=''; \
		test -f ./$@.in || srcdir=$(srcdir)/; \
		$(edit_cmd) $${srcdir}$@.in >$@.tmp
	@ mv $@.tmp $@

include data/Makemodule.am
include doc/Makemodule.am
include nls/Makemodule.am
include src/FbTk/Makemodule.am
include src/Makemodule.am
include src/tests/Makemodule.am
include util/Makemodule.am
include util/fbrun/Makemodule.am

uninstall-hook:
	rmdir $(DESTDIR)$(pkgdatadir)

source-doc:
	doxygen Doxyfile
