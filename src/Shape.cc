// Shape.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "Shape.hh"

#include "FbTk/FbWindow.hh"
#include "FbTk/App.hh"
#include "FbTk/GContext.hh"
#include "FbTk/FbPixmap.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <algorithm>

using std::min;

// ignore_border basically means do clip shape instead of bounding shape
FbTk::FbPixmap *Shape::createShape(bool ignore_border) {
    if (m_win->window() == 0 || m_shapeplaces == 0 ||
        m_win->width() < 3 || m_win->height() < 3)
        return 0;

    static char left_bits[] = { 0xc0, 0xf8, 0xfc, 0xfe, 0xfe, 0xfe, 0xff, 0xff };
    static char right_bits[] = { 0x03, 0x1f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0xff};
    static char bottom_left_bits[] = { 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xf8, 0xc0 };
    static char bottom_right_bits[] = { 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x1f, 0x03 };

    const int borderw = m_win->borderWidth();
    const int win_width = m_win->width() + (ignore_border?0:2*borderw);
    const int win_height = m_win->height() + (ignore_border?0:2*borderw);
    const int pixmap_width = min(8, win_width);
    const int pixmap_height = min(8, win_height);

    Display *disp = FbTk::App::instance()->display();
    const size_t data_size = win_width * win_height;
    // we use calloc here so we get consistent C alloc/free with XDestroyImage
    // and no warnings in valgrind :)
    char *data = (char *)calloc(data_size, sizeof (char));
    if (data == 0)
        return 0;

    memset(data, 0xFF, data_size);

    XImage *ximage = XCreateImage(disp,
                                  DefaultVisual(disp, m_win->screenNumber()),
                                  1,
                                  XYPixmap, 0,
                                  data,
                                  win_width, win_height,
                                  32, 0);
    if (ximage == 0)
        return 0;

    XInitImage(ximage);

    // shape corners

    if (m_shapeplaces & Shape::TOPLEFT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x, y, (left_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    if (m_shapeplaces & Shape::TOPRIGHT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + win_width - pixmap_width, y,
                          (right_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    if (m_shapeplaces & Shape::BOTTOMLEFT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x, y + win_height - pixmap_height,
                          (bottom_left_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    if (m_shapeplaces & Shape::BOTTOMRIGHT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + win_width - pixmap_width, y + win_height - pixmap_height,
                          (bottom_right_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    FbTk::FbPixmap *pm = new FbTk::FbPixmap(*m_win, win_width, win_height, 1);


    FbTk::GContext gc(*pm);

    XPutImage(disp, pm->drawable(), gc.gc(), ximage, 0, 0, 0, 0,
              win_width, win_height);

    XDestroyImage(ximage);

    return pm;

}

Shape::Shape(FbTk::FbWindow &win, int shapeplaces):
    m_win(&win),
    m_shapeplaces(shapeplaces) {

    m_clipshape.reset(createShape(true));
    m_boundingshape.reset(createShape(false));
}

Shape::~Shape() {

#ifdef SHAPE
    if (m_win != 0 && m_win->window()) {
        // Reset shape of window
        XShapeCombineMask(FbTk::App::instance()->display(),
                          m_win->window(),
                          ShapeClip,
                          0, 0,
                          0,
                          ShapeSet);
        // Reset shape of window
        if (m_boundingshape.get()) {
            XShapeCombineMask(FbTk::App::instance()->display(),
                              m_win->window(),
                              ShapeBounding,
                              0, 0,
                              0,
                              ShapeSet);
        }
            }
#endif // SHAPE
}

void Shape::setPlaces(int shapeplaces) {
    m_shapeplaces = shapeplaces;
}

void Shape::update() {
    if (m_win == 0 || m_win->window() == 0)
        return;
#ifdef SHAPE
    if (m_clipshape.get() == 0 ||
        m_win->width() != clipWidth() ||
        m_win->height() != clipHeight()) {
        m_clipshape.reset(createShape(true));
    }
    if (m_boundingshape.get() == 0 ||
        (m_win->width()+m_win->borderWidth()*2) != width() ||
        (m_win->height()+m_win->borderWidth()*2) != height()) {
        if (m_win->borderWidth() != 0)
            m_boundingshape.reset(createShape(false));
        else 
            m_boundingshape.reset(0);

    }

    // the m_shape can be = 0 which will just raeset the shape mask
    // and make the window normal
    XShapeCombineMask(FbTk::App::instance()->display(),
                      m_win->window(),
                      ShapeClip,
                      0, 0,
                      (m_clipshape.get() != 0)? m_clipshape->drawable() : 0,
                      ShapeSet);

    // the m_shape can be = 0 which will just raeset the shape mask
    // and make the window normal
    XShapeCombineMask(FbTk::App::instance()->display(),
                      m_win->window(),
                      ShapeBounding,
                      -m_win->borderWidth(), -m_win->borderWidth(),
                      (m_boundingshape.get() != 0)? m_boundingshape->drawable() :
                      ((m_clipshape.get() != 0)? m_clipshape->drawable(): 0),
                      ShapeSet);


#endif // SHAPE

}

void Shape::setWindow(FbTk::FbWindow &win) {
    m_win = &win;
    update();
}

void Shape::setShapeNotify(const FbTk::FbWindow &win) {
#ifdef SHAPE
    XShapeSelectInput(FbTk::App::instance()->display(),
                      win.window(), ShapeNotifyMask);
#endif // SHAPE
}

bool Shape::isShaped(const FbTk::FbWindow &win) {
    int shaped = 0;

#ifdef SHAPE
    int not_used;
    unsigned int not_used2;
    XShapeQueryExtents(FbTk::App::instance()->display(),
                       win.window(),
                       &shaped,  /// bShaped
                       &not_used, &not_used,  // xbs, ybs
                       &not_used2, &not_used2, // wbs, hbs
                       &not_used, // cShaped
                       &not_used, &not_used, // xcs, ycs
                       &not_used2, &not_used2); // wcs, hcs
#endif // SHAPE

    return (shaped != 0 ? true : false);
}

unsigned int Shape::width() const {
    return m_boundingshape.get() ? m_boundingshape->width() : 0;
}

unsigned int Shape::height() const {
    return m_boundingshape.get() ? m_boundingshape->height() : 0;
}

unsigned int Shape::clipWidth() const {
    return m_clipshape.get() ? m_clipshape->width() : 0;
}

unsigned int Shape::clipHeight() const {
    return m_clipshape.get() ? m_clipshape->height() : 0;
}
