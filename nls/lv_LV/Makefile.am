# nls/lv_LV/Makefile.am for Blackbox-0.61.x - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m fluxbox.m common.m main.m bsetroot.m
EXTRA_DIST=$(MFILES)
all-local: fluxbox.cat
install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/lv_LV"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/lv_LV; \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/lv_LV; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/lv; \
		ln -fs lv_LV $(DESTDIR)$(pkgdatadir)/nls/lv; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/lv_LV/fluxbox.cat; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/lv_LV; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		LANG=lv_LV $(gencat_cmd) fluxbox.cat Translation.m; \
	fi
