// DrawUtil.cc for fluxbox 
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: DrawUtil.cc,v 1.10 2002/11/26 15:50:46 fluxgen Exp $

#include "DrawUtil.hh"

namespace DrawUtil {

int doAlignment(int max_width, int bevel, Font::FontJustify justify, 
	const FbTk::Font &font, const char * const text, size_t textlen, size_t &newlen) {

	if (text == 0 || textlen == 0)
		return 0;

	int l = font.textWidth(text, textlen) + bevel;
	size_t dlen = textlen;
	int dx = bevel;
	if (l > max_width) {
		for (; dlen > 0; dlen--) {
			l = font.textWidth(text, dlen) + bevel;
			if (l<=max_width)
				break;
		}
	}

	newlen = dlen;

	switch (justify) {
	case DrawUtil::Font::RIGHT:
		dx = max_width - l - bevel;
	break;
	case DrawUtil::Font::CENTER:
		dx = (max_width - l)/2;
	break;
	case DrawUtil::Font::LEFT:
	break;
	}
	
	return dx;
}

}; //end namespace DrawUtil
