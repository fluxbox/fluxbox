// FbDrawable.hh for FbTk - Fluxbox ToolKit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbDrawable.hh,v 1.4 2003/12/16 17:06:49 fluxgen Exp $
#ifndef FBTK_FBDRAWABLE_HH
#define FBTK_FBDRAWABLE_HH

#include <X11/Xlib.h>

namespace FbTk {

/// Basic drawing functions for X drawables
class FbDrawable {
public:
    virtual ~FbDrawable() { }
    virtual void copyArea(Drawable src, GC gc,
                          int src_x, int src_y,
                          int dest_x, int dest_y,
                          unsigned int width, unsigned int height);

    virtual void fillRectangle(GC gc, int x, int y,
                               unsigned int width, unsigned int height);

    virtual void drawRectangle(GC gc, int x, int y, 
                               unsigned int width, unsigned int height);

    virtual void drawLine(GC gc, int start_x, int start_y, 
                          int end_x, int end_y);
    virtual void fillPolygon(GC gc, XPoint *points, int npoints,
                             int shape, int mode);

    virtual void drawPoint(GC gc, int x, int y);

    virtual XImage *image(int x, int y, unsigned int width, unsigned int height) const;

    /// X drawable
    virtual Drawable drawable() const = 0;
    virtual unsigned int width() const = 0;
    virtual unsigned int height() const = 0;
};

} // end namespace FbTk

#endif // FBTK_FBDRAWABLE_HH
