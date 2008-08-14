# nls/Makefile.am for fluxbox - http://fluxbox.org

LOCALE_PATH = @LOCALE_PATH@
NLSTEST = @NLS@
SUBDIRS = C cs_CZ da_DK es_AR es_ES et_EE fr_FR pt_BR ru_RU \
		  sv_SE tr_TR it_IT pt_PT bg_BG ja_JP ko_KR \
		  lv_LV de_DE nl_NL no_NO sl_SI pl_PL vi_VN be_BY \
		  uk_UA el_GR nb_NO zh_CN fi_FI en_US en_GB de_CH \
                  de_AT fr_CH sk_SK zh_TW mk_MK

MAINTAINERCLEANFILES = Makefile.in fluxbox-nls.hh
EXTRA_DIST=fluxbox-nls.hh nlsinfo
BUILT_SOURCES=fluxbox-nls.hh

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(LOCALE_PATH); \
	fi
distclean-local:
	rm -f *\~

fluxbox-nls.hh:
	chmod +x $(srcdir)/nlsinfo
	$(srcdir)/nlsinfo -H -N FBNLS $(top_srcdir)/src $(top_srcdir)/util > fluxbox-nls.hh
