# nls/fr_FR/Makefile.am for Fluxbox - www.fluxbox.org

THE_LANG = fr_FR

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in 
MFILES = Translation.m
EXTRA_DIST= $(MFILES)

all-local: fluxbox.cat
install-data-local: fluxbox.cat
        @if test x$(NLSTEST) = "x-DNLS"; then                 echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/$(THE_LA
NG)";                 $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG);                 $(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/$(THE_LA
NG);         fi

uninstall-local:
        @if test x$(NLSTEST) = "x-DNLS"; then                 rm -f $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG)/fluxbox.cat;                 rmdir $(DESTDIR)$(pkgdatadir)/nls/$(THE_LANG);         fi

fluxbox.cat: Translation.m
        @if test x$(NLSTEST) = "x-DNLS"; then                 $(gencat_cmd) fluxbox.cat Translation.m;         fi

