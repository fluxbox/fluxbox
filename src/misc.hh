// misc.hh for fluxbox 
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


#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif //HAVE_CONFIG_H

#include <X11/Xlib.h>
#include "Theme.hh"

#ifndef _MISC_HH_
#define _MISC_HH_

// ---- start code stealing -----

// ----------------------------------------------------------------------
// xvertext, Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma)
// ----------------------------------------------------------------------

#define XV_NOFONT	 1  // no such font on X server
#define XV_NOMEM	 2  // couldn't do malloc
#define XV_NOXIMAGE	 3  // couldn't create an XImage

unsigned int XRotTextWidth(XRotFontStruct *rotfont, char *str, int len);
void XRotDrawString(Display *dpy, XRotFontStruct *rotfont, Drawable drawable,
					GC gc, int x, int y, char *str, int len);

//int xv_errno; //TODO: ?

// --- stop code stealing ---

void DrawString(Display *display, Window w, GC gc, FFont *font,
					unsigned int text_w, unsigned int size_w,
					unsigned int bevel_w, char *text);

void DrawRotString(Display *display, Window w, GC gc, XRotFontStruct *font,
					unsigned int align, unsigned int text_w,
					unsigned int size_w, unsigned int size_h,
					unsigned int bevel_w, char *text);

#endif //_MISC_HH_
