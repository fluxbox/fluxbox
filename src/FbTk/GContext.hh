// GContext.hh for FbTk - fluxbox toolkit
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: GContext.hh,v 1.2 2003/09/10 21:27:02 fluxgen Exp $

#ifndef FBTK_GCONTEXT_HH
#define FBTK_GCONTEXT_HH

#include <X11/Xlib.h>

namespace FbTk {

class FbDrawable;
class FbPixmap;
class Font;
class Color;

/// wrapper for X GC
class GContext {
public:
    /// for FbTk drawable
    explicit GContext(const FbTk::FbDrawable &drawable);
    /// for X drawable
    explicit GContext(Drawable drawable);

    virtual ~GContext();

    void setForeground(const FbTk::Color &color);
    void setForeground(long pixel_value);
    void setBackground(const FbTk::Color &color);
    void setBackground(long pixel_value);
    void setFont(const FbTk::Font &font);
    void setClipMask(const FbTk::FbPixmap &pm);
    void setClipOrigin(int x, int y);
    void setGraphicsExposure(bool value);
    
    GC gc() const { return m_gc; }

private:
    GC m_gc;
};

} // end namespace FbTk

#endif // FBTK_GCONTEXT_HH
