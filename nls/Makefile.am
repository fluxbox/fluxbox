# nls/Makefile.am for Blackbox 0.61.x - an X11 Window Manager

NLSTEST = @NLS@
SUBDIRS = C da_DK es_ES et_EE fr_FR pt_BR ru_RU sv_SE tr_TR it_IT
MAINTAINERCLEANFILES = Makefile.in blackbox-nls.hh
EXTRA_DIST=blackbox-nls.hh convert.awk

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(pkgdatadir)/nls; \
	fi
distclean-local:
	rm -f *\~
