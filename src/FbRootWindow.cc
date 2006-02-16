// FbRootWindow.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "FbRootWindow.hh"
#include "App.hh"

#include <X11/Xutil.h>

FbRootWindow::FbRootWindow(int screen_num):
    FbTk::FbWindow(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_visual(0),
    m_colormap(0) {

    Display *disp = FbTk::App::instance()->display();

    // search for a TrueColor Visual... if we can't find one... we will use the
    // default visual for the screen
    XVisualInfo vinfo_template, *vinfo_return;
    int vinfo_nitems;

    vinfo_template.screen = screen_num;
    vinfo_template.c_class = TrueColor;
    if ((vinfo_return = XGetVisualInfo(disp,
                                       VisualScreenMask | VisualClassMask,
                                       &vinfo_template, &vinfo_nitems)) &&
        vinfo_nitems > 0) {

        for (int i = 0; i < vinfo_nitems; i++) {
            // We can't handle 32-bit visuals just yet (Composite ARGB)
            if (vinfo_return[i].depth != 32 && DefaultDepth(disp, screen_num) < vinfo_return[i].depth)
                m_visual = vinfo_return[i].visual;
        }

        XFree(vinfo_return);
    }

    if (m_visual) {
        m_colormap = XCreateColormap(disp, window(),
                                     m_visual, AllocNone);
    } else {
        m_visual = DefaultVisual(disp, screen_num);
        m_colormap = DefaultColormap(disp, screen_num);
    }
}

void FbRootWindow::updateGeometry() {
    FbTk::FbWindow::updateGeometry();
}
