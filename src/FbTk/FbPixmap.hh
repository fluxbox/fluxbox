// FbPixmap.hh for FbTk - Fluxbox ToolKit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_FBPIXMAP_HH
#define FBTK_FBPIXMAP_HH

#include "FbDrawable.hh"
#include "Orientation.hh"

namespace FbTk {

/// a wrapper for X Pixmap
class FbPixmap:public FbDrawable {
public:
    FbPixmap();
    /// copy pixmap
    FbPixmap(const FbPixmap &copy);
    /// creates a FbPixmap from X pixmap
    explicit FbPixmap(Pixmap pm);
    FbPixmap(const FbDrawable &src,
             unsigned int width, unsigned int height,
             unsigned int depth);
    FbPixmap(Drawable src,
             unsigned int width, unsigned int height,
             unsigned int depth);

    virtual ~FbPixmap();

    void copy(const FbPixmap &the_copy);
    void copy(Pixmap pixmap, unsigned int depth_convert, int screen_num);
    /// rotates the pixmap to specified orientation (assumes ROT0 now)
    void rotate(FbTk::Orientation orient);
    /// scales the pixmap to specified size
    void scale(unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);
    /// tiles the pixmap to specified size
    void tile(unsigned int width, unsigned int height);
    /// drops pixmap and returns it
    Pixmap release();

    FbPixmap &operator = (const FbPixmap &copy);
    /// sets new pixmap
    FbPixmap &operator = (Pixmap pm);

    Drawable drawable() const { return m_pm; }
    unsigned int width() const { return m_width; }
    unsigned int height() const { return m_height; }
    unsigned int depth() const { return m_depth; }

    static Pixmap getRootPixmap(int screen_num, bool force_update=false);
    static bool setRootPixmap(int screen_num, Pixmap pm);
    static bool rootwinPropertyNotify(int screen_num, Atom atom);

    void create(Drawable src,
                unsigned int width, unsigned int height,
                unsigned int depth);

    /* Will be reset to false whenever this pixmap is reassigned */
    void dontFree() { m_dont_free = true; }

private:
    void free();

    Pixmap m_pm;
    unsigned int m_width, m_height;
    unsigned int m_depth;

    // if pixmap not *owned* by this object (eg assigned from cache object)
    bool m_dont_free;

};

} // end namespace FbTk

#endif // FBTK_FBPIXMAP_HH

