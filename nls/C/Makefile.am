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

# note... when adding new catalogs... you do not need the
# header=../fluxbox-nls.hh which is passed to awk... we
# only need to generate it once.

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		echo "//This file is generated from nls/C" > ../fluxbox-nls.hh; \
		echo "#ifndef BLACKBOX_NLS_HH" >> ../fluxbox-nls.hh; \
		echo "#define BLACKBOX_NLS_HH" >> ../fluxbox-nls.hh; \
		echo "namespace FBNLS {" >> ../fluxbox-nls.hh;	\
		echo "enum {" >> ../fluxbox-nls.hh;	\
		awk -f ../convert.awk header=../fluxbox-nls.hh \
			output=Translation.m $(MFILES); \
		echo "	dummy_not_used = 0 //just for the ending" >> ../fluxbox-nls.hh; \
		echo "}; //end enum" >> ../fluxbox-nls.hh; \
		echo "}; //end namespace" >> ../fluxbox-nls.hh; \
		echo "#endif //BLACKBOX_NLS_HH" >> ../fluxbox-nls.hh; \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) fluxbox.cat Translation.m; \
	fi
