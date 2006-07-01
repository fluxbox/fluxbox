// TextureRender.hh for fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

// $Id$

#ifndef FBTK_TEXTURRENDER_HH
#define FBTK_TEXTURRENDER_HH

#include "Texture.hh"
#include "Text.hh"

#include <X11/Xlib.h>

namespace FbTk {

class ImageControl;


/// Renders texture to pixmap
/**
  This is used with BImageControl to render textures
*/
class TextureRender {
public:
    TextureRender(ImageControl &ic, unsigned int width, unsigned int height, 
                  Orientation orient = ROT0,
                  XColor *_colors=0, size_t num_colors=0);
    ~TextureRender();
    /// render to pixmap
    Pixmap render(const FbTk::Texture &src_texture);
    /// render solid texture to pixmap
    Pixmap renderSolid(const FbTk::Texture &src_texture);
    /// render gradient texture to pixmap
    Pixmap renderGradient(const FbTk::Texture &src_texture);
    /// scales and renders a pixmap
    Pixmap renderPixmap(const FbTk::Texture &src_texture);
private:
    /// allocates red, green and blue for gradient rendering
    void allocateColorTables();
    /**
       Render to pixmap
       @return rendered pixmap
    */
    Pixmap renderPixmap();
    /**
       Render to XImage
       @returns allocated and rendered XImage, user is responsible to deallocate
    */
    XImage *renderXImage();
    /**
       @name render functions
    */
    //@{
    void invert();
    void bevel1();
    void bevel2();
    void dgradient();
    void egradient();
    void hgradient();
    void pgradient();
    void rgradient();
    void vgradient();
    void cdgradient();
    void pcgradient();
    //@}

    ImageControl &control;
    bool interlaced;

    XColor *colors; // color table

    const FbTk::Color *from, *to;
    int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
        ncolors, cpc, cpccpc;
    unsigned char *red, *green, *blue;
    const unsigned char *red_table, *green_table, *blue_table;
    Orientation orientation;
    unsigned int width, height;
    unsigned int *xtable, *ytable;
};

}; // end namespace FbTk 
#endif // FBTK_TEXTURERENDER_HH
