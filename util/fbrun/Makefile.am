FLUXBOX_SRC_DIR=../../src/
INCLUDES = -I${FLUXBOX_SRC_DIR}
bin_PROGRAMS = fbrun
fbrun_SOURCES = FbRun.hh FbRun.cc main.cc
fbrun_LDADD = ${FLUXBOX_SRC_DIR}Font.o

fbrun.o: ${FLUXBOX_SRC_DIR}/Font.hh
