// testDemandAttention.cc for fbtk test suite
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "FbTk/App.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/Font.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/GContext.hh"
#include "FbTk/Color.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Timer.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <string.h>
#include <iostream>

using namespace std;

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname):
        FbTk::App(displayname),
        m_win(DefaultScreen(display()),
              0, 0, 640, 480, KeyPressMask | ExposureMask | 
              ButtonPressMask | ButtonReleaseMask | ButtonMotionMask) { 

        m_win.setName("Demand Attention");
        m_win.show();
        m_win.setBackgroundColor(FbTk::Color("white", m_win.screenNumber()));
        FbTk::EventManager::instance()->add(*this, m_win);

        m_net_wm_state = XInternAtom(m_win.display(),
                                     "_NET_WM_STATE",
                                     False);
        m_net_wm_state_demands_attention = 
            XInternAtom(m_win.display(),
                        "_NET_WM_STATE_DEMANDS_ATTENTION",
                        False);
        FbTk::RefCount<FbTk::Command<void> > cmd(new FbTk::SimpleCommand<App>
                                                 (*this,
                                                  &App::demandAttention));
        m_timer.setTimeout(5 * FbTk::FbTime::IN_SECONDS);
        m_timer.setCommand(cmd);
        m_timer.fireOnce(false);
        m_timer.start();
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
    void exposeEvent(XExposeEvent &event) {
        redraw();
    }

    void redraw() {
        m_win.clear();
    }

    void demandAttention() {
        cerr << "Demand attention!" << endl;
        XEvent event;
        event.type = ClientMessage;
        event.xclient.message_type = m_net_wm_state;
        event.xclient.display = m_win.display();
        event.xclient.format = 32;
        event.xclient.window = m_win.window();
        event.xclient.data.l[0] = 1; // STATE_ADD
        event.xclient.data.l[1] = m_net_wm_state_demands_attention;
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;
        XSendEvent(display(), DefaultRootWindow(display()), False, 
                   SubstructureRedirectMask | SubstructureNotifyMask, 
                   &event);
    }

private:
    FbTk::FbWindow m_win;
    FbTk::Timer m_timer;
    Atom m_net_wm_state;
    Atom m_net_wm_state_demands_attention;
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
