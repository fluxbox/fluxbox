// XFontImp.cc for FbTk fluxbox toolkit
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

#include "XFontImp.hh"
#include "App.hh"
#include "GContext.hh"
#include "FbPixmap.hh"
#include "I18n.hh"

#include <X11/Xutil.h>

#include <iostream>
#include <new>
#include <cstdio>
#include <cstring>
#include <algorithm>


using std::cerr;
using std::endl;
using std::string;
using std::nothrow;

namespace FbTk {

XFontImp::XFontImp(const char *fontname):m_fontstruct(0) {
    for (int i = ROT0; i <= ROT270; ++i) {
        m_rotfonts[i] = 0;
        m_rotfonts_loaded[i] = false;
    }

    if (fontname != 0)
        load(fontname);
}

XFontImp::~XFontImp() {
    if (m_fontstruct != 0)
        XFreeFont(App::instance()->display(), m_fontstruct);

    for (int i = ROT0; i <= ROT270; ++i)
        if (m_rotfonts[i] != 0)
            freeRotFont(m_rotfonts[i]);
}

int XFontImp::ascent() const {
    if (m_fontstruct == 0)
        return 0;

    return m_fontstruct->ascent;
}

bool XFontImp::load(const string &fontname) {

    XFontStruct *font = XLoadQueryFont(App::instance()->display(), fontname.c_str());
    if (font == 0)
        return false;
    if (m_fontstruct != 0) // free old font struct, if any
        XFreeFont(App::instance()->display(), m_fontstruct);

    m_fontstruct = font; //set new font

    for (int i = ROT0; i <= ROT270; ++i) {
        m_rotfonts_loaded[i] = false;
        if (m_rotfonts[i] != 0) {
            freeRotFont(m_rotfonts[i]);
            m_rotfonts[i] = 0;
        }
    }

    return true;
}

void XFontImp::drawText(const FbDrawable &w, int screen, GC gc, const char* text, size_t len, int x, int y, FbTk::Orientation orient) {

    if (!text || !*text || m_fontstruct == 0)
        return;

    std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, len));

    // use roated font functions?
    if (orient != ROT0 && validOrientation(orient)) {
        drawRotText(w.drawable(), screen, gc, localestr.c_str(), localestr.size(), x, y, orient);
        return;
    }

    XSetFont(w.display(), gc, m_fontstruct->fid);
    XDrawString(w.display(), w.drawable(), gc, x, y, localestr.data(), localestr.size());
}

unsigned int XFontImp::textWidth(const char* text, unsigned int size) const {

    if (!text || !*text || m_fontstruct == 0)
        return 0;

    std::string localestr = FbStringUtil::FbStrToLocale(FbString(text, size));

    return XTextWidth(m_fontstruct, localestr.data(), localestr.size());
}

unsigned int XFontImp::height() const {
    if (m_fontstruct == 0)
        return 0;

    return m_fontstruct->ascent + m_fontstruct->descent;
}

