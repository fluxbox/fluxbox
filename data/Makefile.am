# data/Makefile.am for Fluxbox - an X11 Window manager

PROGRAM_PREFIX=@program_prefix@
PROGRAM_SUFFIX=@program_suffix@

DEFAULT_MENU = @DEFAULT_MENU@
DEFAULT_STYLE = @DEFAULT_STYLE@
DEFAULT_KEYS = @DEFAULT_KEYS@
DEFAULT_INIT = @DEFAULT_INIT@

SUBDIRS= 		styles
CLEANFILES= 		menu menu.in menu.pre menu.bak init
MAINTAINERCLEANFILES= 	Makefile.in
pkgdata_DATA= 		keys menu init
EXTRA_DIST= 		keys init.in
all-local: 		menu init
install-pkgdataDATA:
	@echo "Installing menu file in $(DESTDIR)$(DEFAULT_MENU)"
	$(mkinstalldirs) $(DESTDIR)$(dir $(DEFAULT_MENU))
	$(INSTALL_DATA) menu $(DESTDIR)$(DEFAULT_MENU)
	@echo "Installing keys file in $(DESTDIR)$(DEFAULT_KEYS)"
	$(mkinstalldirs) $(DESTDIR)$(dir $(DEFAULT_KEYS))
	$(INSTALL_DATA) $(srcdir)/keys $(DESTDIR)$(DEFAULT_KEYS)
	@echo "Installing init file in $(DESTDIR)$(DEFAULT_INIT)"
	$(mkinstalldirs) $(DESTDIR)$(dir $(DEFAULT_INIT))
	$(INSTALL_DATA) init $(DESTDIR)$(DEFAULT_INIT)

distclean-local:
		rm -f *\~
init: 			init.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" \
			-e "s,@pkgprefix@,$(PROGRAM_PREFIX:NONE=),g" \
			-e "s,@pkgsuffix@,$(PROGRAM_SUFFIX:NONE=),g" \
			-e "s,@default_style@,$(DEFAULT_STYLE),g" $(srcdir)/init.in > init

menu: 			../util/fluxbox-generate_menu
	env	MENUFILENAME=menu \
		PREFIX="${prefix}" \
		MENUTITLE="Fluxbox-${VERSION}" \
		INSTALL=Yes \
		../util/fluxbox-generate_menu

../util/fluxbox-generate_menu:
	cd ../util && ${MAKE} fluxbox-generate_menu
