// XftFontImp.cc  Xft font implementation for FbTk
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

//$Id: XftFontImp.cc,v 1.2 2002/12/01 13:42:15 rathnor Exp $

#include "XftFontImp.hh"
#include "App.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H
namespace FbTk {

XftFontImp::XftFontImp(const char *name, bool utf8):m_xftfont(0),
                                                    m_utf8mode(utf8) {
    if (name != 0)
        load(name);
}

XftFontImp::~XftFontImp() {
    if (m_xftfont != 0)
        XftFontClose(App::instance()->display(), m_xftfont);
}

bool XftFontImp::load(const std::string &name) {
    //Note: assumes screen 0 for now, changes on draw if needed
	
    Display *disp = App::instance()->display();
    XftFont *newxftfont = XftFontOpenName(disp, 0, name.c_str());
		
    if (newxftfont == 0) { // failed to open font, lets test with XLFD
        newxftfont = XftFontOpenXlfd(disp, 0, name.c_str());
        if (newxftfont == 0)
            return false;
    }
    // destroy old font and set new
    if (m_xftfont != 0)
        XftFontClose(disp, m_xftfont);

    m_xftfont = newxftfont;

    return true;
}

void XftFontImp::drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const {
    if (m_xftfont == 0)
        return;
    Display *disp = App::instance()->display();
    XftDraw *draw = XftDrawCreate(disp,
                                  w,
                                  DefaultVisual(disp, screen),
                                  DefaultColormap(disp, screen));

    XGCValues gc_val;

    // get foreground pixel value and convert it to XRenderColor value
    // TODO: we should probably check return status
    XGetGCValues(disp, gc, GCForeground, &gc_val);

    // get red, green, blue values
    XColor xcol;
    xcol.pixel = gc_val.foreground;
    XQueryColor(disp, DefaultColormap(disp, screen), &xcol);

    // convert xcolor to XftColor
    XRenderColor rendcol;
    rendcol.red = xcol.red;
    rendcol.green = xcol.green;
    rendcol.blue = xcol.blue;
    rendcol.alpha = 0xFFFF;
    XftColor xftcolor;	
    XftColorAllocValue(disp, DefaultVisual(disp, screen), DefaultColormap(disp, screen),
                       &rendcol, &xftcolor);

    // draw string
#ifdef HAVE_XFT_UTF8_STRING
    if (m_utf8mode) {
        XftDrawStringUtf8(draw,
                          &xftcolor,
                          m_xftfont,
                          x, y,
                          (XftChar8 *)(text), len);
    } else 
#endif // HAVE_XFT_UTF8_STRING
	{
            XftDrawString8(draw,
                           &xftcolor,
                           m_xftfont,
                           x, y,
                           (XftChar8 *)(text), len);
	}

    XftColorFree(disp, DefaultVisual(disp, screen), 
                 DefaultColormap(disp, screen), &xftcolor);
    XftDrawDestroy(draw);
}

unsigned int XftFontImp::textWidth(const char * const text, unsigned int len) const {
    if (m_xftfont == 0)
        return 0;
    XGlyphInfo ginfo;
#ifdef HAVE_XFT_UTF8_STRING
    if (m_utf8mode) {
        XftTextExtentsUtf8(App::instance()->display(),
                           m_xftfont,
                           (XftChar8 *)text, len,
                           &ginfo);
    } else 
#endif  //HAVE_XFT_UTF8_STRING
	{
            XftTextExtents8(App::instance()->display(),
                            m_xftfont,
                            (XftChar8 *)text, len,
                            &ginfo);
	}
    return ginfo.xOff;
}

unsigned int XftFontImp::height() const {
    if (m_xftfont == 0)
        return 0;
    return m_xftfont->height;
}

}; // end namespace FbTk
