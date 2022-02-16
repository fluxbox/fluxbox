// FbWindow.hh for FbTk - fluxbox toolkit
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_FBWINDOW_HH
#define FBTK_FBWINDOW_HH

#include "FbDrawable.hh"
#include "FbString.hh"

#include <memory>
#include <string>
#include <set>
#include <cmath>

namespace FbTk {

class Color;
class Transparent;
class FbPixmap;
class FbWindowRenderer;

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

    static Window rootWindow(Display* dpy, Drawable win);

    FbWindow();
    FbWindow(const FbWindow &win_copy);

    FbWindow(int screen_num,
             int x, int y, unsigned int width, unsigned int height, long eventmask,
             bool overrride_redirect = false,
             bool save_unders = false,
             unsigned int depth = CopyFromParent,
             int class_type = InputOutput,
             Visual *visual = CopyFromParent,
             Colormap cmap = CopyFromParent);

    FbWindow(const FbWindow &parent,
             int x, int y,
             unsigned int width, unsigned int height,
             long eventmask,
             bool overrride_redirect = false,
             bool save_unders = false,
             unsigned int depth = CopyFromParent,
             int class_type = InputOutput,
             Visual *visual = CopyFromParent,
             Colormap cmap = CopyFromParent);

    virtual ~FbWindow();
    virtual void setBackgroundColor(const FbTk::Color &bg_color);
    virtual void setBackgroundPixmap(Pixmap bg_pixmap);
    // call when background is freed, and new one not ready yet
    virtual void invalidateBackground();
    virtual void setBorderColor(const FbTk::Color &border_color);
    virtual void setBorderWidth(unsigned int size);
    /// set window name ("title")
    void setName(const char *name);
    /// set window role
    void setWindowRole(const char *windowRole);
    void setEventMask(long mask);
    /// clear window with background pixmap or color
    virtual void clear();
    /// @param exposures wheter Expose event should be generated
    virtual void clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposures = false);
    void updateTransparent(int x = -1, int y = -1, unsigned int width = 0,
                           unsigned int height = 0, Pixmap dest_override = None,
                           bool override_is_offset = false);

    void setAlpha(int alpha);

    virtual FbWindow &operator = (const FbWindow &win);
    /// assign a new X window to this
    virtual FbWindow &operator = (Window win);
    virtual void hide();
    virtual void show();
    virtual void showSubwindows();

    /// Notify that the parent window was moved,
    /// thus the absolute position of this one moved
    virtual void parentMoved() {
        updateBackground(true);
    }

    virtual void move(int x, int y) {
        if (x == m_x && y == m_y)
            return;
        XMoveWindow(display(), m_window, x, y);
        m_x = x;
        m_y = y;
        updateBackground(true);
    }

    virtual void resize(unsigned int width, unsigned int height) {
        if (width == m_width && height == m_height)
            return;
        XResizeWindow(display(), m_window, width, height);
        m_width = width;
        m_height = height;
        updateBackground(false);
    }

    virtual void moveResize(int x, int y, unsigned int width, unsigned int height) {
        if (x == m_x && y == m_y && width == m_width && height == m_height)
            return;
        XMoveResizeWindow(display(), m_window, x, y, width, height);
        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;
        updateBackground(false);

    }
    virtual void lower();
    virtual void raise();
    void setInputFocus(int revert_to, int time);
    /// defines a cursor for this window
    void setCursor(Cursor cur);

    void reparent(const FbWindow &parent, int x, int y, bool continuing = true);

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

    void deleteProperty(Atom property);

    long cardinalProperty(Atom property, bool*exists=NULL) const;
    FbTk::FbString textProperty(Atom property,bool*exists=NULL) const;

    void addToSaveSet();
    void removeFromSaveSet();

    /// @return parent FbWindow
    const FbWindow *parent() const { return m_parent; }
    /// @return real X window
    Window window() const { return m_window; }
    /// @return drawable (the X window)
    Drawable drawable() const { return window(); }
    int x() const { return m_x; }
    int y() const { return m_y; }
    unsigned int width() const { return m_width; }
    unsigned int height() const { return m_height; }
    unsigned int borderWidth() const { return m_border_width; }
    unsigned long borderColor() const { return m_border_color; }
    unsigned int depth() const { return m_depth; }
    int alpha() const;
    int screenNumber() const;
    long eventMask() const;

    /// compare X window
    bool operator == (Window win) const { return m_window == win; }
    bool operator != (Window win) const { return m_window != win; }
    /// compare two windows
    bool operator == (const FbWindow &win) const { return m_window == win.m_window; }
    bool operator != (const FbWindow &win) const { return m_window != win.m_window; }

    // used for composite
    void setOpaque(int alpha);

    void setRenderer(FbWindowRenderer &renderer) { m_renderer = &renderer; }
    void sendConfigureNotify(int x, int y, unsigned int width,
                             unsigned int height, unsigned int bw = 0);

    /// forces full background change, recalcing of alpha values if necessary
    void updateBackground(bool only_if_alpha);

    static void updatedAlphaBackground(int screen);

    /// updates x,y, width, height and screen num from X window
    bool updateGeometry();

protected:
    /// creates a window with x window client (m_window = client)
    explicit FbWindow(Window client);

    void setDepth(unsigned int depth) { m_depth = depth; }

private:
    /// sets new X window and destroys old
    void setNew(Window win);
    /// creates a new X window
    void create(Window parent, int x, int y, unsigned int width, unsigned int height,
                long eventmask,
                bool override_redirect,
                bool save_unders,
                unsigned int depth,
                int class_type,
                Visual *visual,
                Colormap cmap);

    const FbWindow *m_parent; ///< parent FbWindow
    int m_screen_num;  ///< screen num on which this window exist
    mutable Window m_window; ///< the X window
    int m_x, m_y; ///< position of window
    unsigned int m_width, m_height;  ///< size of window
    unsigned int m_border_width; ///< border size
    unsigned long m_border_color; ///< border color
    unsigned int m_depth; ///< bit depth
    bool m_destroy; ///< wheter the x window was created before
    std::unique_ptr<FbTk::Transparent> m_transparent;
    bool m_lastbg_color_set;
    unsigned long m_lastbg_color;
    Pixmap m_lastbg_pm;

    FbWindowRenderer *m_renderer;

    static void addAlphaWin(FbWindow &win);
    static void removeAlphaWin(FbWindow &win);

    typedef std::set<FbWindow *> FbWinList;
    static FbWinList m_alpha_wins;
};

bool operator == (Window win, const FbWindow &fbwin);

/// Interface class to render FbWindow foregrounds.
class FbWindowRenderer {
public:
    virtual void renderForeground(FbWindow &win, FbDrawable &drawable) = 0;
    virtual ~FbWindowRenderer() { }
};



} // end namespace FbTk

#endif // FBTK_FBWINDOW_HH
