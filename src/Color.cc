// Color.cc for Fluxbox window manager
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@users.sourceforge.net)
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

// $Id: Color.cc,v 1.1 2002/09/14 12:47:50 fluxgen Exp $

#include "Color.hh"

namespace {
unsigned char maxValue(unsigned short colval) {
	if (colval == 65535) 
		return 0xFF;

	return static_cast<unsigned char>(colval/0xFF);
}

};

namespace FbTk {
Color::Color():
m_allocated(false) {

}

Color::Color(unsigned char red, unsigned char green, unsigned char blue, int screen):
m_red(red),	m_green(green), m_blue(blue), 
m_pixel(0), m_allocated(false) { 
	
	Display *disp = BaseDisplay::getXDisplay();
	XColor color;
	// fill xcolor structure
	color.red = red;
	color.blue = blue;
	color.green = green;
	
	if (!XAllocColor(disp, DefaultColormap(disp, screen), &color)) {
		cerr<<"FbTk::Color: Allocation error."<<endl;
	} else {
		setRGB(maxValue(color.red),
			maxValue(color.green),
			maxValue(color.blue));
		setPixel(color.pixel);
		setAllocated(true);
	}
}

Color::Color(const char *color_string, int screen):
m_allocated(false) {
	setFromString(color_string, screen);
}

bool Color::setFromString(const char *color_string, int screen) {

	if (color_string == 0) {
		free();
		return false;
	}

	Display *disp = BaseDisplay::instance()->getXDisplay();
	Colormap colm = DefaultColormap(disp, screen);

	XColor color;

	if (! XParseColor(disp, colm, color_string, &color))
		cerr<<"FbTk::Color: Parse color error: \""<<color_string<<"\""<<endl;
	else if (! XAllocColor(disp, colm, &color))
		cerr<<"FbTk::Color: Allocation error: \""<<color_string<<"\""<<endl;



	setPixel(color.pixel);
	setRGB(maxValue(color.red), 
		maxValue(color.green),
		maxValue(color.blue));
	setAllocated(true);

	return true;
}

void Color::free() {
	if (isAllocated()) {
		unsigned long pixel = col.pixel();
		XFreeColors(disp, colm, &pixel, 1, 0);
		setPixel(0);
		setRGB(0, 0, 0);
		setAllocated(false);
	}	
}

};
