# Makefile.am for Fluxbox - http://fluxbox.org

ACLOCAL_AMFLAGS = -I m4
if ENABLE_PCH
# fbpch.hh brings config.h itself; see "precompiled header" below
AM_CPPFLAGS = \
	-include $(top_builddir)/fbpch.hh \
	-fpch-preprocess
else
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h
endif

# the defaults.hh is wrote to builddir
src_incdir = $(top_srcdir)/src -I$(top_builddir)/src
fbtk_incdir = $(top_srcdir)/src/FbTk
nls_incdir = $(top_srcdir)/nls

EXTRA_DIST = \
	3rd/vim/ \
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

# --- precompiled header -------------------------------------------------
# src/fbpch.hh is copied to the build dir and precompiled into the directory
# fbpch.hh.gch/. GCC tries every file in that directory and uses the one
# whose macro state and code-generation options match the current compile;
# it does not compare -I paths. So one variant is built per distinct set of
# such flags among the programs (-D/-U, -pthread, -f, -O, -m), not one per
# program: typically one for libFbTk (pkg-config adds -pthread & co. there)
# and one for everything else. A compile matching no variant silently falls
# back to the plain header (correct, just not faster).
# When GCC uses a PCH, the dependency files it writes list neither the PCH
# nor fbpch.hh/config.h (not even with -fpch-deps). So every object depends
# on $(PCH_STAMP): a change to config.h or fbpch.hh rebuilds the variants and
# then every object. (Changed system headers are not tracked; after a system
# upgrade, run make clean.)
EXTRA_DIST += src/fbpch.hh

if ENABLE_PCH
PCH_HEADER = $(top_builddir)/fbpch.hh
PCH_DIR = $(top_builddir)/fbpch.hh.gch
PCH_STAMP = $(top_builddir)/fbpch.stamp
BUILT_SOURCES += $(PCH_HEADER) $(PCH_STAMP)

$(PCH_HEADER): $(top_srcdir)/src/fbpch.hh
	$(AM_V_GEN)cp $(top_srcdir)/src/fbpch.hh $@

# each program's preprocessor flags, minus the force-include of the header
pch_strip = -include $(PCH_HEADER)
pch_sets = \
	'$(filter-out $(pch_strip),$(AM_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(libFbTk_a_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(fluxbox_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(fbrun_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(fbsetroot_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(fluxbox_remote_CPPFLAGS))' \
	'$(filter-out $(pch_strip),$(fluxbox_update_configs_CPPFLAGS))'

# key = the flags GCC checks; a variant is compiled only for a new key.
# eval: the flags may carry shell quoting (-DX=\"...\"), as in a normal compile.
$(PCH_STAMP): $(PCH_HEADER) $(top_builddir)/config.h
	$(AM_V_GEN)rm -rf $(PCH_DIR) && $(MKDIR_P) $(PCH_DIR) && \
	seen=; n=0; \
	for flags in $(pch_sets); do \
	    key=`for w in $$flags; do case $$w in -D*|-U*|-p*|-f*|-O*|-m*) echo "$$w";; esac; done | sort | tr '\n' ' '`; \
	    case "$$seen" in *"<$$key>"*) continue;; esac; \
	    seen="$$seen<$$key>"; n=`expr $$n + 1`; \
	    eval "$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $$flags $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -x c++-header -o $(PCH_DIR)/v$$n.gch $(PCH_HEADER)" || exit 1; \
	done && \
	touch $@

$(libFbTk_a_OBJECTS) $(fluxbox_OBJECTS) $(fbrun_OBJECTS) $(fbsetroot_OBJECTS) \
$(fluxbox_remote_OBJECTS) $(fluxbox_update_configs_OBJECTS): $(PCH_STAMP)
# built with the plain AM_CPPFLAGS for LDADD (see src/Makemodule.am)
src/defaults.$(OBJEXT): $(PCH_STAMP)

clean-local:
	-rm -rf $(PCH_DIR) $(PCH_HEADER) $(PCH_STAMP)
endif

uninstall-hook:
	rmdir $(DESTDIR)$(pkgdatadir)

source-doc:
	doxygen Doxyfile
