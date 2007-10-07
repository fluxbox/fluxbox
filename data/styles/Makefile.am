SUBDIRS=Emerge BlueFlux bora_black bora_blue bora_green \
	zimek_bisque zimek_darkblue zimek_green \
	arch carp ostrich green_tea bloe

styledir = $(pkgdatadir)/styles
MAINTAINERCLEANFILES = Makefile.in

style_DATA = Makro MerleyKay Artwiz Flux \
	Nyz Operation Outcomes Results Shade LemonSpace \
	Twice qnx-photon BlueNight Meta

EXTRA_DIST=$(style_DATA)

uninstall-local:
	rmdir $(styledir)
distclean-local:
	rm -f *\~
