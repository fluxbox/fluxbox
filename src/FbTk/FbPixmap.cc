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

// $Id: FbPixmap.cc,v 1.10 2004/01/11 12:48:46 fluxgen Exp $

#include "FbPixmap.hh"
#include "App.hh"
#include "GContext.hh"

#include <X11/Xutil.h>
#include <iostream>
using namespace std;

namespace FbTk {

FbPixmap::FbPixmap():m_pm(0), 
                     m_width(0), m_height(0), 
                     m_depth(0) { }

FbPixmap::FbPixmap(const FbPixmap &the_copy):m_pm(0), 
                                             m_width(0), m_height(0), 
                                             m_depth(0) {
    copy(the_copy);
}

FbPixmap::FbPixmap(Pixmap pm):m_pm(0), 
                              m_width(0), m_height(0),
                              m_depth(0) {
    if (pm == 0)
        return;
    // assign X pixmap to this
    (*this) = pm;
}

FbPixmap::FbPixmap(const FbDrawable &src, 
                   unsigned int width, unsigned int height,
                   int depth):m_pm(0), 
                              m_width(0), m_height(0), 
                              m_depth(0) {

    create(src.drawable(), width, height, depth);
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

FbPixmap &FbPixmap::operator = (const FbPixmap &the_copy) {
    copy(the_copy);
    return *this;
}

FbPixmap &FbPixmap::operator = (Pixmap pm) {
    // free pixmap before we set new
    free();

    if (pm == 0)
        return *this;

    // get width, height and depth for the pixmap
    Window root;
    int x, y;
    unsigned int border_width, bpp;    
    XGetGeometry(FbTk::App::instance()->display(),
                 pm,
                 &root,
                 &x, &y,
                 &m_width, &m_height,
                 &border_width,
                 &bpp);

    m_depth = bpp;

    m_pm = pm;

    return *this;
}

void FbPixmap::copy(const FbPixmap &the_copy) {

    bool create_new = false;

    if (the_copy.width() != width() ||
        the_copy.height() != height() ||
        the_copy.depth() != depth() ||
        drawable() == 0)
        create_new = true;
    
    if (create_new)    
        free();

    if (the_copy.drawable() != 0) {
        if (create_new) {
            create(the_copy.drawable(), 
                   the_copy.width(), the_copy.height(),
                   the_copy.depth());
        }
        
        if (drawable()) {
            GContext gc(drawable());
                        
            copyArea(the_copy.drawable(),
                     gc.gc(),
                     0, 0,
                     0, 0,
                     width(), height());
        }
    }
}

void FbPixmap::copy(Pixmap pm) {
    free();
    if (pm == 0)
        return;

    // get width, height and depth for the pixmap
    Window root;
    int x, y;
    unsigned int border_width, bpp;
    unsigned int new_width, new_height;

    XGetGeometry(FbTk::App::instance()->display(),
                 pm,
                 &root,
                 &x, &y,
                 &new_width, &new_height,
                 &border_width,
                 &bpp);
    // create new pixmap and copy area
    create(root, new_width, new_height, bpp);
    
    Display *disp = FbTk::App::instance()->display();

    GC gc = XCreateGC(disp, drawable(), 0, 0);

    XCopyArea(disp, pm, drawable(), gc, 
              0, 0,
              width(), height(),
              0, 0);

    XFreeGC(disp, gc);
}

void FbPixmap::rotate() {

    Display *dpy = FbTk::App::instance()->display();

    // make an image copy
    XImage *src_image = XGetImage(dpy, drawable(),
                                  0, 0, // pos
                                  width(), height(), // size
                                  ~0, // plane mask
                                  ZPixmap); // format
    // reverse height/width for new pixmap
    FbPixmap new_pm(drawable(), height(), width(), depth());

    GContext gc(drawable());

    // copy new area
    for (int y = 0; y < static_cast<signed>(height()); ++y) {
        for (int x = 0; x < static_cast<signed>(width()); ++x) {
            gc.setForeground(XGetPixel(src_image, x, y));
            // revers coordinates
            XDrawPoint(dpy, new_pm.drawable(), gc.gc(), y, x);
        }
    }

    XDestroyImage(src_image);
    // free old pixmap and set new from new_pm
    free();

    m_width = new_pm.width();
    m_height = new_pm.height();
    m_depth = new_pm.depth();
    m_pm = new_pm.release();
}

void FbPixmap::scale(unsigned int dest_width, unsigned int dest_height) {
    if (drawable() == 0 || 
        (dest_width == width() && dest_height == height()))
        return;

    Display *dpy = FbTk::App::instance()->display();

    XImage *src_image = XGetImage(dpy, drawable(),
                                  0, 0, // pos
                                  width(), height(), // size
                                  ~0, // plane mask
                                  ZPixmap); // format
    if (src_image == 0)
        return;

    // create new pixmap with dest size
    FbPixmap new_pm(drawable(), dest_width, dest_height, depth());

    GContext gc(drawable());
    // calc zoom
    float zoom_x = static_cast<float>(width())/static_cast<float>(dest_width);
    float zoom_y = static_cast<float>(height())/static_cast<float>(dest_height);

    // start scaling
    float src_x = 0, src_y = 0;
    for (int tx=0; tx < static_cast<signed>(dest_width); ++tx, src_x += zoom_x) {
        src_y = 0;
        for (int ty=0; ty < static_cast<signed>(dest_height); ++ty, src_y += zoom_y) {	
            gc.setForeground(XGetPixel(src_image,
                                       static_cast<int>(src_x),
                                       static_cast<int>(src_y)));
            XDrawPoint(dpy, new_pm.drawable(), gc.gc(), tx, ty);
        }
    }

    XDestroyImage(src_image);

    // free old pixmap and set new from new_pm
    free();

    m_width = new_pm.width();
    m_height = new_pm.height();
    m_depth = new_pm.depth();
    m_pm = new_pm.release();
}

void FbPixmap::resize(unsigned int width, unsigned int height) {
    FbPixmap pm(drawable(), width, height, depth());
    *this = pm.release();
}

Pixmap FbPixmap::release() {
    Pixmap ret = m_pm;
    m_pm = 0;
    m_width = 0;
    m_height = 0;
    m_depth = 0;
    return ret;
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
