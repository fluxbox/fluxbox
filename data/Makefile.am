# data/Makefile.am for Fluxbox - an X11 Window manager

SUBDIRS = styles
bin_SCRIPTS = fluxbox-generate_menu
CLEANFILES = menu menu.in menu.pre init
MAINTAINERCLEANFILES = Makefile.in
pkgdata_DATA = keys menu init
EXTRA_DIST= keys init.in fluxbox-generate_menu README.style README.menu README
all-local: menu init
distclean-local:
	rm -f *\~
init: init.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" init.in > init
menu: fluxbox-generate_menu
	sh ./fluxbox-generate_menu -o menu -p ${prefix}/share -m "Fluxbox-${VERSION}"
