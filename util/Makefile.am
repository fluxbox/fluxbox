# util/Makefile.am for Fluxbox 
SUBDIRS = fbrun
INCLUDES=-I../src/FbTk/
bin_SCRIPTS = fbsetbg fluxbox-generate_menu
bin_PROGRAMS =		bsetroot
bsetroot_SOURCES =	bsetroot.cc bsetroot.hh
bsetroot_LDADD =	../src/BaseDisplay.o ../src/i18n.o ../src/FbAtoms.o \
	../src/FbTk/libFbTk.a

MAINTAINERCLEANFILES =	Makefile.in
EXTRA_DIST=fbsetbg fluxbox-generate_menu

distclean-local:
	rm -f *\~ Makefile.in

bsetroot.o: bsetroot.cc ../config.h bsetroot.hh ../src/BaseDisplay.hh ../src/FbAtoms.hh

