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

#include "Shape.hh"

#include "FbWindow.hh"
#include "App.hh"
#include "GContext.hh"
#include "FbPixmap.hh"

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <algorithm>
#include <vector>
#include <iostream>

using std::min;

namespace FbTk {

namespace {
/* rows is an array of 8 bytes, i.e. 8x8 bits */
Pixmap makePixmap(Display* disp, int screen_nr, Window parent, const unsigned char rows[]) {

    const size_t data_size = 8 * 8;
    // we use malloc here so we get consistent C alloc/free with XDestroyImage
    // and no warnings in valgrind :)
    char *data = (char *)malloc(data_size * sizeof (char));
    if (data == 0)
        return 0;

    memset(data, 0xFF, data_size);

    XImage *ximage = XCreateImage(disp,
                                  DefaultVisual(disp, screen_nr),
                                  1,
                                  XYPixmap, 0,
                                  data,
                                  8, 8,
                                  32, 0);
    if (ximage == 0) {
        free(data);
        return 0;
    }

    XInitImage(ximage);

    for (int y=0; y<8; y++) {
        for (int x=0; x<8; x++) {
            XPutPixel(ximage, x, y, (rows[y] & (0x01 << x)) ? 0 : 1); // inverted, it is subtracted
        }
    }

    FbPixmap pm(parent, 8, 8, 1);
    GContext gc(pm);

    XPutImage(disp, pm.drawable(), gc.gc(), ximage, 0, 0, 0, 0,
              8, 8);

    XDestroyImage(ximage);

    return pm.release();
}

struct CornerPixmaps {
    CornerPixmaps() : do_create(true) { }

    FbPixmap topleft;
    FbPixmap topright;
    FbPixmap botleft;
    FbPixmap botright;

