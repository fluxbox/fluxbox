// FbWindow.hh for FbTk - fluxbox toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWindow.hh,v 1.5 2002/12/25 11:29:34 fluxgen Exp $

#ifndef FBTK_FBWINDOW_HH
#define FBTK_FBWINDOW_HH

#include <X11/Xlib.h>

namespace FbTk {

class Color;

/**
   Wrapper for X window
 */
class FbWindow {
public:
    FbWindow();
    FbWindow(const FbWindow &win_copy);
    FbWindow(int screen_num,
             int x, int y, size_t width, size_t height, long eventmask, 
             bool overrride_redirect = false,
             int depth = CopyFromParent, 
             int class_type = InputOutput);
    FbWindow(const FbWindow &parent,
             int x, int y, size_t width, size_t height, long eventmask, 
             bool overrride_redirect = false,
             int depth = CopyFromParent, 
             int class_type = InputOutput);

    virtual ~FbWindow();
    void setBackgroundColor(const FbTk::Color &bg_color);
    void setBackgroundPixmap(Pixmap bg_pixmap);
    void setBorderColor(const FbTk::Color &border_color);
    void setBorderWidth(size_t size);
    /// set window name (for title)
    void setName(const char *name);
    void setEventMask(long mask);
    /// clear window with background pixmap or color
    void clear();
    /// assign a new X window to this
    virtual FbWindow &operator = (Window win);
    virtual void hide();
    virtual void show();
    virtual void showSubwindows();

    virtual void move(int x, int y);
    virtual void resize(size_t width, size_t height);
    virtual void moveResize(int x, int y, size_t width, size_t height);
    void lower();
    void raise();

    const FbWindow *parent() const { return m_parent; }
    Window window() const { return m_window; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    int screenNumber() const;
    /// compare X window
    bool operator == (Window win) const { return m_window == win; }	
    bool operator != (Window win) const { return m_window != win; }
    /// compare two windows
    bool operator == (const FbWindow &win) const { return m_window == win.m_window; }
    bool operator != (const FbWindow &win) const { return m_window != win.m_window; }

private:
    void updateGeometry();
    void create(Window parent, int x, int y, size_t width, size_t height, long eventmask, 
                bool override_redirect, 
                int depth, 
                int class_type);
    static Display *s_display;
    const FbWindow *m_parent;
    int m_screen_num;
    Window m_window; ///< X window
    int m_x, m_y; ///< position
    size_t m_width, m_height; 
};

bool operator == (Window win, const FbWindow &fbwin);

}; // end namespace FbTk

#endif // FBTK_FBWINDOW_HH
