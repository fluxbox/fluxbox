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

#include "misc.hh"
#include "i18n.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <X11/Xutil.h>
using namespace std;

//------- strdup ------------------------
//TODO: comment this
//----------------------------------------
char *Misc::strdup(const char *s) {
  int l = strlen(s) + 1;
  char *n = new char[l];
  strncpy(n, s, l);
  return n;
}


// ----------------------------------------------------------------------
// xvertext, Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma)
// ----------------------------------------------------------------------

//------- XRotLoadFont -------------------
// Load the rotated version of a given font
//----------------------------------------
Misc::XRotFontStruct *Misc::XRotLoadFont(Display *dpy, char *fontname, float angle) {
	char val;
	XImage *I1, *I2;
	Pixmap canvas;
	Window root;
	int screen;
	GC font_gc;
	char text[3];/*, errstr[300];*/
	XFontStruct *fontstruct;
	XRotFontStruct *rotfont;
	int ichar, i, j, index, boxlen = 60, dir;
	int vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
	int min_char, max_char;
	unsigned char *vertdata, *bitdata;
	int ascent, descent, lbearing, rbearing;
	int on = 1, off = 0;

	/* make angle positive ... */
	if (angle < 0) do angle += 360; while (angle < 0);

	/* get nearest vertical or horizontal direction ... */
	dir = (int)((angle+45.)/90.)%4;

	/* useful macros ... */
	screen = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);

	/* create the depth 1 canvas bitmap ... */
	canvas = XCreatePixmap(dpy, root, boxlen, boxlen, 1);
 
	/* create a GC ... */
	font_gc = XCreateGC(dpy, canvas, 0, 0);
	XSetBackground(dpy, font_gc, off);

	/* load the font ... */
	fontstruct = XLoadQueryFont(dpy, fontname);
	if (fontstruct == NULL) {
		cerr<<"Fluxbox::Misc: No font"<<endl;
		return 0;
	}
 
	XSetFont(dpy, font_gc, fontstruct->fid);

	/* allocate space for rotated font ... */
 	rotfont = (XRotFontStruct *)malloc((unsigned)sizeof(XRotFontStruct));

	if (rotfont == 0) {
		cerr<<"Fluxbox::Misc: out of memory"<<endl;
		return 0;
	}
   
	/* determine which characters are defined in font ... */
	min_char = fontstruct->min_char_or_byte2; 
	max_char = fontstruct->max_char_or_byte2;
 
	/* we only want printing characters ... */
	if (min_char<32)  min_char = 32;
	if (max_char>126) max_char = 126;
     
	/* some overall font data ... */
	rotfont->name = Misc::strdup(fontname);
	rotfont->dir = dir;
	rotfont->min_char = min_char;
	rotfont->max_char = max_char;
	rotfont->max_ascent = fontstruct->max_bounds.ascent;
	rotfont->max_descent = fontstruct->max_bounds.descent;   
	rotfont->height = rotfont->max_ascent+rotfont->max_descent;

	/* remember xfontstruct for `normal' text ... */
	if (dir == 0)
		rotfont->xfontstruct = fontstruct;
	else {
		/* font needs rotation ... */
		/* loop through each character ... */
		for (ichar = min_char; ichar <= max_char; ichar++) {
			index = ichar-fontstruct->min_char_or_byte2;
 
			/* per char dimensions ... */
			ascent = rotfont->per_char[ichar-32].ascent = 
				fontstruct->per_char[index].ascent;
			descent =  rotfont->per_char[ichar-32].descent = 
				fontstruct->per_char[index].descent;
			lbearing = rotfont->per_char[ichar-32].lbearing = 
				fontstruct->per_char[index].lbearing;
			rbearing = rotfont->per_char[ichar-32].rbearing = 
				fontstruct->per_char[index].rbearing;
			rotfont->per_char[ichar-32].width = 
				fontstruct->per_char[index].width;

			/* some space chars have zero body, but a bitmap can't have ... */
			if (!ascent && !descent)   
				ascent = rotfont->per_char[ichar-32].ascent =   1;
			if (!lbearing && !rbearing) 
				rbearing = rotfont->per_char[ichar-32].rbearing = 1;

			/* glyph width and height when vertical ... */
			vert_w = rbearing-lbearing;
			vert_h = ascent+descent;

			/* width in bytes ... */
			vert_len = (vert_w-1)/8+1;   
 
			XSetForeground(dpy, font_gc, off);
			XFillRectangle(dpy, canvas, font_gc, 0, 0, boxlen, boxlen);

			/* draw the character centre top right on canvas ... */
			sprintf(text, "%c", ichar);
			XSetForeground(dpy, font_gc, on);
			XDrawImageString(dpy, canvas, font_gc, boxlen/2 - lbearing,
							boxlen/2 - descent, text, 1);

			/* reserve memory for first XImage ... */
 			vertdata = (unsigned char *) malloc((unsigned)(vert_len*vert_h));

			/* create the XImage ... */
			I1 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap,
								0, (char *)vertdata, vert_w, vert_h, 8, 0);

			if (I1 == NULL) {				
				cerr<<"Fluxbox::Misc: Cant create ximage."<<endl;
				return NULL;
			}
 
			I1->byte_order = I1->bitmap_bit_order = MSBFirst;
   
			/* extract character from canvas ... */
			XGetSubImage(dpy, canvas, boxlen/2, boxlen/2-vert_h,
						vert_w, vert_h, 1, XYPixmap, I1, 0, 0);
			I1->format = XYBitmap; 
 
			/* width, height of rotated character ... */
			if (dir == 2) { 
				bit_w = vert_w;
				bit_h = vert_h; 
			} else {
				bit_w = vert_h;
				bit_h = vert_w; 
			}

			/* width in bytes ... */
			bit_len = (bit_w-1)/8 + 1;

			rotfont->per_char[ichar-32].glyph.bit_w = bit_w;
			rotfont->per_char[ichar-32].glyph.bit_h = bit_h;

			/* reserve memory for the rotated image ... */
 			bitdata = (unsigned char *)calloc((unsigned)(bit_h*bit_len), 1);

			/* create the image ... */
			I2 = XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap, 0,
								(char *)bitdata, bit_w, bit_h, 8, 0); 
 
			if (I2 == NULL) {
				cerr<<"Font::Misc: Cant create ximage!"<<endl;
				return 0;
			}

			I2->byte_order = I2->bitmap_bit_order = MSBFirst;
 
			/* map vertical data to rotated character ... */
			for (j = 0; j < bit_h; j++) {
				for (i = 0; i < bit_w; i++) {
					/* map bits ... */
					if (dir == 1) {
						val = vertdata[i*vert_len + (vert_w-j-1)/8] &
								(128>>((vert_w-j-1)%8));
  					} else if (dir == 2) {
					    val = vertdata[(vert_h-j-1)*vert_len +
							(vert_w-i-1)/8] & (128>>((vert_w-i-1)%8));
					} else {
						val = vertdata[(vert_h-i-1)*vert_len + j/8] & 
							(128>>(j%8));
       				} 
					if (val) {
						bitdata[j*bit_len + i/8] = bitdata[j*bit_len + i/8] |
							(128>>(i%8));
					}
				}
			}
   
			/* create this character's bitmap ... */
			rotfont->per_char[ichar-32].glyph.bm = 
				XCreatePixmap(dpy, root, bit_w, bit_h, 1);
     
			/* put the image into the bitmap ... */
			XPutImage(dpy, rotfont->per_char[ichar-32].glyph.bm, 
						font_gc, I2, 0, 0, 0, 0, bit_w, bit_h);
  
			/* free the image and data ... */
			XDestroyImage(I1);
			XDestroyImage(I2);
			/*      free((char *)bitdata);  -- XDestroyImage does this
			free((char *)vertdata);*/
		}
		XFreeFont(dpy, fontstruct);
	}

	/* free pixmap and GC ... */
	XFreePixmap(dpy, canvas);
	XFreeGC(dpy, font_gc);

	return rotfont;
}

