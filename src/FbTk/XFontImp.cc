// XFontImp.cc for FbTk fluxbox toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: XFontImp.cc,v 1.4 2002/12/01 13:42:14 rathnor Exp $

#include "XFontImp.hh"
#include "App.hh"

#include <X11/Xutil.h>

#include <iostream>
#include <new>
#include <cstdio>
using namespace std;

namespace FbTk {

XFontImp::XFontImp(const char *fontname):m_rotfont(0), m_fontstruct(0),
                                         m_angle(0) {
    if (fontname != 0)
        load(fontname);	
}

XFontImp::~XFontImp() {
    if (m_fontstruct != 0)
        XFreeFont(App::instance()->display(), m_fontstruct);
    if (m_rotfont != 0)
        freeRotFont();
}

int XFontImp::ascent() const {
    if (m_fontstruct == 0)
        return 0;
    if (m_rotfont != 0)
        return m_rotfont->max_ascent;
		
    return m_fontstruct->ascent;
}

bool XFontImp::load(const std::string &fontname) {
    XFontStruct *font = XLoadQueryFont(App::instance()->display(), fontname.c_str());
    if (font == 0)
        return false;
    if (m_fontstruct != 0) // free old font struct, if any
        XFreeFont(App::instance()->display(), m_fontstruct);

    m_fontstruct = font; //set new font
    return true;
}

void XFontImp::drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const {
    if (m_fontstruct == 0)
        return;
    // use roated font functions?
    if (m_rotfont != 0) {
        drawRotText(w, screen, gc, text, len, x, y);
        return;
    }

    Display *disp = App::instance()->display();
    XSetFont(disp, gc, m_fontstruct->fid);
    XDrawString(disp, w, gc, x, y, text, len);
}

unsigned int XFontImp::textWidth(const char * const text, unsigned int size) const {
    if (text == 0)
        return 0;
    if (m_fontstruct == 0)
        return 0;
    // check rotated font?
    if (m_rotfont != 0)
        return rotTextWidth(text, size);

    return XTextWidth(m_fontstruct, text, size);
}

unsigned int XFontImp::height() const {
    if (m_fontstruct == 0)
        return 0;

    return m_fontstruct->ascent + m_fontstruct->descent;
}

void XFontImp::rotate(float angle) {
    //we must have a font loaded before we rotate
    if (m_fontstruct == 0)
        return;
    if (m_rotfont != 0)
        freeRotFont();
    // no need for rotating, use regular font
    if (angle == 0)
        return;

    //get positive angle
    while (angle < 0)
        angle += 360;

    char val;
    XImage *I1, *I2;
    // X system default vars
    Display *dpy = App::instance()->display();
    Window rootwin = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);

    GC font_gc;
    char text[3];
    int ichar, i, j, index, boxlen = 60;
    int vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
    int min_char, max_char;
    unsigned char *vertdata, *bitdata;
    int ascent, descent, lbearing, rbearing;

    // get nearest vertical or horizontal direction 
    int dir = (int)((angle+45.0)/90.0)%4;

    if (dir == 0) // no rotation
        return;

    // create the depth 1 canvas bitmap
    Pixmap canvas = XCreatePixmap(dpy, rootwin, boxlen, boxlen, 1);
 
    // create graphic context for our canvas
    font_gc = XCreateGC(dpy, canvas, 0, 0);

    XSetBackground(dpy, font_gc, None);

    XSetFont(dpy, font_gc, m_fontstruct->fid);

    // allocate space for rotated font
    m_rotfont = new(nothrow) XRotFontStruct;

