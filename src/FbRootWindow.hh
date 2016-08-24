// FbRootWindow.hh for fluxbox
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

#ifndef FBROOTWINDOW_HH
#define FBROOTWINDOW_HH

#include "FbTk/FbWindow.hh"

class FbRootWindow: public FbTk::FbWindow {
public:
    explicit FbRootWindow(int screen_num);
    // disable functions that we can't do on root window
    void move(int x, int y) { }
    void resize(unsigned int width, unsigned int height) { }
    void moveResize(int x, int y, unsigned int width, unsigned int height) { }
    void show() { }
    void hide() { }
    // we should not assign a new window to this
    FbTk::FbWindow &operator = (Window win) { return *this; }
    Visual *visual() const { return m_visual; }
    Colormap colormap() const { return m_colormap; } 

    int decorationDepth() const { return m_decorationDepth; }
    Visual *decorationVisual() const { return m_decorationVisual; }
    Colormap decorationColormap() const { return m_decorationColormap; }
    int maxDepth() const { return m_maxDepth; }

private:
    Visual *m_visual;
    Colormap m_colormap;

    int m_decorationDepth;
    Visual *m_decorationVisual;
    Colormap m_decorationColormap;
    int m_maxDepth;
};

#endif // FBROOTWINDOW_HH
