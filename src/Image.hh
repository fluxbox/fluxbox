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

// $Id: Image.hh,v 1.8 2002/07/19 20:33:15 fluxgen Exp $

#ifndef	 IMAGE_HH
#define	 IMAGE_HH

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Timer.hh"
#include "BaseDisplay.hh"

#include <list>

class BImage;
class BImageControl;


class BColor {
public:
	BColor(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0):
	m_red(red),	m_green(green), m_blue(blue), m_pixel(0), m_allocated(false) { }

	inline int isAllocated() const { return m_allocated; }

	inline unsigned char red() const { return m_red; }
	inline unsigned char green() const { return m_green; }
	inline unsigned char blue() const { return m_blue; }

	inline unsigned long pixel() const { return m_pixel; }

	inline void setAllocated(bool a) { m_allocated = a; }
	inline void setRGB(char red, char green, char blue) { m_red = red; m_green = green; m_blue = blue; }
	inline void setPixel(unsigned long pixel) { m_pixel = pixel; }

private:
	unsigned char m_red, m_green, m_blue;
	unsigned long m_pixel;
	bool m_allocated;
};


class BTexture {
public:
	BTexture():m_texture(0) { }

	inline const BColor &color() const { return m_color; }
	inline const BColor &colorTo() const { return m_color_to; }
	inline const BColor &hiColor() const { return m_hicolor; }
	inline const BColor &loColor() const { return m_locolor; }

	inline BColor &color() { return m_color; }
	inline BColor &colorTo() { return m_color_to; }
	inline BColor &hiColor() { return m_hicolor; }
	inline BColor &loColor() { return m_locolor; }

	inline unsigned long getTexture() const { return m_texture; }

	inline void setTexture(unsigned long t) { m_texture = t; }
	inline void addTexture(unsigned long t) { m_texture |= t; }

private:
	BColor m_color, m_color_to, m_hicolor, m_locolor;
	unsigned long m_texture;
};



class BImage {
private:
	BImageControl *control;

#ifdef		INTERLACE
	Bool interlaced;
#endif // INTERLACE

	XColor *colors;

	BColor *from, *to;
	int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
		ncolors, cpc, cpccpc;
	unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
	unsigned int width, height, *xtable, *ytable;


protected:
	Pixmap renderPixmap(void);

	XImage *renderXImage(void);

	void invert(void);
	void bevel1(void);
	void bevel2(void);
	void dgradient(void);
	void egradient(void);
	void hgradient(void);
	void pgradient(void);
	void rgradient(void);
	void vgradient(void);
	void cdgradient(void);
	void pcgradient(void);


public:
	enum Bevel{FLAT=0x00002, SUNKEN=0x00004, RAISED=0x00008};
	enum Textures{SOLID=0x00010, GRADIENT=0x00020};
	enum Gradients{
		HORIZONTAL=0x00040,
		VERTICAL=0x00080,
		DIAGONAL=0x00100,
		CROSSDIAGONAL=0x00200,
		RECTANGLE=0x00400,
		PYRAMID=0x00800,
		PIPECROSS=0x01000,
		ELLIPTIC=0x02000		
	};
	
	enum {
		BEVEL1=0x04000, BEVEL2=0x08000, // bevel types
		INVERT=0x010000, //inverted image
		PARENTRELATIVE=0x20000,
		INTERLACED=0x40000
	};
	
	BImage(BImageControl *, unsigned int, unsigned int);
	~BImage(void);

	Pixmap render(BTexture *);
	Pixmap render_solid(BTexture *);
	Pixmap render_gradient(BTexture *);
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

	inline const Window &drawable() const { return window; }

	inline Visual *visual() { return screeninfo->getVisual(); }

	inline int bitsPerPixel() const { return bits_per_pixel; }
	inline int depth() const { return screen_depth; }
	inline int colorsPerChannel(void) const	{ return colors_per_channel; }

	unsigned long color(const char *colorname);
	unsigned long color(const char *, unsigned char *, unsigned char *,
												 unsigned char *);
	unsigned long getSqrt(unsigned int);

	Pixmap renderImage(unsigned int, unsigned int, BTexture *);

	void installRootColormap();
	void removeImage(Pixmap thepix);
	void colorTables(unsigned char **, unsigned char **, unsigned char **,
											int *, int *, int *, int *, int *, int *);
	void getXColorTable(XColor **, int *);
	void getGradientBuffers(unsigned int, unsigned int,
													unsigned int **, unsigned int **);
	void setDither(Bool d) { dither = d; }
	void setColorsPerChannel(int);
	void parseTexture(BTexture *, char *);
	void parseColor(BColor *, char * = 0);

	virtual void timeout(void);

private:
	Bool dither;
	BaseDisplay *basedisplay;
	ScreenInfo *screeninfo;
#ifdef		TIMEDCACHE
	BTimer timer;
#endif // TIMEDCACHE

	Colormap m_colormap;

	Window window;
	XColor *colors;
	int colors_per_channel, ncolors, screen_number, screen_depth,
		bits_per_pixel, red_offset, green_offset, blue_offset,
		red_bits, green_bits, blue_bits;
	unsigned char red_color_table[256], green_color_table[256],
		blue_color_table[256];
	unsigned int *grad_xbuffer, *grad_ybuffer, grad_buffer_width,
		grad_buffer_height;
	unsigned long *sqrt_table, cache_max;

	typedef struct Cache {
		Pixmap pixmap;

		unsigned int count, width, height;
		unsigned long pixel1, pixel2, texture;
	} Cache;

	typedef std::list<Cache *> CacheList;

	CacheList cache;

protected:
	Pixmap searchCache(unsigned int, unsigned int, unsigned long, BColor *, BColor *);
};


#endif // __Image_hh

