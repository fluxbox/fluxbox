// FbPixmap.cc for FbTk - Fluxbox ToolKit
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

// $Id: FbPixmap.cc,v 1.1 2003/04/25 12:29:49 fluxgen Exp $

#include "FbPixmap.hh"
#include "App.hh"

namespace FbTk {

FbPixmap::FbPixmap():m_pm(0), 
                     m_width(0), m_height(0), 
                     m_depth(0) { }

FbPixmap::FbPixmap(const FbPixmap &the_copy):m_pm(0), 
                                         m_width(0), m_height(0), 
                                         m_depth(0) {
    copy(the_copy);
}

FbPixmap::FbPixmap(Drawable src, 
                   unsigned int width, unsigned int height,
                   int depth):m_pm(0), 
                              m_width(0), m_height(0), 
                              m_depth(0) {

    create(src, width, height, depth);
}

FbPixmap::~FbPixmap() {
    free();
}

void FbPixmap::copyArea(Drawable src, GC gc,
                        int src_x, int src_y,
                        int dest_x, int dest_y,
                        unsigned int width, unsigned int height) {
    if (m_pm == 0 || src == 0 || gc == 0)
        return;
    XCopyArea(FbTk::App::instance()->display(),
              src, m_pm, gc,
              src_x, src_y,
              dest_x, dest_y,
              width, height);
}

void FbPixmap::fillRectangle(GC gc, int x, int y,
                             unsigned int width, unsigned int height) {
    if (m_pm == 0 || gc == 0)
        return;
    XFillRectangle(FbTk::App::instance()->display(),
                   m_pm, gc,
                   x, y,
                   width, height);
}

void FbPixmap::drawRectangle(GC gc, int x, int y, 
                             unsigned int width, unsigned int height) {
    if (m_pm == 0 || gc == 0)
        return;
    XDrawRectangle(FbTk::App::instance()->display(),
                   m_pm, gc,
                   x, y,
                   width, height);
}

void FbPixmap::fillPolygon(GC gc, XPoint *points, int npoints,
                           int shape, int mode) {
    if (m_pm == 0 || gc == 0 || points == 0 || npoints == 0)
        return;
    XFillPolygon(FbTk::App::instance()->display(),
                 m_pm, gc, points, npoints,
                 shape, mode);
}

FbPixmap &FbPixmap::operator = (const FbPixmap &the_copy) {
    copy(the_copy);
    return *this;
}

void FbPixmap::copy(const FbPixmap &the_copy) {
    free();
    create(the_copy.drawable(), the_copy.width(), the_copy.height(), the_copy.depth());
}

void FbPixmap::free() {
    if (m_pm != 0) {
        XFreePixmap(FbTk::App::instance()->display(), m_pm);
        m_pm = 0;
    }
    m_width = 0;
    m_height = 0;
    m_depth = 0;
}

void FbPixmap::create(Drawable src, 
                      unsigned int width, unsigned int height, 
                      int depth) {
    if (src == 0)
        return;

    m_pm = XCreatePixmap(FbTk::App::instance()->display(),
                         src, width, height, depth);
    if (m_pm == 0)
        return;

    m_width = width;
    m_height = height;
    m_depth = depth;
}

}; // end namespace FbTk
