# util/Makefile.am for Fluxbox
SUBDIRS=		fbrun
INCLUDES= 		-I../src/FbTk/
bin_SCRIPTS= 		fbsetbg fluxbox-generate_menu startfluxbox
bin_PROGRAMS=		bsetroot
bsetroot_SOURCES=	bsetroot.cc bsetroot.hh
bsetroot_LDADD=		../src/FbRootWindow.o ../src/I18n.o \
			../src/FbAtoms.o ../src/FbTk/libFbTk.a

MAINTAINERCLEANFILES=	Makefile.in
EXTRA_DIST=		fbsetbg fluxbox-generate_menu startfluxbox.in

all-local:	startfluxbox

distclean-local:
		rm -f *\~ Makefile.in

clean-local:
		rm -f startfluxbox

bsetroot.o: 	bsetroot.cc ../config.h bsetroot.hh ../src/FbRootWindow.hh ../src/FbAtoms.hh

startfluxbox: 	startfluxbox.in
		@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" \
		            -e "s,@pkgbindir@,$(bindir),g" \
				startfluxbox.in > startfluxbox

../src/FbRootWindow.o:
	cd ../src && make FbRootWindow.o
../src/I18n.o:
	cd ../src && make I18n.o
../src/FbAtoms.o:
	cd ../src && make FbAtoms.o
