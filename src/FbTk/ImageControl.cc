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

#include "ImageControl.hh"

#include "TextureRender.hh"
#include "Texture.hh"
#include "App.hh"
#include "SimpleCommand.hh"
#include "I18n.hh"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
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

namespace { // anonymous

#ifdef TIMEDCACHE
bool s_timed_cache = true;
#else
bool s_timed_cache = false;
#endif // TIMEDCACHE


void initColortables(unsigned char red[256], unsigned char green[256], unsigned char blue[256],
      int red_bits, int green_bits, int blue_bits) {

    for (unsigned int i = 0; i < 256; i++) {
        red[i] = i / red_bits;
        green[i] = i / green_bits;
        blue[i] = i / blue_bits;
    }
}

// tries to allocate all unallocated 'colors' by finding a close color based
// upon entries in the colormap.
//
void allocateUnallocatedColors(std::vector<XColor> colors, Display* dpy, Colormap cmap, int screen_depth) {

    unsigned int i;

    bool done = true;

    // first run, just try to allocate the colors
    for (i = 0; i < colors.size(); i++) {

        if (colors[i].flags == 0) {
            if (! XAllocColor(dpy, cmap, &colors[i])) {
                fprintf(stderr, "couldn't alloc color %i %i %i\n",
                        colors[i].red, colors[i].green, colors[i].blue);
                colors[i].flags = 0;
                done = false;
            } else
                colors[i].flags = DoRed|DoGreen|DoBlue;
        }
    }

    if (done)
        return;

    // 'icolors' will hold the first 'nr_icolors' colors of the
    // given (indexed) colormap.
    const size_t nr_icolors = std::min(256, 1 << screen_depth);
    std::vector<XColor> icolors(nr_icolors);

    // give each icolor an index
    for (i = 0; i < nr_icolors; i++)
        icolors[i].pixel = i;

    // query the colors of the colormap and store them into 'icolors'
    XQueryColors(dpy, cmap, &icolors[0], nr_icolors);

    // try to find a close color for all not allocated colors
    for (i = 0; i < colors.size(); i++) {

        if (colors[i].flags == 0) { // color is not allocated

            unsigned long chk = 0xffffffff;
            unsigned long close = 0;

            // iterate over the indexed colors 'icolors' and find
            // a close color. 
            //
            // 2 passes to improve the result of the first pass

            char pass = 2;
            while (pass--) {
                for (unsigned int ii = 0; ii < nr_icolors; ii++) {

                    int r = (colors[i].red - icolors[i].red) >> 8;
                    int g = (colors[i].green - icolors[i].green) >> 8;
                    int b = (colors[i].blue - icolors[i].blue) >> 8;
                    unsigned long pixel = (r * r) + (g * g) + (b * b);

                    if (pixel < chk) {
                        chk = pixel;
                        close = ii;
                    }

                    // store the indexed color
                    colors[i].red = icolors[close].red;
                    colors[i].green = icolors[close].green;
                    colors[i].blue = icolors[close].blue;

                    // try to allocate it
                    if (XAllocColor(dpy, cmap, &colors[i])) {
                        colors[i].flags = DoRed|DoGreen|DoBlue; // mark it allocated
                        break;
                    }
                }
            }
        }
    }
}


} // end anonymous namespace

struct ImageControl::Cache {
    Pixmap pixmap;
    Pixmap texture_pixmap;
    Orientation orient;
    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
};

ImageControl::ImageControl(int screen_num,
                           int cpc, unsigned long cache_timeout, unsigned long cmax):
    m_colors_per_channel(cpc),
    m_screen_num(screen_num) {

    Display *disp = FbTk::App::instance()->display();

    m_screen_depth = DefaultDepth(disp, screen_num);
    m_visual = DefaultVisual(disp, screen_num);
    m_colormap = DefaultColormap(disp, screen_num);

    cache_max = cmax;

    if (cache_timeout && s_timed_cache) {
        m_timer.setTimeout(cache_timeout * FbTk::FbTime::IN_MILLISECONDS);
        RefCount<Command<void> > clean_cache(new SimpleCommand<ImageControl>(*this, &ImageControl::cleanCache));
        m_timer.setCommand(clean_cache);
        m_timer.start();
    }

    createColorTable();
}


