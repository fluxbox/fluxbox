// XFontImp.hh for FbTk fluxbox toolkit
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

// $Id: XFontImp.hh,v 1.5 2003/12/16 17:06:52 fluxgen Exp $

#ifndef FBTK_XFONTIMP_HH
#define FBTK_XFONTIMP_HH

#include "FontImp.hh"

#include <X11/Xlib.h>

namespace FbTk {

/// regular X font implementation for FbTk
class XFontImp:public FbTk::FontImp {
public:
    explicit XFontImp(const char *filename = 0);
    ~XFontImp();
    bool load(const std::string &filename);
    unsigned int textWidth(const char * const text, unsigned int size) const;
    unsigned int height() const;
    float angle() const { return m_angle; }
    int ascent() const;
    int descent() const { return m_fontstruct ? m_fontstruct->descent : 0; }
    void drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
    bool loaded() const { return m_fontstruct != 0; }
    void rotate(float angle);
    /// enable/disable rotation witout alloc/dealloc rotfont structures
    void setRotate(bool val) { m_rotate = val; }
private:
    void freeRotFont();
    void drawRotText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
    unsigned int rotTextWidth(const char * const text, unsigned int size) const;
    struct BitmapStruct {
        int	bit_w;
        int bit_h;

        Pixmap bm;
    };

    struct XRotCharStruct {
        int	ascent;
        int	descent;
        int	lbearing;
        int	rbearing;
        int width;

        BitmapStruct glyph;
    };

    struct XRotFontStruct {
        int	dir;
        int	height;
        int	max_ascent;
        int	max_descent;
        int	max_char;
        int min_char;

        XRotCharStruct per_char[95];
    };
    XRotFontStruct *m_rotfont; ///< rotated font structure
    XFontStruct *m_fontstruct; ///< X font structure
    float m_angle; ///< the rotated angle
    bool m_rotate; ///< used to disable/enable rotation temprarly without reallocating m_rotfont
};

} // end namespace FbTk

#endif // XFONTIMP_HH