    if (m_rotfont == 0) {
        cerr<<"RotFont: out of memory"<<endl;
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
    m_rotfont->dir = dir;
    m_rotfont->min_char = min_char;
    m_rotfont->max_char = max_char;
    m_rotfont->max_ascent = m_fontstruct->max_bounds.ascent;
    m_rotfont->max_descent = m_fontstruct->max_bounds.descent;   
    m_rotfont->height = m_rotfont->max_ascent + m_rotfont->max_descent;

    // font needs rotation
    // loop through each character
    for (ichar = min_char; ichar <= max_char; ichar++) {
        index = ichar - m_fontstruct->min_char_or_byte2;

        // per char dimensions ...
        ascent = m_rotfont->per_char[ichar-32].ascent = m_fontstruct->per_char[index].ascent;
        descent =  m_rotfont->per_char[ichar-32].descent = m_fontstruct->per_char[index].descent;
        lbearing = m_rotfont->per_char[ichar-32].lbearing = m_fontstruct->per_char[index].lbearing;
        rbearing = m_rotfont->per_char[ichar-32].rbearing = m_fontstruct->per_char[index].rbearing;
        m_rotfont->per_char[ichar-32].width = m_fontstruct->per_char[index].width;

        // some space chars have zero body, but a bitmap can't have
        if (!ascent && !descent)   
            ascent = m_rotfont->per_char[ichar-32].ascent =   1;
        if (!lbearing && !rbearing) 
            rbearing = m_rotfont->per_char[ichar-32].rbearing = 1;

        // glyph width and height when vertical
        vert_w = rbearing - lbearing;
        vert_h = ascent + descent;

        // width in bytes
        vert_len = (vert_w-1)/8+1;   

        XSetForeground(dpy, font_gc, None);
        XFillRectangle(dpy, canvas, font_gc, 0, 0, boxlen, boxlen);

        // draw the character centre top right on canvas
        sprintf(text, "%c", ichar);
        XSetForeground(dpy, font_gc, 1);
        XDrawImageString(dpy, canvas, font_gc, boxlen/2 - lbearing,
                         boxlen/2 - descent, text, 1);

        // reserve memory for first XImage
        vertdata = (unsigned char *) malloc((unsigned)(vert_len*vert_h));

        /* create the XImage ... */
        I1 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap,
                          0, (char *)vertdata, vert_w, vert_h, 8, 0);

        if (I1 == None) {				
            cerr<<"RotFont: Cant create ximage."<<endl;
            delete m_rotfont;
            m_rotfont = 0;			
            return;
        }

        I1->byte_order = I1->bitmap_bit_order = MSBFirst;

        /* extract character from canvas ... */
        XGetSubImage(dpy, canvas, boxlen/2, boxlen/2 - vert_h,
                     vert_w, vert_h, 1, XYPixmap, I1, 0, 0);
        I1->format = XYBitmap; 

        /* width, height of rotated character ... */
        if (dir == 2) { 
            bit_w = vert_w;
            bit_h = vert_h; 
        } else {
            bit_w = vert_h;
            bit_h = vert_w; 
        }

        /* width in bytes ... */
        bit_len = (bit_w-1)/8 + 1;

        m_rotfont->per_char[ichar-32].glyph.bit_w = bit_w;
        m_rotfont->per_char[ichar-32].glyph.bit_h = bit_h;

        /* reserve memory for the rotated image ... */
        bitdata = (unsigned char *)calloc((unsigned)(bit_h * bit_len), 1);

        /* create the image ... */
        I2 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap, 0,
                          (char *)bitdata, bit_w, bit_h, 8, 0); 

        if (I2 == None) {
            cerr<<"XFontImp: Cant create ximage!"<<endl;
            delete m_rotfont;
            m_rotfont = 0;
            return;
        }

        I2->byte_order = I2->bitmap_bit_order = MSBFirst;

        /* map vertical data to rotated character ... */
        for (j = 0; j < bit_h; j++) {
            for (i = 0; i < bit_w; i++) {
				/* map bits ... */
                if (dir == 1) {
                    val = vertdata[i*vert_len + (vert_w-j-1)/8] &
                        (128>>((vert_w-j-1)%8));
                } else if (dir == 2) {
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
        m_rotfont->per_char[ichar-32].glyph.bm = 
            XCreatePixmap(dpy, rootwin, bit_w, bit_h, 1);
 
        // put the image into the bitmap 
        XPutImage(dpy, m_rotfont->per_char[ichar-32].glyph.bm, 
                  font_gc, I2, 0, 0, 0, 0, bit_w, bit_h);

        // free the image and data
        XDestroyImage(I1);
        XDestroyImage(I2);
    }

    /* free pixmap and GC ... */
    XFreePixmap(dpy, canvas);
    XFreeGC(dpy, font_gc);

}

void XFontImp::freeRotFont() {
    if (m_rotfont == 0)
        return;
    // loop through each character and free its pixmap
    for (int ichar = m_rotfont->min_char - 32; 
         ichar <= m_rotfont->max_char - 32; ++ichar) {
        XFreePixmap(App::instance()->display(), m_rotfont->per_char[ichar].glyph.bm);
    }

    delete m_rotfont;
    m_rotfont = 0;
}

void XFontImp::drawRotText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const {            

    Display *dpy = App::instance()->display();
    static GC my_gc = 0;
    int xp, yp, dir, ichar;

    if (text == NULL || len<1)
        return;

    dir = m_rotfont->dir;
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
            if (dir == 1) {
                xp = x-m_rotfont->per_char[ichar].ascent;
                yp = y-m_rotfont->per_char[ichar].rbearing; 
            } else if (dir == 2) {
                xp = x-m_rotfont->per_char[ichar].rbearing;
                yp = y-m_rotfont->per_char[ichar].descent+1; 
            } else {
                xp = x-m_rotfont->per_char[ichar].descent+1;  
                yp = y+m_rotfont->per_char[ichar].lbearing; 
            }
                   
            // draw the glyph
            XSetStipple(dpy, my_gc, m_rotfont->per_char[ichar].glyph.bm);
    
            XSetTSOrigin(dpy, my_gc, xp, yp);
      
            XFillRectangle(dpy, w, my_gc, xp, yp,
                           m_rotfont->per_char[ichar].glyph.bit_w,
                           m_rotfont->per_char[ichar].glyph.bit_h);
    
            // advance position
            if (dir == 1)
                y -= m_rotfont->per_char[ichar].width;
            else if (dir == 2)
                x -= m_rotfont->per_char[ichar].width;
            else 
                y += m_rotfont->per_char[ichar].width;
        }
    }
}


unsigned int XFontImp::rotTextWidth(const char * const text, unsigned int size) const {

    if (text == 0)
        return 0;

    unsigned int width = 0;	
    for (size_t i = 0; i<size; i++) {
        int ichar = text[i] - 32;  
        // make sure it's a printing character
        if (ichar >= 0 && ichar < 95) 
            width += m_rotfont->per_char[ichar].width;
    }

    return width;
}

};
