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
#include "ColorLUT.hh"

#include <X11/Xutil.h>
#include <iostream>

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
using FbTk::ColorLUT::PRE_MULTIPLY_1_5;
using FbTk::ColorLUT::PRE_MULTIPLY_1_125;
using FbTk::ColorLUT::PRE_MULTIPLY_0_75;

namespace FbTk {

struct RGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a; // align RGBA to 32bit, it's of no use (yet)

    // use of 'static void function()' here to have
    // simple function-pointers for interlace-code
    // (and avoid *this 'overhead')

    // 1.5 of current value, clamp to ~0 (0xff)
    static void brighten_1_5(RGBA& color) {
        color.r = PRE_MULTIPLY_1_5[color.r];
        color.g = PRE_MULTIPLY_1_5[color.g];
        color.b = PRE_MULTIPLY_1_5[color.b];
    }


    // 1.125 of current value
    static void brighten_1_125(RGBA& color) {
        color.r = PRE_MULTIPLY_1_125[color.r];
        color.g = PRE_MULTIPLY_1_125[color.g];
        color.b = PRE_MULTIPLY_1_125[color.b];
    }

    // 0.75 of old value
    static void darken(RGBA& color) {
        color.r = PRE_MULTIPLY_0_75[color.r];
        color.g = PRE_MULTIPLY_0_75[color.g];
        color.b = PRE_MULTIPLY_0_75[color.b];
    }


    typedef void (*colorFunc)(RGBA&);
    static const colorFunc pseudoInterlaceFuncs[2];
};

const RGBA::colorFunc RGBA::pseudoInterlaceFuncs[2] = {
    RGBA::brighten_1_125,
    RGBA::darken
};

}

namespace {

struct Vec2 {
    int x;
    int y;

