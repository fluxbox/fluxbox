// XFontImp.hh for FbTk fluxbox toolkit
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

// $Id: XFontImp.hh,v 1.3 2002/10/19 13:58:47 fluxgen Exp $

#ifndef XFONTIMP_HH
#define XFONTIMP_HH

#include "FontImp.hh"

class XFontImp:public FbTk::FontImp {
public:
	explicit XFontImp(const char *filename = 0);
	~XFontImp();
	bool load(const std::string &filename);
	unsigned int textWidth(const char * const text, unsigned int size) const;
	unsigned int height() const;
	int ascent() const { return m_fontstruct ? m_fontstruct->ascent : 0; }
	int descent() const { return m_fontstruct ? m_fontstruct->descent : 0; }
	void drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
	bool loaded() const { return m_fontstruct != 0; }
private:
	XFontStruct *m_fontstruct;
};

#endif // XFONTIMP_HH
