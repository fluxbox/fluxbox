// ImageControl.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxbox at users.sourceforge.net)
//
// Image.cc for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#include "ImageControl.hh"

#include "TextureRender.hh"
#include "App.hh"
#include "SimpleCommand.hh"
#include "I18n.hh"

//use GNU extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#include <iostream>

using std::cerr;
using std::endl;
using std::list;

namespace FbTk {

// lookup table for texture
unsigned long *ImageControl::sqrt_table = 0;
#ifdef TIMEDCACHE
bool ImageControl::s_timed_cache = true;
#else
bool ImageControl::s_timed_cache = false;
#endif // TIMEDCACHE

namespace { // anonymous

inline unsigned long bsqrt(unsigned long x) {
    if (x <= 0) return 0;
    if (x == 1) return 1;

    unsigned long r = x >> 1;
    unsigned long q;

    while (1) {
        q = x / r;
        if (q >= r) return r;
        r = (r + q) >> 1;
    }
}

}; // end anonymous namespace

ImageControl::ImageControl(int screen_num, bool dither,
                           int cpc, unsigned long cache_timeout, unsigned long cmax):
    m_dither(dither),
    m_colors(0),
    m_num_colors(0),
    m_colors_per_channel(cpc) {

    Display *disp = FbTk::App::instance()->display();

    m_screen_depth = DefaultDepth(disp, screen_num);
    m_screen_num = screen_num;
    m_root_window = RootWindow(disp, screen_num);
    m_visual = DefaultVisual(disp, screen_num);
    m_colormap = DefaultColormap(disp, screen_num);

    cache_max = cmax;

    if (cache_timeout && s_timed_cache) {
        m_timer.setTimeout(cache_timeout);
        RefCount<Command> clean_cache(new SimpleCommand<ImageControl>(*this, &ImageControl::cleanCache));
        m_timer.setCommand(clean_cache);
        m_timer.start();
    }

    createColorTable();
}


ImageControl::~ImageControl() {
    if (sqrt_table) {
        delete [] sqrt_table;
        sqrt_table = 0;
    }

    if (grad_xbuffer) {
        delete [] grad_xbuffer;
    }

    if (grad_ybuffer) {
        delete [] grad_ybuffer;
    }

    Display *disp = FbTk::App::instance()->display();

    if (m_colors) {
        unsigned long *pixels = new unsigned long [m_num_colors];

        for (unsigned int color = 0; color < m_num_colors; color++)
            *(pixels + color) = (*(m_colors + color)).pixel;

        XFreeColors(disp, m_colormap, pixels, m_num_colors, 0);

        delete [] m_colors;
    }

    if (cache.size() > 0) {
        CacheList::iterator it = cache.begin();
        CacheList::iterator it_end = cache.end();
        for (; it != it_end; ++it) {
            XFreePixmap(disp, (*it)->pixmap);
            delete (*it);
        }

    }

}


Pixmap ImageControl::searchCache(unsigned int width, unsigned int height,
                                 const Texture &text, FbTk::Orientation orient) const {

    if (text.pixmap().drawable() != None) {
        // do comparsion with width/height and texture_pixmap
        CacheList::iterator it = cache.begin();
        CacheList::iterator it_end = cache.end();
        for (; it != it_end; ++it) {
            if ((*it)->texture_pixmap == text.pixmap().drawable() &&
                (*it)->orient == orient &&
                (*it)->width == width &&
                (*it)->height == height &&
                (*it)->texture == text.type()) {
                (*it)->count++;
                return (*it)->pixmap;
            }
        }
        return None;
    }

    /*    Cache tmp;
    tmp.texture_pixmap = text.pixmap().drawable();
    tmp.width = width;
    tmp.height = height;
    tmp.texture = text.type();
    tmp.pixel1 = text.color().pixel();
    tmp.pixel2 = text.colorTo().pixel();
    */

    CacheList::iterator it = cache.begin();
    CacheList::iterator it_end = cache.end();
    for (; it != it_end; ++it) {
        if (((*it)->width == width) &&
            ((*it)->orient == orient) &&
            ((*it)->height == height) &&
            ((*it)->texture == text.type()) &&
            ((*it)->pixel1 == text.color().pixel())) {
            if (text.type() & FbTk::Texture::GRADIENT) {
                if ((*it)->pixel2 == text.colorTo().pixel()) {
                    (*it)->count++;
                    return (*it)->pixmap;
                }
            } else {
                (*it)->count++;
                return (*it)->pixmap;
            }
        }
    }

    return None;

}


Pixmap ImageControl::renderImage(unsigned int width, unsigned int height,
                                 const FbTk::Texture &texture,
                                 FbTk::Orientation orient) {

    if (texture.type() & FbTk::Texture::PARENTRELATIVE)
        return ParentRelative;

    // search cache first
    Pixmap pixmap = searchCache(width, height, texture, orient);
    if (pixmap) {
        return pixmap; // return cache item
    }

    // render new image

    TextureRender image(*this, width, height, orient, m_colors, m_num_colors);
    pixmap = image.render(texture);

    if (pixmap) {
        // create new cache item and add it to cache list

        Cache *tmp = new Cache;

        tmp->pixmap = pixmap;
        tmp->texture_pixmap = texture.pixmap().drawable();
        tmp->orient = orient;
        tmp->width = width;
        tmp->height = height;
        tmp->count = 1;
        tmp->texture = texture.type();
        tmp->pixel1 = texture.color().pixel();

        if (texture.type() & FbTk::Texture::GRADIENT)
            tmp->pixel2 = texture.colorTo().pixel();
        else
            tmp->pixel2 = 0l;

        cache.push_back(tmp);

        if (cache.size() > cache_max)
            cleanCache();

        return pixmap;
    }

    return None;
}


void ImageControl::removeImage(Pixmap pixmap) {
    if (!pixmap)
        return;

    CacheList::iterator it = cache.begin();
    CacheList::iterator it_end = cache.end();
    for (; it != it_end; ++it) {
        if ((*it)->pixmap == pixmap) {
            if ((*it)->count) {
                (*it)->count--;
                if (s_timed_cache) {
                    cleanCache();
                    return;
                }
            }

            if ((*it)->count <= 0)
                cleanCache();

            return;
        }
    }
}


void ImageControl::colorTables(const unsigned char **rmt, const unsigned char **gmt,
                               const unsigned char **bmt,
                               int *roff, int *goff, int *boff,
                               int *rbit, int *gbit, int *bbit) const {

    if (rmt) *rmt = red_color_table;
    if (gmt) *gmt = green_color_table;
    if (bmt) *bmt = blue_color_table;

    if (roff) *roff = red_offset;
    if (goff) *goff = green_offset;
    if (boff) *boff = blue_offset;

    if (rbit) *rbit = red_bits;
    if (gbit) *gbit = green_bits;
    if (bbit) *bbit = blue_bits;
}

#ifdef NOT_USED
void ImageControl::getXColorTable(XColor **c, int *n) {
    if (c) *c = m_colors;
    if (n) *n = m_num_colors;
}
#endif

void ImageControl::getGradientBuffers(unsigned int w,
                                      unsigned int h,
                                      unsigned int **xbuf,
                                      unsigned int **ybuf) {

    if (w > grad_buffer_width) {
        if (grad_xbuffer) {
            delete [] grad_xbuffer;
        }

        grad_buffer_width = w;

        grad_xbuffer = new unsigned int[grad_buffer_width * 3];
    }

    if (h > grad_buffer_height) {
        if (grad_ybuffer) {
            delete [] grad_ybuffer;
        }

        grad_buffer_height = h;

        grad_ybuffer = new unsigned int[grad_buffer_height * 3];
    }

    *xbuf = grad_xbuffer;
    *ybuf = grad_ybuffer;
}


void ImageControl::installRootColormap() {

    Display *disp = FbTk::App::instance()->display();
    XGrabServer(disp);

    bool install = true;
    int i = 0, ncmap = 0;
    Colormap *cmaps =
        XListInstalledColormaps(disp, m_root_window, &ncmap);

    if (cmaps) {
        for (i = 0; i < ncmap; i++) {
            if (*(cmaps + i) == m_colormap)
                install = false;
        }

        if (install)
            XInstallColormap(disp, m_colormap);

        XFree(cmaps);
    }

    XUngrabServer(disp);
}

#ifdef NOT_USED
void ImageControl::setColorsPerChannel(int cpc) {
    if (cpc < 2) cpc = 2;
    if (cpc > 6) cpc = 6;

    m_colors_per_channel = cpc;
}
#endif

unsigned long ImageControl::getSqrt(unsigned int x) const {
    if (! sqrt_table) {
        // build sqrt table for use with elliptic gradient

        sqrt_table = new unsigned long[256 * 256 * 2 + 1];
        int i = 0;

        for (; i < (256 * 256 * 2); i++)
            sqrt_table[i] = bsqrt(i);
    }

    return sqrt_table[x];
}

void ImageControl::cleanCache() {
    Display *disp = FbTk::App::instance()->display();
    list<CacheList::iterator> deadlist;
    CacheList::iterator it = cache.begin();
    CacheList::iterator it_end = cache.end();
    for (; it != it_end; ++it) {
        Cache *tmp = (*it);
        if (tmp->count <= 0) {
            XFreePixmap(disp, tmp->pixmap);
            deadlist.push_back(it);
            delete tmp;
            tmp=0;
        }
    }

    list<CacheList::iterator>::iterator dead_it = deadlist.begin();
    list<CacheList::iterator>::iterator dead_it_end = deadlist.end();
    for (; dead_it != dead_it_end; ++dead_it) {
        cache.erase(*dead_it);
    }

}

void ImageControl::createColorTable() {
    Display *disp = FbTk::App::instance()->display();

    grad_xbuffer = grad_ybuffer = (unsigned int *) 0;
    grad_buffer_width = grad_buffer_height = 0;

    int count;
    XPixmapFormatValues *pmv = XListPixmapFormats(disp, &count);

    if (pmv) {
        bits_per_pixel = 0;
        for (int i = 0; i < count; i++) {
            if (pmv[i].depth == m_screen_depth) {
                bits_per_pixel = pmv[i].bits_per_pixel;
                break;
            }
        }

        XFree(pmv);
    }

    if (bits_per_pixel == 0)
        bits_per_pixel = m_screen_depth;
    if (bits_per_pixel >= 24)
        setDither(false);

    red_offset = green_offset = blue_offset = 0;

    switch (visual()->c_class) {
    case TrueColor: {
        int i;

        // compute color tables
        unsigned long red_mask = visual()->red_mask,
            green_mask = visual()->green_mask,
            blue_mask = visual()->blue_mask;

        while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
        while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
        while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

        red_bits = 255 / red_mask;
        green_bits = 255 / green_mask;
        blue_bits = 255 / blue_mask;

        for (i = 0; i < 256; i++) {
            red_color_table[i] = i / red_bits;
            green_color_table[i] = i / green_bits;
            blue_color_table[i] = i / blue_bits;
        }
    }

        break;

    case PseudoColor:
    case StaticColor: {

        m_num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;

        if (m_num_colors > static_cast<unsigned int>(1 << m_screen_depth)) {
            m_colors_per_channel = (1 << m_screen_depth) / 3;
            m_num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;
        }

        if (m_colors_per_channel < 2 || m_num_colors > static_cast<unsigned int>(1 << m_screen_depth)) {
            fprintf(stderr, "ImageControl::ImageControl: invalid colormap size %d "
                    "(%d/%d/%d) - reducing",
                    m_num_colors, m_colors_per_channel, m_colors_per_channel,
                    m_colors_per_channel);

            m_colors_per_channel = (1 << m_screen_depth) / 3;
        }

        m_colors = new XColor[m_num_colors];

        int bits = 256 / m_colors_per_channel;

#ifndef ORDEREDPSEUDO
        bits = 255 / (m_colors_per_channel - 1);
#endif // ORDEREDPSEUDO

        red_bits = green_bits = blue_bits = bits;

        for (unsigned int i = 0; i < 256; i++) {
            red_color_table[i] = green_color_table[i] = blue_color_table[i] =
                i / bits;
        }

        for (int r = 0, i = 0; r < m_colors_per_channel; r++) {
            for (int g = 0; g < m_colors_per_channel; g++) {
                for (int b = 0; b < m_colors_per_channel; b++, i++) {
                    m_colors[i].red = (r * 0xffff) / (m_colors_per_channel - 1);
                    m_colors[i].green = (g * 0xffff) / (m_colors_per_channel - 1);
                    m_colors[i].blue = (b * 0xffff) / (m_colors_per_channel - 1);;
                    m_colors[i].flags = DoRed|DoGreen|DoBlue;
                }
            }
        }

        for (unsigned int i = 0; i < m_num_colors; i++) {
            if (! XAllocColor(disp, m_colormap, &m_colors[i])) {
                fprintf(stderr, "couldn't alloc color %i %i %i\n",
                        m_colors[i].red, m_colors[i].green, m_colors[i].blue);
                m_colors[i].flags = 0;
            } else
                m_colors[i].flags = DoRed|DoGreen|DoBlue;
        }

        XColor icolors[256];
        unsigned int incolors = (((1 << m_screen_depth) > 256) ? 256 : (1 << m_screen_depth));

        for (unsigned int i = 0; i < incolors; i++)
            icolors[i].pixel = i;

        XQueryColors(disp, m_colormap, icolors, incolors);
        for (unsigned int i = 0; i < m_num_colors; i++) {
            if (! m_colors[i].flags) {
                unsigned long chk = 0xffffffff, pixel, close = 0;
                char p = 2;

                while (p--) {
                    for (unsigned int ii = 0; ii < incolors; ii++) {
                        int r = (m_colors[i].red - icolors[i].red) >> 8;
                        int g = (m_colors[i].green - icolors[i].green) >> 8;
                        int b = (m_colors[i].blue - icolors[i].blue) >> 8;
                        pixel = (r * r) + (g * g) + (b * b);

                        if (pixel < chk) {
                            chk = pixel;
                            close = ii;
                        }

                        m_colors[i].red = icolors[close].red;
                        m_colors[i].green = icolors[close].green;
                        m_colors[i].blue = icolors[close].blue;

                        if (XAllocColor(disp, m_colormap,
                                        &m_colors[i])) {
                            m_colors[i].flags = DoRed|DoGreen|DoBlue;
                            break;
                        }
                    }
                }
            }
        }

        break;
    }

    case GrayScale:
    case StaticGray:
        {

            if (visual()->c_class == StaticGray) {
                m_num_colors = 1 << m_screen_depth;
            } else {
                m_num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;

                if (m_num_colors > static_cast<unsigned int>(1 << m_screen_depth)) {
                    m_colors_per_channel = (1 << m_screen_depth) / 3;
                    m_num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;
                }
            }

            if (m_colors_per_channel < 2 || m_num_colors > static_cast<unsigned int>(1 << m_screen_depth)) {
                fprintf(stderr,"FbTk::ImageControl: invalid colormap size %d "
                        "(%d/%d/%d) - reducing",
                        m_num_colors, m_colors_per_channel, m_colors_per_channel,
                        m_colors_per_channel);

                m_colors_per_channel = (1 << m_screen_depth) / 3;
            }

            m_colors = new XColor[m_num_colors];

            int p, bits = 255 / (m_colors_per_channel - 1);
            red_bits = green_bits = blue_bits = bits;

            for (unsigned int i = 0; i < 256; i++)
                red_color_table[i] = green_color_table[i] = blue_color_table[i] =
                    i / bits;

            for (unsigned int i = 0; i < m_num_colors; i++) {
                m_colors[i].red = (i * 0xffff) / (m_colors_per_channel - 1);
                m_colors[i].green = (i * 0xffff) / (m_colors_per_channel - 1);
                m_colors[i].blue = (i * 0xffff) / (m_colors_per_channel - 1);;
                m_colors[i].flags = DoRed|DoGreen|DoBlue;

                if (! XAllocColor(disp, m_colormap,
                                  &m_colors[i])) {
                    fprintf(stderr, "Couldn't alloc color %i %i %i\n",
                            m_colors[i].red, m_colors[i].green, m_colors[i].blue);
                    m_colors[i].flags = 0;
                } else
                    m_colors[i].flags = DoRed|DoGreen|DoBlue;
            }


            XColor icolors[256];
            unsigned int incolors = (((1 << m_screen_depth) > 256) ? 256 :
                                     (1 << m_screen_depth));

            for (unsigned int i = 0; i < incolors; i++)
                icolors[i].pixel = i;

            XQueryColors(disp, m_colormap, icolors, incolors);
            for (unsigned int i = 0; i < m_num_colors; i++) {
                if (! m_colors[i].flags) {
                    unsigned long chk = 0xffffffff, pixel, close = 0;

                    p = 2;
                    while (p--) {
                        for (unsigned int ii = 0; ii < incolors; ii++) {
                            int r = (m_colors[i].red - icolors[i].red) >> 8;
                            int g = (m_colors[i].green - icolors[i].green) >> 8;
                            int b = (m_colors[i].blue - icolors[i].blue) >> 8;
                            pixel = (r * r) + (g * g) + (b * b);

                            if (pixel < chk) {
                                chk = pixel;
                                close = ii;
                            }

                            m_colors[i].red = icolors[close].red;
                            m_colors[i].green = icolors[close].green;
                            m_colors[i].blue = icolors[close].blue;

                            if (XAllocColor(disp, m_colormap, &m_colors[i])) {
                                m_colors[i].flags = DoRed|DoGreen|DoBlue;
                                break;
                            }
                        }
                    }
                }
            }

            break;
        }

    default:
        _FB_USES_NLS;
        cerr<<"FbTk::ImageControl: "<<_FBTK_CONSOLETEXT(Error, UnsupportedVisual, "Unsupported visual", "A visual is a technical term in X")<<endl;
        break;
    }
}

}; // end namespace FbTk
