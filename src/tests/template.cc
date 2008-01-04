// template.cc for fbtk test suite
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

/*
 * Use this file as a template for new test applications
 *
 */
#include "App.hh"
#include "FbWindow.hh"
#include "Font.hh"
#include "EventHandler.hh"
#include "EventManager.hh"
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
              0, 0, 640, 480, KeyPressMask | ExposureMask | 
              ButtonPressMask | ButtonReleaseMask | ButtonMotionMask) { 

        m_win.show();
        m_win.setBackgroundColor(FbTk::Color("white", m_win.screenNumber()));
        FbTk::EventManager::instance()->add(*this, m_win);
    }

    ~App() {
    }

    void keyPressEvent(XKeyEvent &ke) {
        cerr<<"KeyPress"<<endl;
        KeySym ks;
        char keychar[1];
        XLookupString(&ke, keychar, 1, &ks, 0);
        if (ks == XK_Escape)
            end();
    }
    void buttonPressEvent(XButtonEvent &be) {
        cerr<<"ButtonPress"<<endl;
    }
    void buttonReleaseEvent(XButtonEvent &be) {
        cerr<<"ButtonRelease"<<endl;
    }
    void motionNotifyEvent(XMotionEvent &event) {
        cerr<<"MotionNotify"<<endl;
    }
    void exposeEvent(XExposeEvent &event) {
        cerr<<"ExposeEvent"<<endl;
        redraw();
    }

    void redraw() {

    }


private:
    FbTk::FbWindow m_win;

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
