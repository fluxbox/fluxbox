# doc/Makefile.am for Fluxbox 0.9  - an X11 Window manager
# Makefile.am for fluxbox/doc

SUBDIRS = ru
CLEANFILES = fluxbox.1 fbrun.1 fluxbox-apps.5 fluxbox-keys.5 fluxbox-menu.5 fluxbox-remote.1 fluxbox-style.5
MAINTAINERCLEANFILES = Makefile.in
man_MANS = fbsetroot.1 fbrun.1 fbsetbg.1 startfluxbox.1 \
	fluxbox.1 fluxbox-apps.5 fluxbox-keys.5 fluxbox-menu.5 fluxbox-remote.1 fluxbox-style.5
EXTRA_DIST=fbsetroot.1 fbsetbg.1 startfluxbox.1 CODESTYLE \
	fluxbox.1.in fbrun.1.in fluxbox-apps.5.in fluxbox-keys.5.in fluxbox-menu.5.in fluxbox-remote.1.in fluxbox-style.5.in
distclean-local:
	rm -f *\~

%: %.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," $^ > $@
