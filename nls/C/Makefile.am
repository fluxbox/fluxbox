# nls/C/Makefile.am for Blackbox-0.61.x - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m
EXTRA_DIST= $(MFILES)
all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/C"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/C; \
		$(INSTALL_DATA) blackbox.cat $(DESTDIR)$(pkgdatadir)/nls/C; \
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
		rm -f $(DESTDIR)$(pkgdatadir)/nls/C/blackbox.cat; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/US_ASCII; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/POSIX; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en_US; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/en; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/C; \
	fi

# note... when adding new catalogs... you do not need the
# header=../blackbox-nls.hh which is passed to awk... we
# only need to generate it once.

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		echo "//This file is generated from nls/C" > ../blackbox-nls.hh; \
		echo "#ifndef BLACKBOX_NLS_HH" >> ../blackbox-nls.hh; \
		echo "#define BLACKBOX_NLS_HH" >> ../blackbox-nls.hh; \
		echo "namespace FBNLS {" >> ../blackbox-nls.hh;	\
		echo "enum {" >> ../blackbox-nls.hh;	\
		awk -f ../convert.awk header=../blackbox-nls.hh \
			output=Translation.m $(MFILES); \
		echo "	dummy_not_used = 0 //just for the ending" >> ../blackbox-nls.hh; \
		echo "}; //end enum" >> ../blackbox-nls.hh; \
		echo "}; //end namespace" >> ../blackbox-nls.hh; \
		echo "#endif //BLACKBOX_NLS_HH" >> ../blackbox-nls.hh; \
	fi

blackbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) blackbox.cat Translation.m; \
	fi
