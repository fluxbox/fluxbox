// FbDrawable.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbDrawable.cc,v 1.2 2003/09/06 15:39:06 fluxgen Exp $

#include "FbDrawable.hh"

#include "App.hh"

namespace FbTk {

void FbDrawable::copyArea(Drawable src, GC gc,
                        int src_x, int src_y,
                        int dest_x, int dest_y,
                        unsigned int width, unsigned int height) {
    if (drawable() == 0 || src == 0 || gc == 0)
        return;
    XCopyArea(FbTk::App::instance()->display(),
              src, drawable(), gc,
              src_x, src_y,
              width, height,
              dest_x, dest_y);
}

void FbDrawable::fillRectangle(GC gc, int x, int y,
                             unsigned int width, unsigned int height) {
    if (drawable() == 0 || gc == 0)
        return;
    XFillRectangle(FbTk::App::instance()->display(),
                   drawable(), gc,
                   x, y,
                   width, height);
}

void FbDrawable::drawRectangle(GC gc, int x, int y, 
                             unsigned int width, unsigned int height) {
    if (drawable() == 0 || gc == 0)
        return;
    XDrawRectangle(FbTk::App::instance()->display(),
                   drawable(), gc,
                   x, y,
                   width, height);
}

void FbDrawable::drawLine(GC gc, int start_x, int start_y, 
                        int end_x, int end_y) {
    if (drawable() == 0 || gc == 0)
        return;
    XDrawLine(FbTk::App::instance()->display(),
              drawable(),
              gc,
              start_x, start_y,
              end_x, end_y);
}

void FbDrawable::fillPolygon(GC gc, XPoint *points, int npoints,
                           int shape, int mode) {
    if (drawable() == 0 || gc == 0 || points == 0 || npoints == 0)
        return;
    XFillPolygon(FbTk::App::instance()->display(),
                 drawable(), gc, points, npoints,
                 shape, mode);
}

void FbDrawable::drawPoint(GC gc, int x, int y) {
    if (drawable() == 0 || gc == 0)
        return;    
    XDrawPoint(FbTk::App::instance()->display(), drawable(), gc, x, y);
}

XImage *FbDrawable::image(int x, int y, unsigned int width, unsigned int height) const {
    return XGetImage(FbTk::App::instance()->display(), drawable(), 
                     x, y, width, height, 
                     AllPlanes, // plane mask
                     ZPixmap);
}

}; // end namespace FbTk