void XFontImp::rotate(FbTk::Orientation orient) {
    //we must have a font loaded before we rotate
    if (m_fontstruct == 0 || m_fontstruct->per_char == 0 || orient == ROT0)
        return;

    _FB_USES_NLS;

    // X system default vars
    Display *dpy = App::instance()->display();
    Window rootwin = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);

    char text[3];
    int ichar, i, j, index, boxlen = 60;
    int vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
    int min_char, max_char;
    unsigned char *vertdata, *bitdata;
    int ascent, descent, lbearing, rbearing;

    // create the depth 1 canvas bitmap
    FbTk::FbPixmap canvas(rootwin, boxlen, boxlen, 1);

    // create graphic context for our canvas
    FbTk::GContext font_gc(canvas);
    font_gc.setBackground(None);
    font_gc.setFont(m_fontstruct->fid);

    // allocate space for rotated font
    m_rotfonts[orient] = new(nothrow) XRotFontStruct;
    XRotFontStruct *rotfont = m_rotfonts[orient];

    if (rotfont == 0) {
        cerr<<"RotFont: "<<_FBTK_CONSOLETEXT(Error, OutOfMemory, "Out of memory", "Something couldn't allocate memory")<<endl;
        return;
    }

    // determine which characters are defined in font
    min_char = m_fontstruct->min_char_or_byte2;
    max_char = m_fontstruct->max_char_or_byte2;

    // we only want printable chars
    if (min_char<32)
        min_char = 32;
    if (max_char>126)
        max_char = 126;

    /* some overall font data ... */
    rotfont->min_char = min_char;
    rotfont->max_char = max_char;
    rotfont->max_ascent = m_fontstruct->max_bounds.ascent;
    rotfont->max_descent = m_fontstruct->max_bounds.descent;
    rotfont->height = rotfont->max_ascent + rotfont->max_descent;

    // font needs rotation
    // loop through each character
    for (ichar = min_char; ichar <= max_char; ichar++) {
        index = ichar - m_fontstruct->min_char_or_byte2;

        // per char dimensions ...
        ascent = rotfont->per_char[ichar-32].ascent = m_fontstruct->per_char[index].ascent;
        descent =  rotfont->per_char[ichar-32].descent = m_fontstruct->per_char[index].descent;
        lbearing = rotfont->per_char[ichar-32].lbearing = m_fontstruct->per_char[index].lbearing;
        rbearing = rotfont->per_char[ichar-32].rbearing = m_fontstruct->per_char[index].rbearing;
        rotfont->per_char[ichar-32].width = m_fontstruct->per_char[index].width;

        // some space chars have zero body, but a bitmap can't have
        if (!ascent && !descent)
            ascent = rotfont->per_char[ichar-32].ascent =   1;
        if (!lbearing && !rbearing)
            rbearing = rotfont->per_char[ichar-32].rbearing = 1;

        // glyph width and height when vertical
        vert_w = rbearing - lbearing;
        vert_h = ascent + descent;

        // width in bytes
        vert_len = (vert_w-1)/8+1;

        font_gc.setForeground(None);
        canvas.fillRectangle(font_gc.gc(),
                             0, 0,
                             boxlen, boxlen);
        // draw the character centre top right on canvas
        snprintf(text, 1, "%c", ichar);
        font_gc.setForeground(1);
        XDrawImageString(dpy, canvas.drawable(), font_gc.gc(),
                         boxlen/2 - lbearing,
                         boxlen/2 - descent, text, 1);

        // reserve memory for first XImage
        vertdata = (unsigned char *)calloc((unsigned)(vert_len * vert_h), 1);

        XImage *I1 = XCreateImage(dpy, DefaultVisual(dpy, screen),
                                  1, XYBitmap,
                                  0, (char *)vertdata,
                                  vert_w, vert_h, 8, 0);

        if (I1 == None) {
            cerr << "RotFont: " << _FBTK_CONSOLETEXT(Error, CreateXImage,
                                         "Can't create XImage",
                                         "XCreateImage failed for some reason")
                 << "." << endl;
            free(vertdata);
            delete rotfont;
            m_rotfonts[orient] = 0;
            return;
        }

        I1->byte_order = I1->bitmap_bit_order = MSBFirst;

        // extract character from canvas
        XGetSubImage(dpy, canvas.drawable(),
                     boxlen/2, boxlen/2 - vert_h,
                     vert_w, vert_h, 1, XYPixmap, I1, 0, 0);

        I1->format = XYBitmap;

        // width, height of rotated character
        if (orient == ROT180) {
            bit_w = vert_w;
            bit_h = vert_h;
        } else {
            bit_w = vert_h;
            bit_h = vert_w;
        }

        // width in bytes
        bit_len = (bit_w-1)/8 + 1;

        rotfont->per_char[ichar-32].glyph.bit_w = bit_w;
        rotfont->per_char[ichar-32].glyph.bit_h = bit_h;

        // reserve memory for the rotated image
        bitdata = (unsigned char *)calloc((unsigned)(bit_h * bit_len), 1);

        // create the image
        XImage *I2 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap, 0,
                          (char *)bitdata, bit_w, bit_h, 8, 0);

        if (I2 == None) {
            cerr << "XFontImp: " <<_FBTK_CONSOLETEXT(Error, CreateXImage,
                                          "Can't create XImage",
                                          "XCreateImage failed for some reason")
                 << "." << endl;
            XDestroyImage(I1);
            free(bitdata);
            delete rotfont;
            m_rotfonts[orient] = 0;
            return;
        }

        I2->byte_order = I2->bitmap_bit_order = MSBFirst;

        // map vertical data to rotated character
        for (j = 0; j < bit_h; j++) {
            for (i = 0; i < bit_w; i++) {
                char val = 0;
                if (orient == ROT270) {
                    val = vertdata[i*vert_len + (vert_w-j-1)/8] &
                        (128>>((vert_w-j-1)%8));
                } else if (orient == ROT180) {
                    val = vertdata[(vert_h-j-1)*vert_len +
                                   (vert_w-i-1)/8] & (128>>((vert_w-i-1)%8));
                } else {
                    val = vertdata[(vert_h-i-1)*vert_len + j/8] &
                        (128>>(j%8));
                }
                if (val) {
                    bitdata[j*bit_len + i/8] = bitdata[j*bit_len + i/8] |
                        (128>>(i%8));
                }
            }
        }

        // create this character's bitmap
        rotfont->per_char[ichar-32].glyph.bm =
            XCreatePixmap(dpy, rootwin, bit_w, bit_h, 1);

        // put the image into the bitmap
        XPutImage(dpy, rotfont->per_char[ichar-32].glyph.bm,
                  font_gc.gc(), I2, 0, 0, 0, 0, bit_w, bit_h);

        // free the image and data
        XDestroyImage(I1);
        XDestroyImage(I2);
    }

}

