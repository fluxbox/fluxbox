# util/Makefile.am for Fluxbox
SUBDIRS=		fbrun
INCLUDES= 		-I../src/FbTk/
bin_SCRIPTS= 		fbsetbg fluxbox-generate_menu startfluxbox
bin_PROGRAMS=		bsetroot
bsetroot_SOURCES=	bsetroot.cc bsetroot.hh
bsetroot_LDADD=		../src/FbRootWindow.o ../src/I18n.o \
			../src/FbAtoms.o ../src/FbTk/libFbTk.a

MAINTAINERCLEANFILES=	Makefile.in
EXTRA_DIST=		fbsetbg fluxbox-generate_menu.in \
				startfluxbox.in

all-local:	startfluxbox

clean-local:
		rm -f startfluxbox fluxbox-generate_menu

bsetroot.o: 	bsetroot.cc ../config.h bsetroot.hh \
			../src/FbRootWindow.hh ../src/FbAtoms.hh

startfluxbox: 	startfluxbox.in
		@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" \
		            -e "s,@pkgbindir@,$(bindir),g" \
				startfluxbox.in > startfluxbox
		-chmod 755 startfluxbox

fluxbox-generate_menu: fluxbox-generate_menu.in
		@regex_cmd@ -e "s,@PREFIX@,$(prefix),g" \
				fluxbox-generate_menu.in \
				> fluxbox-generate_menu
		-chmod 755 fluxbox-generate_menu

../src/FbRootWindow.o:
	cd ../src && make FbRootWindow.o
../src/I18n.o:
	cd ../src && make I18n.o
../src/FbAtoms.o:
	cd ../src && make FbAtoms.o
