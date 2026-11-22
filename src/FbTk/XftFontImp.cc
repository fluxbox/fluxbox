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
    m_utf8mode(utf8), m_name(""), m_maxlength(0x8000), m_pattern(0), m_coverage(0) {

    for (int r = ROT0; r <= ROT270; r++) {
        m_xftfonts[r] = 0;
        m_xftfonts_loaded[r] = false;
    }

    if (name != 0)
        load(name);
}

XftFontImp::~XftFontImp() {
    closeFonts();
}

void XftFontImp::closeFonts() {
    Display *disp = App::instance()->display();
    for (int r = ROT0; r <= ROT270; r++) {
        m_xftfonts_loaded[r] = false;
        if (m_xftfonts[r] != 0) {
            XftFontClose(disp, m_xftfonts[r]);
            m_xftfonts[r] = 0;
        }
        for (size_t i = 0; i < m_fallbacks[r].size(); i++)
            XftFontClose(disp, m_fallbacks[r][i]);
        m_fallbacks[r].clear();
    }
    m_nofont.clear();
    if (m_coverage != 0) {
        FcCharSetDestroy(m_coverage);
        m_coverage = 0;
    }
    if (m_pattern != 0) {
        FcPatternDestroy(m_pattern);
        m_pattern = 0;
    }
}

namespace {

// matches req (consumed) against the installed fonts and opens the result,
// rotated for orient
XftFont* openFont(Display* disp, FcPattern* req, Orientation orient) {
    XftResult result;
    FcPattern* match = XftFontMatch(disp, 0, req, &result);
    FcPatternDestroy(req);
    if (match == 0)
        return 0;

    // radians is actually anti-clockwise, so we reverse it
    double radians = -(orient) * 90 * M_PI / 180;
    XftMatrix matrix;
    XftMatrixInit(&matrix);
    XftMatrixRotate(&matrix, cos(radians), sin(radians));
    XftPatternAddMatrix(match, XFT_MATRIX, &matrix);

    XftFont* font = XftFontOpenPattern(disp, match);
    if (font == 0)
        FcPatternDestroy(match);
    return font;
}

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
    closeFonts();

    m_pattern = XftXlfdParse(name.c_str(), False, False);
    if (m_pattern == 0)
        m_pattern = FcNameParse((const FcChar8*)name.c_str());

    m_xftfonts[ROT0] = newxftfont;
    m_xftfonts_loaded[ROT0] = true;
    m_name = name;

    // XGlyphInfo (used by XftFontImp::textWidth() / XftTextExtents8() etc)
    // holds only type 'short' or 'unsigned short'. any text bigger than that
    // yields either some negative or some 'wrapped' values ('integer
    // overflow'). to prevent something like this we detect the maximium
    // number of glyphs by calculating the amount of 'WW' (pretending a 'wide'
    // glyph) fitting into 32k pixels 
    unsigned int tw = std::max<unsigned int>(textWidth("WW", 2), 1);
    m_maxlength = 0x8000 / tw;

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
    // like Xft: utf8 up to the first invalid byte (truncation cuts by bytes,
    // so the last char may be partial); undecodable from the start is 8bit
    FcChar32 first;
    if (m_utf8mode && FcUtf8ToUcs4((const FcChar8*)text, &first, len) > 0) {
        // draw run by run, each with a font that has its glyphs, advancing
        // like Xft does within a run (rotated fonts give rotated offsets)
        for (size_t i = 0; i < len; ) {
            XftFont* run_font;
            size_t n = nextRun(text + i, len - i, orient, run_font);
            if (n == 0)
                break;
            XftDrawStringUtf8(draw, &xftcolor, run_font, x, y, (XftChar8 *)text + i, n);
            XGlyphInfo ginfo;
            XftTextExtentsUtf8(w.display(), run_font, (XftChar8 *)text + i, n, &ginfo);
            x += ginfo.xOff;
            y += ginfo.yOff;
            i += n;
        }
        XftColorFree(w.display(), def_visual, def_colmap, &xftcolor);
        XftDrawDestroy(draw);
        return;
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
    // same utf8 rule as drawText(), else the locale extents
    FcChar32 first;
    if (m_utf8mode && FcUtf8ToUcs4((const FcChar8*)text, &first, len) > 0) {
        unsigned int width = 0;
        for (unsigned int i = 0; i < len; ) {
            XftFont* run_font;
            size_t n = nextRun(text + i, len - i, ROT0, run_font);
            if (n == 0)
                break;
            XftTextExtentsUtf8(disp, run_font, (XftChar8 *)text + i, n, &ginfo);
            width += ginfo.xOff;
            i += n;
        }
        return width;
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
    if (m_pattern == 0)
        return false;

    XftFont * new_font = openFont(App::instance()->display(),
                                  FcPatternDuplicate(m_pattern), orient);
    if (new_font == 0)
        return false;

    m_xftfonts[orient] = new_font;

    return true;
}

XftFont* XftFontImp::fontFor(FcChar32 ch, Orientation orient) const {
    Display *disp = App::instance()->display();
    XftFont* font = m_xftfonts[orient];

    if (XftCharExists(disp, font, ch) || m_pattern == 0 || m_nofont.count(ch))
        return font;

    for (size_t i = 0; i < m_fallbacks[orient].size(); i++) {
        if (XftCharExists(disp, m_fallbacks[orient][i], ch))
            return m_fallbacks[orient][i];
    }

    // one FcFontSort tells what all installed fonts cover together; a char
    // outside it skips the costly match below, so a client can't stall us
    // with a title full of distinct glyph-less code points. computed on the
    // first miss only: fonts that never miss don't pay for it
    if (m_coverage == 0) {
        FcPattern* all = FcPatternDuplicate(m_pattern);
        FcConfigSubstitute(0, all, FcMatchPattern);
        FcDefaultSubstitute(all);
        FcResult result;
        FcFontSet* fonts = FcFontSort(0, all, FcTrue, &m_coverage, &result);
        if (fonts != 0)
            FcFontSetDestroy(fonts);
        FcPatternDestroy(all);
        if (m_coverage == 0)
            m_coverage = FcCharSetCreate(); // no fonts: nothing to fall back to
    }
    if (!FcCharSetHasChar(m_coverage, ch))
        return font;

    // ask fontconfig for the font closest to ours that has ch
    FcPattern* req = FcPatternDuplicate(m_pattern);
    FcCharSet* charset = FcCharSetCreate();
    FcCharSetAddChar(charset, ch);
    FcPatternAddCharSet(req, FC_CHARSET, charset);
    FcCharSetDestroy(charset);

    XftFont* fallback = openFont(disp, req, orient);
    if (fallback != 0 && XftCharExists(disp, fallback, ch)) {
        m_fallbacks[orient].push_back(fallback);
        return fallback;
    }
    if (fallback != 0)
        XftFontClose(disp, fallback);

    m_nofont.insert(ch); // don't ask fontconfig again on every redraw
    return font;
}

size_t XftFontImp::nextRun(const char* text, size_t len, Orientation orient, XftFont*& font) const {
    font = 0;
    size_t i = 0;
    while (i < len) {
        FcChar32 ch;
        int n = FcUtf8ToUcs4((const FcChar8*)text + i, &ch, len - i);
        if (n <= 0)
            break;
        XftFont* f = fontFor(ch, orient);
        if (font != 0 && f != font)
            break;
        font = f;
        i += n;
    }
    return i; // 0: text starts with an invalid byte
}



} // end namespace FbTk
