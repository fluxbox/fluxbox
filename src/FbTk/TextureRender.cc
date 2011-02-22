// TextureRender.cc for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// from Image.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

#include "TextureRender.hh"

#include "ImageControl.hh"
#include "TextUtils.hh"
#include "Texture.hh"
#include "App.hh"
#include "FbPixmap.hh"
#include "GContext.hh"
#include "I18n.hh"
#include "StringUtil.hh"

#include <X11/Xutil.h>

#include <iostream>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

// mipspro has no new(nothrow)
#if defined sgi && ! defined GCC
#define FB_new_nothrow new
#else
#define FB_new_nothrow new(std::nothrow)
#endif

using std::cerr;
using std::endl;
using std::string;
using std::max;
using std::min;

namespace {

unsigned long bsqrt(unsigned int x) {

    static const size_t SQRT_TABLE_ENTRIES = 256 * 256 * 2;

    // '362' == bsqrt(256 * 256 * 2), fits into a short
    static unsigned short sqrt_table[SQRT_TABLE_ENTRIES] = { 1 };

    // build sqrt table for use with elliptic gradient
    // sqrt_table[0] is set to 0 after the initial run
    if (sqrt_table[0] == 1) {
        sqrt_table[0] = 0;
        sqrt_table[1] = 1;

        unsigned long r;
        unsigned long q;
        for (x = 2; x < SQRT_TABLE_ENTRIES; x++) {
            r = x >> 1;
            while (1) {
                q = x / r;
                if (q >= r) {
                    sqrt_table[x] = static_cast<unsigned short>(r);
                    break;
                }
                r = (r + q) >> 1;
            }
        }
    }

    return sqrt_table[std::min(static_cast<size_t>(x), SQRT_TABLE_ENTRIES - 1)];
}

/*

   x1 y1 ---- gc1 ---- x2 y1
     |                   |
     |                   |
    gc2                 gc1
     |                   |
     |                   |
   x1 y2 ---- gc2 ---- x2 y2

 */
void drawBevelRectangle(FbTk::FbDrawable& d, GC gc1, GC gc2, int x1, int y1, int x2, int y2) {
    d.drawLine(gc1, x1, y1, x2, y1);
    d.drawLine(gc1, x2, y1, x2, y2);
    d.drawLine(gc2, x1, y2, x2, y2);
    d.drawLine(gc2, x1, y1, x1, y2);
}





void renderBevel1(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* red, unsigned char* green, unsigned char* blue,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    if (! (width > 2 && height > 2))
        return;

    unsigned char *pr = red, *pg = green, *pb = blue;

    register unsigned char r, g, b, rr ,gg ,bb;
    register unsigned int w = width, h = height - 1, wh = w * h;

    while (--w) {
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;

        *pr = rr;
        *pg = gg;
        *pb = bb;

        r = *(pr + wh);
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *(pg + wh);
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *(pb + wh);
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;

        *((pr++) + wh) = rr;
        *((pg++) + wh) = gg;
        *((pb++) + wh) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    r = *(pr + wh);
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *(pg + wh);
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *(pb + wh);
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *(pr + wh) = rr;
    *(pg + wh) = gg;
    *(pb + wh) = bb;

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;

        *pr = rr;
        *pg = gg;
        *pb = bb;

        pr += width - 1;
        pg += width - 1;
        pb += width - 1;

        r = *pr;
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *pg;
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *pb;
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;

        *(pr++) = rr;
        *(pg++) = gg;
        *(pb++) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    pr += width - 1;
    pg += width - 1;
    pb += width - 1;

    r = *pr;
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *pg;
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *pb;
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *pr = rr;
    *pg = gg;
    *pb = bb;
}



void renderBevel2(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* red, unsigned char* green, unsigned char* blue,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    if (! (width > 4 && height > 4))
        return;

    unsigned char r, g, b, rr ,gg ,bb, *pr = red + width + 1,
        *pg = green + width + 1, *pb = blue + width + 1;
    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);

    while (--w) {
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;

        *pr = rr;
        *pg = gg;
        *pb = bb;

        r = *(pr + wh);
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *(pg + wh);
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *(pb + wh);
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;

        *((pr++) + wh) = rr;
        *((pg++) + wh) = gg;
        *((pb++) + wh) = bb;
    }

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;

        *(++pr) = rr;
        *(++pg) = gg;
        *(++pb) = bb;

        pr += width - 3;
        pg += width - 3;
        pb += width - 3;

        r = *pr;
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *pg;
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *pb;
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;

        *(pr++) = rr;
        *(pg++) = gg;
        *(pb++) = bb;

        pr++; pg++; pb++;
    }
}




