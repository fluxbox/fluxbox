CXX=g++
CXXFLAGS= -I.. -DDEBUG -DUSE_XFT -Wall -g -O2
LIBS=
XFLAGS= -I/usr/X11R6/include
XLIBS= -L/usr/X11R6/lib -lX11 -lXft
COMPILEFILE=$(CXX) -c $(CXXFLAGS)
FONT_OBJ = ../BaseDisplay.o ../Font.o ../XFontImp.o ../XftFontImp.o ../XmbFontImp.o \
	../Timer.o ../StringUtil.o ../i18n.o

all: testStringUtil testKeys testResource testSignal

.cc.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@
	
../XftFontImp.o: ../XftFontImp.hh ../XftFontImp.cc

../XmbFontImp.o: ../XmbFontImp.hh ../XmbFontImp.cc

../XFontImp.o: ../XFontImp.hh ../XFontImp.cc

../Font.o: ../XFontImp.hh ../XftFontImp.hh ../XmbFontImp.hh ../Font.hh ../Font.cc

StringUtil.o: ../StringUtil.cc ../StringUtil.hh
	$(CXX) -c $(CXXFLAGS) ../StringUtil.cc -o StringUtil.o

Keys.o: ../Keys.cc ../Keys.hh
	$(CXX) -c $(CXXFLAGS) $(XFLAGS) ../Keys.cc -o Keys.o
	
Resource.o: ../Resource.cc ../Resource.hh
	$(CXX) -c $(CXXFLAGS) $(XFLAGS) ../Resource.cc -o Resource.o

../SignalHandler.o:
	${COMPILEFILE} ../SignalHandler.cc -o ../SignalHandler.o

testStringUtil: StringUtiltest.o StringUtil.o
	$(CXX) $(LIBS) StringUtiltest.o StringUtil.o -o testStringUtil

testKeys: Keys.o testKeys.o StringUtil.o
	$(CXX) $(LIBS) $(XLIBS) StringUtil.o Keys.o testKeys.o -o testKeys

testResource: Resourcetest.o Resource.o
	${CXX} ${LIBS} ${XLIBS} Resourcetest.o Resource.o -o testResource

testSignal: signaltest.cc ../SignalHandler.o
	${CXX} ${LIBS} signaltest.cc ../SignalHandler.o -o testSignal

testFont: testFont.cc ${FONT_OBJ}
	${CXX} ${CXXFLAGS} ${XLIBS} testFont.cc ${FONT_OBJ} -o testFont

run: testResource testKeys testStringUtil testFont
	./testKeys
	./testStringUtil
	./testResource
	./testSignal
	./testFont
clean:
	rm -f *.o
