# nls/C/Makefile.am for Fluxbox - www.fluxbox.org

THE_LANG = C

LOCALE_PATH = @LOCALE_PATH@
NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in 
MFILES = Translation.m
EXTRA_DIST= $(MFILES)

all-local: fluxbox.cat
install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG)"; \
		$(mkinstalldirs) $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG); \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG); \
	fi

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG)/fluxbox.cat; \
		rmdir $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) fluxbox.cat $(srcdir)/Translation.m; \
	fi

