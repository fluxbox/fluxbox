CXX=g++
CXXFLAGS= -I.. -I../FbTk -I../.. -DDEBUG -DUSE_XFT -Wall -g -O2 ../FbTk/libFbTk.a
LIBS= 
XFLAGS= -I/usr/X11R6/include
XLIBS= -L/usr/X11R6/lib -lX11 -lXft -lXpm -lImlib2
COMPILEFILE=$(CXX) -c $(CXXFLAGS)
FONT_OBJ = ../FbTk/libFbTk.a
COMPILE = ${CXX} ${CXXFLAGS} ${XLIBS}

all: testMenu testFont testTexture movetest

.cc.o:
	$(CXX) -c $(CXXFLAGS) $<

glxtest: ../FbTk/App.hh glxtest.cc
	${CXX} glxtest.cc ${CXXFLAGS} ${XLIBS} -lGL -lGLU -lXpm -o glxtest

testStringUtil: StringUtiltest.o
	$(CXX) $(LIBS) StringUtiltest.o ../FbTk/libFbTk.a -o testStringUtil

testKeys:  testKeys.o
	$(CXX) $(LIBS) $(XLIBS) testKeys.o -o testKeys

testResource: Resourcetest.o Resource.o
	${CXX} ${LIBS} ${XLIBS} Resourcetest.o Resource.o -o testResource

testTitle: titletest.cc
	${CXX} ${CXXFLAGS} ${LIBS} ${XLIBS} titletest.cc ../FbTk/libFbTk.a -o testTitle

signaltest:
	${COMPILE} ../FbTk/SignalHandler.o signaltest.cc -o signaltest

testFont: testFont.cc ${FONT_OBJ}
	${CXX} testFont.cc -o testFont ${CXXFLAGS} ${XLIBS} 

testMenu: menutest.cc 
	${CXX} menutest.cc -o testMenu ${CXXFLAGS} ${XLIBS} ../FbMenuParser.o ../FbTk/libFbTk.a -lXpm -lImlib2 ../Parser.o

testLayers: testLayers.cc
	${CXX} ${CXXFLAGS} ${XLIBS} testLayers.cc ../FbTk/libFbTk.a -o testLayers

testTransp: transpTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} transpTest.cc ../FbTk/libFbTk.a -o testTransp

testScale: scaleTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} scaleTest.cc ../FbTk/libFbTk.a -lXpm -o testScale

testTexture: texturetest.cc
	${COMPILE} texturetest.cc ../FbTk/libFbTk.a -lXpm -o testTexture

testDemandAttention: testDemandAttention.cc
	${COMPILE} testDemandAttention.cc ../FbTk/libFbTk.a -lXpm -o testDemandAttention

testFullscreen: fullscreentest.cc
	${COMPILE} fullscreentest.cc ../FbTk/libFbTk.a -o testFullscreen

Parser.o: ../Parser.hh ../Parser.cc
	${COMPILE} ${CXXFLAGS} -c ../Parser.cc

FbMenuParser.o: ../FbMenuParser.hh ../FbMenuParser.cc ../Parser.hh
	${COMPILE} ${CXXFLAGS} -c ../FbMenuParser.cc

parsertest: parsertest.cc  ../Parser.hh FbMenuParser.o Parser.o
	${COMPILE} parsertest.cc FbMenuParser.o Parser.o ../FbTk/StringUtil.o -o parsertest

movetest: movetest.cc
	${COMPILE} movetest.cc ../FbTk/libFbTk.a -lXpm -o movetest


run: testLayers testResource testKeys testStringUtil testFont
	./testLayers
	./testKeys
	./testStringUtil
	./testResource
	./testSignal
	./testFont
clean:
	rm -f *.o
