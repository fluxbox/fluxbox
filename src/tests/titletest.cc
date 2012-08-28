// titletest.cc for fbtk test suite
// Copyright (c) 2007 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "App.hh"
#include "FbWindow.hh"
#include "Font.hh"
#include "EventHandler.hh"
#include "EventManager.hh"
#include "Timer.hh"
#include "SimpleCommand.hh"
#include "stringstream.hh"
#include "GContext.hh"
#include "Color.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <iostream>
using namespace std;

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname):
        FbTk::App(displayname),
        m_win(DefaultScreen(display()),
              0, 0, 640, 100, KeyPressMask | ExposureMask | 
              ButtonPressMask | ButtonReleaseMask | ButtonMotionMask),
        m_tick( 0 ) { 

        m_win.show();
        m_win.setBackgroundColor(FbTk::Color("white", m_win.screenNumber()));
        FbTk::EventManager::instance()->add(*this, m_win);
        FbTk::RefCount<FbTk::Command> cmd(new FbTk::SimpleCommand<App>
                                          (*this,
                                           &App::updateTitle));
        m_timer.setTimeout(150 * FbTk::FbTime::IN_MILLISECONDS);
        m_timer.setCommand(cmd);
        m_timer.fireOnce(false);
        m_timer.start();

        updateTitle();
        m_win.clear();
        srand( time( 0 ) );
    }

    ~App() {
    }
    void eventLoop() {
        XEvent ev;
        while (!done()) {
            if (XPending(display())) {
                XNextEvent(display(), &ev);
                FbTk::EventManager::instance()->handleEvent(ev);
            } else {
                FbTk::Timer::updateTimers(ConnectionNumber(display()));
            }
        }
    }

    void updateTitle() {
        FbTk_ostringstream str;
        ++m_tick;
        for (int i = 0, n = rand( ) % 10; i < max( 1, n ); ++i) {
            str << " Tick #" << m_tick;            
        }

        m_win.setName( str.str().c_str() );
        // set _NET_WM_NAME
        Atom net_wm_name = XInternAtom(display(), "_NET_WM_NAME", False);
        Atom utf8_string = XInternAtom(display(), "UTF8_STRING", False);
        XChangeProperty(display(), m_win.window(), 
                        net_wm_name, utf8_string, 8,
                        PropModeReplace, 
                        (unsigned char*)str.str().c_str(), str.str().size() );
    }


private:
    FbTk::FbWindow m_win;
    FbTk::Timer m_timer;
    unsigned int m_tick;
};

int main(int argc, char **argv) {
    string displayname("");
    for (int a=1; a<argc; ++a) {
        if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        }
    }

    App app(displayname.c_str());

    app.eventLoop();
	
}
