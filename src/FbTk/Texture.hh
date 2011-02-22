// Texture.hh for Fluxbox Window Manager
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxbox<at>users.sourceforge.net)
//
// from Image.hh for Blackbox - an X11 Window manager
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

#ifndef FBTK_TEXTURE_HH
#define FBTK_TEXTURE_HH

#include "Color.hh"
#include "FbPixmap.hh"

namespace FbTk  {

/**
   Holds texture type and info
*/
class Texture {
public:

    enum Bevel {
        // why are we not using the lowest-order bit?
        FLAT =     0x00002,
        SUNKEN =   0x00004,
        RAISED =   0x00008,
        DEFAULT_LEVEL = FLAT
    };

    enum Textures {
        NONE =     0x00000,
        SOLID =    0x00010,
        GRADIENT = 0x00020,
        DEFAULT_TEXTURE = SOLID
    };

    enum Gradients {
        HORIZONTAL =     0x00040,
        VERTICAL =       0x00080,
        DIAGONAL =       0x00100,
        CROSSDIAGONAL =  0x00200,
        RECTANGLE =      0x00400,
        PYRAMID =        0x00800,
        PIPECROSS =      0x01000,
        ELLIPTIC =       0x02000
    };

    enum {
        BEVEL1 =         0x04000,
        BEVEL2 =         0x08000, // bevel types
        INVERT =         0x10000, ///< inverted image
        PARENTRELATIVE = 0x20000,
        INTERLACED =     0x40000,
        TILED =          0x80000  ///< tiled pixmap
    };

    Texture():m_type(0) { }

    void setType(unsigned long t) { m_type = t; }
    void addType(unsigned long t) { m_type |= t; }
    void setFromString(const char * const str);

    Color &color() { return m_color; }
    Color &colorTo() { return m_color_to; }
    Color &hiColor() { return m_hicolor; }
    Color &loColor() { return m_locolor; }

    FbPixmap &pixmap() { return m_pixmap; }

    void calcHiLoColors(int screen_num);

    const Color &color() const { return m_color; }
    const Color &colorTo() const { return m_color_to; }
    const Color &hiColor() const { return m_hicolor; }
    const Color &loColor() const { return m_locolor; }
    const FbTk::FbPixmap &pixmap() const { return m_pixmap; }
    unsigned long type() const { return m_type; }
    bool usePixmap() const { return !( type() == (FLAT | SOLID) && pixmap().drawable() == 0); }

private:
    FbTk::Color m_color, m_color_to, m_hicolor, m_locolor;
    FbTk::FbPixmap m_pixmap;
    unsigned long m_type;
};

} // end namespace FbTk

#endif // FBTK_TEXTURE_HH
