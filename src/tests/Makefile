OBJ=StringUtiltest.o ../StringUtil.o
CXX=g++
CXXFLAGS= -I.. -DDEBUG -Wall -g -O2
LIBS=
XFLAGS= -I/usr/X11R6/include
XLIBS= -L/usr/X11R6/lib -lX11
COMPILEFILE=$(CXX) -c $(CXXFLAGS)

all: testStringUtil testKeys testResource testSignal

.cc.o:
	$(CXX) -c $(CXXFLAGS) $<
	
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


run: testResource testKeys testStringUtil 
	./testKeys
	./testStringUtil
	./testResource
	./testSignal
clean:
	rm -f *.o
