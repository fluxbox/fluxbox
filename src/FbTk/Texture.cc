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

#include "ColorLUT.hh"

namespace {

unsigned short inline brighten(unsigned short c) {
    return 0x101 * FbTk::ColorLUT::BRIGHTER_8[c];
}

unsigned short inline darken(unsigned short c) {
    return 0x101 * FbTk::ColorLUT::PRE_MULTIPLY_0_75[c];
}

}

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
    Colormap colm = DefaultColormap(disp, screen_num);
    XColor xcol;

    xcol.red = ::brighten(m_color.red());
    xcol.green = ::brighten(m_color.green());
    xcol.blue = ::brighten(m_color.blue());

    if (! XAllocColor(disp, colm, &xcol))
        xcol.pixel = 0;

    m_hicolor.setPixel(xcol.pixel);

    xcol.red = ::darken(m_color.red());
    xcol.green = ::darken(m_color.green());
    xcol.blue = ::darken(m_color.blue());

    if (! XAllocColor(disp, colm, &xcol))
        xcol.pixel = 0;

    m_locolor.setPixel(xcol.pixel);
}

} // end namespace FbTk
