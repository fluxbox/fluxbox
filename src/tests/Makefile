CXX=g++
CXXFLAGS= -I.. -I../FbTk -DDEBUG -DUSE_XFT -Wall -g -O2 ../FbTk/libFbTk.a
LIBS= 
XFLAGS= -I/usr/X11R6/include
XLIBS= -L/usr/X11R6/lib -lX11 -lXft
COMPILEFILE=$(CXX) -c $(CXXFLAGS)
FONT_OBJ = ../BaseDisplay.o ../FbTk/Font.o ../FbTk/XFontImp.o ../FbTk/XftFontImp.o ../FbTk/XmbFontImp.o \
	../FbTk/Timer.o ../StringUtil.o ../i18n.o

all: testLayers testStringUtil testKeys testResource testSignal

.cc.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

../FbTk/XftFontImp.o: ../FbTk/XftFontImp.hh ../FbTk/XftFontImp.cc

../FbTk/XmbFontImp.o: ../FbTk/XmbFontImp.hh ../FbTk/XmbFontImp.cc

../FbTk/XFontImp.o: ../FbTk/XFontImp.hh ../FbTk/XFontImp.cc

../FbTk/Font.o: ../FbTk/XFontImp.hh ../FbTk/XftFontImp.hh ../FbTk/XmbFontImp.hh ../FbTk/Font.hh ../FbTk/Font.cc

StringUtil.o: ../StringUtil.cc ../StringUtil.hh
	$(CXX) -c $(CXXFLAGS) ../StringUtil.cc -o StringUtil.o

Keys.o: ../Keys.cc ../Keys.hh
	$(CXX) -c $(CXXFLAGS) $(XFLAGS) ../Keys.cc -o Keys.o

Resource.o: ../Resource.cc ../Resource.hh
	$(CXX) -c $(CXXFLAGS) $(XFLAGS) ../Resource.cc -o Resource.o

../FbTk/SignalHandler.o:
	${COMPILEFILE} ../FbTk/SignalHandler.cc -o ../FbTk/SignalHandler.o

testStringUtil: StringUtiltest.o StringUtil.o
	$(CXX) $(LIBS) StringUtiltest.o StringUtil.o -o testStringUtil

testKeys: Keys.o testKeys.o StringUtil.o
	$(CXX) $(LIBS) $(XLIBS) StringUtil.o Keys.o testKeys.o -o testKeys

testResource: Resourcetest.o Resource.o
	${CXX} ${LIBS} ${XLIBS} Resourcetest.o Resource.o -o testResource

testSignal: signaltest.cc ../FbTk/SignalHandler.o
	${CXX} ${LIBS} signaltest.cc ../FbTk/SignalHandler.o -o testSignal

testFont: testFont.cc ${FONT_OBJ}
	${CXX} ${CXXFLAGS} ${XLIBS} testFont.cc ${FONT_OBJ} -o testFont

testLayers: testLayers.cc
	${CXX} ${CXXFLAGS} ${XLIBS} testLayers.cc ../FbTk/libFbTk.a -o testLayers

testTransp: transpTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} transpTest.cc ../FbTk/libFbTk.a -o testTransp

testScale: scaleTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} scaleTest.cc ../FbTk/libFbTk.a -lXpm -o testScale

run: testLayers testResource testKeys testStringUtil testFont
	./testLayers
	./testKeys
	./testStringUtil
	./testResource
	./testSignal
	./testFont
clean:
	rm -f *.o
