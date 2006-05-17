# doc/Makefile.am for Fluxbox 0.9  - an X11 Window manager
# Makefile.am for fluxbox/doc

SUBDIRS = ru
CLEANFILES = fluxbox.1 fluxstyle.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = fluxbox.1 fbsetroot.1 fbrun.1 fbsetbg.1 \
	startfluxbox.1 fluxstyle.1 fbrun.1 
EXTRA_DIST=fluxbox.1.in fbsetroot.1 fbrun.1 startfluxbox.1 fbsetbg.1 \
	fluxstyle.1.in CODESTYLE
distclean-local:
	rm -f *\~

fluxbox.1: fluxbox.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," $(srcdir)/fluxbox.1.in > fluxbox.1

fluxstyle.1: fluxstyle.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," $(srcdir)/fluxstyle.1.in > fluxstyle.1
