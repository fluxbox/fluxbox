// Transparent.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Transparent.hh"
#include "App.hh"
#include "I18n.hh"

#ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>

#include <iostream>
#include <cstdio>

using std::cerr;
using std::endl;

#endif // HAVE_XRENDER


namespace {
#ifdef HAVE_XRENDER
Picture createAlphaPic(Window drawable, int alpha) {
    Display *disp = FbTk::App::instance()->display();
    _FB_USES_NLS;

    // try to find a specific render format
    XRenderPictFormat pic_format;
    pic_format.type  = PictTypeDirect;
    pic_format.depth = 8; // alpha with bit depth 8
    pic_format.direct.alphaMask = 0xff;
    XRenderPictFormat *format = XRenderFindFormat(disp, PictFormatType |
                                                  PictFormatDepth | PictFormatAlphaMask,
                                                  &pic_format, 0);
    if (format == 0) {
        cerr<<"FbTk::Transparent: "<<_FBTK_CONSOLETEXT(Error, NoRenderFormat,
                                                       "Warning: Failed to find valid format for alpha.",
                                                       "transparency requires a pict format, can't get one...")<<endl;
        return 0;
    }

    // create one pixel pixmap with depth 8 for alpha
    Pixmap alpha_pm = XCreatePixmap(disp, drawable,
                                    1, 1, 8);
    if (alpha_pm == 0) {
        cerr<<"FbTk::Transparent: "<<_FBTK_CONSOLETEXT(Error, NoRenderPixmap,
                                                       "Warning: Failed to create alpha pixmap.",
                                                       "XCreatePixmap failed for our transparency pixmap")<<endl;
        return 0;
    }

    // create picture with alpha_pm as repeated background
    XRenderPictureAttributes attr;
    attr.repeat = True; // small bitmap repeated
    Picture alpha_pic = XRenderCreatePicture(disp, alpha_pm,
                                             format, CPRepeat, &attr);
    if (alpha_pic == 0) {
        XFreePixmap(disp, alpha_pm);
        cerr<<"FbTk::Transparent: "<<_FBTK_CONSOLETEXT(Error, NoRenderPicture,
                                                       "Warning: Failed to create alpha picture.",
                                                       "XRenderCreatePicture failed")<<endl;
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

bool s_init = false;
bool s_render = false;
bool s_composite = false;
bool s_use_composite = false;

void init() {

    Display *disp = FbTk::App::instance()->display();

    int major_opcode, first_event, first_error;
    if (XQueryExtension(disp, "RENDER",
                        &major_opcode,
                        &first_event, &first_error)) {
        // we have XRENDER support
        s_render = true;

        if (XQueryExtension(disp, "Composite",
                            &major_opcode,
                            &first_event, &first_error)) {
            // we have Composite support
            s_composite = true;
            s_use_composite = true;
        }
    }
    s_init = true;
}

}

namespace FbTk {

bool Transparent::haveRender() {
    if (!s_init)
        init();
    return s_render;
}

void Transparent::usePseudoTransparent(bool force) {
    if (!s_init)
        init();
    s_use_composite = (!force && s_composite);
}

bool Transparent::haveComposite(bool for_real) {
    if (!s_init)
        init();

    if (for_real)
        return s_composite;
    else
        return s_use_composite;
}

Transparent::Transparent(Drawable src, Drawable dest, int alpha, int screen_num):
    m_alpha_pic(0), m_src_pic(0), m_dest_pic(0),
    m_source(src), m_dest(dest), m_alpha(alpha) {

    Display *disp = FbTk::App::instance()->display();

    // check for Extension support
    if (!s_init)
        init();

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

void Transparent::setAlpha(int alpha) {
    if (m_source == 0 || !s_render)
        return;

    freeAlpha();
    allocAlpha(alpha);
}

void Transparent::freeDest() {
#ifdef HAVE_XRENDER
    if (m_dest_pic != 0) {
        Display *disp = FbTk::App::instance()->display();
        XRenderFreePicture(disp, m_dest_pic);
        m_dest_pic = 0;
    }
    m_dest = None;
#endif
}

void Transparent::setDest(Drawable dest, int screen_num) {
#ifdef HAVE_XRENDER
    if (m_dest == dest || !s_render)
        return;

    Display *disp = FbTk::App::instance()->display();

    freeDest();
    // create new dest pic if we have a valid dest drawable
    if (dest != 0) {

        XRenderPictFormat *format =
            XRenderFindVisualFormat(disp,
                                    DefaultVisual(disp, screen_num));
        if (format == 0) {
            _FB_USES_NLS;
            cerr<<"FbTk::Transparent: ";
            fprintf(stderr,
                    _FBTK_CONSOLETEXT(Error, NoRenderVisualFormat,
                                      "Failed to find format for screen(%d)",
                                      "XRenderFindVisualFormat failed... include %d for screen number").
                    c_str(), screen_num);

            cerr<<endl;
        } else {
            m_dest_pic = XRenderCreatePicture(disp, dest, format, 0, 0);
        }

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
    int old_alpha = m_alpha;
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
        if (format == 0) {
            _FB_USES_NLS;
            cerr<<"FbTk::Transparent: ";
            fprintf(stderr, _FBTK_CONSOLETEXT(Error, NoRenderVisualFormat,
                                              "Failed to find format for screen(%d)",
                                              "XRenderFindVisualFormat failed... include %d for screen number").
                    c_str(), screen_num);
            cerr<<endl;
        } else {
            m_src_pic = XRenderCreatePicture(disp, m_source, format,
                                             0, 0);
        }
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

void Transparent::allocAlpha(int alpha) {
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

} // end namespace FbTk



