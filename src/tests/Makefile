CXX=g++
CXXFLAGS= -I.. -I../FbTk -DDEBUG -DUSE_XFT -Wall -g -O2 ../FbTk/libFbTk.a
LIBS= 
XFLAGS= -I/usr/X11R6/include
XLIBS= -L/usr/X11R6/lib -lX11 -lXft
COMPILEFILE=$(CXX) -c $(CXXFLAGS)
FONT_OBJ = ../FbTk/libFbTk.a
COMPILE = ${CXX} ${CXXFLAGS} ${XLIBS}

all: testFont testTexture

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

testSignal: signaltest.cc 
	${CXX} ${LIBS} signaltest.cc -o testSignal

testFont: testFont.cc ${FONT_OBJ}
	${CXX} testFont.cc -o testFont ${CXXFLAGS} ${XLIBS} 

testLayers: testLayers.cc
	${CXX} ${CXXFLAGS} ${XLIBS} testLayers.cc ../FbTk/libFbTk.a -o testLayers

testTransp: transpTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} transpTest.cc ../FbTk/libFbTk.a -o testTransp

testScale: scaleTest.cc
	${CXX} ${CXXFLAGS} ${XLIBS} scaleTest.cc ../FbTk/libFbTk.a -lXpm -o testScale

testTexture: texturetest.cc
	${COMPILE} texturetest.cc ../FbTk/libFbTk.a -lXpm -o testTexture


run: testLayers testResource testKeys testStringUtil testFont
	./testLayers
	./testKeys
	./testStringUtil
	./testResource
	./testSignal
	./testFont
clean:
	rm -f *.o
