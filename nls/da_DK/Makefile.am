# nls/da_DK/Makefile.am for Blackbox-0.61.x - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m fluxbox.m common.m main.m bsetroot.m
EXTRA_DIST= $(MFILES)
all-local: fluxbox.cat
install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/da_DK"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/da_DK; \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/da_DK; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/da_DK/fluxbox.cat; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/da_DK; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		LANG=da_DK $(gencat_cmd) fluxbox.cat Translation.m; \
	fi
