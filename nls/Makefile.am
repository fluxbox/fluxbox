# nls/Makefile.am for fluxbox - http://fluxbox.sf.net

NLSTEST = @NLS@
SUBDIRS = C da_DK es_ES et_EE fr_FR pt_BR ru_RU sv_SE \
	tr_TR it_IT pt_PT bg_BG ja_JP lv_LV de_DE nl_NL
MAINTAINERCLEANFILES = Makefile.in blackbox-nls.hh
EXTRA_DIST=blackbox-nls.hh convert.awk

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(pkgdatadir)/nls; \
	fi
distclean-local:
	rm -f *\~
