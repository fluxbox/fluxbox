# Makefile.am for Fluxbox - http://fluxbox.org

# data dir after util so that utils are built for path searching
# when generating menu

SUBDIRS = doc nls src util data
MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in stamp-h.in
ACLOCAL_AMFLAGS = -I .
EXTRA_DIST = acinclude.m4

uninstall-local:
	rmdir $(pkgdatadir)
distclean-local:
	rm -f *\~

source-doc:
	doxygen Doxyfile
