# data/Makefile.am for Fluxbox - an X11 Window manager

SUBDIRS= 		styles
CLEANFILES= 		menu menu.in menu.pre menu.bak init
MAINTAINERCLEANFILES= 	Makefile.in
pkgdata_DATA= 		keys menu init
EXTRA_DIST= 		keys init.in README.style README.menu README
all-local: 		menu init
distclean-local:
		rm -f *\~
init: 			init.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" $(srcdir)/init.in > init

menu: 			../util/fluxbox-generate_menu
	env	MENUFILENAME=menu \
		PREFIX="${prefix}" \
		MENUTITLE="Fluxbox-${VERSION}" \
		INSTALL=Yes \
		../util/fluxbox-generate_menu

../util/fluxbox-generate_menu:
	cd ../util && ${MAKE} fluxbox-generate_menu
