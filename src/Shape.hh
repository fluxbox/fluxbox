// Shape.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: Shape.hh,v 1.1 2003/07/10 12:04:46 fluxgen Exp $

#ifndef SHAPE_HH
#define SHAPE_HH

#include <X11/Xlib.h>

namespace FbTk {
class FbWindow;
};

/// creates round corners on windows
class Shape {
public:
    enum ShapePlace { 
        NONE = 0,
        BOTTOMRIGHT = 0x01, 
        TOPRIGHT = 0x02,
        BOTTOMLEFT = 0x04,
        TOPLEFT = 0x08,
    };

    Shape(FbTk::FbWindow &win, int shapeplaces);
    ~Shape();
    /// set new shape places
    void setPlaces(int shapeplaces);
    /// update our shape
    void update(); 
    /// assign a new window
    void setWindow(FbTk::FbWindow &win);
private:
    FbTk::FbWindow *m_win; ///< window to be shaped
    Pixmap m_shape; ///< our shape pixmap
    int m_shapeplaces; ///< places to shape
    unsigned int m_width; ///< width of window (the "old" size), if width != window width then shape resizes
    unsigned int m_height; ///< height of window (the "old" size), if height != window height then shape resizes
};

#endif // SHAPE_HH
