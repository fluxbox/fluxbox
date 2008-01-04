// PixmapWithMask.hh for FbTk - Fluxbox ToolKit
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

#ifndef FBTK_PIXMAPWITHMASK_HH
#define FBTK_PIXMAPWITHMASK_HH

#include "FbPixmap.hh"
namespace FbTk {

class PixmapWithMask {
public:
    PixmapWithMask() { }
    PixmapWithMask(Pixmap pm, Pixmap mask):m_pixmap(pm), m_mask(mask) { }

    void scale(unsigned int width, unsigned int height) {
        pixmap().scale(width, height);
        mask().scale(width, height);
    }
    unsigned int width() const { return m_pixmap.width(); }
    unsigned int height() const { return m_pixmap.height(); }
    FbPixmap &pixmap() { return m_pixmap; }
    FbPixmap &mask() { return m_mask; }

    const FbPixmap &pixmap() const { return m_pixmap; }
    const FbPixmap &mask() const { return m_mask; }

private:
    FbPixmap m_pixmap;
    FbPixmap m_mask;
};

} // end namespace FbTk

#endif // FBTK_PIXMAPWITHMASK_HH
