// XmbFontImp.cc for FbTk fluxbox toolkit
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

// $Id: XmbFontImp.cc,v 1.6 2003/04/26 18:57:51 fluxgen Exp $

#include "XmbFontImp.hh"

#include "App.hh"
#include "StringUtil.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif // HAVE_SETLOCALE

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <cstring>

using namespace std;

namespace {

#ifndef HAVE_STRCASESTR
//!! TODO this is moved to StringUtil 
// Tries to find a string in another and
// ignoring the case of the characters
// Returns 0 on success else pointer to str.
const char *strcasestr(const char *str, const char *ptn) {
    const char *s2, *p2;
    for( ; *str; str++) {
        for(s2=str, p2=ptn; ; s2++,p2++) {	
            // check if we reached the end of ptn, if so, return str
            if (!*p2) return str; 
            // check if the chars match(ignoring case)
            if (toupper(*s2) != toupper(*p2)) break; 
        }
    }
    return 0;
}
#endif //HAVE_STRCASESTR

const char *getFontSize(const char *pattern, int *size) {
    const char *p;
    const char *p2=0;
    int n=0;

    for (p=pattern; 1; p++) {
        if (!*p) {
            if (p2!=0 && n>1 && n<72) {
                *size = n; return p2+1;
            } else {
                *size = 16; return 0;
            }
        } else if (*p=='-') {
            if (n>1 && n<72 && p2!=0) {
                *size = n;
                return p2+1;
            }
            p2=p; n=0;
        } else if (*p>='0' && *p<='9' && p2!=0) {
            n *= 10;
            n += *p-'0';
        } else {
            p2=0; n=0;
        }
    }
}

const char *getFontElement(const char *pattern, char *buf, int bufsiz, ...) {
    const char *p, *v;
    char *p2;
    va_list va;

    va_start(va, bufsiz);
    buf[bufsiz-1] = 0;
    buf[bufsiz-2] = '*';
    while((v = va_arg(va, char *)) != 0) {
        p = FbTk::StringUtil::strcasestr(pattern, v);
        if (p) {
            std::strncpy(buf, p+1, bufsiz-2);
            p2 = strchr(buf, '-');
            if (p2) *p2=0;
            va_end(va);
            return p;
        }
    }
    va_end(va);
    std::strncpy(buf, "*", bufsiz);
    return 0;
}

XFontSet createFontSet(const char *fontname) {
    Display *display = FbTk::App::instance()->display();
    XFontSet fs;
    const int FONT_ELEMENT_SIZE=50;
    char **missing, *def = "-";
    int nmissing, pixel_size = 0, buf_size = 0;
    char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

    fs = XCreateFontSet(display,
                        fontname, &missing, &nmissing, &def);
    if (fs && (! nmissing)) return fs;

#ifdef HAVE_SETLOCALE
    if (! fs) {
        if (nmissing) XFreeStringList(missing);

        setlocale(LC_CTYPE, "C");
        fs = XCreateFontSet(display, fontname,
                            &missing, &nmissing, &def);
        setlocale(LC_CTYPE, "");
    }
#endif // HAVE_SETLOCALE

    if (fs) {
        XFontStruct **fontstructs;
        char **fontnames;
        XFontsOfFontSet(fs, &fontstructs, &fontnames);
        fontname = fontnames[0];
    }

    getFontElement(fontname, weight, FONT_ELEMENT_SIZE,
                   "-medium-", "-bold-", "-demibold-", "-regular-", 0);
    getFontElement(fontname, slant, FONT_ELEMENT_SIZE,
                   "-r-", "-i-", "-o-", "-ri-", "-ro-", 0);
    getFontSize(fontname, &pixel_size);

    if (! strcmp(weight, "*")) 
        std::strncpy(weight, "medium", FONT_ELEMENT_SIZE);
    if (! strcmp(slant, "*")) 
        std::strncpy(slant, "r", FONT_ELEMENT_SIZE);
    if (pixel_size < 3) 
        pixel_size = 3;
    else if (pixel_size > 97) 
        pixel_size = 97;

    buf_size = strlen(fontname) + (FONT_ELEMENT_SIZE * 2) + 64;
    char *pattern2 = new char[buf_size];
    snprintf(pattern2, buf_size - 1,
             "%s,"
             "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
             "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
             fontname, weight, slant, pixel_size, pixel_size);
    fontname = pattern2;

    if (nmissing)
        XFreeStringList(missing);
    if (fs)
        XFreeFontSet(display, fs);

    fs = XCreateFontSet(display, fontname,
                        &missing, &nmissing, &def);
    delete [] pattern2;

    return fs;
}

};
namespace FbTk {

XmbFontImp::XmbFontImp(const char *filename, bool utf8):m_fontset(0), m_utf8mode(utf8) {
#ifdef DEBUG
#ifdef X_HAVE_UTF8_STRING
    cerr<<"Using utf8 = "<<utf8<<endl;
#else // X_HAVE_UTF8_STRING
    cerr<<"Using uft8 = false"<<endl;
#endif //X_HAVE_UTF8_STRING
#endif // DEBUG
    if (filename != 0)
        load(filename);
}

XmbFontImp::~XmbFontImp() {
    if (m_fontset != 0)
        XFreeFontSet(App::instance()->display(), m_fontset);
}

bool XmbFontImp::load(const std::string &fontname) {
    if (fontname.size() == 0)
        return false;
    XFontSet set = createFontSet(fontname.c_str());
    if (set == 0)
        return false;
    if (m_fontset != 0)
        XFreeFontSet(App::instance()->display(), m_fontset);
    m_fontset = set;
    m_setextents = XExtentsOfFontSet(m_fontset);

    return true;	
}

void XmbFontImp::drawText(Drawable w, int screen, GC gc, const char *text, 
                          size_t len, int x, int y) const {

    if (text == 0 || len == 0 || w == 0 || m_fontset == 0)
        return;
#ifdef X_HAVE_UTF8_STRING
    if (m_utf8mode) {
        Xutf8DrawString(App::instance()->display(), w, m_fontset,
			gc, x, y,
			text, len);
    } else 
#endif //X_HAVE_UTF8_STRING
	{
            XmbDrawString(App::instance()->display(), w, m_fontset,
                          gc, x, y,
                          text, len);
	}
}

unsigned int XmbFontImp::textWidth(const char * const text, unsigned int len) const {
    if (m_fontset == 0)
        return 0;
    XRectangle ink, logical;
#ifdef X_HAVE_UTF8_STRING
    if (m_utf8mode) {
        Xutf8TextExtents(m_fontset, text, len,
                         &ink, &logical);
    } else 
#endif // X_HAVE_UTF8_STRING
	{
            XmbTextExtents(m_fontset, text, len,
                           &ink, &logical);
	}

    return logical.width;
}

unsigned int XmbFontImp::height() const {
    if (m_fontset == 0)
        return 0;
    return m_setextents->max_ink_extent.height;
}

}; // end namespace FbTk

