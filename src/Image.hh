// Image.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxbox@linuxmail.org)
//
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// $Id: Image.hh,v 1.10 2002/07/23 17:11:59 fluxgen Exp $

#ifndef	 IMAGE_HH
#define	 IMAGE_HH

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Timer.hh"
#include "BaseDisplay.hh"

#include "Color.hh"
#include "Texture.hh"

#include <list>

class BImageControl;

class BImage {
public:
	BImage(BImageControl *ic, unsigned int, unsigned int);
	~BImage();
	/// render to pixmap
	Pixmap render(const FbTk::Texture *src_texture);
	/// render solid texture to pixmap
	Pixmap renderSolid(const FbTk::Texture *src_texture);
	/// render gradient texture to pixmap
	Pixmap renderGradient(const FbTk::Texture *src_texture);

protected:
	/**
		Render to pixmap
		@return rendered pixmap
	*/
	Pixmap renderPixmap();
	/**
		Render to XImage
		@returns allocated and rendered XImage, user is responsible to deallocate
	*/
	XImage *renderXImage();

	void invert();
	void bevel1();
	void bevel2();
	void dgradient();
	void egradient();
	void hgradient();
	void pgradient();
	void rgradient();
	void vgradient();
	void cdgradient();
	void pcgradient();

private:
	BImageControl *control;

#ifdef		INTERLACE
	bool interlaced;
#endif // INTERLACE

	XColor *colors; // color table

	const FbTk::Color *from, *to;
	int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
		ncolors, cpc, cpccpc;
	unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
	unsigned int width, height, *xtable, *ytable;
};


class BImageControl : public TimeoutHandler {
public:
	BImageControl(BaseDisplay *disp, ScreenInfo *screen, bool = False, int = 4,
		unsigned long = 300000l, unsigned long = 200l);
	virtual ~BImageControl();

	inline BaseDisplay *baseDisplay() { return basedisplay; }

	inline bool doDither() { return dither; }
	inline const Colormap &colormap() const { return m_colormap; }
	inline ScreenInfo *getScreenInfo() { return screeninfo; }

	inline Window drawable() const { return window; }

	inline Visual *visual() { return screeninfo->getVisual(); }

	inline int bitsPerPixel() const { return bits_per_pixel; }
	inline int depth() const { return screen_depth; }
	inline int colorsPerChannel() const	{ return colors_per_channel; }

	unsigned long color(const char *colorname);
	unsigned long color(const char *, unsigned char *, unsigned char *,
												 unsigned char *);
	unsigned long getSqrt(unsigned int val);

	Pixmap renderImage(unsigned int width, unsigned int height,
		const FbTk::Texture *src_texture);

	void installRootColormap();
	void removeImage(Pixmap thepix);
	void colorTables(unsigned char **, unsigned char **, unsigned char **,
			int *, int *, int *, int *, int *, int *);
	void getXColorTable(XColor **, int *);
	void getGradientBuffers(unsigned int, unsigned int,
			unsigned int **, unsigned int **);
	void setDither(bool d) { dither = d; }
	void setColorsPerChannel(int cpc);
	void parseTexture(FbTk::Texture *ret_texture, const char *sval);
	void parseColor(FbTk::Color *ret_color, const char *sval = 0);

	virtual void timeout();

protected:
	/** 
		Search cache for a specific pixmap
		@return None if no cache was found
	*/
	Pixmap searchCache(unsigned int width, unsigned int height, unsigned long texture_type, 
		const FbTk::Color &color, const FbTk::Color &color_to);

private:
	bool dither;
	BaseDisplay *basedisplay;
	ScreenInfo *screeninfo;
#ifdef		TIMEDCACHE
	BTimer timer;
#endif // TIMEDCACHE

	Colormap m_colormap;

	Window window;
	XColor *colors; // color table
	int colors_per_channel, ncolors, screen_number, screen_depth,
		bits_per_pixel, red_offset, green_offset, blue_offset,
		red_bits, green_bits, blue_bits;

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

	CacheList cache;
};


#endif // __Image_hh

