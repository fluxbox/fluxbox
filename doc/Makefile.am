# doc/Makefile.am for Blackbox 0.61.x  - an X11 Window manager

CLEANFILES = fluxbox.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = fluxbox.1 bsetroot.1
EXTRA_DIST=fluxbox.1.in bsetroot.1 Coding_style
distclean-local:
	rm -f *\~

fluxbox.1: fluxbox.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," fluxbox.1.in > fluxbox.1
