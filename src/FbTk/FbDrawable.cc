// FbDrawable.cc for FbTk - Fluxbox ToolKit
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

#include "FbDrawable.hh"

#include "App.hh"

namespace FbTk {

Display *FbDrawable::s_display = 0;

FbDrawable::FbDrawable() {

    if (s_display == 0) {
        s_display = FbTk::App::instance()->display();
    }
}

void FbDrawable::copyArea(Drawable src, GC gc,
                          int src_x, int src_y,
                          int dest_x, int dest_y,
                          unsigned int width, unsigned int height) {
    if (drawable() == 0 || src == 0 || gc == 0)
        return;
    XCopyArea(display(),
              src, drawable(), gc,
              src_x, src_y,
              width, height,
              dest_x, dest_y);
}

void FbDrawable::fillRectangle(GC gc, int x, int y,
                               unsigned int width, unsigned int height) {
    if (drawable() == 0 || gc == 0)
        return;
    XFillRectangle(display(),
                   drawable(), gc,
                   x, y,
                   width, height);
}

void FbDrawable::drawRectangle(GC gc, int x, int y,
                               unsigned int width, unsigned int height) {
    if (drawable() == 0 || gc == 0)
        return;
    XDrawRectangle(display(),
                   drawable(), gc,
                   x, y,
                   width, height);
}

void FbDrawable::drawLine(GC gc, int start_x, int start_y,
                          int end_x, int end_y) {
    if (drawable() == 0 || gc == 0)
        return;
    XDrawLine(display(),
              drawable(),
              gc,
              start_x, start_y,
              end_x, end_y);
}

void FbDrawable::fillPolygon(GC gc, XPoint *points, int npoints,
                             int shape, int mode) {
    if (drawable() == 0 || gc == 0 || points == 0 || npoints == 0)
        return;
    XFillPolygon(display(),
                 drawable(), gc, points, npoints,
                 shape, mode);
}

// x, y, width and height define a space within which we're drawing a triangle (centred)
// scale defines number of triangles that'd fit in a space of 100 width x 100 height
// (i.e. 200 = half size, 300 = a third). Its a bit backwards but it allows more flexibility
void FbDrawable::drawTriangle(GC gc, FbDrawable::TriangleType type,
                              int x, int y, unsigned int width, unsigned int height,
                              int scale) {
    if (drawable() == 0 || gc == 0 || width == 0 || height == 0)
        return;

    XPoint pts[3];

    if (scale < 100) scale = 100; // not bigger than the space allowed
    else if (scale > 10000) scale = 10000; // not too small...

    int arrowscale_n = scale;
    int arrowscale_d = 100;
    unsigned int ax = arrowscale_d * width / arrowscale_n;
    unsigned int ay = arrowscale_d * height / arrowscale_n;
    // if these aren't an even number, left and right arrows end up different
    if (type == FbTk::FbDrawable::LEFT ||
        type == FbTk::FbDrawable::RIGHT) {
        if (( ax % 2 ) == 1) ax--;
        if (( ay % 2 ) == 1) ay--;
    } else {
        if (( ax % 2 ) == 0) ax--;
    }

    switch (type) {
    case FbTk::FbDrawable::LEFT:
        // start at the tip
        pts[0].x = (width / 2) - (ax / 2); pts[0].y = height / 2;
        pts[1].x = ax; pts[1].y = -ay / 2;
        pts[2].x = 0; pts[2].y = ay;
        break;
    case FbTk::FbDrawable::RIGHT:
        pts[0].x = (width / 2) + (ax / 2); pts[0].y = height / 2;
        pts[1].x = - ax; pts[1].y = ay / 2;
        pts[2].x = 0; pts[2].y = - ay;
        break;
    case FbTk::FbDrawable::UP:
        pts[0].x = (width / 2); pts[0].y = (height / 2) - (ay / 2)-1;
        pts[1].x = ax / 2; pts[1].y = ay+1;
        pts[2].x = - ax; pts[2].y = 0;
        break;
    case FbTk::FbDrawable::DOWN:
        /* I tried and tried, but couldn't get the left diagonal of the down
           arrow to be symmetrical with the right (for small widths)!
           So we opt for this setup. It is symmetrical with larger widths */
        pts[0].x = (width / 2) ; pts[0].y = (height / 2) + (ay / 2);
        pts[1].x = -ax/2+1; pts[1].y = -ay;
        pts[2].x = ax-1; pts[2].y = 0;
        break;

    }

    // re-centre on the specified points
    pts[0].x += x;
    pts[0].y += y;

    fillPolygon(gc,
                pts, 3,
                Convex, CoordModePrevious);
}

XImage *FbDrawable::image(int x, int y, unsigned int width, unsigned int height) const {
    return XGetImage(display(), drawable(),
                     x, y, width, height,
                     AllPlanes, // plane mask
                     ZPixmap);
}

} // end namespace FbTk
