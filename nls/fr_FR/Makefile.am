# nls/fr_FR/Makefile.am for Blackbox-0.61.x - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m fluxbox.m common.m main.m bsetroot.m
EXTRA_DIST=$(MFILES)

all-local: fluxbox.cat
install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/fr_FR"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/fr_FR; \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/fr_FR; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/fr; \
		ln -fs fr_FR $(DESTDIR)$(pkgdatadir)/nls/fr; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/fr_FR/fluxbox.cat; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/fr; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/fr_FR; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		LANG=fr_FR $(gencat_cmd) fluxbox.cat Translation.m; \
	fi
