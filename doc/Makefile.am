# doc/Makefile.am for Fluxbox 0.9  - an X11 Window manager

SUBDIRS = ru
CLEANFILES = fluxbox.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = fluxbox.1 bsetroot.1 fbrun.1 fbsetbg.1 \
	startfluxbox.1 fluxstyle.1 fbrun.1 
EXTRA_DIST=fluxbox.1.in bsetroot.1 fbrun.1 startfluxbox.1 fbsetbg.1 \
	fluxstyle.1 Coding_style
distclean-local:
	rm -f *\~

fluxbox.1: fluxbox.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," $(srcdir)/fluxbox.1.in > fluxbox.1
