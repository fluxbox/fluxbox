FLUXBOX_SRC_DIR=	../../src/
INCLUDES= 		-I$(top_srcdir)/src -I$(top_srcdir)/src/FbTk
bin_PROGRAMS= 		fbrun
fbrun_SOURCES= 		FbRun.hh FbRun.cc main.cc fbrun.xpm
fbrun_LDADD=		${FLUXBOX_SRC_DIR}FbTk/libFbTk.a

${FLUXBOX_SRC_DIR}FbTk/libFbTk.a:
	cd ${FLUXBOX_SRC_DIR}FbTk && make