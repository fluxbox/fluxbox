// FbWindow.hh for FbTk - fluxbox toolkit
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWindow.hh,v 1.11 2003/04/25 17:31:38 fluxgen Exp $

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
             int x, int y, unsigned int width, unsigned int height, long eventmask, 
             bool overrride_redirect = false,
             int depth = CopyFromParent, 
             int class_type = InputOutput);
    FbWindow(const FbWindow &parent,
             int x, int y, 
             unsigned int width, unsigned int height, 
             long eventmask, 
             bool overrride_redirect = false,
             int depth = CopyFromParent, 
             int class_type = InputOutput);

    virtual ~FbWindow();
    void setBackgroundColor(const FbTk::Color &bg_color);
    void setBackgroundPixmap(Pixmap bg_pixmap);
    void setBorderColor(const FbTk::Color &border_color);
    void setBorderWidth(unsigned int size);
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
    virtual void resize(unsigned int width, unsigned int height);
    virtual void moveResize(int x, int y, unsigned int width, unsigned int height);
    virtual void lower();
    virtual void raise();

    void copyArea(Drawable src, GC gc,
                  int src_x, int src_y,
                  int dest_x, int dest_y,
                  unsigned int width, unsigned int height);
    void fillRectangle(GC gc, int x, int y,
                       unsigned int width, unsigned int height);
    void drawRectangle(GC gc, int x, int y, 
                  unsigned int width, unsigned int height);
    void fillPolygon(GC gc, XPoint *points, int npoints,
                     int shape, int mode);
    void drawLine(GC gc, int start_x, int start_y, 
                  int end_x, int end_y);

    const FbWindow *parent() const { return m_parent; }
    Window window() const { return m_window; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    unsigned int width() const { return m_width; }
    unsigned int height() const { return m_height; }
    unsigned int borderWidth() const { return m_border_width; }
    int screenNumber() const;
    /// compare X window
    bool operator == (Window win) const { return m_window == win; }	
    bool operator != (Window win) const { return m_window != win; }
    /// compare two windows
    bool operator == (const FbWindow &win) const { return m_window == win.m_window; }
    bool operator != (const FbWindow &win) const { return m_window != win.m_window; }

protected:
    explicit FbWindow(Window client);
private:
    void updateGeometry();
    void create(Window parent, int x, int y, unsigned int width, unsigned int height,
                long eventmask, 
                bool override_redirect, 
                int depth, 
                int class_type);
    static Display *s_display;
    const FbWindow *m_parent;    
    int m_screen_num;
    Window m_window; ///< the X window
    int m_x, m_y; ///< position of window
    unsigned int m_width, m_height;  ///< size of window
    unsigned int m_border_width; // border size
    bool m_destroy; ///< wheter the x window was created before
};

bool operator == (Window win, const FbWindow &fbwin);

class ChangeProperty {
public:
    ChangeProperty(Display *disp, Atom prop, int mode,
                   unsigned char *state, int num):m_disp(disp),
    m_prop(prop), m_state(state), m_num(num), m_mode(mode){
        
    }
    void operator () (FbTk::FbWindow *win) {
        XChangeProperty(m_disp, win->window(), m_prop, m_prop, 32, m_mode,
                        m_state, m_num);
    }
private:
    Display *m_disp;
    Atom m_prop;
    unsigned char *m_state;
    int m_num;
    int m_mode;
};

}; // end namespace FbTk

#endif // FBTK_FBWINDOW_HH
