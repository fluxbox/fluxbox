FLUXBOX_SRC_DIR=../../src/
INCLUDES = -I${FLUXBOX_SRC_DIR}
bin_PROGRAMS = fbrun
fbrun_SOURCES = FbRun.hh FbRun.cc main.cc

if XFT
xft_OBJ = ${FLUXBOX_SRC_DIR}/XftFontImp.o
endif
if MULTIBYTE
xmb_OBJ = ${FLUXBOX_SRC_DIR}/XmbFontImp.o
endif

fbrun_LDADD = ${FLUXBOX_SRC_DIR}Font.o ${FLUXBOX_SRC_DIR}StringUtil.o ${FLUXBOX_SRC_DIR}/Timer.o \
	${FLUXBOX_SRC_DIR}/BaseDisplay.o ${FLUXBOX_SRC_DIR}/i18n.o \
	${xft_OBJ} ${xmb_OBJ} ${FLUXBOX_SRC_DIR}/XFontImp.o

fbrun.o: ${FLUXBOX_SRC_DIR}/Font.hh ${FLUXBOX_SRC_DIR}/BaseDisplay.hh
