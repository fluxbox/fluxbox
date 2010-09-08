// XFontImp.hh for FbTk fluxbox toolkit
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

#ifndef FBTK_XFONTIMP_HH
#define FBTK_XFONTIMP_HH

#include "FontImp.hh"

namespace FbTk {

/// regular X font implementation for FbTk
class XFontImp:public FbTk::FontImp {
public:
    explicit XFontImp(const char *filename = 0);
    ~XFontImp();
    bool load(const std::string &filename);
    unsigned int textWidth(const char* text, unsigned int len) const;
    unsigned int height() const;
    int ascent() const;
    int descent() const { return m_fontstruct ? m_fontstruct->descent : 0; }
    void drawText(const FbDrawable &w, int screen, GC gc, const char* text, size_t len, int x, int y, FbTk::Orientation orient);

    bool validOrientation(FbTk::Orientation orient);

    bool loaded() const { return m_fontstruct != 0; }

private:
    struct BitmapStruct {
        int bit_w;
        int bit_h;

        Pixmap bm;
    };
    struct XRotCharStruct {
        int ascent;
        int descent;
        int lbearing;
        int rbearing;
        int width;

        BitmapStruct glyph;
    };

    struct XRotFontStruct {
        int height;
        int max_ascent;
        int max_descent;
        int max_char;
        int min_char;

        XRotCharStruct per_char[95];
    };

    void rotate(FbTk::Orientation orient);

    void freeRotFont(XRotFontStruct * rotfont);
    void drawRotText(Drawable w, int screen, GC gc, const char* text, size_t len, int x, int y, FbTk::Orientation orient) const;

    XRotFontStruct *m_rotfonts[4]; ///< rotated font structure (only 3 used)
    bool m_rotfonts_loaded[4]; // whether we've tried yet
    XFontStruct *m_fontstruct; ///< X font structure

};

} // end namespace FbTk

#endif // XFONTIMP_HH
