// DrawUtil.hh for fluxbox 
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
#ifndef _DRAWUTIL_HH_
#define _DRAWUTIL_HH_

#include <X11/Xlib.h>

struct DrawUtil
{
	struct Font 
	{
		enum FontJustify {LEFT=0, RIGHT, CENTER};
	
		XFontSet set;
		XFontSetExtents *set_extents;
		XFontStruct *fontstruct;
		FontJustify justify;
	};
	
static void DrawString(Display *display, Window w, GC gc, DrawUtil::Font *font,
					unsigned int text_w, unsigned int size_w,
					unsigned int bevel_w, char *text);

// ----------------------------------------------------------------------
// xvertext, Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma)
// ----------------------------------------------------------------------

	/* *** The font structures *** */

	struct BitmapStruct {
		int			 bit_w;
		int			 bit_h;

		Pixmap bm;
	};

	struct XRotCharStruct {
		int			 ascent;
		int			 descent;
		int			 lbearing;
		int			 rbearing;
		int			 width;

		BitmapStruct	 glyph;
	};

	struct XRotFontStruct {
		int			 dir;
		int			 height;
		int			 max_ascent;
		int			 max_descent;
		int			 max_char;
		int			 min_char;
		char 		*name;

		XFontStruct		*xfontstruct;

		DrawUtil::XRotCharStruct	 per_char[95];
	};
static unsigned int XRotTextWidth(DrawUtil::XRotFontStruct *rotfont, char *str, int len);
static void XRotDrawString(Display *dpy, DrawUtil::XRotFontStruct *rotfont, Drawable drawable,
					GC gc, int x, int y, char *str, int len);

static void DrawRotString(Display *display, Window w, GC gc, DrawUtil::XRotFontStruct *font,
					unsigned int align, unsigned int text_w,
					unsigned int size_w, unsigned int size_h,
					unsigned int bevel_w, char *text);
					
static DrawUtil::XRotFontStruct *XRotLoadFont(Display *dpy, char *fontname, float angle);
static void XRotUnloadFont(Display *dpy, DrawUtil::XRotFontStruct *rotfont);

};

#endif //_DRAWUTIL_HH_
