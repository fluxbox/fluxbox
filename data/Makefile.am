# data/Makefile.am for Fluxbox - an X11 Window manager

SUBDIRS = styles
CLEANFILES = menu menu.in menu.pre init
MAINTAINERCLEANFILES = Makefile.in
pkgdata_DATA = titlebar keys menu init
EXTRA_DIST= titlebar keys init.in generate_menu README.style README.menu README
all-local: menu init
distclean-local:
	rm -f *\~
init: init.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," init.in > init
menu: generate_menu
	sh ./generate_menu
	@regex_cmd@ -e "s,\@VERSION\@,$(VERSION)," menu.in > menu.pre
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," menu.pre > menu
