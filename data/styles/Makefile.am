# data/styles/Makefile.am for Blackbox 0.61.x  - an X11 Window manager
SUBDIRS=Emerge BlueFlux
styledir = $(pkgdatadir)/styles
MAINTAINERCLEANFILES = Makefile.in

style_DATA = Makro MerleyKay Clean Artwiz Cthulhain Flux Minimal \
	Nyz Operation Outcomes Rampage Results Shade Spiff LemonSpace \
	Twice Carbondioxide qnx-photon BlueNight Meta
EXTRA_DIST=$(style_DATA)

uninstall-local:
	rmdir $(styledir)
distclean-local:
	rm -f *\~