ImageControl::~ImageControl() {

    Display *disp = FbTk::App::instance()->display();

    if (!m_colors.empty()) {
        std::vector<unsigned long> pixels(m_colors.size());

        for (unsigned int i = 0; i < m_colors.size(); i++)
            pixels[i] = m_colors[i].pixel;

        XFreeColors(disp, m_colormap, &pixels[0], pixels.size(), 0);
    }

    if (!cache.empty()) {
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
                                 FbTk::Orientation orient,
                                 bool use_cache ) {

    if (texture.type() & FbTk::Texture::PARENTRELATIVE)
        return ParentRelative;

    // If we are not suppose to cache this pixmap, just render and return it
    if ( ! use_cache) {
        TextureRender image(*this, width, height, orient);
        return image.render(texture);
    }

    // search cache first
    Pixmap pixmap = searchCache(width, height, texture, orient);
    if (pixmap) {
        return pixmap; // return cache item
    }

    // render new image

    TextureRender image(*this, width, height, orient);
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

void ImageControl::getGradientBuffers(unsigned int w,
                                      unsigned int h,
                                      unsigned int **xbuf,
                                      unsigned int **ybuf) {

    if (w > grad_xbuffer.size())
        grad_xbuffer.resize(w);

    if (h > grad_ybuffer.size())
        grad_ybuffer.resize(h);

    *xbuf = &grad_xbuffer[0];
    *ybuf = &grad_ybuffer[0];
}


void ImageControl::installRootColormap() {

    Display *disp = FbTk::App::instance()->display();
    XGrabServer(disp);

    bool install = true;
    int i = 0, ncmap = 0;
    Colormap *cmaps =
        XListInstalledColormaps(disp, RootWindow(disp, screenNumber()), &ncmap);

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

    red_offset = green_offset = blue_offset = 0;

    switch (visual()->c_class) {
    case TrueColor: {
        unsigned long red_mask = visual()->red_mask,
            green_mask = visual()->green_mask,
            blue_mask = visual()->blue_mask;

        while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
        while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
        while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

        red_bits = 255 / red_mask;
        green_bits = 255 / green_mask;
        blue_bits = 255 / blue_mask;

        initColortables(red_color_table, green_color_table, blue_color_table,
                red_bits, green_bits, blue_bits);
    }

        break;

    case PseudoColor:
    case StaticColor: {

        size_t num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;

        if (num_colors > static_cast<unsigned>(1 << m_screen_depth)) {
            m_colors_per_channel = (1 << m_screen_depth) / 3;
            num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;
        }

        if (m_colors_per_channel < 2 || num_colors > static_cast<unsigned>(1 << m_screen_depth)) {
            fprintf(stderr, "ImageControl::ImageControl: invalid colormap size %zd "
                    "(%d/%d/%d) - reducing",
                    num_colors, m_colors_per_channel, m_colors_per_channel,
                    m_colors_per_channel);

            m_colors_per_channel = (1 << m_screen_depth) / 3;
        }

        m_colors.resize(num_colors);

        int bits = 255 / (m_colors_per_channel - 1);
        red_bits = green_bits = blue_bits = bits;

        initColortables(red_color_table, green_color_table, blue_color_table,
                red_bits, green_bits, blue_bits);

        for (int r = 0, i = 0; r < m_colors_per_channel; r++) {
            for (int g = 0; g < m_colors_per_channel; g++) {
                for (int b = 0; b < m_colors_per_channel; b++, i++) {
                    m_colors[i].red = (r * 0xffff) / (m_colors_per_channel - 1);
                    m_colors[i].green = (g * 0xffff) / (m_colors_per_channel - 1);
                    m_colors[i].blue = (b * 0xffff) / (m_colors_per_channel - 1);
                    m_colors[i].flags = DoRed|DoGreen|DoBlue;
                }
            }
        }

        allocateUnallocatedColors(m_colors, disp, m_colormap, m_screen_depth);
        break;
    }

    case GrayScale:
    case StaticGray:
        {
            size_t num_colors;

            if (visual()->c_class == StaticGray) {
                num_colors = 1 << m_screen_depth;
            } else {
                num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;

                if (num_colors > static_cast<unsigned>(1 << m_screen_depth)) {
                    m_colors_per_channel = (1 << m_screen_depth) / 3;
                    num_colors = m_colors_per_channel * m_colors_per_channel * m_colors_per_channel;
                }
            }

            if (m_colors_per_channel < 2 || num_colors > static_cast<unsigned>(1 << m_screen_depth)) {
                fprintf(stderr,"FbTk::ImageControl: invalid colormap size %zd "
                        "(%d/%d/%d) - reducing",
                        num_colors, m_colors_per_channel, m_colors_per_channel,
                        m_colors_per_channel);

                m_colors_per_channel = (1 << m_screen_depth) / 3;
            }

            m_colors.resize(num_colors);

            int bits = 255 / (m_colors_per_channel - 1);
            red_bits = green_bits = blue_bits = bits;

            initColortables(red_color_table, green_color_table, blue_color_table,
                red_bits, green_bits, blue_bits);

            for (unsigned int i = 0; i < num_colors; i++) {
                m_colors[i].red = (i * 0xffff) / (m_colors_per_channel - 1);
                m_colors[i].green = (i * 0xffff) / (m_colors_per_channel - 1);
                m_colors[i].blue = (i * 0xffff) / (m_colors_per_channel - 1);
                m_colors[i].flags = DoRed|DoGreen|DoBlue;
            }

            allocateUnallocatedColors(m_colors, disp, m_colormap, m_screen_depth);
            break;
        }

    default:
        _FB_USES_NLS;
        cerr<<"FbTk::ImageControl: "<<_FBTK_CONSOLETEXT(Error, UnsupportedVisual, "Unsupported visual", "A visual is a technical term in X")<<endl;
        break;
    }
}

} // end namespace FbTk