void invertRGB(unsigned int w, unsigned int h,
        unsigned char* r, unsigned char* g, unsigned char* b) {

    register unsigned int i, j, wh = (w * h) - 1;

    for (i = 0, j = wh; j > i; j--, i++) {
        std::swap(*(r + j), *(r + i));
        std::swap(*(g + j), *(g + i));
        std::swap(*(b + j), *(b + i));
    }
}


void renderHGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float drx, dgx, dbx,
        xr = (float) from->red(),
        xg = (float) from->green(),
        xb = (float) from->blue();

    unsigned char* red = r;
    unsigned char* green = g;
    unsigned char* blue = b;
    register unsigned int x, y;

    drx = (float) (to->red() - from->red());
    dgx = (float) (to->green() - from->green());
    dbx = (float) (to->blue() - from->blue());

    drx /= width;
    dgx /= width;
    dbx /= width;

    if (interlaced && height > 2) {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (x = 0; x < width; x++, r++, g++, b++) {
            channel = (unsigned char) xr;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *r = channel2;

            channel = (unsigned char) xg;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *g = channel2;

            channel = (unsigned char) xb;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *b = channel2;


            channel = (unsigned char) xr;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(r + width) = channel2;

            channel = (unsigned char) xg;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(g + width) = channel2;

            channel = (unsigned char) xb;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(b + width) = channel2;

            xr += drx;
            xg += dgx;
            xb += dbx;
        }

        r += width;
        g += width;
        b += width;

        int offset;

        for (y = 2; y < height; y++, r += width, g += width, b += width) {
            if (y & 1) offset = width; else offset = 0;

            memcpy(r, (red + offset), width);
            memcpy(g, (green + offset), width);
            memcpy(b, (blue + offset), width);
        }
    } else {

        // normal hgradient
        for (x = 0; x < width; x++) {
            *(r++) = (unsigned char) (xr);
            *(g++) = (unsigned char) (xg);
            *(b++) = (unsigned char) (xb);

            xr += drx;
            xg += dgx;
            xb += dbx;
        }

        for (y = 1; y < height; y++, r += width, g += width, b += width) {
            memcpy(r, red, width);
            memcpy(g, green, width);
            memcpy(b, blue, width);
        }

    }

}

void renderVGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float dry, dgy, dby,
        yr = (float) from->red(),
        yg = (float) from->green(),
        yb = (float) from->blue();

    register unsigned int y;

    dry = (float) (to->red() - from->red());
    dgy = (float) (to->green() - from->green());
    dby = (float) (to->blue() - from->blue());

    dry /= height;
    dgy /= height;
    dby /= height;

    if (interlaced) {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (y = 0; y < height; y++, r += width, g += width, b += width) {
            if (y & 1) {
                channel = (unsigned char) yr;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(r, channel2, width);

                channel = (unsigned char) yg;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(g, channel2, width);

                channel = (unsigned char) yb;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(b, channel2, width);
            } else {
                channel = (unsigned char) yr;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(r, channel2, width);

                channel = (unsigned char) yg;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(g, channel2, width);

                channel = (unsigned char) yb;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(b, channel2, width);
            }

            yr += dry;
            yg += dgy;
            yb += dby;
        }
    } else {

        // normal vgradient
        for (y = 0; y < height; y++, r += width, g += width, b += width) {
            memset(r, (unsigned char) yr, width);
            memset(g, (unsigned char) yg, width);
            memset(b, (unsigned char) yb, width);

            yr += dry;
            yg += dgy;
            yb += dby;
        }
    }


}


