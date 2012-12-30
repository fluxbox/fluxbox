# nls/uk_UA/Makefile.am for Fluxbox - www.fluxbox.org

THE_LANG = uk_UA
SRC_CODESET = KOI8-U
DEST_CODESETS = KOI8-U UTF-8

LOCALE_PATH = @LOCALE_PATH@
NLSTEST = @NLS@
MFILES = Translation.m
GENERATED_MFILES = $(patsubst %,generated-%.m,$(DEST_CODESETS))
MAINTAINERCLEANFILES = Makefile.in $(GENERATED_MFILES)
CATFILES = $(patsubst %,fluxbox-%.cat,$(DEST_CODESETS))

# We distribute the generated files so that users don't need iconv
EXTRA_DIST= $(MFILES) $(GENERATED_MFILES)
CLEANFILES = $(CATFILES)

all-local: $(CATFILES)
install-data-local: $(CATFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		for codeset in $(DEST_CODESETS); do \
			echo "Installing catalog in $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG).$${codeset}"; \
			$(mkinstalldirs) $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG).$${codeset}; \
			$(INSTALL_DATA) fluxbox-$${codeset}.cat $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG).$${codeset}/fluxbox.cat; \
		done; \
	fi

# not part of the normal build process
translations: $(GENERATED_MFILES)

generated-%.m: Translation.m
	iconv -f $(SRC_CODESET) -t $* $(srcdir)/Translation.m | sed s/$(SRC_CODESET)/$*/ > $@

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		for codeset in $(DEST_CODESETS); do \
			rm -f $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG).$${codeset}/fluxbox.cat; \
			rmdir $(DESTDIR)$(LOCALE_PATH)/$(THE_LANG).$${codeset}; \
		done; \
	fi

fluxbox-%.cat: generated-%.m Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Creating catfile for $*"; \
		$(gencat_cmd) fluxbox-$*.cat $(srcdir)/generated-$*.m; \
	fi
