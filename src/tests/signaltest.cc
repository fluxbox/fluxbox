// signaltest.cc for testing signal handler in fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "../FbTk/SignalHandler.hh"

#include <iostream>
#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif

using namespace std;
using namespace FbTk;

class IntSig:public SignalEventHandler {
public:
    void handleSignal(int signum) {
        assert(signum == SIGINT);
        cerr<<"Signal SIGINT!"<<endl;
        exit(0);
    }
};

class AllSig:public SignalEventHandler {
public:
    void handleSignal(int signum) {
        switch (signum) {
        case SIGSEGV:
            cerr<<"SIGSEGV";
            break;
        case SIGTERM:
            cerr<<"SIGTERM";
            break;
        case SIGUSR1:
            cerr<<"SIGUSR1";
            break;
        case SIGUSR2:
            cerr<<"SIGUSR2";
            break;
        default:
            cerr<<"signum = "<<signum;
        }
        cerr<<endl;
        if (signum == SIGTERM)
            exit(1); // end program
    }
};

int main(int argc, char **argv) {
    SignalHandler &sigh = SignalHandler::instance();
    
    IntSig handler;
    AllSig allhand;
	
    sigh.registerHandler(SIGINT, &handler);
    sigh.registerHandler(SIGSEGV, &allhand);
    sigh.registerHandler(SIGTERM, &allhand);
    sigh.registerHandler(SIGUSR1, &allhand);
    sigh.registerHandler(SIGUSR2, &allhand);

    cerr<<"Send signals to me :)"<<endl;
    while (1) {
	
    }
}


