# nls/it_IT/Makefile.am for Fluxbox - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m
EXTRA_DIST= $(MFILES)

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/it_IT"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/it_IT; \
		$(INSTALL_DATA) blackbox.cat $(DESTDIR)$(pkgdatadir)/nls/it_IT; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/it_IT/blackbox.cat; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/it_IT; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

blackbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) blackbox.cat Translation.m; \
	fi
