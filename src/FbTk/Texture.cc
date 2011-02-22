// Texture.hh for Fluxbox Window Manager 
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// from Image.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "Texture.hh"
#include "App.hh"
#include "StringUtil.hh"
#include <X11/Xlib.h>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

namespace FbTk {

void Texture::setFromString(const char * const texture_str) {
    if (texture_str == 0)
        return;

    const std::string t = FbTk::StringUtil::toLower(texture_str);
    const char* ts = t.c_str();

    if (strstr(ts, "parentrelative")) {
        setType(Texture::PARENTRELATIVE);
    } else {
        setType(Texture::NONE);

        if (strstr(ts, "gradient")) {
            addType(Texture::GRADIENT);
            if (strstr(ts, "crossdiagonal"))
                addType(Texture::CROSSDIAGONAL);
            else if (strstr(ts, "rectangle"))
                addType(Texture::RECTANGLE);
            else if (strstr(ts, "pyramid"))
                addType(Texture::PYRAMID);
            else if (strstr(ts, "pipecross"))
                addType(Texture::PIPECROSS);
            else if (strstr(ts, "elliptic"))
                addType(Texture::ELLIPTIC);
            else if (strstr(ts, "diagonal"))
                addType(Texture::DIAGONAL);
            else if (strstr(ts, "horizontal"))
                addType(Texture::HORIZONTAL);
            else if (strstr(ts, "vertical"))
                addType(Texture::VERTICAL);
            else
                addType(Texture::DIAGONAL);
        } else if (strstr(ts, "solid"))
            addType(Texture::SOLID);
        else
            addType(Texture::DEFAULT_TEXTURE);

        if (strstr(ts, "raised"))
            addType(Texture::RAISED);
        else if (strstr(ts, "sunken"))
            addType(Texture::SUNKEN);
        else if (strstr(ts, "flat"))
            addType(Texture::FLAT);
        else
            addType(Texture::DEFAULT_LEVEL);

        if (! (type() & Texture::FLAT)) {
            if (strstr(ts, "bevel2"))
                addType(Texture::BEVEL2);
            else
                addType(Texture::BEVEL1);
        }

        if (strstr(ts, "invert"))
            addType(Texture::INVERT);

        if (strstr(ts, "interlaced"))
            addType(Texture::INTERLACED);

        if (strstr(ts, "tiled"))
            addType(Texture::TILED);
    }
}

void Texture::calcHiLoColors(int screen_num) {
    Display *disp = FbTk::App::instance()->display();
    XColor xcol;
    Colormap colm = DefaultColormap(disp, screen_num);

    xcol.red = (unsigned int) (m_color.red() + (m_color.red() >> 1));
    if (xcol.red >= 0xff) xcol.red = 0xffff;
    else xcol.red *= 0x101;
    xcol.green = (unsigned int) (m_color.green() + (m_color.green() >> 1));
    if (xcol.green >= 0xff) xcol.green = 0xffff;
    else xcol.green *= 0x101;
    xcol.blue = (unsigned int) (m_color.blue() + (m_color.blue() >> 1));
    if (xcol.blue >= 0xff) xcol.blue = 0xffff;
    else xcol.blue *= 0x101;

    if (! XAllocColor(disp, colm, &xcol))
        xcol.pixel = 0;

    m_hicolor.setPixel(xcol.pixel);

    xcol.red = (unsigned int) ((m_color.red() >> 2) + (m_color.red() >> 1)) * 0x101;
    xcol.green = (unsigned int) ((m_color.green() >> 2) + (m_color.green() >> 1)) * 0x101;
    xcol.blue = (unsigned int) ((m_color.blue() >> 2) + (m_color.blue() >> 1)) * 0x101;

    if (! XAllocColor(disp, colm, &xcol))
        xcol.pixel = 0;

    m_locolor.setPixel(xcol.pixel);
}

} // end namespace FbTk
