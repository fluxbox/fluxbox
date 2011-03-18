// fullscreentest.cc for EWMH test suite
// Copyright (c) 2007 Fluxbox TEam (fluxgen at fluxbox dot org)
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

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <cstring>
#include <string>
#include <iostream>
using namespace std;
/// Motif wm Hints
enum {
    MwmHintsFunctions   = (1l << 0), ///< use motif wm functions
    MwmHintsDecorations	= (1l << 1) ///< use motif wm decorations
};
/// Motif wm functions
enum MwmFunc{
    MwmFuncAll          = (1l << 0), ///< all motif wm functions
    MwmFuncResize       = (1l << 1), ///< resize
    MwmFuncMove         = (1l << 2), ///< move
    MwmFuncIconify      = (1l << 3), ///< iconify
    MwmFuncMaximize     = (1l << 4), ///< maximize
    MwmFuncClose        = (1l << 5)  ///< close
};

typedef struct MwmHints {
    unsigned long flags;       // Motif wm flags
    unsigned long functions;   // Motif wm functions
    unsigned long decorations; // Motif wm decorations
} MwmHints;

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname, bool fullscreen):
        FbTk::App(displayname),
        m_win(DefaultScreen(display()),
              0, 0, 100, 100, KeyPressMask | ExposureMask ),
        m_fullscreen(fullscreen) { 

        if (fullscreen) {
            
            // setup fullscreen as initial state
            Atom net_wm_state = XInternAtom(display(), "_NET_WM_STATE", False);
            Atom state_fullscreen = XInternAtom(display(), "_NET_WM_STATE_FULLSCREEN", False);
            m_win.changeProperty(net_wm_state, XA_ATOM, 32, PropModeReplace,
                                 reinterpret_cast<unsigned char*>(&state_fullscreen),
                                 1 );
            MwmHints hints;
            hints.flags = MwmHintsFunctions;
            hints.functions = MwmFuncIconify | MwmFuncClose;

            // disable resize
            Atom mwm_hints = XInternAtom(display(), "_MOTIF_WM_HINTS", False);
            
            m_win.changeProperty(mwm_hints, mwm_hints, 32, PropModeReplace,
                                 reinterpret_cast<unsigned char*>(&hints), 3);
                                 
        }

        m_win.show();

        m_win.setBackgroundColor(FbTk::Color("white", m_win.screenNumber()));
        FbTk::EventManager::instance()->add(*this, m_win);
        m_win.setName("Fullscreen test.");
    }

    ~App() {
    }

    void keyPressEvent(XKeyEvent &ke) {
        KeySym ks;
        char keychar[1];
        XLookupString(&ke, keychar, 1, &ks, 0);
        if (ks == XK_Escape)
            end();
        else
            toggleFullscreen();
    }
    void exposeEvent(XExposeEvent &event) {
        redraw();
    }

    void redraw() {
        m_win.clear();
    }

    void toggleFullscreen() {
        setFullscreen(!m_fullscreen);
    }
    void setFullscreen( bool state ) {
        m_fullscreen = state;
        Atom net_wm_state = XInternAtom(display(), "_NET_WM_STATE", False);
        Atom net_wm_state_fullscreen = XInternAtom(display(), "_NET_WM_STATE_FULLSCREEN", False);
        XEvent event;
        event.type = ClientMessage;
        event.xclient.message_type = net_wm_state;
        event.xclient.display = m_win.display();
        event.xclient.format = 32;
        event.xclient.window = m_win.window();
        event.xclient.data.l[0] = state; 
        event.xclient.data.l[1] = net_wm_state_fullscreen;
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;
        XSendEvent(display(), DefaultRootWindow(display()), False, 
                   SubstructureRedirectMask | SubstructureNotifyMask, 
                   &event);
        if ( ! state ) { 
            // if no fullscreen then
            // enable all functions
            MwmHints hints;
            hints.flags = MwmHintsFunctions;
            hints.functions = MwmFuncAll;

            // disable resize
            Atom mwm_hints = XInternAtom(display(), "_MOTIF_WM_HINTS", False);
            
            m_win.changeProperty(mwm_hints, mwm_hints, 32, PropModeReplace,
                                 reinterpret_cast<unsigned char*>(&hints), 3);
        }
    }
private:
    FbTk::FbWindow m_win;
    bool m_fullscreen;
};

int main(int argc, char **argv) {
    string displayname("");
    bool fullscreen = false;
    for (int a=1; a<argc; ++a) {
        if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        } else if (strcmp("-f", argv[a]) == 0) {
            fullscreen = true;
        }
    }

    App app(displayname.c_str(), fullscreen);

    app.eventLoop();
	
}
