# Makefile.am for Fluxbox 0.1.x 

SUBDIRS = data doc nls src util
MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in stamp-h.in
ACLOCAL_AMFLAGS = -I .

uninstall-local:
	rmdir $(pkgdatadir)
distclean-local:
	rm -f *\~

source-doc:
	doxygen Doxyfile
