// Transparent.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: Transparent.cc,v 1.5 2003/05/13 20:50:56 fluxgen Exp $

#include "Transparent.hh"
#include "App.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>
#endif // HAVE_XRENDER

#include <iostream>
using namespace std;

namespace {
#ifdef HAVE_XRENDER
Picture createAlphaPic(Window drawable, unsigned char alpha) {
    Display *disp = FbTk::App::instance()->display();

    // try to find a specific render format
    XRenderPictFormat pic_format;
    pic_format.type  = PictTypeDirect;
    pic_format.depth = 8; // alpha with bit depth 8
    pic_format.direct.alphaMask = 0xff;    
    XRenderPictFormat *format = XRenderFindFormat(disp, PictFormatType |
                                                  PictFormatDepth | PictFormatAlphaMask,
                                                  &pic_format, 0);
    if (format == 0) {
        cerr<<"Warning! FbTk::Transparent:  Failed to find valid format for alpha."<<endl;
        return 0;
    }

    // create one pixel pixmap with depth 8 for alpha
    Pixmap alpha_pm = XCreatePixmap(disp, drawable,
                                    1, 1, 8);
    if (alpha_pm == 0) {
        cerr<<"Warning! FbTk::Transparent: Failed to create alpha pixmap."<<endl;
        return 0;
    }

    // create picture with alpha_pm as repeated background
    XRenderPictureAttributes attr;
    attr.repeat = True; // small bitmap repeated
    Picture alpha_pic = XRenderCreatePicture(disp, alpha_pm,
                                             format, CPRepeat, &attr);
    if (alpha_pic == 0) {
        XFreePixmap(disp, alpha_pm);
        cerr<<"Warning! FbTk::Transparent: Failed to create alpha picture."<<endl;
        return 0;
    }

    // finaly set alpha and fill with it
    XRenderColor color;
    // calculate alpha percent and then scale it to short
    color.red = 0xFF;
    color.blue = 0xFF;
    color.green = 0xFF;
    color.alpha = ((unsigned short) (255 * alpha) << 8);
    if (alpha == 0)
        color.alpha = 0xFF00;

    XRenderFillRectangle(disp, PictOpSrc, alpha_pic, &color,
                         0, 0, 1, 1);

    XFreePixmap(disp, alpha_pm);

    return alpha_pic;
}
#endif //  HAVE_XRENDER
};

namespace FbTk {

bool Transparent::s_init = false;
bool Transparent::s_render = false;

Transparent::Transparent(Drawable src, Drawable dest, unsigned char alpha, int screen_num):
    m_alpha_pic(0), m_src_pic(0), m_dest_pic(0),
    m_source(src), m_dest(dest), m_alpha(alpha) {

    Display *disp = FbTk::App::instance()->display();

    // check for RENDER support
    if (!s_init) {
        int major_opcode, first_event, first_error;
        if (XQueryExtension(disp, "RENDER", 
                            &major_opcode, 
                            &first_event, &first_error) == False) {
            s_render = false;
        } else { // we got RENDER support
            s_render = true;
        }
        s_init = true;
    }

    
#ifdef HAVE_XRENDER
    if (!s_render)
        return;

    allocAlpha(m_alpha);


    XRenderPictFormat *format = 
        XRenderFindVisualFormat(disp, 
                                DefaultVisual(disp, screen_num));


    if (src != 0 && format != 0) {
        m_src_pic = XRenderCreatePicture(disp, src, format, 
                                         0, 0);
    }

    if (dest != 0 && format != 0) {
        m_dest_pic = XRenderCreatePicture(disp, dest, format,
                                          0, 0);
    }
#endif // HAVE_XRENDER
}

Transparent::~Transparent() {
#ifdef HAVE_XRENDER
    if (m_alpha_pic != 0 && s_render)
        freeAlpha();

    Display *disp = FbTk::App::instance()->display();

    if (m_dest_pic != 0 && s_render)
        XRenderFreePicture(disp, m_dest_pic);

    if (m_src_pic != 0  && s_render)
        XRenderFreePicture(disp, m_src_pic);
#endif // HAVE_XRENDER
}

void Transparent::setAlpha(unsigned char alpha) {
    if (m_source == 0 || !s_render)
        return;

    freeAlpha();
    allocAlpha(alpha);
}

void Transparent::setDest(Drawable dest, int screen_num) {
#ifdef HAVE_XRENDER
    if (m_dest == dest || !s_render)
        return;

    Display *disp = FbTk::App::instance()->display();

    if (m_dest_pic != 0) {
        XRenderFreePicture(disp, m_dest_pic);
        m_dest_pic = 0;
    }
    // create new dest pic if we have a valid dest drawable
    if (dest != 0) {

        XRenderPictFormat *format = 
            XRenderFindVisualFormat(disp, 
                                    DefaultVisual(disp, screen_num));
        if (format == 0)
            cerr<<"Warning! FbTk::Transparent: Failed to find format for screen("<<screen_num<<")"<<endl;
        m_dest_pic = XRenderCreatePicture(disp, dest, format, 0, 0);
       

    }
    m_dest = dest;
#endif // HAVE_XRENDER
}

void Transparent::setSource(Drawable source, int screen_num) {
#ifdef HAVE_XRENDER
    if (m_source == source || !s_render)
        return;
    // save old alpha value so we can recreate new later
    // with the same value
    unsigned char old_alpha = m_alpha;
    if (m_alpha_pic != 0)
        freeAlpha();

    Display *disp = FbTk::App::instance()->display();   

    if (m_src_pic != 0) {
        XRenderFreePicture(disp, m_src_pic);
        m_src_pic = 0;
    }

    m_source = source;

    // create new source pic if we have a valid source drawable
    if (m_source != 0) {

        XRenderPictFormat *format = 
            XRenderFindVisualFormat(disp, 
                                    DefaultVisual(disp, screen_num));
        if (format == 0)
            cerr<<"Warning! FbTk::Transparent: Failed to find format for screen("<<screen_num<<")"<<endl;
        m_src_pic = XRenderCreatePicture(disp, m_source, format, 
                                         0, 0);    
    }

    // recreate new alpha
    allocAlpha(old_alpha);

#endif // HAVE_XRENDER
}

void Transparent::render(int src_x, int src_y,
                         int dest_x, int dest_y,
                         unsigned int width, unsigned int height) const {
#ifdef HAVE_XRENDER
    if (m_src_pic == 0 || m_dest_pic == 0 ||
        m_alpha_pic  == 0 || !s_render)
        return;
    // render src+alpha to dest picture
    XRenderComposite(FbTk::App::instance()->display(), 
                     PictOpOver, 
                     m_src_pic,
                     m_alpha_pic, 
                     m_dest_pic, 
                     src_x, src_y,
                     0, 0, 
                     dest_x, dest_y,
                     width, height);

#endif // HAVE_XRENDER
}

void Transparent::allocAlpha(unsigned char alpha) {
#ifdef HAVE_XRENDER
    if (m_source == 0 || !s_render)
        return;
    if (m_alpha_pic != 0)
        freeAlpha();

    m_alpha_pic = createAlphaPic(m_source, alpha);
    m_alpha = alpha;
#endif // HAVE_XRENDER
}

void Transparent::freeAlpha() {
#ifdef HAVE_XRENDER
    if (s_render && m_alpha_pic != 0)
        XRenderFreePicture(FbTk::App::instance()->display(), m_alpha_pic);
#endif // HAVE_XRENDER
    m_alpha_pic = 0;
    m_alpha = 255;
}

}; // end namespace FbTk


 
