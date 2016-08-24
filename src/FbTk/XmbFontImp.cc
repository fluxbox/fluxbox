// XmbFontImp.cc for FbTk fluxbox toolkit
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

#include "XmbFontImp.hh"

#include "App.hh"
#include "TextUtils.hh"
#include "StringUtil.hh"
#include "FbPixmap.hh"
#include "GContext.hh"

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif // HAVE_SETLOCALE

#include <cstdarg>
#include <cstring>

using std::string;

namespace {

XFontSet createFontSet(const char *fontname, bool& utf8mode) {

    Display *display = FbTk::App::instance()->display();
    XFontSet fs = 0;
    char **missing;
    const char *constdef = "-";
    char *def = const_cast<char *>(constdef);
    int nmissing;
    string orig_locale = "";

#ifdef HAVE_SETLOCALE
    if (utf8mode) {
        orig_locale = setlocale(LC_CTYPE, NULL);
        if (setlocale(LC_CTYPE, "UTF-8") == NULL) {
            utf8mode = false;
        }
    }
#endif // HAVE_SETLOCALE
    fs = XCreateFontSet(display,
                        fontname, &missing, &nmissing, &def);

    if (fs && (! nmissing)) {
#ifdef HAVE_SETLOCALE
        if (utf8mode)
            setlocale(LC_CTYPE, orig_locale.c_str());
#endif // HAVE_SETLOCALE
        return fs;
    }

#ifdef HAVE_SETLOCALE
    if (! fs) {
        if (nmissing)
            XFreeStringList(missing);

        setlocale(LC_CTYPE, "C");
        fs = XCreateFontSet(display, fontname,
                            &missing, &nmissing, &def);
        setlocale(LC_CTYPE, orig_locale.c_str());
        return fs;
    }
    if (utf8mode)
        setlocale(LC_CTYPE, orig_locale.c_str());
#endif // HAVE_SETLOCALE

    // set to false because our strings won't be utf8-happy
    utf8mode = false;

    return fs;
}

}

namespace FbTk {

XmbFontImp::XmbFontImp(const char *filename, bool utf8) : m_fontset(0), m_setextents(0), m_utf8mode(utf8) {
    if (filename != 0)
        load(filename);
}

XmbFontImp::~XmbFontImp() {
    if (m_fontset != 0)
        XFreeFontSet(App::instance()->display(), m_fontset);
}

bool XmbFontImp::load(const string &fontname) {
    if (fontname.empty())
        return false;

    XFontSet set = createFontSet(fontname.c_str(), m_utf8mode);
    if (set == 0)
        return false;

    if (m_fontset != 0)
        XFreeFontSet(App::instance()->display(), m_fontset);

    m_fontset = set;
    m_setextents = XExtentsOfFontSet(m_fontset);

    return true;
}

void XmbFontImp::drawText(const FbDrawable &d, int screen, GC main_gc, const char* text,
                          size_t len, int x, int y, FbTk::Orientation orient) {

    if (!text || !*text || m_fontset == 0)
        return;

    if (orient == ROT0) {
#ifdef X_HAVE_UTF8_STRING
        if (m_utf8mode) {
            Xutf8DrawString(d.display(), d.drawable(), m_fontset,
                            main_gc, x, y,
                            text, len);
        } else
#endif //X_HAVE_UTF8_STRING
            {
                std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, len));
                XmbDrawString(d.display(), d.drawable(), m_fontset,
                              main_gc, x, y,
                              localestr.data(), localestr.size());
            }
        return;
    }

    Display *dpy = App::instance()->display();

    int xpos = x, ypos = y;
    unsigned int w = d.width();
    unsigned int h = d.height();

    translateSize(orient, w, h);
    untranslateCoords(orient, xpos, ypos, w, h);

    // not straight forward, we actually draw it elsewhere, then rotate it
    FbTk::FbPixmap canvas(d.drawable(), w, h, 1);

    // create graphic context for our canvas
    FbTk::GContext font_gc(canvas);
    font_gc.setBackground(None);
    font_gc.setForeground(None);

    XFillRectangle(dpy, canvas.drawable(), font_gc.gc(), 0, 0, canvas.width(), canvas.height());
    font_gc.setForeground(1);


#ifdef X_HAVE_UTF8_STRING
    if (m_utf8mode) {
        Xutf8DrawString(dpy, canvas.drawable(), m_fontset,
                             font_gc.gc(), xpos, ypos,
                             text, len);
    } else
#endif //X_HAVE_UTF8_STRING
    {
        std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, len));
        XmbDrawString(dpy, canvas.drawable(), m_fontset,
                           font_gc.gc(), xpos, ypos,
                           localestr.data(), localestr.size());
    }

    canvas.rotate(orient);

    GC my_gc = XCreateGC(dpy, d.drawable(), 0, 0);

    XCopyGC(dpy, main_gc, GCForeground|GCBackground, my_gc);

    // vertical or upside down
    XSetFillStyle(dpy, my_gc, FillStippled);
    XSetStipple(dpy, my_gc, canvas.drawable());
    XSetTSOrigin(dpy, my_gc, 0, 0);

    XFillRectangle(dpy, d.drawable(), my_gc, 0, 0,
                   canvas.width(),
                   canvas.height());

    XFreeGC(dpy, my_gc);

}

unsigned int XmbFontImp::textWidth(const char* text, unsigned int len) const {

    if (m_fontset == 0)
        return 0;

    XRectangle ink, logical;
#ifdef X_HAVE_UTF8_STRING
    if (m_utf8mode) {
        Xutf8TextExtents(m_fontset, text, len, &ink, &logical);
        if (logical.width != 0)
            return logical.width;
    }
#endif // X_HAVE_UTF8_STRING

    std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, len));
    XmbTextExtents(m_fontset, localestr.data(), localestr.size(),
                   &ink, &logical);
    return logical.width;
}

unsigned int XmbFontImp::height() const {
    if (m_fontset == 0)
        return 0;
    return m_setextents->max_ink_extent.height;
}

} // end namespace FbTk

