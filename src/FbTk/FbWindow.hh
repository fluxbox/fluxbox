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

// $Id: FbWindow.hh,v 1.27 2003/12/30 17:17:05 fluxgen Exp $

#ifndef FBTK_FBWINDOW_HH
#define FBTK_FBWINDOW_HH

#include "FbDrawable.hh"

#include <X11/Xlib.h>
#include <memory>


namespace FbTk {

class Color;
class Transparent;

///   Wrapper for X window
/**
 * Example:
 * FbWindow window(0, 10, 10, 100, 100, ExposeMask | ButtonPressMask); \n
 * this will create a window on screen 0, position 10 10, size 100 100 \n
 * and with eventmask Expose and ButtonPress. \n
 * You need to register it to some eventhandler so you can catch events: \n
 * EventManager::instance()->add(your_eventhandler, window); \n
 * @see EventHandler
 * @see EventManager
 */
class FbWindow: public FbDrawable {
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
    virtual void setBackgroundColor(const FbTk::Color &bg_color);
    virtual void setBackgroundPixmap(Pixmap bg_pixmap);
    virtual void setBorderColor(const FbTk::Color &border_color);
    virtual void setBorderWidth(unsigned int size);
    /// set window name ("title")
    void setName(const char *name);
    void setEventMask(long mask);
    /// clear window with background pixmap or color
    virtual void clear();
    /// @param exposures wheter Expose event should be generated
    virtual void clearArea(int x, int y, 
                           unsigned int width, unsigned int height, 
                           bool exposures = false);
    void updateTransparent(int x = -1, int y = -1, unsigned int width = 0, unsigned int height = 0);

    void setAlpha(unsigned char alpha);

    virtual FbWindow &operator = (const FbWindow &win);
    /// assign a new X window to this
    virtual FbWindow &operator = (Window win);    
    virtual void hide();
    virtual void show();
    virtual void showSubwindows();

    virtual inline void move(int x, int y) {
        XMoveWindow(s_display, m_window, x, y);
        m_x = x;
        m_y = y;
    }

    virtual inline void resize(unsigned int width, unsigned int height) {
            XResizeWindow(s_display, m_window, width, height);
            m_width = width;
            m_height = height;
    }

    virtual inline void moveResize(int x, int y, unsigned int width, unsigned int height) {
        XMoveResizeWindow(s_display, m_window, x, y, width, height);
        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;
    }
    virtual void lower();
    virtual void raise();
    void setInputFocus(int revert_to, int time);
    /// defines a cursor for this window
    void setCursor(Cursor cur);
    /// uses the parents cursor instead
    void unsetCursor();
    void reparent(const FbWindow &parent, int x, int y);

    bool property(Atom property,
                  long long_offset, long long_length,
                  bool do_delete,
                  Atom req_type,
                  Atom *actual_type_return,
                  int *actual_format_return,
                  unsigned long *nitems_return,
                  unsigned long *bytes_after_return,
                  unsigned char **prop_return) const;

    void changeProperty(Atom property, Atom type,
                        int format,
                        int mode,
                        unsigned char *data,
                        int nelements);

    /// @return parent FbWindow
    const FbWindow *parent() const { return m_parent; }
    /// @return real X window
    inline Window window() const { return m_window; }
    /// @return drawable (the X window)
    inline Drawable drawable() const { return window(); }
    int x() const { return m_x; }
    int y() const { return m_y; }
    unsigned int width() const { return m_width; }
    unsigned int height() const { return m_height; }
    unsigned int borderWidth() const { return m_border_width; }
    int depth() const { return m_depth; }
    int screenNumber() const;
    long eventMask() const;
    /// compare X window
    bool operator == (Window win) const { return m_window == win; }	
    bool operator != (Window win) const { return m_window != win; }
    /// compare two windows
    bool operator == (const FbWindow &win) const { return m_window == win.m_window; }
    bool operator != (const FbWindow &win) const { return m_window != win.m_window; }

protected:
    /// creates a window with x window client (m_window = client)
    explicit FbWindow(Window client);
    void setBufferPixmap(Pixmap pm);
    /// updates x,y, width, height and screen num from X window
    void updateGeometry();
private:
    /// sets new X window and destroys old
    void setNew(Window win);
    /// creates a new X window
    void create(Window parent, int x, int y, unsigned int width, unsigned int height,
                long eventmask, 
                bool override_redirect, 
                int depth, 
                int class_type);
    static Display *s_display; ///< display connection
    const FbWindow *m_parent; ///< parent FbWindow
    int m_screen_num;  ///< screen num on which this window exist
    mutable Window m_window; ///< the X window
    int m_x, m_y; ///< position of window
    unsigned int m_width, m_height;  ///< size of window
    unsigned int m_border_width; ///< border size
    int m_depth; ///< bit depth
    bool m_destroy; ///< wheter the x window was created before
    std::auto_ptr<FbTk::Transparent> m_transparent;
    Pixmap m_buffer_pm;
};

bool operator == (Window win, const FbWindow &fbwin);

/// helper class for some STL routines
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

} // end namespace FbTk

#endif // FBTK_FBWINDOW_HH