// pyramid gradient -	based on original dgradient, written by
// Mosfet (mosfet@kde.org)
// adapted from kde sources for Blackbox by Brad Hughes
void renderPGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float yr, yg, yb, drx, dgx, dbx, dry, dgy, dby,
        xr, xg, xb;
    int rsign, gsign, bsign;
    unsigned int tr = to->red(), tg = to->green(), tb = to->blue();
    unsigned int* xtable;
    unsigned int* ytable;
    unsigned int* xt;
    unsigned int* yt;

    register unsigned int x, y;

    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    rsign = (drx < 0) ? -1 : 1;
    gsign = (dgx < 0) ? -1 : 1;
    bsign = (dbx < 0) ? -1 : 1;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient


    if (! interlaced) {

        // normal pgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
                *(g++) = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
                *(b++) = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }

}



// rectangle gradient -	based on original dgradient, written by
// Mosfet (mosfet@kde.org)
// adapted from kde sources for Blackbox by Brad Hughes
void renderRGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
    int rsign, gsign, bsign;
    unsigned int tr = to->red(), tg = to->green(), tb = to->blue();
    unsigned int* xtable;
    unsigned int* ytable;
    unsigned int* xt;
    unsigned int* yt;

    register unsigned int x, y;

    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    rsign = (drx < 0) ? -2 : 2;
    gsign = (dgx < 0) ? -2 : 2;
    bsign = (dbx < 0) ? -2 : 2;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient


    if (! interlaced) {

        // normal rgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
                *(g++) = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
                *(b++) = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }
}




// diagonal gradient code was written by Mike Cole <mike@mydot.com>
// modified for interlacing by Brad Hughes
void renderDGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
        xr = (float) from->red(),
        xg = (float) from->green(),
        xb = (float) from->blue();

    unsigned int w = width * 2;
    unsigned int h = height * 2;
    unsigned int* xtable;
    unsigned int* ytable;
    unsigned int* xt;
    unsigned int* yt;
    register unsigned int x, y;

    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    // Create X table
    drx /= w;
    dgx /= w;
    dbx /= w;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) (xr);
        *(xt++) = (unsigned char) (xg);
        *(xt++) = (unsigned char) (xb);

        xr += drx;
        xg += dgx;
        xb += dbx;
    }

    // Create Y table
    dry /= h;
    dgy /= h;
    dby /= h;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) yr);
        *(yt++) = ((unsigned char) yg);
        *(yt++) = ((unsigned char) yb);

        yr += dry;
        yg += dgy;
        yb += dby;
    }

    // Combine tables to create gradient
    if (! interlaced) {

        // normal dgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = *(xt++) + *(yt);
                *(g++) = *(xt++) + *(yt + 1);
                *(b++) = *(xt++) + *(yt + 2);
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = *(xt++) + *(yt);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = *(xt++) + *(yt);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }


}




