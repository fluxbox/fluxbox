// Netizen.hh for Fluxbox 
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Netizen.hh for Blackbox - An X11 Window Manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef	 NETIZEN_HH
#define	 NETIZEN_HH

#include <X11/Xlib.h>

class BScreen;

class Netizen {
public:
    Netizen(const BScreen &scr, Window w);

    inline Window window() const { return m_window; }

    void sendWorkspaceCount();
    void sendCurrentWorkspace();

    void sendWindowFocus(Window w);
    void sendWindowAdd(Window w, unsigned long wkspc);
    void sendWindowDel(Window w);
    void sendWindowRaise(Window w);
    void sendWindowLower(Window w);

    void sendConfigNotify(XEvent &xe);
private:
    const BScreen &m_screen;
    Display *m_display; ///< display connection
    Window m_window;
    XEvent event;

};


#endif // _NETIZEN_HH_

