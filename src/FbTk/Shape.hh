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

#ifndef SHAPE_HH
#define SHAPE_HH

#include "FbPixmap.hh"

namespace FbTk {
class FbWindow;

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

    Shape(FbWindow &win, int shapeplaces);
    ~Shape();
    /// set new shape places
    void setPlaces(int shapeplaces);
    /// update our shape
    void update(); 
    /// assign a new window
    void setWindow(FbWindow &win);
    /// Assign a window to merge our shape with.
    /// (note that this is currently specific to frames)
    void setShapeSource(FbWindow *win, int xoff, int yoff, bool always_update);
    void setShapeOffsets(int xoff, int yoff);
    unsigned int width() const;
    unsigned int height() const;
    unsigned int clipWidth() const;
    unsigned int clipHeight() const;
    // sets shape notify mask
    static void setShapeNotify(const FbWindow &win);
    /// @return true if window has shape
    static bool isShaped(const FbWindow &win);
private:
    FbWindow *m_win; ///< window to be shaped
    FbWindow *m_shapesource; ///< window to pull shape from
    int m_shapesource_xoff, m_shapesource_yoff;

    int m_shapeplaces; ///< places to shape
};

} // end namespace FbTk

#endif // SHAPE_HH
