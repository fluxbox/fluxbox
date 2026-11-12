// FbPixmap.cc for FbTk - Fluxbox ToolKit
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

#include "FbPixmap.hh"
#include "App.hh"
#include "GContext.hh"
#include "Transparent.hh"
#include "FbWindow.hh"
#include "TextUtils.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <iostream>
#include <algorithm>
#include <vector>
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

using std::cerr;

namespace FbTk {

namespace {

std::vector <Pixmap> s_root_pixmaps;

struct RootProps {
    const char* name;
    Atom atom;
};

struct RootProps root_props[] = {
    { "_XROOTPMAP_ID", None },
    { "_XSETROOT_ID", None }
};

void checkAtoms() {

    Display* display = FbTk::App::instance()->display();
    for (size_t i = 0; i < sizeof(root_props)/sizeof(RootProps); ++i) {
        if (root_props[i].atom == None) {
            root_props[i].atom = XInternAtom(display, root_props[i].name, False);
        }
    }
}

// an empty client side image of the given size in the format of 'src':
// fill it with XPutPixel(), send it with one XPutImage()
XImage *createImageLike(Display *display, const XImage *src,
                        unsigned int width, unsigned int height) {

    // XPutPixel() does not need the visual's color masks
    XImage *image = XCreateImage(display, 0, src->depth, ZPixmap, 0, 0,
                                 width, height, src->bitmap_pad, 0);
    if (image == 0)
        return 0;

    // malloc(), because XDestroyImage() releases the data with free()
    image->data = static_cast<char *>(malloc(
        static_cast<size_t>(image->bytes_per_line) * height));
    if (image->data == 0) {
        XDestroyImage(image);
        return 0;
    }

    return image;
}

// X pixmap sizes are 16 bit; bigger values would also overflow
// the line size XCreateImage() computes in older libX11
const unsigned int MAX_SCALE_SIZE = 65535;

bool validScaleSize(unsigned int width, unsigned int height) {
    return width > 0 && height > 0 &&
        width <= MAX_SCALE_SIZE && height <= MAX_SCALE_SIZE;
}

// one color component of a pixel value
struct Channel {
    unsigned long mask;
    unsigned int shift;
};

const size_t MAX_CHANNELS = 4;

void addChannel(Channel *channels, size_t &nr, unsigned long mask) {
    if (mask == 0 || nr == MAX_CHANNELS) {
        return;
    }
    unsigned int shift = 0;
    while (((mask >> shift) & 1) == 0) {
        ++shift;
    }
    channels[nr].mask = mask;
    channels[nr].shift = shift;
    ++nr;
}

// Splits the pixel values of 'depth' bit pixmaps on the screen of
// 'drawable' into red, green, blue and whatever is left (the alpha of
// a 32 bit ARGB visual). Returns 0 if there is no TrueColor or
// DirectColor visual of that depth: then the bits are no intensities
// and must not be averaged.
size_t colorChannels(Display *display, Drawable drawable,
                     unsigned int depth, Channel *channels) {

    if (depth < 2 || depth > 32) {
        return 0;
    }

    Window root;
    int x, y;
    unsigned int w, h, border, d;
    if (!XGetGeometry(display, drawable, &root, &x, &y, &w, &h, &border, &d)) {
        return 0;
    }

    int screen = 0;
    for (int i = 0; i < ScreenCount(display); ++i) {
        if (RootWindow(display, i) == root) {
            screen = i;
            break;
        }
    }

    XVisualInfo info;
    Visual *visual = DefaultVisual(display, screen);
    if (static_cast<unsigned int>(DefaultDepth(display, screen)) == depth &&
        (visual->c_class == TrueColor || visual->c_class == DirectColor)) {
        info.red_mask = visual->red_mask;
        info.green_mask = visual->green_mask;
        info.blue_mask = visual->blue_mask;
    } else if (!XMatchVisualInfo(display, screen, depth, TrueColor, &info)) {
        return 0;
    }

    unsigned long all = (depth == 32) ? 0xffffffffUL : ((1UL << depth) - 1);
    size_t nr = 0;
    addChannel(channels, nr, info.red_mask & all);
    addChannel(channels, nr, info.green_mask & all);
    addChannel(channels, nr, info.blue_mask & all);
    addChannel(channels, nr,
               all & ~(info.red_mask | info.green_mask | info.blue_mask));
    return nr;
}

// a source pixel and how much of it a destination pixel covers
struct Tap {
    unsigned int index;
    unsigned long long weight;
};

// Lists the source pixels behind each of the 'dst' destination pixels
// along one axis; the taps of pixel i are taps[first[i]..first[i+1]).
// Shrinking ('smooth' and src > dst) uses every source pixel the
// destination pixel overlaps, weighted by the overlap. Otherwise the
// source pixel under the center of the destination pixel is used.
void makeTaps(unsigned int src, unsigned int dst, bool smooth,
              std::vector<Tap> &taps, std::vector<size_t> &first) {

    const unsigned long long s = src, d = dst;

    taps.clear();
    first.clear();
    for (unsigned long long i = 0; i < d; ++i) {
        first.push_back(taps.size());

        if (!smooth || s <= d) {
            Tap tap = { static_cast<unsigned int>((2 * i + 1) * s / (2 * d)), 1 };
            taps.push_back(tap);
            continue;
        }

        // in units of 1/(s*d): destination pixel i covers
        // [i*s, (i+1)*s), source pixel j covers [j*d, (j+1)*d)
        const unsigned long long begin = i * s, end = (i + 1) * s;
        for (unsigned long long j = begin / d; j * d < end; ++j) {
            unsigned long long from = std::max(begin, j * d);
            unsigned long long to = std::min(end, (j + 1) * d);
            Tap tap = { static_cast<unsigned int>(j), to - from };
            taps.push_back(tap);
        }
    }
    first.push_back(taps.size());
}

// Scales 'src' and / or the 1 bit 'src_mask' (same size) into 'dest'
// and 'dest_mask'. A set mask bit is an opaque pixel. The colors of a
// shrunk pixel are the average of its opaque source pixels (of all, if
// none is opaque), so the transparent background does not bleed into
// the edges. A destination mask bit is set if at least half of the
// covered area is opaque. Lower thresholds keep thin outlines at odd
// ratios, but turn the rim of round shapes into single stray pixels
// whose color comes from one or two source pixels; shrinking by 2:1
// (see the icon choice in Ewmh.cc) keeps outlines with this rule.
// Without channels the colors cannot be averaged, so both images use
// the nearest pixel.
void scaleImages(const XImage *src, const XImage *src_mask,
                 XImage *dest, XImage *dest_mask,
                 const Channel *channels, size_t nr_channels) {

    const XImage *ref = src ? src : src_mask;
    const XImage *out = dest ? dest : dest_mask;
    const unsigned int src_w = ref->width, src_h = ref->height;
    const unsigned int dest_w = out->width, dest_h = out->height;
    const bool smooth = (src == 0 || nr_channels > 0);

    std::vector<Tap> taps_x, taps_y;
    std::vector<size_t> first_x, first_y;
    makeTaps(src_w, dest_w, smooth, taps_x, first_x);
    makeTaps(src_h, dest_h, smooth, taps_y, first_y);

    // XGetPixel() takes a non-const XImage
    XImage *in = const_cast<XImage *>(src);
    XImage *in_mask = const_cast<XImage *>(src_mask);

    for (unsigned int ty = 0; ty < dest_h; ++ty) {
        for (unsigned int tx = 0; tx < dest_w; ++tx) {

            unsigned long long total = 0, opaque = 0;
            unsigned long long sum_all[MAX_CHANNELS] = { 0 };
            unsigned long long sum_opaque[MAX_CHANNELS] = { 0 };
            unsigned long pixel = 0;

            for (size_t ny = first_y[ty]; ny < first_y[ty + 1]; ++ny) {
                const Tap &tap_y = taps_y[ny];
                for (size_t nx = first_x[tx]; nx < first_x[tx + 1]; ++nx) {
                    const Tap &tap_x = taps_x[nx];
                    const unsigned long long w = tap_x.weight * tap_y.weight;
                    const bool on = in_mask == 0 ||
                        XGetPixel(in_mask, tap_x.index, tap_y.index) != 0;

                    total += w;
                    if (on) {
                        opaque += w;
                    }

                    if (in == 0) {
                        continue;
                    }

                    pixel = XGetPixel(in, tap_x.index, tap_y.index);
                    for (size_t c = 0; c < nr_channels; ++c) {
                        unsigned long long v =
                            (pixel & channels[c].mask) >> channels[c].shift;
                        sum_all[c] += v * w;
                        if (on) {
                            sum_opaque[c] += v * w;
                        }
                    }
                }
            }

            if (dest_mask) {
                XPutPixel(dest_mask, tx, ty, (2 * opaque >= total) ? 1 : 0);
            }

            if (dest == 0) {
                continue;
            }

            if (nr_channels > 0) {
                const unsigned long long *sum = opaque ? sum_opaque : sum_all;
                const unsigned long long div = opaque ? opaque : total;
                pixel = 0;
                for (size_t c = 0; c < nr_channels; ++c) {
                    unsigned long long v = (sum[c] + div / 2) / div;
                    pixel |= (static_cast<unsigned long>(v) << channels[c].shift)
                        & channels[c].mask;
                }
            }
            // else: a single tap, 'pixel' is the nearest source pixel

            XPutPixel(dest, tx, ty, pixel);
        }
    }
}

} // end of anonymous namespace

FbPixmap::FbPixmap():m_pm(0),
                     m_width(0), m_height(0),
                     m_depth(0), m_dont_free(false) {
}

FbPixmap::FbPixmap(const FbPixmap &the_copy):FbDrawable(), m_pm(0),
                                             m_width(0), m_height(0),
                                             m_depth(0), m_dont_free(false) {
    copy(the_copy);
}

FbPixmap::FbPixmap(Pixmap pm):m_pm(0),
                              m_width(0), m_height(0),
                              m_depth(0), m_dont_free(false) {
    if (pm == 0)
        return;
    // assign X pixmap to this
    (*this) = pm;
}

FbPixmap::FbPixmap(const FbDrawable &src,
                   unsigned int width, unsigned int height,
                   unsigned int depth):m_pm(0),
                              m_width(0), m_height(0),
                              m_depth(0), m_dont_free(false) {

    create(src.drawable(), width, height, depth);
}

FbPixmap::FbPixmap(Drawable src,
                   unsigned int width, unsigned int height,
                   unsigned int depth):m_pm(0),
                              m_width(0), m_height(0),
                              m_depth(0), m_dont_free(false) {

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
    if (!XGetGeometry(display(),
                      pm,
                      &root,
                      &x, &y,
                      &m_width, &m_height,
                      &border_width,
                      &bpp))
        return *this;

    m_depth = bpp;

    m_pm = pm;

    return *this;
}

void FbPixmap::copy(const FbPixmap &the_copy) {
    /* This function previously retained the old pixmap and copied in
       the new contents if they had the same dimensions.
       This broke the image cache, so we don't do that now. If you want to
       do it, then you'll need to invalidate all copies of this pixmap in
       the cache */
    free();

    if (the_copy.drawable() != 0) {
        create(the_copy.drawable(),
               the_copy.width(), the_copy.height(),
               the_copy.depth());

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

// screen doesn't count if depth is "zero"...
void FbPixmap::copy(Pixmap pm, unsigned int depth, int screen_num) {
    free();
    if (pm == 0)
        return;

    // get width, height and depth for the pixmap
    Window root;
    int x, y;
    unsigned int border_width, bpp;
    unsigned int new_width, new_height;

    if (!XGetGeometry(display(),
                      pm,
                      &root,
                      &x, &y,
                      &new_width, &new_height,
                      &border_width,
                      &bpp))
        return;

    if (depth == 0)
        depth = bpp;

    // create new pixmap and copy area
    create(root, new_width, new_height, depth);

    GC gc = XCreateGC(display(), drawable(), 0, 0);

    if (depth == bpp) {
        XCopyArea(display(), pm, drawable(), gc,
                  0, 0,
                  width(), height(),
                  0, 0);
    } else {
        XSetForeground(display(), gc, Color("black", screen_num).pixel());
        XSetBackground(display(), gc, Color("white", screen_num).pixel());
        XCopyPlane(display(), pm, drawable(), gc,
                   0, 0,
                   width(), height(),
                   0, 0, 1);
    }

    XFreeGC(display(), gc);
}

void FbPixmap::rotate(FbTk::Orientation orient) {
    if (orient == ROT0)
        return;

    unsigned int oldw = width(), oldh = height();
    unsigned int neww = oldw, newh = oldh;
    translateSize(orient, neww, newh);

    // reverse height/width for new pixmap
    FbPixmap new_pm(drawable(), neww, newh, depth());

    // width|height could be 0. this happens (for example) if
    // the systemtray-tool is ROT90. in that case 'src_image'
    // becomes NULL and caused a SIGSEV upon XDestroyImage()
    // TODO: catch dimensions with '0' earlier?
    //
    // make an image copy
    XImage *src_image = XGetImage(display(), drawable(),
                                  0, 0, // pos
                                  oldw, oldh, // size
                                  ~0, // plane mask
                                  ZPixmap); // format
    // TODO: with XRender the server could rotate, using a rotation
    // transform instead of the scaling one; see scale() for the issues
    XImage *dest_image = 0;
    if (src_image)
        dest_image = createImageLike(display(), src_image, neww, newh);

    if (dest_image) {

        if (orient == ROT180) {
            unsigned int srcx, srcy, destx, desty;
            for (srcy = 0, desty = oldh - 1; srcy < oldh; ++srcy, --desty) {
                for (srcx = 0, destx = oldw - 1; srcx < oldw; ++srcx, --destx) {
                    XPutPixel(dest_image, destx, desty,
                              XGetPixel(src_image, srcx, srcy));
                }
            }
        } else {
            // need to flip x and y

            // set start, end and direction based on rotation
            // NOTE that startx etc are in the direction of the OLD pixmap
            unsigned int startx = 0, starty = 0;
            int dirx = 0, diry = 0;
            switch (orient) {
            case ROT90:
                startx = neww-1;
                starty = 0;
                dirx = -1;
                diry = 1;
                break;
            case ROT270:
                startx = 0;
                starty = newh-1;
                dirx = 1;
                diry = -1;
                break;
            default: // kill warning
                break;
            }


            // copy new area
            unsigned int srcx, srcy, destx, desty;
            for (srcy = 0, destx = startx; srcy < oldh; ++srcy, destx+=dirx) {
                for (srcx = 0, desty = starty; srcx < oldw; ++srcx, desty+=diry) {
                    XPutPixel(dest_image, destx, desty,
                              XGetPixel(src_image, srcx, srcy));
                }
            }
        }

        GContext gc(drawable());
        XPutImage(display(), new_pm.drawable(), gc.gc(), dest_image,
                  0, 0, 0, 0, neww, newh);

        XDestroyImage(dest_image);
    }

    if (src_image)
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
        (dest_width == width() && dest_height == height()) ||
        !validScaleSize(dest_width, dest_height)) {
        return;
    }

    XImage *src_image = XGetImage(display(), drawable(),
                                  0, 0, // pos
                                  width(), height(), // size
                                  ~0, // plane mask
                                  ZPixmap); // format
    if (src_image == 0) {
        return;
    }

    // TODO: with XRender the server could scale: wrap source and
    // destination in Pictures, set a scaling transform and a filter,
    // then call XRenderComposite() once. Issues: XRender is optional at
    // build and run time, so this code stays as the fallback; not every
    // depth has a Picture format; a pixmap and its mask have to take
    // the same path and filter, or they can end up a pixel apart.
    XImage *dest_image = createImageLike(display(), src_image,
                                         dest_width, dest_height);
    if (dest_image == 0) {
        XDestroyImage(src_image);
        return;
    }

    if (depth() == 1) {
        // a mask: keep the pixels which are at least half covered
        scaleImages(0, src_image, 0, dest_image, 0, 0);
    } else {
        Channel channels[MAX_CHANNELS];
        size_t nr = colorChannels(display(), drawable(), depth(), channels);
        scaleImages(src_image, 0, dest_image, 0, channels, nr);
    }

    XDestroyImage(src_image);
    replaceWith(dest_image);
    XDestroyImage(dest_image);
}

void FbPixmap::scaleWithMask(FbPixmap &mask,
                             unsigned int dest_width, unsigned int dest_height) {

    if (drawable() == 0 || mask.drawable() == 0 || mask.depth() != 1 ||
        mask.width() != width() || mask.height() != height()) {
        scale(dest_width, dest_height);
        mask.scale(dest_width, dest_height);
        return;
    }

    if ((dest_width == width() && dest_height == height()) ||
        !validScaleSize(dest_width, dest_height)) {
        return;
    }

    XImage *src_image = XGetImage(display(), drawable(), 0, 0,
                                  width(), height(), ~0, ZPixmap);
    XImage *src_mask = XGetImage(display(), mask.drawable(), 0, 0,
                                 width(), height(), ~0, ZPixmap);
    XImage *dest_image = 0;
    XImage *dest_mask = 0;
    if (src_image && src_mask) {
        dest_image = createImageLike(display(), src_image,
                                     dest_width, dest_height);
        dest_mask = createImageLike(display(), src_mask,
                                    dest_width, dest_height);
    }

    if (dest_image && dest_mask) {
        Channel channels[MAX_CHANNELS];
        size_t nr = colorChannels(display(), drawable(), depth(), channels);
        scaleImages(src_image, src_mask, dest_image, dest_mask, channels, nr);
        replaceWith(dest_image);
        mask.replaceWith(dest_mask);
    }

    XImage *images[] = { src_image, src_mask, dest_image, dest_mask };
    for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); ++i) {
        if (images[i]) {
            XDestroyImage(images[i]);
        }
    }
}

