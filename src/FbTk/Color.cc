// Color.cc for Fluxbox window manager
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: Color.cc,v 1.8 2004/01/09 21:36:21 fluxgen Exp $

#include "Color.hh"

#include "App.hh"
#include "StringUtil.hh"

#include <iostream>
using namespace std;

namespace {
unsigned char maxValue(unsigned short colval) {
    if (colval == 65535) 
        return 0xFF;

    return static_cast<unsigned char>(colval/0xFF);
}

};

namespace FbTk {

Color::Color():
    m_red(0), m_green(0), m_blue(0),
    m_pixel(0),
    m_allocated(false),
    m_screen(0) {

}

Color::Color(const Color &col_copy):
    m_red(0), m_green(0), m_blue(0),
    m_pixel(0),
    m_allocated(false), m_screen(0) {
    copy(col_copy);
}

Color::Color(unsigned short red, unsigned short green, unsigned short blue, int screen):
    m_red(red),	m_green(green), m_blue(blue), 
    m_pixel(0), m_allocated(false),
    m_screen(screen) { 
    allocate(red, green, blue, screen);
}

Color::Color(const char *color_string, int screen):
    m_red(0), m_green(0), m_blue(0),
    m_pixel(0),
    m_allocated(false),
    m_screen(screen) {
    setFromString(color_string, screen);
}

Color::~Color() {
    free();
}

bool Color::setFromString(const char *color_string, int screen) {

    if (color_string == 0) {
        free();
        return false;
    }
    string color_string_tmp = color_string;
    StringUtil::removeFirstWhitespace(color_string_tmp);
    StringUtil::removeTrailingWhitespace(color_string_tmp);

    Display *disp = App::instance()->display();
    Colormap colm = DefaultColormap(disp, screen);

    XColor color;

    if (! XParseColor(disp, colm, color_string_tmp.c_str(), &color))
        return false;
    else if (! XAllocColor(disp, colm, &color))
        return false;

    setPixel(color.pixel);
    setRGB(maxValue(color.red), 
           maxValue(color.green),
           maxValue(color.blue));
    setAllocated(true);
    m_screen = screen;

    return true;
}

/*
  Color &Color::Color::operator  = (const Color &col_copy) {
  // check for aliasing
  if (this == &col_copy)
  return *this;

  copy(col_copy);
  return *this;
  }
*/

void Color::free() {
    if (isAllocated()) {
        unsigned long pixel = m_pixel;
        Display *disp = App::instance()->display();
        XFreeColors(disp, DefaultColormap(disp, m_screen), &pixel, 1, 0);
        setPixel(0);
        setRGB(0, 0, 0);
        setAllocated(false);
    }	
}

void Color::copy(const Color &col_copy) {
    if (!col_copy.isAllocated()) {
        free();
        setRGB(col_copy.red(), col_copy.green(), col_copy.blue());
        setPixel(col_copy.pixel());
        return;
    }

    free();
		
    allocate(col_copy.red(), 
             col_copy.green(),
             col_copy.blue(),
             col_copy.m_screen);
	
}

void Color::allocate(unsigned short red, unsigned short green, unsigned short blue, int screen) {

    Display *disp = App::instance()->display();
    XColor color;
    // fill xcolor structure
    color.red = red;
    color.green = green;	
    color.blue = blue;
	
	
    if (!XAllocColor(disp, DefaultColormap(disp, screen), &color)) {
        cerr<<"FbTk::Color: Allocation error."<<endl;
    } else {
        setRGB(maxValue(color.red),
               maxValue(color.green),
               maxValue(color.blue));
        setPixel(color.pixel);
        setAllocated(true);
    }
	
    m_screen = screen;
}

void Color::setRGB(unsigned short red, unsigned short green, unsigned short blue) {
    m_red = red;
    m_green = green;
    m_blue = blue;
}

};
