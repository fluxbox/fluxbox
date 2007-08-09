// Shape.hh
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef SHAPE_HH
#define SHAPE_HH

#include "FbTk/FbPixmap.hh"

#include <X11/Xlib.h>
#include <memory>
#include <vector>

namespace FbTk {
class FbWindow;
}

/// creates round corners on windows
class Shape {
public:
    enum ShapePlace { 
        NONE = 0,
        BOTTOMRIGHT = 0x01, 
        TOPRIGHT = 0x02,
        BOTTOMLEFT = 0x04,
        TOPLEFT = 0x08
    };

    Shape(FbTk::FbWindow &win, int shapeplaces);
    ~Shape();
    /// set new shape places
    void setPlaces(int shapeplaces);
    /// update our shape
    void update(); 
    /// assign a new window
    void setWindow(FbTk::FbWindow &win);
    /// Assign a window to merge our shape with.
    /// (note that this is currently specific to frames)
    void setShapeSource(FbTk::FbWindow *win, int xoff, int yoff, bool always_update);
    void setShapeOffsets(int xoff, int yoff);
    unsigned int width() const;
    unsigned int height() const;
    unsigned int clipWidth() const;
    unsigned int clipHeight() const;
    // sets shape notify mask
    static void setShapeNotify(const FbTk::FbWindow &win);
    /// @return true if window has shape
    static bool isShaped(const FbTk::FbWindow &win);
private:
    FbTk::FbWindow *m_win; ///< window to be shaped
    FbTk::FbWindow *m_shapesource; ///< window to pull shape from
    int m_shapesource_xoff, m_shapesource_yoff;

    void initCorners(int screen_num);

    struct CornerPixmaps {
        FbTk::FbPixmap topleft;
        FbTk::FbPixmap topright;
        FbTk::FbPixmap botleft;
        FbTk::FbPixmap botright;
    };

    // unfortunately, we need a separate pixmap per screen
    static std::vector<CornerPixmaps> s_corners;
    int m_shapeplaces; ///< places to shape

};

#endif // SHAPE_HH