    bool do_create;
};

// unfortunately, we need a separate pixmap per screen
std::vector<CornerPixmaps> s_corners;

unsigned long nr_shapes = 0;

void initCorners(int screen) {

    Display* disp = App::instance()->display();
    if (s_corners.empty())
        s_corners.resize(ScreenCount(disp));


    if (screen < 0 || screen > static_cast<int>(s_corners.size())) {
        std::cerr << "FbTk/Shape.cc:initCorners(), invalid argument: " << screen << "\n";
        return;
    }

    static const unsigned char left_bits[] = { 0xc0, 0xf8, 0xfc, 0xfe, 0xfe, 0xfe, 0xff, 0xff };
    static const unsigned char right_bits[] = { 0x03, 0x1f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0xff};
    static const unsigned char bottom_left_bits[] = { 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xf8, 0xc0 };
    static const unsigned char bottom_right_bits[] = { 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x1f, 0x03 };

    CornerPixmaps& corners = s_corners[screen];
    if (corners.do_create) {

        Window root = RootWindow(disp, screen);
        corners.topleft = makePixmap(disp, screen, root, left_bits);
        corners.topright = makePixmap(disp, screen, root, right_bits);
        corners.botleft = makePixmap(disp, screen, root, bottom_left_bits);
        corners.botright = makePixmap(disp, screen, root, bottom_right_bits);
        corners.do_create = false;
    }

    nr_shapes++; // refcounting
}

void cleanCorners() {

    if (nr_shapes == 1) {
        s_corners.clear();
    }

    if (nr_shapes > 0) {
        nr_shapes--; // refcounting
    }
}

} // end of anonymous namespace

Shape::Shape(FbWindow &win, int shapeplaces):
    m_win(&win),
    m_shapesource(0),
    m_shapesource_xoff(0),
    m_shapesource_yoff(0),
    m_shapeplaces(shapeplaces) {

#ifdef SHAPE
    initCorners(win.screenNumber());
#endif

    update();
}

Shape::~Shape() {

#ifdef SHAPE
    if (m_win != 0 && m_win->window()) {
        // Reset shape of window
        XShapeCombineMask(App::instance()->display(),
                          m_win->window(),
                          ShapeClip,
                          0, 0,
                          0,
                          ShapeSet);
        XShapeCombineMask(App::instance()->display(),
                          m_win->window(),
                          ShapeBounding,
                          0, 0,
                          0,
                          ShapeSet);
    }

    cleanCorners();
#endif // SHAPE
}

void Shape::setPlaces(int shapeplaces) {
    m_shapeplaces = shapeplaces;
}

void Shape::update() {
    if (m_win == 0 || m_win->window() == 0)
        return;

#ifdef SHAPE
    /**
     * Set the client's shape in position,
     * or wipe the shape and return.
     */
    Display *display = App::instance()->display();
    int bw = m_win->borderWidth();
    int width = m_win->width();
    int height = m_win->height();

    if (m_shapesource == 0 && m_shapeplaces == 0) {
        /* clear the shape and return */
        XShapeCombineMask(display,
                          m_win->window(), ShapeClip,
                          0, 0,
                          None, ShapeSet);
        XShapeCombineMask(display,
                          m_win->window(), ShapeBounding,
                          0, 0,
                          None, ShapeSet);
        return;
    }

    Region clip = XCreateRegion();
    Region bound = XCreateRegion();

    XRectangle rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;

    XUnionRectWithRegion(&rect, clip, clip);

    rect.x = -bw;
    rect.y = -bw;
    rect.width = width+2*bw;
    rect.height = height+2*bw;

    XUnionRectWithRegion(&rect, bound, bound);

    if (m_shapesource != 0) {

        /*
          Copy the shape from the source.
          We achieve this by subtracting the client-area size from the shape, and then
          unioning in the client's mask.
        */
        rect.x = m_shapesource_xoff;
        rect.y = m_shapesource_yoff;
        rect.width = m_shapesource->width();
        rect.height = m_shapesource->height();

        Region clientarea = XCreateRegion();
        XUnionRectWithRegion(&rect, clientarea, clientarea);
        XSubtractRegion(clip, clientarea, clip);
        XSubtractRegion(bound, clientarea, bound);
        XDestroyRegion(clientarea);

        XShapeCombineShape(display,
                           m_win->window(), ShapeClip,
                           rect.x, rect.y, // xOff, yOff
                           m_shapesource->window(),
                           ShapeClip, ShapeSet);

        XShapeCombineRegion(display,
                            m_win->window(), ShapeClip,
                            0, 0, // offsets
                            clip, ShapeUnion);

        /* 
           Now the bounding rectangle. Note that the frame has a shared border with the region above the 
           client (i.e. titlebar), so we don't want to wipe the shared border, hence the adjustments.
        */

        XShapeCombineShape(display,
                           m_win->window(), ShapeBounding,
                           rect.x, rect.y, // xOff, yOff
                           m_shapesource->window(),
                           ShapeBounding, ShapeSet);

        XShapeCombineRegion(display,
                            m_win->window(), ShapeBounding,
                            0, 0, // offsets
                            bound, ShapeUnion);
    } else {
        XShapeCombineRegion(display,
                            m_win->window(), ShapeClip,
                            0, 0, // offsets
                            clip, ShapeSet);
        XShapeCombineRegion(display,
                            m_win->window(), ShapeBounding,
                            0, 0, // offsets
                            bound, ShapeSet);
    }

    XDestroyRegion(clip);
    XDestroyRegion(bound);

   const CornerPixmaps &corners = s_corners[m_win->screenNumber()];
#define SHAPECORNER(corner, x, y, shapekind)            \
    XShapeCombineMask(App::instance()->display(), \
                      m_win->window(),                  \
                      shapekind,                        \
                      x, y,                             \
                      corners.corner.drawable(),      \
                      ShapeSubtract); 

    /**
     * Set the top corners if the y offset is nonzero.
     */
    if (m_shapesource == 0 || m_shapesource_yoff != 0) {
        if (m_shapeplaces & TOPLEFT) {
            SHAPECORNER(topleft, 0, 0, ShapeClip);
            SHAPECORNER(topleft, -bw, -bw, ShapeBounding);
        }
        if (m_shapeplaces & TOPRIGHT) {
            SHAPECORNER(topright, width-8, 0, ShapeClip);
            SHAPECORNER(topright, width+bw-8, -bw, ShapeBounding);
        }
    }

    // note that the bottom corners y-vals are offset by 8 (the height of the corner pixmaps)
    if (m_shapesource == 0 || (m_shapesource_yoff+(signed) m_shapesource->height()) < height
        || m_shapesource_yoff >= height /* shaded */) {
        if (m_shapeplaces & BOTTOMLEFT) {
            SHAPECORNER(botleft, 0, height-8, ShapeClip);
            SHAPECORNER(botleft, -bw, height+bw-8, ShapeBounding);
        }
        if (m_shapeplaces & BOTTOMRIGHT) {
            SHAPECORNER(botright, width-8, height-8, ShapeClip);
            SHAPECORNER(botright, width+bw-8, height+bw-8, ShapeBounding);
        }
    }

#endif // SHAPE

}

void Shape::setWindow(FbWindow &win) {
    m_win = &win;
    update();
}

/**
 * set the shape source to the given window.
 * This is purely for client windows at the moment, where the offsets and height/width of the 
 * target window and the source window are used to determine whether to shape a given corner.
 *
 * (note: xoffset will always be zero, and widths always match, so we ignore those)
 * 
 * i.e. if the yoffset is not zero, then the top corners are shaped.
 * if the target height is bigger than the source plus yoffset, then the bottom corners are
 * shaped.
 *
 * If *either* the top or bottom corners are not shaped due to this, but a shape source window
 * is given, then the bounding shape has the borders alongside the source window deleted, otherwise
 * they are left hanging outside the client's shape.
 */
void Shape::setShapeSource(FbWindow *win, int xoff, int yoff, bool always_update) {
    if (win != 0 && !isShaped(*win)) {
        win = 0;
        if (m_shapesource == 0 && !always_update)
            return;
    }

    // even if source is same, want to update the shape on it
    m_shapesource = win;
    m_shapesource_xoff = xoff;
    m_shapesource_yoff = yoff;
    update();
}

void Shape::setShapeOffsets(int xoff, int yoff) {
    m_shapesource_xoff = xoff;
    m_shapesource_yoff = yoff;
    update();
}

void Shape::setShapeNotify(const FbWindow &win) {
#ifdef SHAPE
    XShapeSelectInput(App::instance()->display(),
                      win.window(), ShapeNotifyMask);
#endif // SHAPE
}

bool Shape::isShaped(const FbWindow &win) {
    int shaped = 0;

#ifdef SHAPE
    int not_used;
    unsigned int not_used2;
    XShapeQueryExtents(App::instance()->display(),
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

} // end namespace FbTk
