# doc/ru/Makefile.am for Fluxbox 0.9  - an X11 Window manager

CLEANFILES = fluxbox.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = fluxbox.1 
EXTRA_DIST=fluxbox.1.in 
distclean-local:
	rm -f *\~

fluxbox.1: fluxbox.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," $(srcdir)/fluxbox.1.in > fluxbox.1