void XFontImp::freeRotFont(XRotFontStruct *rotfont) {
    // loop through each character and free its pixmap
    for (int ichar = rotfont->min_char - 32;
         ichar <= rotfont->max_char - 32; ++ichar) {
        XFreePixmap(App::instance()->display(), rotfont->per_char[ichar].glyph.bm);
    }

    delete rotfont;
    rotfont = 0;
}

void XFontImp::drawRotText(Drawable w, int screen, GC gc, const char* text, size_t len, int x, int y, FbTk::Orientation orient) const {

    if (!text || !*text || len<1)
        return;

    Display *dpy = App::instance()->display();
    static GC my_gc = 0;
    int xp, yp, ichar;

    XRotFontStruct *rotfont = m_rotfonts[orient];

    if (my_gc == 0)
        my_gc = XCreateGC(dpy, w, 0, 0);

    XCopyGC(dpy, gc, GCForeground|GCBackground, my_gc);

    // vertical or upside down
    XSetFillStyle(dpy, my_gc, FillStippled);

    // loop through each character in texting
    for (size_t i = 0; i<len; i++) {
        ichar = text[i]-32;

        // make sure it's a printing character
        if (ichar >= 0 && ichar<95) {
            // suitable offset
            if (orient == ROT270) {
                xp = x-rotfont->per_char[ichar].ascent;
                yp = y-rotfont->per_char[ichar].rbearing;
            } else if (orient == ROT180) {
                xp = x-rotfont->per_char[ichar].rbearing;
                yp = y-rotfont->per_char[ichar].descent+1;
            } else { // ROT90
                xp = x-rotfont->per_char[ichar].descent;
                yp = y+rotfont->per_char[ichar].lbearing;
            }

            // draw the glyph
            XSetStipple(dpy, my_gc, rotfont->per_char[ichar].glyph.bm);

            XSetTSOrigin(dpy, my_gc, xp, yp);

            XFillRectangle(dpy, w, my_gc, xp, yp,
                           rotfont->per_char[ichar].glyph.bit_w,
                           rotfont->per_char[ichar].glyph.bit_h);

            // advance position
            if (orient == ROT270)
                y -= rotfont->per_char[ichar].width;
            else if (orient == ROT180)
                x -= rotfont->per_char[ichar].width;
            else
                y += rotfont->per_char[ichar].width;
        }
    }
}


bool XFontImp::validOrientation(FbTk::Orientation orient) {
    if (orient == ROT0 || m_rotfonts[orient])
        return true;

    if (m_rotfonts_loaded[orient])
        return false; // load must have failed

    m_rotfonts_loaded[orient] = true;
    rotate(orient);

    return m_rotfonts[orient] != 0;
}

}
