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

//$Id: Font.hh,v 1.2 2002/03/27 15:37:19 fluxgen Exp $

#ifndef FBTK_FONT_HH
#define FBTK_FONT_HH

#include <X11/Xlib.h>
#include <X11/Xresource.h>

namespace FbTk
{

class Font
{
public:
	Font(Display *display, const char *name=0);
	virtual ~Font();
	//manipulators
	bool load(const char *name);
	bool loadFromDatabase(XrmDatabase &database, const char *rname, const char *rclass);
	//accessors
	inline bool isLoaded() const { return m_loaded; }
	inline XFontStruct *getFontStruct() const { return m_font.fontstruct; }
	inline XFontSet &getFontSet() { return m_font.set; }
	inline XFontSetExtents *getFontSetExtents() const { return m_font.set_extents; }
	static inline bool multibyte() { return m_multibyte; }
	unsigned int getTextWidth(const char *text, unsigned int size) const;
	unsigned int getHeight() const;
	Display *getDisplay() const { return m_display; }
private:
	void freeFont();
	static XFontSet createFontSet(Display *display, const char *fontname);
	static const char *getFontSize(const char *pattern, int *size);
	static const char *getFontElement(const char *pattern, char *buf, int bufsiz, ...);

	struct FontType
	{
		XFontSet set;
		XFontSetExtents *set_extents;
		XFontStruct *fontstruct;
	} m_font;

	bool m_loaded;
	static bool m_multibyte;
	Display *m_display;
};

}; //end namespace FbTk

#endif //FBTK_FONT_HH