//------- XRotUnloadFont -----------------
// Free the resources associated with a
// rotated font
//----------------------------------------
void Misc::XRotUnloadFont(Display *dpy, XRotFontStruct *rotfont)
{
	int ichar;

	if (rotfont->dir == 0)
		XFreeFont(dpy, rotfont->xfontstruct);
	else {
		/* loop through each character, freeing its pixmap ... */
		for (ichar = rotfont->min_char-32; ichar <= rotfont->max_char-32;
				ichar++)
		XFreePixmap(dpy, rotfont->per_char[ichar].glyph.bm);
	}
	
	free((char *)rotfont->name);
 	free((char *)rotfont);
}

//------- XRotTextWidth ------------------
// Returns the width of a rotated string
//----------------------------------------
unsigned int Misc::XRotTextWidth(XRotFontStruct *rotfont, char *str, int len)
{
	int i, width = 0, ichar;

	if (str == NULL)
		return 0;

	if (rotfont->dir == 0)
		width = XTextWidth(rotfont->xfontstruct, str, strlen(str));
	else {
		for (i = 0; i<len; i++) {
			ichar = str[i]-32;
  
			/* make sure it's a printing character ... */
			if (ichar >= 0 && ichar<95) 
				width += rotfont->per_char[ichar].width;
		}
	}
	return width;
}


