// Texture.hh for Fluxbox Window Manager 
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@users.sourceforge.net)
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

// $Id: Texture.cc,v 1.8 2004/01/11 21:04:21 fluxgen Exp $

#include "Texture.hh"

#include <cstring>
#include <cctype>

namespace FbTk {

void Texture::setFromString(const char * const texture_str) {
    if (texture_str == 0)
        return;
    int t_len = strlen(texture_str) + 1;
    char *ts = new char[t_len];
    strcpy(ts, texture_str);

    // to lower
    for (size_t byte_pos = 0; byte_pos < strlen(ts); ++byte_pos)
        ts[byte_pos] = tolower(ts[byte_pos]);

    if (strstr(ts, "parentrelative")) {
        setType(Texture::PARENTRELATIVE);
    } else {
        setType(Texture::NONE);

        if (strstr(ts, "solid"))
            addType(Texture::SOLID);
        else if (strstr(ts, "gradient")) {
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
        } else
            addType(Texture::SOLID);

        if (strstr(ts, "raised"))
            addType(Texture::RAISED);
        else if (strstr(ts, "sunken"))
            addType(Texture::SUNKEN);
        else if (strstr(ts, "flat"))
            addType(Texture::FLAT);        
        else
            addType(Texture::RAISED);

        if (! (type() & Texture::FLAT))
            if (strstr(ts, "bevel2"))
                addType(Texture::BEVEL2);
            else
                addType(Texture::BEVEL1);

        if (strstr(ts, "invert"))
            addType(Texture::INVERT);

        if (strstr(ts, "interlaced"))
            addType(Texture::INTERLACED);

        if (strstr(ts, "tiled"))
            addType(Texture::TILED);
    }

    delete [] ts;
}

}; // end namespace FbTk
