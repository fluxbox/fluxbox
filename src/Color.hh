// Color.hh for Fluxbox Window Manager 
// Copyright (c) 2002 Henrik Kinnunen (fluxbox@linuxmail.org)
//
// from Image.hh for Blackbox - an X11 Window manager
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

// $Id: Color.hh,v 1.1 2002/07/23 16:23:15 fluxgen Exp $

#ifndef FBTK_COLOR_HH
#define FBTK_COLOR_HH

namespace FbTk {
/**
	Holds rgb color and pixel value
*/
class Color {
public:
	Color(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0):
	m_red(red),	m_green(green), m_blue(blue), m_pixel(0), m_allocated(false) { }

	inline void setAllocated(bool a) { m_allocated = a; }
	inline void setRGB(char red, char green, char blue) { m_red = red; m_green = green; m_blue = blue; }
	inline void setPixel(unsigned long pixel) { m_pixel = pixel; }

	inline bool isAllocated() const { return m_allocated; }

	inline unsigned char red() const { return m_red; }
	inline unsigned char green() const { return m_green; }
	inline unsigned char blue() const { return m_blue; }

	inline unsigned long pixel() const { return m_pixel; }

private:
	unsigned char m_red, m_green, m_blue;
	unsigned long m_pixel;
	bool m_allocated;
};

}; // end namespace FbTk

#endif // FBTK_COLOR_HH