//------- XRotDrawString -----------------
// A front end to XRotDrawString : mimics XDrawString
//----------------------------------------
void Misc::XRotDrawString(Display *dpy, XRotFontStruct *rotfont, Drawable drawable,
		    GC gc, int x, int y, char *str, int len)
{            
	static GC my_gc = 0;
	int i, xp, yp, dir, ichar;

	if (str == NULL || len<1)
		return;

	dir = rotfont->dir;
	if (my_gc == 0)
		my_gc = XCreateGC(dpy, drawable, 0, 0);

	XCopyGC(dpy, gc, GCForeground|GCBackground, my_gc);

	/* a horizontal string is easy ... */
	if (dir == 0) {
		XSetFillStyle(dpy, my_gc, FillSolid);
		XSetFont(dpy, my_gc, rotfont->xfontstruct->fid);
		XDrawString(dpy, drawable, my_gc, x, y, str, len);
		return;
	}

	/* vertical or upside down ... */

	XSetFillStyle(dpy, my_gc, FillStippled);

	/* loop through each character in string ... */
	for (i = 0; i<len; i++) {
		ichar = str[i]-32;

		/* make sure it's a printing character ... */
		if (ichar >= 0 && ichar<95) {
			/* suitable offset ... */
			if (dir == 1) {
				xp = x-rotfont->per_char[ichar].ascent;
				yp = y-rotfont->per_char[ichar].rbearing; 
			} else if (dir == 2) {
				xp = x-rotfont->per_char[ichar].rbearing;
				yp = y-rotfont->per_char[ichar].descent+1; 
			} else {
				xp = x-rotfont->per_char[ichar].descent+1;  
				yp = y+rotfont->per_char[ichar].lbearing; 
			}
                   
			/* draw the glyph ... */
			XSetStipple(dpy, my_gc, rotfont->per_char[ichar].glyph.bm);
    
			XSetTSOrigin(dpy, my_gc, xp, yp);
      
			XFillRectangle(dpy, drawable, my_gc, xp, yp,
				rotfont->per_char[ichar].glyph.bit_w,
				rotfont->per_char[ichar].glyph.bit_h);
    
			/* advance position ... */
			if (dir == 1)
				y -= rotfont->per_char[ichar].width;
			else if (dir == 2)
				x -= rotfont->per_char[ichar].width;
			else 
				y += rotfont->per_char[ichar].width;
		}
	}
}


//Draw title string	
void Misc::DrawString(Display *display, Window w, GC gc, Misc::Font *font, 
					unsigned int text_w, unsigned int size_w, 
					unsigned int bevel_w, char *text) {

	if (!text || text_w<1 || size_w < 1 || !font || !display)
		return;
		
	unsigned int l = text_w;
	int dlen=strlen(text);
	int dx=bevel_w*2;
	
		
	if (text_w > size_w) {
		for (; dlen >= 0; dlen--) {
			if (I18n::instance()->multibyte()) {
				XRectangle ink, logical;
				XmbTextExtents(font->set, text, dlen,
											&ink, &logical);
				l = logical.width;
			} else
				l = XTextWidth(font->fontstruct, text, dlen);
			
			l += (dx * 4);

			if (l < size_w)
				break;
		}
	}
	
	switch (font->justify) {
	case Misc::Font::RIGHT:
		dx += size_w - l;
		break;

	case Misc::Font::CENTER:
		dx += (size_w - l) / 2;
		break;
	default:
		break;
	}

	//Draw title to m_tabwin

	XClearWindow(display, w);		
	
	if (I18n::instance()->multibyte()) {
		XmbDrawString(display, w,
			font->set, gc, dx, 1 - font->set_extents->max_ink_extent.y,
			text, dlen);
	} else {
		XDrawString(display, w,
			gc, dx,	font->fontstruct->ascent + 1, 
			text, dlen);
	}
	
}


void Misc::DrawRotString(Display *display, Window w, GC gc, XRotFontStruct *font,
					unsigned int align, unsigned int text_w, 
					unsigned int size_w, unsigned int size_h,
					unsigned int bevel_w, char *text) {

	if (!text || text_w<1 || size_w < 1 || !font || !display)
		return;
		
	unsigned int l = text_w;
	int dlen = strlen(text);
	int dx = bevel_w * 2;
	
	if (text_w > size_w) {
		for (; dlen >= 0; dlen--) {
				l = XRotTextWidth(font, text, dlen);
			
			l += (dx * 4);

			if (l < size_h)
				break;
		}
	}

	if (align == Misc::Font::RIGHT)
		size_h = l;
	else if (align == Misc::Font::CENTER)
		size_h = (size_h + l) / 2;
	else //LEFT
		size_h -= (dx * 4);

	// To get it in the "center" of the tab
	size_w = (size_w + XRotTextWidth(font, "/", 1)) / 2;

	//Draw title to m_tabwin
	XClearWindow(display, w);
	XRotDrawString(display, font, w, gc, size_w, size_h, text, dlen);	
}
