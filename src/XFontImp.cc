// XFontImp.cc for FbTk fluxbox toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: XFontImp.cc,v 1.1 2002/10/13 22:22:14 fluxgen Exp $

#include "XFontImp.hh"
#include "BaseDisplay.hh"

#include <iostream>
using namespace std;

XFontImp::XFontImp(const char *fontname):m_fontstruct(0) {
	if (fontname != 0)
		load(fontname);	
}

XFontImp::~XFontImp() {
	if (m_fontstruct != 0)
		XFreeFont(BaseDisplay::getXDisplay(), m_fontstruct);
}

bool XFontImp::load(const std::string &fontname) {
	XFontStruct *font = XLoadQueryFont(BaseDisplay::getXDisplay(), fontname.c_str());
	if (font == 0)
		return false;
	if (m_fontstruct != 0) // free old font struct, if any
		XFreeFont(BaseDisplay::getXDisplay(), m_fontstruct);

	m_fontstruct = font; //set new font
	return true;
}

void XFontImp::drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const {
	if (m_fontstruct == 0)
		return;
	Display *disp = BaseDisplay::getXDisplay();
	XSetFont(disp, gc, m_fontstruct->fid);
	XDrawString(disp, w, gc, x, y, text, len);
}

unsigned int XFontImp::textWidth(const char * const text, unsigned int size) const {
	if (m_fontstruct == 0)
		return 0;
	return XTextWidth(m_fontstruct, text, size);
}

unsigned int XFontImp::height() const {
	if (m_fontstruct == 0)
		return 0;

	return m_fontstruct->ascent + m_fontstruct->descent;
}