void FbPixmap::replaceWith(XImage *image) {

    FbPixmap new_pm(drawable(), image->width, image->height, depth());

    GContext gc(drawable());
    XPutImage(display(), new_pm.drawable(), gc.gc(), image,
              0, 0, 0, 0, image->width, image->height);

    // free old pixmap and set new from new_pm
    free();

    m_width = new_pm.width();
    m_height = new_pm.height();
    m_depth = new_pm.depth();
    m_pm = new_pm.release();
}

void FbPixmap::tile(unsigned int dest_width, unsigned int dest_height) {
    if (drawable() == 0 ||
        (dest_width == width() && dest_height == height()))
        return;

    FbPixmap new_pm(drawable(), width(), height(), depth());

    new_pm.copy(m_pm, 0, 0);

    resize(dest_width, dest_height);

    FbTk::GContext gc(*this);

    gc.setTile(new_pm);
    gc.setFillStyle(FillTiled);

    fillRectangle(gc.gc(), 0, 0, dest_width, dest_height);

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

// returns whether or not the background was changed
bool FbPixmap::rootwinPropertyNotify(int screen_num, Atom atom) {
    if (!FbTk::Transparent::haveRender())
        return false;

    checkAtoms();
    for (size_t i = 0; i < sizeof(root_props)/sizeof(RootProps); ++i) {
        if (root_props[i].atom == atom) {
            Pixmap root_pm = None;
            Atom real_type;
            int real_format;
            unsigned long items_read, items_left;
            unsigned long *data;

            if (XGetWindowProperty(display(),
                                   RootWindow(display(), screen_num),
                                   root_props[i].atom,
                                   0l, 1l,
                                   False, XA_PIXMAP,
                                   &real_type, &real_format,
                                   &items_read, &items_left,
                                   (unsigned char **) &data) == Success) {
                if (real_format == 32 && items_read == 1) {
                    root_pm = (Pixmap) (*data);
                }
                XFree(data);
                if (root_pm != None)
                    return setRootPixmap(screen_num, root_pm);
            }
            return false;
        }
    }
    return false;
}

// returns whether or not the background was changed
bool FbPixmap::setRootPixmap(int screen_num, Pixmap pm) {

    if (s_root_pixmaps.empty()) {
        int i;
        for (i = 0; i < ScreenCount(display()); ++i)
            s_root_pixmaps.push_back(None);
    }

    if (s_root_pixmaps[screen_num] != pm) {
        s_root_pixmaps[screen_num] = pm;
        FbWindow::updatedAlphaBackground(screen_num);
        return true;
    }
    return false;
}

Pixmap FbPixmap::getRootPixmap(int screen_num, bool force_update) {
    /*
      if (!FbTk::Transparent::haveRender())
      return None;
    */

    // check and see if if we have the pixmaps in cache
    if (!s_root_pixmaps.empty() && !force_update)
        return s_root_pixmaps[screen_num];

    checkAtoms();

    // else setup pixmap cache
    int numscreens = ScreenCount(display());
    for (int i=0; i < numscreens; ++i) {
        Atom real_type;
        int real_format;
        unsigned long items_read, items_left;
        unsigned long *data;

        static bool print_error = true; // print error_message only once

        Pixmap root_pm = None;

        unsigned int prop = 0;
        for (prop = 0; prop < sizeof(root_props)/sizeof(RootProps); ++prop) {
            if (XGetWindowProperty(display(),
                                   RootWindow(display(), i),
                                   root_props[prop].atom,
                                   0l, 1l,
                                   False, XA_PIXMAP,
                                   &real_type, &real_format,
                                   &items_read, &items_left,
                                   (unsigned char **) &data) == Success) {
                if (real_format == 32 && items_read == 1) {
                    if (print_error && strcmp(root_props[prop].name, "_XSETROOT_ID") == 0) {
                        static const char* error_message = {
                            "\n\n !!! WARNING WARNING WARNING WARNING !!!!!\n"
                            "   if you experience problems with transparency:\n"
                            "   you are using a wallpapersetter that \n"
                            "   uses _XSETROOT_ID .. which we do not support.\n"
                            "   consult 'fbsetbg -i' or try any other wallpapersetter\n"
                            "   that uses _XROOTPMAP_ID !\n"
                            " !!! WARNING WARNING WARNING WARNING !!!!!!\n\n"
                        };
                        cerr<<error_message;
                        print_error = false;
                    } else
                        root_pm = (Pixmap) (*data);
                }
                XFree(data);
                if (root_pm != None)
                    break;
            }
        }
        setRootPixmap(i, root_pm);
    }

    return s_root_pixmaps[screen_num];
}

void FbPixmap::free() {
    if (!m_dont_free && m_pm != 0)
        XFreePixmap(display(), m_pm);

    /* note: m_dont_free shouldnt be required anywhere else,
       because then free() isn't being called appropriately! */
    m_dont_free = false;
    m_pm = 0;
    m_width = 0;
    m_height = 0;
    m_depth = 0;
}

void FbPixmap::create(Drawable src,
                      unsigned int width, unsigned int height,
                      unsigned int depth) {
    if (src == 0)
        return;

    m_pm = XCreatePixmap(display(),
                         src, width, height, depth);
    if (m_pm == 0)
        return;

    m_width = width;
    m_height = height;
    m_depth = depth;
}

} // end namespace FbTk

