// FbPixmap.hh for FbTk - Fluxbox ToolKit
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

// $Id: FbPixmap.hh,v 1.1 2003/04/25 12:29:49 fluxgen Exp $

#ifndef FBTK_FBPIXMAP_HH
#define FBTK_FBPIXMAP_HH

#include <X11/Xlib.h>

namespace FbTk {

/// a wrapper for X Pixmap
class FbPixmap {
public:    
    FbPixmap();
    FbPixmap(const FbPixmap &copy);
    FbPixmap(Drawable src, 
             unsigned int width, unsigned int height,
             int depth);
    ~FbPixmap();
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
    void copy(const FbPixmap &the_copy);
    FbPixmap &operator = (const FbPixmap &copy);

    inline Drawable drawable() const { return m_pm; }
    inline unsigned int width() const { return m_width; }
    inline unsigned int height() const { return m_height; }
    inline int depth() const { return m_depth; }

private:
    void free();
    void create(Drawable src,
                unsigned int width, unsigned int height,
                int depth);
    Pixmap m_pm;
    int m_depth;
    unsigned int m_width, m_height;
};

}; // end namespace FbTk

#endif // FBTK_FBPIXMAP_HH

