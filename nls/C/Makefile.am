# nls/C/Makefile.am for Fluxbox - an X11 Window Manager

THE_LANG = C
# no CODESET

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m fluxbox.m common.m main.m bsetroot.m
EXTRA_DIST= $(MFILES)
all-local: fluxbox.cat
install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG)"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG); \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG); \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/US_ASCII; \
		ln -fs C $(DESTDIR)$(pkgdatadir)/nls/US_ASCII; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/POSIX; \
		ln -fs C $(DESTDIR)$(pkgdatadir)/nls/POSIX; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en_US; \
		ln -fs C $(DESTDIR)$(pkgdatadir)/nls/en_US; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en; \
		ln -fs C $(DESTDIR)$(pkgdatadir)/nls/en; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG)/fluxbox.cat; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/US_ASCII; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/POSIX; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en_US; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG); \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk \
			output=Translation.m $(MFILES); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) fluxbox.cat Translation.m; \
	fi
