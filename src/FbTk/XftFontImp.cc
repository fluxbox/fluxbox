// XftFontImp.cc  Xft font implementation for FbTk
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "XftFontImp.hh"
#include "App.hh"
#include "FbDrawable.hh"

#include <cmath>
#include <cstdio>
#include <algorithm>

namespace FbTk {

XftFontImp::XftFontImp(const char *name, bool utf8):
    m_utf8mode(utf8), m_name(""), m_maxlength(0x8000) {

    for (int r = ROT0; r <= ROT270; r++) {
        m_xftfonts[r] = 0;
        m_xftfonts_loaded[r] = false;
    }

    if (name != 0)
        load(name);
}

XftFontImp::~XftFontImp() {
    for (int r = ROT0; r <= ROT270; r++) 
        if (m_xftfonts[r] != 0)
            XftFontClose(App::instance()->display(), m_xftfonts[r]);
}

bool XftFontImp::load(const std::string &name) {
    //Note: assumes screen 0 for now, changes on draw if needed

    Display *disp = App::instance()->display();

    XftFont *newxftfont = XftFontOpenXlfd(disp, 0, name.c_str());
    if (newxftfont == 0) {
        newxftfont = XftFontOpenName(disp, 0, name.c_str());
        if (newxftfont == 0)
            return false;
    }

    // destroy all old fonts and set new
    for (int r = ROT0; r <= ROT270; r++) {
        m_xftfonts_loaded[r] = false;
        if (m_xftfonts[r] != 0) {
            XftFontClose(disp, m_xftfonts[r]);
            m_xftfonts[r] = 0;
        }
    }

    m_xftfonts[ROT0] = newxftfont;
    m_xftfonts_loaded[ROT0] = true;
    m_name = name;

    // XGlyphInfo (used by XftFontImp::textWidth() / XftTextExtents8() etc)
    // holds only type 'short' or 'unsigned short'. any text bigger than that
    // yields either some negative or some 'wrapped' values ('integer
    // overflow'). to prevent something like this we detect the maximium
    // number of glyphs by calculating the amount of 'WW' (pretending a 'wide'
    // glyph) fitting into 32k pixels 
    unsigned int tw = textWidth("WW", 2);
    m_maxlength = 0x8000 / (tw == 0 ? 1 : tw);

    return true;
}

void XftFontImp::drawText(const FbDrawable &w, int screen, GC gc, const char* text, size_t len, int x, int y, FbTk::Orientation orient) {

    if (!text || !*text)
        return;

    if (!validOrientation(orient))
        return;

    // we adjust y slightly so that the baseline is in the right spot
    // (it is offset one by rotation >=180 degrees)
    switch (orient) {
    case ROT0:
        break;
    case ROT90:
        break;
    case ROT180:
        x+=1;
        y+=1;
        break;
    case ROT270:
        y+=1;
        break;
    }

    Visual* def_visual = DefaultVisual(w.display(), screen);
    Colormap def_colmap = DefaultColormap(w.display(), screen);

    XftFont *font = m_xftfonts[orient];
    XftDraw *draw = XftDrawCreate(w.display(), w.drawable(), def_visual, def_colmap);

    XGCValues gc_val;

    // get foreground pixel value and convert it to XRenderColor value
    // TODO: we should probably check return status
    XGetGCValues(w.display(), gc, GCForeground, &gc_val);

    // get red, green, blue values
    XColor xcol;
    xcol.pixel = gc_val.foreground;
    XQueryColor(w.display(), def_colmap, &xcol);

    // convert xcolor to XftColor
    XRenderColor rendcol;
    rendcol.red = xcol.red;
    rendcol.green = xcol.green;
    rendcol.blue = xcol.blue;
    rendcol.alpha = 0xFFFF;
    XftColor xftcolor;
    XftColorAllocValue(w.display(), def_visual, def_colmap, &rendcol, &xftcolor);

    // draw string
#ifdef HAVE_XFT_UTF8_STRING
    if (m_utf8mode) {
        // check the string size,
        // if the size is zero we use the XftDrawString8 function instead.
        XGlyphInfo ginfo;
        XftTextExtentsUtf8(w.display(), m_xftfonts[ROT0], (XftChar8 *)text, len, &ginfo);
        if (ginfo.xOff != 0) {
            XftDrawStringUtf8(draw, &xftcolor, font, x, y, (XftChar8 *)text, len);
            XftColorFree(w.display(), def_visual, def_colmap, &xftcolor);
            XftDrawDestroy(draw);
            return;
        }
    }
#endif // HAVE_XFT_UTF8_STRING

    XftDrawString8(draw, &xftcolor, font, x, y, (XftChar8 *)text, len);


    XftColorFree(w.display(), def_visual, def_colmap, &xftcolor);
    XftDrawDestroy(draw);
}

unsigned int XftFontImp::textWidth(const char* text, unsigned int len) const {

    if (m_xftfonts[ROT0] == 0)
        return 0;

    XGlyphInfo ginfo;
    Display* disp = App::instance()->display();

    XftFont *font = m_xftfonts[ROT0];

    len = std::min(len, m_maxlength);


#ifdef HAVE_XFT_UTF8_STRING
    if (m_utf8mode) {
        XftTextExtentsUtf8(disp, font, (XftChar8 *)text, len, &ginfo);
        if (ginfo.xOff != 0) {
            return ginfo.xOff;
        }

        // the utf8 failed, try normal extents
    }
#endif  //HAVE_XFT_UTF8_STRING

    std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, len));

    XftTextExtents8(disp,
                    font,
                    (XftChar8 *)localestr.data(), localestr.size(),
                    &ginfo);

    return ginfo.xOff;
}

unsigned int XftFontImp::height() const {
    if (m_xftfonts[ROT0] == 0)
        return 0;
    else
        return m_xftfonts[ROT0]->height;
    //m_xftfont->ascent + m_xftfont->descent;
    // curiously, fonts seem to have a smaller height, but the "height"
    // is specified within the actual font, so it must be right, right?
}

bool XftFontImp::validOrientation(FbTk::Orientation orient) {
    if (orient == ROT0 || m_xftfonts[orient])
        return true;

    if (m_xftfonts_loaded[orient])
        return false; // m_xftfonts is zero here

    if (m_xftfonts[ROT0] == 0)
        return false;

    m_xftfonts_loaded[orient] = true;

    // otherwise, try to load that orientation
    // radians is actually anti-clockwise, so we reverse it
    double radians = -(orient) * 90 * M_PI / 180;

    XftMatrix matrix;
    XftMatrixInit(&matrix);
    XftMatrixRotate(&matrix, cos(radians), sin(radians));

    Display *disp = App::instance()->display();

    XftPattern * pattern = XftNameParse(m_name.c_str());
    XftResult result;
    pattern = XftFontMatch(disp, 0, pattern, &result);
    XftPatternAddMatrix(pattern, XFT_MATRIX, &matrix);
    XftFont * new_font = XftFontOpenPattern(disp, pattern);

    if (new_font == 0)
        return false;

    m_xftfonts[orient] = new_font;

    return true;
}



} // end namespace FbTk