    // positive: 'other' is clockwise of this
    // negative: 'other' is counterclockwise of this
    // 0: same line
    int cross(int other_x, int other_y) const {
        return (x * other_y) - (other_x * y);
    }
};

std::vector<char>& getGradientBuffer(size_t size) {
    static std::vector<char> buffer;
    if (buffer.size() < size)
        buffer.resize(size);
    return buffer;
}


void invertRGB(unsigned int w, unsigned int h, FbTk::RGBA* rgba) {

    FbTk::RGBA* l = rgba;
    FbTk::RGBA* r = rgba + (w * h);

    for (--r; l < r; ++l, --r) { // swapping 32bits (RGBA) at ones.
        std::swap(*((unsigned int*)r), *(unsigned int*)r);
    }
}


void mirrorRGB(unsigned int w, unsigned int h, FbTk::RGBA* rgba) {

    FbTk::RGBA* l = rgba;
    FbTk::RGBA* r = rgba + (w * h);

    for (--r; l < r; ++l, --r) {
        *(unsigned int*)r = *(unsigned int*)l;
    }
}



typedef void (*prepareFunc)(size_t, FbTk::RGBA*, const FbTk::Color*, const FbTk::Color*, double);

//
//
//   To   +          .   From +.
//        |        .          |  .
//        |      .            |    .
//        |    .              |      .
//        |  .                |        .
//        |.                  |          .
//   From +-----------+  To   +-----------+
//        0         size      0         size
//

void prepareLinearTable(size_t size, FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to, double scale) {

    const double delta_r = (double)(to->red() - from->red()) / (double)size;
    const double delta_g = (double)(to->green() - from->green()) / (double)size;
    const double delta_b = (double)(to->blue() - from->blue()) / (double)size;

    double r = from->red();
    double g = from->green();
    double b = from->blue();

    size_t i;
    for (i = 0; i < size; ++i) {
        rgba[i].r = static_cast<unsigned char>(scale * (r + (i * delta_r)));
        rgba[i].g = static_cast<unsigned char>(scale * (g + (i * delta_g)));
        rgba[i].b = static_cast<unsigned char>(scale * (b + (i * delta_b)));
    }
}

void prepareSquareTable(size_t size, FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to, double scale) {

    const double delta_r = (double)(to->red() - from->red());
    const double delta_g = (double)(to->green() - from->green());
    const double delta_b = (double)(to->blue() - from->blue());

    double r = from->red();
    double g = from->green();
    double b = from->blue();

    double s;
    size_t i;
    for (i = 0; i < size; ++i) {
        s = 1.0 - ((double)(i + 1) / (double)size);
        s *=s;
        rgba[i].r = static_cast<unsigned char>(scale * (r + (s * delta_r)));
        rgba[i].g = static_cast<unsigned char>(scale * (g + (s * delta_g)));
        rgba[i].b = static_cast<unsigned char>(scale * (b + (s * delta_b)));
    }
}

//
//
//   To   +     .         From +           .
//        |    . .             |.         .
//        |   .   .            | .       .
//        |  .     .           |  .     .
//        | .       .          |   .   .
//        |.         .         |    . .
//   From +-----------+   To   +-----.-----+
//        0         size       0         size
//
void prepareMirrorTable(prepareFunc prepare, size_t size, FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to, double scale) {

    // for simplicity we just use given 'prepare' func to
    // prepare 2 parts of the 'mirrorTable'
    //
    // 2 cases: odd and even number of 'size'
    //
    //   even:   f..tt..f   (size == 8)
    //   odd:    f..t..f    (size == 7)
    //
    // for 'odd' we habe to 'overwrite' the last value of the left half
    // with the (same) value for 't' again
    //
    //
    // half_size for even: 4
    // half_size for odd: 4
    //
    size_t half_size = (size >> 1) + (size & 1);

    prepare(half_size, &rgba[0], from, to, scale);
    mirrorRGB(size, 1, rgba); // TODO!!
}


inline void pseudoInterlace(FbTk::RGBA& rgba, const size_t& y) {
    FbTk::RGBA::pseudoInterlaceFuncs[y & 1](rgba);
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





/*

    bbbbbbbbbbbbbbbbb
    b               d           b - brighter
    b               d           d - darker
    b               d           D - 2 times dark
    xdddddddddddddddD           x - darker(brighter())

 */

void renderBevel1(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba, const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    if (! (width > 2 && height > 2))
        return;

    const size_t s = width * height;
    size_t i;

    // brighten top line and first pixel of the
    // 2nd line
    for (i = 0; i < width + 1; ++i) {
        FbTk::RGBA::brighten_1_5(rgba[i]);
    }

    // bright and darken left and right border
    for (i = 2 * width - 1; i < s - width; i += width) {
        FbTk::RGBA::darken(rgba[i]); // right border
        FbTk::RGBA::brighten_1_5(rgba[i + 1]);  // left border on the next line
    }

    // darken bottom line, except the first pixel
    for (i = s - width + 1; i < s; ++i) {
        FbTk::RGBA::darken(rgba[i]);
    }

    // and darken the lower corner pixels again
    FbTk::RGBA::darken(rgba[i - 1]);
    FbTk::RGBA::darken(rgba[i - width]);
}


/*
     ...................
     .bbbbbbbbbbbbbbbbd.
     .b...............d.
     .b...............d.    b - brighter
     .b...............d.    d - darker
     .bdddddddddddddddd.
     ...................

   */
void renderBevel2(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    if (! (width > 4 && height > 4))
        return;

    const size_t s = width * height;
    size_t i;

    // top line, but stop 2 pixels before right border
    for (i = (width + 1); i < ((2 * width) - 2); i++) {
        FbTk::RGBA::brighten_1_5(rgba[i]);
    }

    // first darken the right border, then brighten the
    // left border
    for ( ; i < (s - (2 * width) - 1); i += width) {
        FbTk::RGBA::darken(rgba[i]);
        FbTk::RGBA::brighten_1_5(rgba[i + 3]);
    }

    // bottom line
    for (i = (s - (2 * width)) + 2; i < ((s - width) - 1); ++i) {
        FbTk::RGBA::darken(rgba[i]);
    }
}




void renderHorizontalGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    FbTk::RGBA* gradient = (FbTk::RGBA*)&getGradientBuffer(width * sizeof(FbTk::RGBA))[0];
    prepareLinearTable(width, gradient, from, to, 1.0);

    size_t y;
    size_t x;
    size_t i;

    for (i = 0, y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++i) {

            rgba[i] = gradient[x];

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}

void renderVerticalGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    FbTk::RGBA* gradient = (FbTk::RGBA*)&getGradientBuffer(height * sizeof(FbTk::RGBA))[0];
    prepareLinearTable(height, gradient, from, to, 1.0);

    size_t y;
    size_t x;
    size_t i;

    for (i = 0, y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++i) {
            rgba[i] = gradient[y];

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}


void renderPyramidGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {


    const size_t s = width * height;

    // we need 2 gradients but use only 'one' buffer
    FbTk::RGBA* x_gradient = (FbTk::RGBA*)&getGradientBuffer(s * sizeof(FbTk::RGBA))[0];
    FbTk::RGBA* y_gradient = &x_gradient[width];

    prepareMirrorTable(prepareLinearTable, width, x_gradient, from, to, 0.5);
    prepareMirrorTable(prepareLinearTable, height, y_gradient, from, to, 0.5);

    size_t x;
    size_t y;
    size_t i;

    for (i = 0, y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++i) {

            rgba[i].r = x_gradient[x].r + y_gradient[y].r;
            rgba[i].g = x_gradient[x].g + y_gradient[y].g;
            rgba[i].b = x_gradient[x].b + y_gradient[y].b;

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}


/*
 *  .................
 *    .............
 *      .........
 *        ....          '.' - x_gradient
 *          .           ' ' - y_gradient
 *        ....
 *      .........
 *    .............
 *  .................
 */
void renderRectangleGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    const size_t s = width * height;

    // we need 2 gradients but use only 'one' buffer
    FbTk::RGBA* x_gradient = (FbTk::RGBA*)&getGradientBuffer(s * sizeof(FbTk::RGBA))[0];
    FbTk::RGBA* y_gradient = &x_gradient[width];

    prepareMirrorTable(prepareLinearTable, width, x_gradient, from, to, 1.0);
    prepareMirrorTable(prepareLinearTable, height, y_gradient, from, to, 1.0);

    // diagonal vectors
    const Vec2 a = { static_cast<int>(width) - 1,  static_cast<int>(height - 1) };
    const Vec2 b = { a.x, -a.y };

    int x;
    int y;
    size_t i;

    for (i = 0, y = 0; y < static_cast<int>(height); ++y) {
        for (x = 0; x < static_cast<int>(width); ++x, ++i) {

            // check, if the point (x, y) is left or right of the vectors
            // 'a' and 'b'. if the point is on the same side for both 'a' and
            // 'b' (a.cross() is equal to b.cross()) then use the y_gradient,
            // otherwise use y_gradient

            if ((a.cross(x, y) * b.cross(x, b.y + y)) < 0) {
                rgba[i] = x_gradient[x];
            } else {
                rgba[i] = y_gradient[y];
            }

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}

void renderPipeCrossGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    size_t s = width * height;

    // we need 2 gradients but use only 'one' buffer
    FbTk::RGBA* x_gradient = (FbTk::RGBA*)&getGradientBuffer(s * sizeof(FbTk::RGBA))[0];
    FbTk::RGBA* y_gradient = x_gradient + width;

    prepareMirrorTable(prepareLinearTable, width, x_gradient, from, to, 1.0);
    prepareMirrorTable(prepareLinearTable, height, y_gradient, from, to, 1.0);

    // diagonal vectors
    const Vec2 a = { static_cast<int>(width) - 1,  static_cast<int>(height - 1) };
    const Vec2 b = { a.x, -a.y };

    int x;
    int y;
    size_t i;

    for (i = 0, y = 0; y < static_cast<int>(height); ++y) {
        for (x = 0; x < static_cast<int>(width); ++x, ++i) {

            // check, if the point (x, y) is left or right of the vectors
            // 'a' and 'b'. if the point is on the same side for both 'a' and
            // 'b' (a.cross() is equal to b.cross()) then use the x_gradient,
            // otherwise use y_gradient

            if ((a.cross(x, y) * b.cross(x, b.y + y)) > 0) {
                rgba[i] = x_gradient[x];
            } else {
                rgba[i] = y_gradient[y];
            }

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}





void renderDiagonalGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {


    size_t s = width * height;

    // we need 2 gradients but use only 'one' buffer
    FbTk::RGBA* x_gradient = (FbTk::RGBA*)&getGradientBuffer(s * sizeof(FbTk::RGBA))[0];
    FbTk::RGBA* y_gradient = x_gradient + width;

    prepareLinearTable(width, x_gradient, from, to, 0.5);
    prepareLinearTable(height, y_gradient, from, to, 0.5);

    size_t x;
    size_t y;
    size_t i;

    for (i = 0, y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++i) {

            rgba[i].r = x_gradient[x].r + y_gradient[y].r;
            rgba[i].g = x_gradient[x].g + y_gradient[y].g;
            rgba[i].b = x_gradient[x].b + y_gradient[y].b;

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}




void renderEllipticGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    size_t s = width * height;

    size_t i;
    int x;
    int y;

    double dr = (double)to->red()  - (double)from->red()   ;
    double dg = (double)to->green()- (double)from->green() ;
    double db = (double)to->blue() - (double)from->blue()  ;

    int w2 = width/2;
    int h2 = height/2;

    double d;
    double r = to->red();
    double g = to->green();
    double b = to->blue();


    for (i = 0, y = -h2; y < h2; ++y) {
        for (x = -w2; x < w2; ++x, ++i) {

            d = ((double)(x*x) / (double)(w2*w2))
                + ((double)(y*y) / (double)(h2*h2));

            d *= 0.5;

            rgba[i].r = (unsigned char)(r - (d * dr));
            rgba[i].g = (unsigned char)(g - (d * dg));
            rgba[i].b = (unsigned char)(b - (d * db));

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}




void renderCrossDiagonalGradient(bool interlaced,
        unsigned int width, unsigned int height,
        FbTk::RGBA* rgba,
        const FbTk::Color* from, const FbTk::Color* to,
        FbTk::ImageControl& imgctrl) {

    size_t s = width * height;

    // we need 2 gradients but use only 'one' buffer
    FbTk::RGBA* x_gradient = (FbTk::RGBA*)&getGradientBuffer(s * sizeof(FbTk::RGBA))[0];
    FbTk::RGBA* y_gradient = x_gradient + width;

    prepareLinearTable(width, x_gradient, to, from, 0.5);
    prepareLinearTable(height, y_gradient, from, to, 0.5);

    size_t x;
    size_t y;
    size_t i;

    for (i = 0, y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++i) {

            rgba[i].r = x_gradient[x].r + y_gradient[y].r;
            rgba[i].g = x_gradient[x].g + y_gradient[y].g;
            rgba[i].b = x_gradient[x].b + y_gradient[y].b;

            if (interlaced)
                pseudoInterlace(rgba[i], y);
        }
    }
}



struct RendererActions {
    unsigned int type;
    void (*render)(bool, unsigned int, unsigned int,
            FbTk::RGBA*,
            const FbTk::Color*, const FbTk::Color*,
            FbTk::ImageControl&);
};

const RendererActions render_gradient_actions[] = {
    { FbTk::Texture::DIAGONAL, renderDiagonalGradient},
    { FbTk::Texture::ELLIPTIC, renderEllipticGradient },
    { FbTk::Texture::HORIZONTAL, renderHorizontalGradient },
    { FbTk::Texture::PYRAMID, renderPyramidGradient },
    { FbTk::Texture::RECTANGLE, renderRectangleGradient },
    { FbTk::Texture::VERTICAL, renderVerticalGradient },
    { FbTk::Texture::CROSSDIAGONAL, renderCrossDiagonalGradient },
    { FbTk::Texture::PIPECROSS, renderPipeCrossGradient }
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
    cpccpc(cpc * cpc),
    rgba(0),
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
        cerr<<"TextureRender: "<<_FBTK_CONSOLETEXT(Error, BigWidth, 
                "Warning! Width > 3200 setting Width = 3200", "Image width seems too big, clamping")
            <<endl;
        width = texture_max_width;
    }

    if (height > texture_max_height) {
        cerr<<"TextureRender: "<<_FBTK_CONSOLETEXT(Error, BigHeight,
                "Warning! Height > 3200 setting Height = 3200", "Image height seems too big, clamping")
            <<endl;
        height = texture_max_height;
    }
}


TextureRender::~TextureRender() {
    if (rgba)
        delete[] rgba;
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

    const size_t s = width * height;
    rgba = FB_new_nothrow RGBA[s];
    if (rgba == 0) {

        _FB_USES_NLS;
        throw string("TextureRender::TextureRender(): ") +
              string(_FBTK_CONSOLETEXT(Error, OutOfMemoryRed,
                      "Out of memory while allocating red buffer.", "")) +
              StringUtil::number2String(s);
    }
}

Pixmap TextureRender::renderSolid(const FbTk::Texture &texture) {

    FbPixmap pixmap(RootWindow(FbTk::App::instance()->display(),
                               control.screenNumber()),
                    width, height,
                    control.depth());

    if (pixmap.drawable() == None) {
        _FB_USES_NLS;
        cerr << "FbTk::TextureRender::render_solid(): "
            <<_FBTK_CONSOLETEXT(Error, CreatePixmap, "Error creating pixmap", "Couldn't create a pixmap - image - for some reason")
            << endl;
        return None;
    }


    FbTk::GContext gc(pixmap);
    FbTk::GContext hgc(pixmap);
    FbTk::GContext lgc(pixmap);

    gc.setForeground(texture.color());
    gc.setFillStyle(FillSolid);

    hgc.setForeground(texture.hiColor());

    pixmap.fillRectangle(gc.gc(), 0, 0, width, height);

    if (texture.type() & Texture::INTERLACED) {
        lgc.setForeground(texture.colorTo());
        register unsigned int i = 0;
        for (; i < height; i += 2)
            pixmap.drawLine(lgc.gc(), 0, i, width - 1, i);

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

    const Color* from = &(texture.color());
    const Color* to = &(texture.colorTo());

    bool interlaced = texture.type() & Texture::INTERLACED;
    bool inverted = texture.type() & Texture::INVERT;

    if (texture.type() & Texture::SUNKEN) {
        std::swap(from, to);
        inverted = !inverted;
    }

    size_t i;
    // draw gradient
    for (i = 0; i < sizeof(render_gradient_actions)/sizeof(RendererActions); ++i) {
        if (render_gradient_actions[i].type & texture.type()) {
            render_gradient_actions[i].render(interlaced, width, height, rgba, from, to, control);
            break;
        }
    }

    // draw bevel
    for (i = 0; i < sizeof(render_bevel_actions)/sizeof(RendererActions); ++i) {
        if (texture.type() & render_bevel_actions[i].type) {
            render_bevel_actions[i].render(interlaced, width, height, rgba, from, to, control);
            break;
        }
    }

    if (inverted)
        invertRGB(width, height, rgba);

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
                     control.visual(), control.depth(), ZPixmap, 0, 0,
                     width, height, 32, 0);

    if (! image) {
        _FB_USES_NLS;
        cerr << "FbTk::TextureRender::renderXImage(): "
            << _FBTK_CONSOLETEXT(Error, CreateXImage, "Can't create XImage", "Couldn't create an XImage")
            << "." << endl;
        return 0;
    }

    image->data = 0;


    const unsigned char *red_table;
    const unsigned char *green_table;
    const unsigned char *blue_table;
    int red_offset;
    int green_offset;
    int blue_offset;

    control.colorTables(&red_table, &green_table, &blue_table,
                        &red_offset, &green_offset, &blue_offset,
                        0, 0, 0);

    unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];
    register unsigned int x, y, r, g, b, o, offset;

    unsigned char *pixel_data = d, *ppixel_data = d;
    unsigned long pixel;

    o = image->bits_per_pixel + ((image->byte_order == MSBFirst) ? 1 : 0);

    switch (control.visual()->c_class) {
    case StaticColor:
    case PseudoColor:
        for (y = 0, offset = 0; y < height; y++) {
            for (x = 0; x < width; x++, offset++) {
                r = red_table[rgba[offset].r];
                g = green_table[rgba[offset].g];
                b = blue_table[rgba[offset].b];

                pixel_data = (ppixel_data += image->bytes_per_line);
            }
        }
        break;

    case TrueColor:
        for (y = 0, offset = 0; y < height; y++) {
            for (x = 0; x < width; x++, offset++) {
                r = red_table[rgba[offset].r];
                g = green_table[rgba[offset].g];
                b = blue_table[rgba[offset].b];

                pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);

                switch (o) {
                case 8: //	8bpp
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
                r = *(red_table + rgba[offset].r);
                g = *(green_table + rgba[offset].g);
                b = *(blue_table + rgba[offset].b);

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
        cerr << "FbTk::TextureRender::renderPixmap(): "
            << _FBTK_CONSOLETEXT(Error, CreatePixmap, "Error creating pixmap", "Couldn't create a pixmap - image - for some reason") << endl;
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

