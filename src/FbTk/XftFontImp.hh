// XftFontImp.hh  Xft font implementation for FbTk
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

//$Id: XftFontImp.hh,v 1.4 2003/12/16 17:06:52 fluxgen Exp $

#ifndef FBTK_XFTFONTIMP_HH
#define FBTK_XFTFONTIMP_HH

#include "FontImp.hh"

#include <X11/Xft/Xft.h>

namespace FbTk {

/// Handles Xft font drawing
class XftFontImp:public FbTk::FontImp {
public:
    XftFontImp(const char *fontname, bool utf8);
    ~XftFontImp();
    bool load(const std::string &name);
    void drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
    unsigned int textWidth(const char * const text, unsigned int len) const;
    unsigned int height() const;
    int ascent() const { return m_xftfont ? m_xftfont->ascent : 0; }
    int descent() const { return m_xftfont ? m_xftfont->descent : 0; }
    bool loaded() const { return m_xftfont != 0; }
private:
    XftFont *m_xftfont;
    bool m_utf8mode;
};

} // end namespace FbTk

#endif // FBTK_XFTFONTIMP_HH
