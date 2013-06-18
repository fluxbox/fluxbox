// XftFontImp.hh  Xft font implementation for FbTk
// Copyright (c) 2002-2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
    void drawText(const FbDrawable &w, int screen, GC gc, const char* text, size_t len, int x, int y , FbTk::Orientation orient);
    unsigned int textWidth(const char* text, unsigned int len) const;
    unsigned int height() const;
    int ascent() const { return m_xftfonts[0] ? m_xftfonts[0]->ascent : 0; }
    int descent() const { return m_xftfonts[0] ? m_xftfonts[0]->descent : 0; }
    bool loaded() const { return m_xftfonts[0] != 0; }
    bool utf8() const { return m_utf8mode; }
    bool validOrientation(FbTk::Orientation orient);

private:
    XftFont *m_xftfonts[4]; // 4 possible orientations
    bool m_xftfonts_loaded[4]; // whether we've tried loading the orientation
    // rotated xft fonts don't give proper extents info, so we keep the "real"
    // one around for it
    bool m_utf8mode;

    std::string m_name;
    unsigned int m_maxlength;
};

} // end namespace FbTk

#endif // FBTK_XFTFONTIMP_HH
