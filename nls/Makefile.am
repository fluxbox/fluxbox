# nls/Makefile.am for fluxbox - http://fluxbox.sf.net

NLSTEST = @NLS@
SUBDIRS = C cs_CZ da_DK es_ES et_EE fr_FR pt_BR ru_RU \
		  sv_SE tr_TR it_IT pt_PT bg_BG ja_JP ko_KR \
		  lv_LV de_DE nl_NL sl_SI pl_PL vi_VN

MAINTAINERCLEANFILES = Makefile.in fluxbox-nls.hh
EXTRA_DIST=fluxbox-nls.hh nlsinfo
BUILT_SOURCES=fluxbox-nls.hh

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(pkgdatadir)/nls; \
	fi
distclean-local:
	rm -f *\~

fluxbox-nls.hh:
	chmod +x nlsinfo
	./nlsinfo -H -N FBNLS $(top_srcdir)/src $(top_srcdir)/util > fluxbox-nls.hh
