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

// $Id: FbPixmap.hh,v 1.10 2004/01/11 12:48:46 fluxgen Exp $

#ifndef FBTK_FBPIXMAP_HH
#define FBTK_FBPIXMAP_HH

#include "FbDrawable.hh"

#include <X11/Xlib.h>

namespace FbTk {

/// a wrapper for X Pixmap
class FbPixmap:public FbDrawable {
public:    
    FbPixmap();
    /// copy pixmap 
    explicit FbPixmap(const FbPixmap &copy);
    /// creates a FbPixmap from X pixmap
    explicit FbPixmap(Pixmap pm);
    FbPixmap(const FbDrawable &src, 
             unsigned int width, unsigned int height,
             int depth);
    FbPixmap(Drawable src, 
             unsigned int width, unsigned int height,
             int depth);

    virtual ~FbPixmap();

    void copy(const FbPixmap &the_copy);
    void copy(Pixmap pixmap);
    /// rotates the pixmap 90 deg, not implemented!
    void rotate();
    /// scales the pixmap to specified size
    void scale(unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);
    /// drops pixmap and returns it
    Pixmap release();

    FbPixmap &operator = (const FbPixmap &copy);
    /// sets new pixmap
    FbPixmap &operator = (Pixmap pm);

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
    unsigned int m_width, m_height;
    int m_depth;
};

} // end namespace FbTk

#endif // FBTK_FBPIXMAP_HH

