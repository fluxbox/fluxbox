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

// $Id: Image.hh,v 1.17 2002/11/27 22:03:00 fluxgen Exp $

#ifndef	 IMAGE_HH
#define	 IMAGE_HH


#include "BaseDisplay.hh"
#include "Color.hh"
#include "Texture.hh"
#include "Timer.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <list>

class BImageControl;

/**
	Renders to pixmap
*/
class BImage {
public:
	BImage(BImageControl *ic, unsigned int width, unsigned int height);
	~BImage();
	/// render to pixmap
	Pixmap render(const FbTk::Texture &src_texture);
	/// render solid texture to pixmap
	Pixmap renderSolid(const FbTk::Texture &src_texture);
	/// render gradient texture to pixmap
	Pixmap renderGradient(const FbTk::Texture &src_texture);

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
	/**
		@name render functions
	*/
	//@{
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
	//@}

private:
	BImageControl *control;
	bool interlaced;

	XColor *colors; // color table

	const FbTk::Color *from, *to;
	int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
		ncolors, cpc, cpccpc;
	unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
	unsigned int width, height, *xtable, *ytable;
};

/**
	Holds screen info and color tables	
*/
class BImageControl : public TimeoutHandler {
public:
	BImageControl(const ScreenInfo *screen, bool dither = false, int colors_per_channel = 4,
		unsigned long cache_timeout = 300000l, unsigned long cache_max = 200l);
	virtual ~BImageControl();

	inline bool doDither() const { return m_dither; }
	inline Colormap colormap() const { return m_colormap; }
	inline const ScreenInfo *getScreenInfo() const { return screeninfo; }
	inline Window drawable() const { return window; }
	
	/// @return visual of screen
	inline Visual *visual() { return screeninfo->getVisual(); }
	/// @return Bits per pixel of screen
	inline int bitsPerPixel() const { return bits_per_pixel; }
	/// @return depth of screen
	inline int depth() const { return screen_depth; }
	inline int colorsPerChannel() const	{ return colors_per_channel; }

	unsigned long getSqrt(unsigned int val);
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
	void colorTables(unsigned char **, unsigned char **, unsigned char **,
			int *, int *, int *, int *, int *, int *);
	void getXColorTable(XColor **, int *);
	void getGradientBuffers(unsigned int, unsigned int,
			unsigned int **, unsigned int **);
	void setDither(bool d) { m_dither = d; }
	void setColorsPerChannel(int cpc);

	virtual void timeout();

protected:
	/** 
		Search cache for a specific pixmap
		@return None if no cache was found
	*/
	Pixmap searchCache(unsigned int width, unsigned int height, unsigned long texture_type, 
		const FbTk::Color &color, const FbTk::Color &color_to);

private:
	bool m_dither;
	const ScreenInfo *screeninfo;

	BTimer m_timer;

	Colormap m_colormap;

	Window window;
	XColor *colors; ///< color table

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


#endif // IMAGE_HH

