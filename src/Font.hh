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

//$Id: Font.hh,v 1.5 2002/10/13 22:24:14 fluxgen Exp $

#ifndef FBTK_FONT_HH
#define FBTK_FONT_HH

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <string>
#include <memory>

namespace FbTk {

class FontImp;

/**
	Handles the client to fontimp bridge.
*/
class Font {
public:
	Font(const char *name=0, bool antialias = false);
	virtual ~Font();
	/** 
		Load a font
		@return true on success, else false and it'll fall back on the last
		loaded font
	*/
	bool load(const char *name);

	/// @return true if multibyte is enabled, else false
	static bool multibyte() { return m_multibyte; }
	/// @return true if utf-8 mode is enabled, else false
	static bool utf8() { return m_utf8mode; }
	void setAntialias(bool flag);
	/**
		@param text text to check size
		@param size length of text in bytes
		@return size of text in pixels
	*/
	unsigned int textWidth(const char *text, unsigned int size) const;
	unsigned int height() const;
	void drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
private:
	
	std::auto_ptr<FontImp> m_fontimp;
	std::string m_fontstr;
	static bool m_multibyte;
	static bool m_utf8mode;
};

}; //end namespace FbTk

#endif //FBTK_FONT_HH