// elliptic gradient -	based on original dgradient, written by
// Mosfet (mosfet@kde.org)
// adapted from kde sources for Blackbox by Brad Hughes
void renderEGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
    int rsign, gsign, bsign;
    unsigned int* xt;
    unsigned int* xtable;
    unsigned int* yt;
    unsigned int* ytable;
    unsigned int tr = (unsigned long) to->red(),
        tg = (unsigned long) to->green(),
        tb = (unsigned long) to->blue();
    register unsigned int x, y;


    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    rsign = (drx < 0) ? -1 : 1;
    gsign = (dgx < 0) ? -1 : 1;
    bsign = (dbx < 0) ? -1 : 1;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned long) (xr * xr);
        *(xt++) = (unsigned long) (xg * xg);
        *(xt++) = (unsigned long) (xb * xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = (unsigned long) (yr * yr);
        *(yt++) = (unsigned long) (yg * yg);
        *(yt++) = (unsigned long) (yb * yb);

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient
    if (! interlaced) {
        // normal egradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = (unsigned char)
                    (tr - (rsign * bsqrt(*(xt++) + *(yt))));
                *(g++) = (unsigned char)
                    (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
                *(b++) = (unsigned char)
                    (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char)
                        (tr - (rsign * bsqrt(*(xt++) + *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = (unsigned char)
                        (tr - (rsign * bsqrt(*(xt++) + *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }

}



// pipe cross gradient -	based on original dgradient, written by
// Mosfet (mosfet@kde.org)
// adapted from kde sources for Blackbox by Brad Hughes
void renderPCGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
    int rsign, gsign, bsign;
    unsigned int* xtable;
    unsigned int* ytable;
    unsigned int *xt;
    unsigned int *yt;
    unsigned int tr = to->red(),
        tg = to->green(),
        tb = to->blue();
    register unsigned int x, y;

    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    rsign = (drx < 0) ? -2 : 2;
    gsign = (dgx < 0) ? -2 : 2;
    bsign = (dbx < 0) ? -2 : 2;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient
    if (! interlaced) {

        // normal pcgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
                *(g++) = (unsigned char) (tg - (gsign * min(*(xt++), *(yt + 1))));
                *(b++) = (unsigned char) (tb - (bsign * min(*(xt++), *(yt + 2))));
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (bsign * min(*(xt++), *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (gsign * min(*(xt++), *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = (unsigned char) (tg - (gsign * min(*(xt++), *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = (unsigned char) (tb - (bsign * min(*(xt++), *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }
}


// cross diagonal gradient -	based on original dgradient, written by
// Mosfet (mosfet@kde.org)
// adapted from kde sources for Blackbox by Brad Hughes
void renderCDGradient(bool interlaced, 
        unsigned int width, unsigned int height,
        unsigned char* r, unsigned char* g, unsigned char* b,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

        float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
        xr = (float) from->red(),
        xg = (float) from->green(),
        xb = (float) from->blue();
    unsigned int w = width * 2, h = height * 2;
    unsigned int* xtable;
    unsigned int* ytable;
    unsigned int* xt;
    unsigned int* yt;

    register unsigned int x, y;

    imgctrl.getGradientBuffers(width * 3, height * 3, &xtable, &ytable);
    xt = xtable;
    yt = ytable;

    dry = drx = (float) (to->red() - from->red());
    dgy = dgx = (float) (to->green() - from->green());
    dby = dbx = (float) (to->blue() - from->blue());

    // Create X table
    drx /= w;
    dgx /= w;
    dbx /= w;

    for (xt = (xtable + (width * 3) - 1), x = 0; x < width; x++) {
        *(xt--) = (unsigned char) xb;
        *(xt--) = (unsigned char) xg;
        *(xt--) = (unsigned char) xr;

        xr += drx;
        xg += dgx;
        xb += dbx;
    }

    // Create Y table
    dry /= h;
    dgy /= h;
    dby /= h;

    for (yt = ytable, y = 0; y < height; y++) {
        *(yt++) = (unsigned char) yr;
        *(yt++) = (unsigned char) yg;
        *(yt++) = (unsigned char) yb;

        yr += dry;
        yg += dgy;
        yb += dby;
    }

    // Combine tables to create gradient

    if (! interlaced) {
        // normal cdgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(r++) = *(xt++) + *(yt);
                *(g++) = *(xt++) + *(yt + 1);
                *(b++) = *(xt++) + *(yt + 2);
            }
        }

    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = *(xt++) + *(yt);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(r++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(g++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(b++) = channel2;
                } else {
                    channel = *(xt++) + *(yt);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(r++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(g++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(b++) = channel2;
                }
            }
        }
    }
}



struct RendererActions {
    unsigned int type;
    void (*render)(bool, unsigned int, unsigned int, 
            unsigned char*, unsigned char*, unsigned char*,
            const FbTk::Color*, const FbTk::Color*,
            FbTk::ImageControl&);
};

const RendererActions render_gradient_actions[] = {
    { FbTk::Texture::DIAGONAL, renderDGradient },
    { FbTk::Texture::ELLIPTIC, renderEGradient },
    { FbTk::Texture::HORIZONTAL, renderHGradient },
    { FbTk::Texture::PYRAMID, renderPGradient },
    { FbTk::Texture::RECTANGLE, renderRGradient },
    { FbTk::Texture::VERTICAL, renderVGradient },
    { FbTk::Texture::CROSSDIAGONAL, renderCDGradient },
    { FbTk::Texture::PIPECROSS, renderPCGradient }
};

const RendererActions render_bevel_actions[] = {
    { FbTk::Texture::BEVEL1, renderBevel1 },
    { FbTk::Texture::BEVEL2, renderBevel2 }
};

}

namespace FbTk {

TextureRender::TextureRender(ImageControl &imgctrl,
                             unsigned int w, unsigned int h,
                             FbTk::Orientation orient):
    control(imgctrl),
    cpc(imgctrl.colorsPerChannel()),
    red(0), green(0), blue(0),
    orientation(orient),
    width(w),
    height(h) {

    Display* d = App::instance()->display();
    Screen* s = ScreenOfDisplay(d, imgctrl.screenNumber());

    unsigned int texture_max_width = WidthOfScreen(s) * 2;
    unsigned int texture_max_height = HeightOfScreen(s) * 2;

    _FB_USES_NLS;
    // clamp to "normal" size
    if (width > texture_max_width) {
        cerr<<"TextureRender: "<<_FBTK_CONSOLETEXT(Error, BigWidth, "Warning! Width > 3200 setting Width = 3200", "Image width seems too big, clamping")<<endl;
        width = texture_max_width;
    }

    if (height > texture_max_height) {
        cerr<<"TextureRender: "<<_FBTK_CONSOLETEXT(Error, BigHeight, "Warning! Height > 3200 setting Height = 3200", "Image height seems too big, clamping")<<endl;
        height = texture_max_height;
    }

    imgctrl.colorTables(&red_table, &green_table, &blue_table,
                        &red_offset, &green_offset, &blue_offset,
                        &red_bits, &green_bits, &blue_bits);

}


TextureRender::~TextureRender() {
    if (red != 0) delete [] red;
    if (green != 0) delete [] green;
    if (blue != 0) delete [] blue;
}


Pixmap TextureRender::render(const FbTk::Texture &texture) {

    if (width == 0 || height == 0)
        return None;
    else if (texture.pixmap().drawable() != 0)
        return renderPixmap(texture);
    else if (texture.type() & FbTk::Texture::PARENTRELATIVE)
        return ParentRelative;
    else if (texture.type() & FbTk::Texture::SOLID)
        return renderSolid(texture);
    else if (texture.type() & FbTk::Texture::GRADIENT) {
        allocateColorTables();
        return renderGradient(texture);
    }

    return None;
}

void TextureRender::allocateColorTables() {

    _FB_USES_NLS;

    const size_t size = width * height;

    red = FB_new_nothrow unsigned char[size];
    if (red == 0) {
        throw string("TextureRender::TextureRender(): ") +
              string(_FBTK_CONSOLETEXT(Error, OutOfMemoryRed, 
                      "Out of memory while allocating red buffer.", "")) +
              StringUtil::number2String(size);
    }


    green = FB_new_nothrow unsigned char[size];
    if (green == 0) {
        throw string("TextureRender::TextureRender(): ") +
              string(_FBTK_CONSOLETEXT(Error, OutOfMemoryGreen, 
                      "Out of memory while allocating green buffer.", "")) +
              StringUtil::number2String(size);
    }

    blue = FB_new_nothrow unsigned char[size];
    if (blue == 0) {
        throw string("TextureRender::TextureRender(): ") +
              string(_FBTK_CONSOLETEXT(Error, OutOfMemoryBlue, 
                          "Out of memory while allocating blue buffer.", "")) +
              StringUtil::number2String(size);
    }


}

Pixmap TextureRender::renderSolid(const FbTk::Texture &texture) {

    FbPixmap pixmap(RootWindow(FbTk::App::instance()->display(),
                               control.screenNumber()),
                    width, height,
                    control.depth());

    if (pixmap.drawable() == None) {
        _FB_USES_NLS;
        cerr<<"FbTk::TextureRender::render_solid(): "<<_FBTK_CONSOLETEXT(Error, CreatePixmap, "Error creating pixmap", "Couldn't create a pixmap - image - for some reason")<<endl;
        return None;
    }


    FbTk::GContext gc(pixmap),
        hgc(pixmap), lgc(pixmap);

    gc.setForeground(texture.color());
    gc.setFillStyle(FillSolid);

    hgc.setForeground(texture.hiColor());

    pixmap.fillRectangle(gc.gc(), 0, 0, width, height);

    if (texture.type() & Texture::INTERLACED) {
        lgc.setForeground(texture.colorTo());
        register unsigned int i = 0;
        for (; i < height; i += 2)
            pixmap.drawLine(lgc.gc(), 0, i, width, i);

    }

    lgc.setForeground(texture.loColor());

    if (texture.type() & Texture::BEVEL1) {
        if (texture.type() & Texture::RAISED) {
            drawBevelRectangle(pixmap, lgc.gc(), hgc.gc(), 0, height - 1, width -1 , 0);
        } else if (texture.type() & Texture::SUNKEN) {
            drawBevelRectangle(pixmap, hgc.gc(), lgc.gc(), 0, height - 1, width - 1, 0);
        }
    } else if (texture.type() & Texture::BEVEL2) {
        if (texture.type() & Texture::RAISED) {
            drawBevelRectangle(pixmap, lgc.gc(), hgc.gc(), 1, height - 3, width - 3, 1);
        } else if (texture.type() & Texture::SUNKEN) {
            drawBevelRectangle(pixmap, hgc.gc(), lgc.gc(), 1, height - 3, width - 3, 1);
        }
    }

    return pixmap.release();
}


Pixmap TextureRender::renderGradient(const FbTk::Texture &texture) {

    // invert our width and height if necessary
    translateSize(orientation, width, height);

    bool inverted = texture.type() & Texture::INVERT;
    const Color* from = &(texture.color());
    const Color* to = &(texture.colorTo());

    if (texture.type() & Texture::SUNKEN) {
        std::swap(from, to);
        inverted = !inverted;
    }

    size_t i;
    // draw gradient
    for (i = 0; i < sizeof(render_gradient_actions)/sizeof(RendererActions); ++i) {
        if (render_gradient_actions[i].type & texture.type()) {
            render_gradient_actions[i].render(texture.type() & Texture::INTERLACED,
                width, height, red, green, blue, from, to, control);
            break;
        }
    }

    // draw bevel
    for (i = 0; i < sizeof(render_bevel_actions)/sizeof(RendererActions); ++i) {
        if (render_bevel_actions[i].type & texture.type()) {
            render_bevel_actions[i].render(texture.type() & Texture::INTERLACED,
                width, height, red, green, blue, from, to, control);
            break;
        }
    }

    if (inverted)
        invertRGB(width, height, red, green, blue);

    return renderPixmap();

}

Pixmap TextureRender::renderPixmap(const FbTk::Texture &src_texture) {
    unsigned int tmpw = width, tmph = height;
    // we are given width and height in rotated form, we
    // unrotate it here to render it
    translateSize(orientation, tmpw, tmph);
    if (tmpw != src_texture.pixmap().width() ||
        tmph != src_texture.pixmap().height()) {

        // copy src_texture's pixmap and
        // scale/tile to fit our size
        FbPixmap new_pm(src_texture.pixmap());

        if ((src_texture.type() & Texture::TILED)) {
            new_pm.tile(tmpw,tmph);
        } else {
            new_pm.scale(tmpw, tmph);
        }
        new_pm.rotate(orientation);
        return new_pm.release();
    }
    // return copy of pixmap
    FbPixmap pm_copy = FbPixmap(src_texture.pixmap());
    pm_copy.rotate(orientation);

    return pm_copy.release();
}

XImage *TextureRender::renderXImage() {
    Display *disp = FbTk::App::instance()->display();
    XImage *image =
        XCreateImage(disp,
                     DefaultVisual(disp, control.screenNumber()), control.depth(), ZPixmap, 0, 0,
                     width, height, 32, 0);

    if (! image) {
        _FB_USES_NLS;
        cerr << "FbTk::TextureRender::renderXImage(): " << _FBTK_CONSOLETEXT(Error, CreateXImage, "Can't create XImage", "Couldn't create an XImage") << "." << endl;
        return 0;
    }

    image->data = 0;

    unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];
    register unsigned int x, y, r, g, b, o, offset;

    unsigned char *pixel_data = d, *ppixel_data = d;
    unsigned long pixel;

    o = image->bits_per_pixel + ((image->byte_order == MSBFirst) ? 1 : 0);

    switch (control.visual()->c_class) {
    case StaticColor:
    case PseudoColor:
        {
                int cpccpc = cpc * cpc;
            for (y = 0, offset = 0; y < height; y++) {
                for (x = 0; x < width; x++, offset++) {
                    r = red_table[red[offset]];
                    g = green_table[green[offset]];
                    b = blue_table[blue[offset]];

                    pixel = (r * cpccpc) + (g * cpc) + b;
                    *pixel_data++ = control.colors()[pixel].pixel;
                }

                pixel_data = (ppixel_data += image->bytes_per_line);
            }
        }
        break;

    case TrueColor:
        for (y = 0, offset = 0; y < height; y++) {
            for (x = 0; x < width; x++, offset++) {
                r = red_table[red[offset]];
                g = green_table[green[offset]];
                b = blue_table[blue[offset]];

                pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);

                switch (o) {
                case	8: //	8bpp
                    *pixel_data++ = pixel;
                    break;

                case 16: // 16bpp LSB
                    *pixel_data++ = pixel;
                    *pixel_data++ = pixel >> 8;
                    break;

                case 17: // 16bpp MSB
                    *pixel_data++ = pixel >> 8;
                    *pixel_data++ = pixel;
                    break;

                case 24: // 24bpp LSB
                    *pixel_data++ = pixel;
                    *pixel_data++ = pixel >> 8;
                    *pixel_data++ = pixel >> 16;
                    break;

                case 25: // 24bpp MSB
                    *pixel_data++ = pixel >> 16;
                    *pixel_data++ = pixel >> 8;
                    *pixel_data++ = pixel;
                    break;

                case 32: // 32bpp LSB
                    *pixel_data++ = pixel;
                    *pixel_data++ = pixel >> 8;
                    *pixel_data++ = pixel >> 16;
                    *pixel_data++ = pixel >> 24;
                    break;

                case 33: // 32bpp MSB
                    *pixel_data++ = pixel >> 24;
                    *pixel_data++ = pixel >> 16;
                    *pixel_data++ = pixel >> 8;
                    *pixel_data++ = pixel;
                    break;
                }
            }

            pixel_data = (ppixel_data += image->bytes_per_line);
        }

        break;

    case StaticGray:
    case GrayScale:
        for (y = 0, offset = 0; y < height; y++) {
            for (x = 0; x < width; x++, offset++) {
                r = *(red_table + *(red + offset));
                g = *(green_table + *(green + offset));
                b = *(blue_table + *(blue + offset));

                g = ((r * 30) + (g * 59) + (b * 11)) / 100;
                *pixel_data++ = control.colors()[g].pixel;
            }

            pixel_data = (ppixel_data += image->bytes_per_line);
        }

        break;

    default:
        _FB_USES_NLS;
        cerr << "TextureRender::renderXImage(): " <<
            _FBTK_CONSOLETEXT(Error, UnsupportedVisual, "Unsupported visual", "A visual is a technical term in X") << endl;
        delete [] d;
        XDestroyImage(image);
        return (XImage *) 0;
    }

    image->data = (char *) d;
    return image;
}


Pixmap TextureRender::renderPixmap() {
    Display *disp = FbTk::App::instance()->display();
    FbPixmap pixmap(RootWindow(disp, control.screenNumber()),
                    width, height, control.depth());

    if (pixmap.drawable() == None) {
        _FB_USES_NLS;
        cerr<<"FbTk::TextureRender::renderPixmap(): "<<_FBTK_CONSOLETEXT(Error, CreatePixmap, "Error creating pixmap", "Couldn't create a pixmap - image - for some reason")<<endl;
        return None;
    }

    XImage *image = renderXImage();

    if (! image) {
        return None;
    } else if (! image->data) {
        XDestroyImage(image);
        return None;
    }

    XPutImage(disp, pixmap.drawable(),
              DefaultGC(disp, control.screenNumber()),
              image, 0, 0, 0, 0, width, height);

    if (image->data != 0) {
        delete [] image->data;
        image->data = 0;
    }

    XDestroyImage(image);

    pixmap.rotate(orientation);

    return pixmap.release();
}


} // end namespace FbTk

