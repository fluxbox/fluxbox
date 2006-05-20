// MenuIcon.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

#include "MenuIcon.hh"

#include "MenuTheme.hh"
#include "Image.hh"
#include "App.hh"

namespace FbTk {

MenuIcon::MenuIcon(const std::string &filename, FbString &label, int screen_num):
    MenuItem(label),
    m_filename(filename) {
    FbTk::PixmapWithMask *pm = Image::load(filename.c_str(), screen_num);
    if (pm != 0) {
        m_pixmap = pm->pixmap().release();
        m_mask = pm->mask().release();
        delete pm;
    }

}

void MenuIcon::updateTheme(const MenuTheme &theme) {
    FbTk::PixmapWithMask *pm = Image::load(m_filename.c_str(), theme.screenNum());
    if (pm != 0) {
        m_pixmap = pm->pixmap().release();
        m_mask = pm->mask().release();
        delete pm;
    }
}

void MenuIcon::draw(FbDrawable &drawable,
                    const MenuTheme &theme,
                    bool highlight, bool draw_foreground, bool draw_background,
                    int x, int y,
                    unsigned int width, unsigned int height) const {

    // all background
    if (draw_background) {
        Display *disp = FbTk::App::instance()->display();
        if (height - 2*theme.bevelWidth() != m_pixmap.height() &&
            !m_filename.empty()) {
            unsigned int scale_size = height - 2*theme.bevelWidth();
            m_pixmap.scale(scale_size, scale_size);
            m_mask.scale(scale_size, scale_size);
        }

        if (m_pixmap.drawable() != 0) {
            GC gc = theme.frameTextGC().gc();

            // enable clip mask
            XSetClipMask(disp, gc, m_mask.drawable());
            XSetClipOrigin(disp, gc, x + theme.bevelWidth(), y + theme.bevelWidth());

            drawable.copyArea(m_pixmap.drawable(),
                              gc,
                              0, 0,
                              x + theme.bevelWidth(), y + theme.bevelWidth(),
                              m_pixmap.width(), m_pixmap.height());

            // restore clip mask
            XSetClipMask(disp, gc, None);
        }
    }
    FbTk::MenuItem::draw(drawable, theme, highlight, 
                         draw_background, draw_foreground, x, y, width, height);
}

unsigned int MenuIcon::width(const MenuTheme &theme) const {
    return MenuItem::width(theme) + 2 * (theme.bevelWidth()  + height(theme));
}

} // end namespace FbTk

