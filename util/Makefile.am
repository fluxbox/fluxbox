# util/Makefile.am for Blackbox 0.61.x  - an X11 Window manager

CPPFLAGS= @CPPFLAGS@ @DEBUG@ @NLS@ @TIMEDCACHE@ @NEWWMSPEC@ @INTERLACE@ \
@ORDEREDPSEUDO@ @KDE@ @GNOME@ @XINERAMA@

bin_SCRIPTS = bsetbg
bin_PROGRAMS =		bsetroot
bsetroot_SOURCES =	bsetroot.cc bsetroot.hh
bsetroot_LDADD =	../src/BaseDisplay.o ../src/Timer.o ../src/Image.o ../src/i18n.o ../src/FbAtoms.o
MAINTAINERCLEANFILES =	Makefile.in
EXTRA_DIST=bsetbg

distclean-local:
	rm -f *\~

bsetroot.o: bsetroot.cc ../config.h bsetroot.hh ../src/BaseDisplay.hh \
 ../src/Timer.hh ../src/Image.hh ../src/FbAtoms.hh
