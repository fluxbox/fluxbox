FLUXBOX_SRC_DIR=	../../src/
AM_CPPFLAGS= 		-I$(top_srcdir)/src -I$(top_srcdir)/src/FbTk -include $(top_builddir)/config.h
bin_PROGRAMS= 		fbrun
fbrun_SOURCES= 		FbRun.hh FbRun.cc main.cc fbrun.xpm
fbrun_LDADD=		${FLUXBOX_SRC_DIR}FbTk/libFbTk.a
fbrun_LDFLAGS = \
	$(FONTCONFIG_LIBS) \
	$(FRIBIDI_LIBS) \
	$(X11_LIBS) \
	$(XINERAMA_LIBS) \
	$(XFT_LIBS) \
	$(XPM_LIBS) \
	$(XRENDER_LIBS)

${FLUXBOX_SRC_DIR}FbTk/libFbTk.a:
	cd ${FLUXBOX_SRC_DIR}FbTk && make
