// ImageControl.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxbox at users.sourceforge.net)
//
// from Image.hh for Blackbox - an X11 Window manager
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

// $Id: ImageControl.hh,v 1.1 2003/01/09 21:09:49 fluxgen Exp $

#ifndef	 FBTK_IMAGECONTROL_HH
#define	 FBTK_IMAGECONTROL_HH

#include "Texture.hh"
#include "Timer.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <list>

namespace FbTk {

///	Holds screen info, color tables and caches textures
class ImageControl : public TimeoutHandler {
public:
    ImageControl(int screen_num, bool dither = false, int colors_per_channel = 4,
                  unsigned long cache_timeout = 300000l, unsigned long cache_max = 200l);
    virtual ~ImageControl();

    inline bool doDither() const { return m_dither; }
    inline int bitsPerPixel() const { return bits_per_pixel; }
    inline int depth() const { return m_screen_depth; }
    inline int colorsPerChannel() const	{ return m_colors_per_channel; }
    int screenNum() const { return m_screen_num; }
    Visual *visual() const { return m_visual; }
    unsigned long getSqrt(unsigned int val) const;

    /**
       Render to pixmap
       @param width width of pixmap
       @param height height of pixmap
       @param src_texture texture type to render
       @return pixmap of the rendered image, on failure None
    */
    Pixmap renderImage(unsigned int width, unsigned int height,
                       const FbTk::Texture &src_texture);

    void installRootColormap();
    void removeImage(Pixmap thepix);
    void colorTables(const unsigned char **, const unsigned char **, const unsigned char **,
                     int *, int *, int *, int *, int *, int *) const;
    void getXColorTable(XColor **, int *);
    void getGradientBuffers(unsigned int, unsigned int,
                            unsigned int **, unsigned int **);
    void setDither(bool d) { m_dither = d; }
    void setColorsPerChannel(int cpc);

    virtual void timeout();

private:
    /** 
        Search cache for a specific pixmap
        @return None if no cache was found
    */
    Pixmap searchCache(unsigned int width, unsigned int height, unsigned long texture_type, 
                       const FbTk::Color &color, const FbTk::Color &color_to) const;

    void createColorTable();
    bool m_dither;

    Timer m_timer;

    Colormap m_colormap;

    Window m_root_window;

    XColor *m_colors; ///< color table
    unsigned int m_num_colors; ///< number of colors in color table

    Visual *m_visual;

    int bits_per_pixel, red_offset, green_offset, blue_offset,
        red_bits, green_bits, blue_bits;
    int m_colors_per_channel; ///< number of colors per channel
    int m_screen_depth; ///< bit depth of screen
    int m_screen_num;  ///< screen number
    unsigned char red_color_table[256], green_color_table[256],
        blue_color_table[256];
    unsigned int *grad_xbuffer, *grad_ybuffer, grad_buffer_width,
        grad_buffer_height;

    static unsigned long *sqrt_table; /// sqrt lookup table

    typedef struct Cache {
        Pixmap pixmap;

        unsigned int count, width, height;
        unsigned long pixel1, pixel2, texture;
    } Cache;
	
    unsigned long cache_max;
    typedef std::list<Cache *> CacheList;

    mutable CacheList cache;
};

}; // end namespace FbTk

#endif // FBTK_IMAGECONTROL_HH

