// WindowState.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef WINDOWSTATE_HH
#define WINDOWSTATE_HH

#include "Layer.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string>

class SizeHints {
public:
    SizeHints():
        min_width(1), max_width(0), min_height(1), max_height(0),
        width_inc(1), height_inc(1), base_width(0), base_height(0),
        min_aspect_x(0), max_aspect_x(1),
        min_aspect_y(1), max_aspect_y(0),
        win_gravity(0) { }

    void reset(const XSizeHints &sizehint);

    void apply(unsigned int &w, unsigned int &h,
               bool maximizing = false) const;
    bool valid(unsigned int width, unsigned int height) const;
    void displaySize(unsigned int &i, unsigned int &j,
                     unsigned int width, unsigned int height) const;

    bool isResizable() const;

    unsigned int min_width, max_width, min_height, max_height,
                 width_inc, height_inc, base_width, base_height,
                 min_aspect_x, max_aspect_x, min_aspect_y, max_aspect_y;
    int win_gravity;
};

class WindowState {
public:

    /**
     * Types of maximization
     */
    enum MaximizeMode {
        MAX_NONE = 0, ///< normal state
        MAX_HORZ = 1, ///< maximize horizontal
        MAX_VERT = 2, ///< maximize vertical
        MAX_FULL = 3  ///< maximize full
    };

    /**
       This enumeration represents individual decoration
       attributes, they can be OR-d together to get a mask.
       Useful for saving.
    */
    enum DecorationMask {
        DECORM_TITLEBAR = (1<<0),
        DECORM_HANDLE   = (1<<1),
        DECORM_BORDER   = (1<<2),
        DECORM_ICONIFY  = (1<<3),
        DECORM_MAXIMIZE = (1<<4),
        DECORM_CLOSE    = (1<<5),
        DECORM_MENU     = (1<<6),
        DECORM_STICKY   = (1<<7),
        DECORM_SHADE    = (1<<8),
        DECORM_TAB      = (1<<9),
        DECORM_ENABLED  = (1<<10),
        DECORM_LAST     = (1<<11) // useful for getting "All"
    };

    enum Decoration {
        DECOR_NONE = 0,
        DECOR_NORMAL = DECORM_LAST - 1,
        DECOR_TINY = DECORM_TITLEBAR|DECORM_ICONIFY,
        DECOR_TOOL = DECORM_TITLEBAR,
        DECOR_BORDER = DECORM_BORDER,
        DECOR_TAB = DECORM_BORDER|DECORM_TAB
    };

    enum WindowType {
        TYPE_NORMAL,
        TYPE_DOCK,
        TYPE_DESKTOP,
        TYPE_SPLASH,
        TYPE_DIALOG,
        TYPE_MENU,
        TYPE_TOOLBAR
    };

    WindowState():
        size_hints(),
        deco_mask(DECOR_NORMAL),
        type(TYPE_NORMAL),
        focused(false),
        shaded(false), fullscreen(false), stuck(false), iconic(false),
        focus_hidden(false), icon_hidden(false),
        maximized(0), layernum(ResourceLayer::NORMAL),
        x(0), y(0), width(1), height(1) { }

    void saveGeometry(int x, int y, unsigned int width, unsigned int height,
                      bool force = false);

    // returns what the state should be set to, without actually setting it
    int queryToggleMaximized(int type) const;

    bool useBorder() const;
    bool useHandle() const;
    bool useTabs() const;
    bool useTitlebar() const;

    bool isMaximized() const { return maximized == MAX_FULL; }
    bool isMaximizedHorz() const { return maximized & MAX_HORZ; }
    bool isMaximizedVert() const { return maximized & MAX_VERT; }

    static int getDecoMaskFromString(const std::string &str);

    SizeHints size_hints;
    unsigned int deco_mask;
    WindowType type;
    bool focused, shaded, fullscreen, stuck, iconic, focus_hidden, icon_hidden;
    int maximized, layernum;
    int x, y;
    unsigned int width, height;
};

#endif // WINDOWSTATE_HH
