# util/Makefile.am for Fluxbox
SUBDIRS=		fbrun
INCLUDES= 		-I$(top_srcdir)/src -I$(top_srcdir)/src/FbTk
bin_SCRIPTS= 		fbsetbg fluxbox-generate_menu startfluxbox
bin_PROGRAMS=		bsetroot
bsetroot_SOURCES=	bsetroot.cc bsetroot.hh
bsetroot_LDADD=		../src/FbRootWindow.o ../src/FbAtoms.o \
			../src/FbTk/libFbTk.a

MAINTAINERCLEANFILES=	Makefile.in
EXTRA_DIST=		fbsetbg fluxbox-generate_menu.in \
				startfluxbox.in

all-local:	startfluxbox

clean-local:
		rm -f startfluxbox fluxbox-generate_menu

bsetroot.o: 	bsetroot.cc ../config.h $(srcdir)/bsetroot.hh \
			$(top_srcdir)/src/FbRootWindow.hh $(top_srcdir)/src/FbAtoms.hh

startfluxbox: 	startfluxbox.in
		@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir),g" \
		            -e "s,@pkgbindir@,$(bindir),g" \
				$(srcdir)/startfluxbox.in > startfluxbox
		-chmod 755 startfluxbox

fluxbox-generate_menu: fluxbox-generate_menu.in
		@regex_cmd@ -e "s,@PREFIX@,$(prefix),g" \
				$(srcdir)/fluxbox-generate_menu.in \
				> fluxbox-generate_menu
		-chmod 755 fluxbox-generate_menu

../src/FbRootWindow.o:
	cd ../src && ${MAKE} FbRootWindow.o
../src/FbAtoms.o:
	cd ../src && ${MAKE} FbAtoms.o
