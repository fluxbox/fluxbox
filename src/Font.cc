// Font.cc
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

//$Id: Font.cc,v 1.6 2002/05/02 07:19:02 fluxgen Exp $


#include "Font.hh"

#include "StringUtil.hh"
//use gnu extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif //_GNU_SOURCE

#include <cstdarg>
#include <iostream> 
#include <cassert>
#include <string>
#include <cstdio>

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H
#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif //HAVE_SETLOCALE
#include "i18n.hh"

namespace FbTk
{

bool Font::m_multibyte = false; //TODO: fix multibyte

Font::Font(Display *display, const char *name):m_loaded(false), 
m_display(display) {
	m_font.fontstruct = 0;
	m_font.set_extents = 0;
	m_font.set = 0;
	//TODO: should only be done once
	m_multibyte = I18n::instance()->multibyte();
	if (name!=0) {
		load(name);
	}

}

Font::~Font() {
	freeFont();
}

bool Font::load(const char *name) {
	if (m_multibyte) {
		XFontSet set = createFontSet(m_display, name);
		if (!set)
			return false;
		freeFont();
		
		m_font.set = set;		
		m_font.set_extents = XExtentsOfFontSet(m_font.set);

	} else {
		XFontStruct *font = XLoadQueryFont(m_display, name);
		if (font==0)
			return false;
		freeFont(); //free old font
		m_font.fontstruct = font; //set new font
	}	

	m_loaded = true; //mark the font loaded
	
	return true;
}

bool Font::loadFromDatabase(XrmDatabase &database, const char *rname, const char *rclass) {
	assert(rname);
	assert(rclass);

	XrmValue value;
	char *value_type;

	//this should probably be moved to a Database class so we can keep
	//track of database init	
	
	if (XrmGetResource(database, rname, rclass, &value_type, &value)) {
		#ifdef DEBUG
		std::cerr<<__FILE__<<"("<<__LINE__<<"): Load font:"<<value.addr<<std::endl;
		#endif
		return load(value.addr);		
	}
	
	return false;

}
unsigned int Font::getTextWidth(const char *text, unsigned int size) const {
	if (text==0)
		return 0;
	if (multibyte()) {
		XRectangle ink, logical;
		XmbTextExtents(m_font.set, text, size,
			&ink, &logical);
		return logical.width;
	} else {
		assert(m_font.fontstruct);
		return XTextWidth(m_font.fontstruct, 
			text, size);
	}
	
	return 0;
}

unsigned int Font::getHeight() const {
	if (multibyte() && getFontSetExtents())
		return getFontSetExtents()->max_ink_extent.height;
	if (getFontStruct())
		return getFontStruct()->ascent + getFontStruct()->descent;
	return 0;
}

XFontSet Font::createFontSet(Display *display, const char *fontname) {
	XFontSet fs;
	const int FONT_ELEMENT_SIZE=50;
	char **missing, *def = "-";
	int nmissing, pixel_size = 0, buf_size = 0;
	char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

	fs = XCreateFontSet(display,
					fontname, &missing, &nmissing, &def);
	if (fs && (! nmissing)) return fs;

#ifdef HAVE_SETLOCALE
	if (! fs) {
		if (nmissing) XFreeStringList(missing);

		setlocale(LC_CTYPE, "C");
		fs = XCreateFontSet(display, fontname,
			&missing, &nmissing, &def);
		setlocale(LC_CTYPE, "");
	}
#endif // HAVE_SETLOCALE

	if (fs) {
		XFontStruct **fontstructs;
		char **fontnames;
		XFontsOfFontSet(fs, &fontstructs, &fontnames);
		fontname = fontnames[0];
	}

	getFontElement(fontname, weight, FONT_ELEMENT_SIZE,
		 "-medium-", "-bold-", "-demibold-", "-regular-", 0);
	getFontElement(fontname, slant, FONT_ELEMENT_SIZE,
		 "-r-", "-i-", "-o-", "-ri-", "-ro-", 0);
	getFontSize(fontname, &pixel_size);

	if (! strcmp(weight, "*")) 
		std::strncpy(weight, "medium", FONT_ELEMENT_SIZE);
	if (! strcmp(slant, "*")) 
		std::strncpy(slant, "r", FONT_ELEMENT_SIZE);
	if (pixel_size < 3) 
		pixel_size = 3;
	else if (pixel_size > 97) 
		pixel_size = 97;

	buf_size = strlen(fontname) + (FONT_ELEMENT_SIZE * 2) + 64;
	char *pattern2 = new char[buf_size];
	snprintf(pattern2, buf_size - 1,
		 "%s,"
		 "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
		 "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
		 fontname, weight, slant, pixel_size, pixel_size);
	fontname = pattern2;

	if (nmissing)
		XFreeStringList(missing);
	if (fs)
		XFreeFontSet(display, fs);

	fs = XCreateFontSet(display, fontname,
					&missing, &nmissing, &def);
	delete [] pattern2;

	return fs;
}

const char *Font::getFontElement(const char *pattern, char *buf, int bufsiz, ...) {
	const char *p, *v;
	char *p2;
	va_list va;

	va_start(va, bufsiz);
	buf[bufsiz-1] = 0;
	buf[bufsiz-2] = '*';
	while((v = va_arg(va, char *)) != 0) {
		p = StringUtil::strcasestr(pattern, v);
		if (p) {
			std::strncpy(buf, p+1, bufsiz-2);
			p2 = strchr(buf, '-');
			if (p2) *p2=0;
			va_end(va);
			return p;
		}
	}
	va_end(va);
	std::strncpy(buf, "*", bufsiz);
	return 0;
}

const char *Font::getFontSize(const char *pattern, int *size) {
	const char *p;
	const char *p2=0;
	int n=0;

	for (p=pattern; 1; p++) {
		if (!*p) {
			if (p2!=0 && n>1 && n<72) {
				*size = n; return p2+1;
			} else {
				*size = 16; return 0;
			}
		} else if (*p=='-') {
			if (n>1 && n<72 && p2!=0) {
				*size = n;
				return p2+1;
			}
			p2=p; n=0;
		} else if (*p>='0' && *p<='9' && p2!=0) {
			n *= 10;
			n += *p-'0';
		} else {
			p2=0; n=0;
		}
	}
}

void Font::freeFont() {
	//free memory
	if (m_font.fontstruct!=0)
		XFreeFont(m_display, m_font.fontstruct);
	if (m_font.set)
		XFreeFontSet(m_display, m_font.set);
		
	//clear struct
	m_font.fontstruct = 0;
	m_font.set = 0;
	m_font.set_extents = 0;

	m_loaded = false; //mark the font not loaded
}

};
