// IconButton.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: IconButton.cc,v 1.1 2003/08/11 15:45:50 fluxgen Exp $

#include "IconButton.hh"

#include "FbTk/App.hh"
#include "FbTk/EventManager.hh"

#include "IconButtonTheme.hh"
#include "Window.hh"
#include "WinClient.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <iostream>
using namespace std;

IconButton::IconButton(const FbTk::FbWindow &parent, const FbTk::Font &font,
                       FluxboxWindow &win):
    TextButton(parent, font, win.winClient().title()),
    m_win(win), 
    m_icon_window(window(), 1, 1, 1, 1, 
                  ExposureMask | ButtonPressMask | ButtonReleaseMask) {

    m_win.hintSig().attach(this);

    FbTk::EventManager::instance()->add(*this, m_icon_window);
    
    update(0);
}

IconButton::~IconButton() {

}

void IconButton::exposeEvent(XExposeEvent &event) {
    if (m_icon_window == event.window)
        m_icon_window.clear();
    else
        FbTk::Button::exposeEvent(event);
}
void IconButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {

    FbTk::Button::moveResize(x, y, width, height);

    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::resize(unsigned int width, unsigned int height) {
    FbTk::Button::resize(width, height);
    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::clear() {
    FbTk::Button::clear();
    setupWindow();
}

void IconButton::update(FbTk::Subject *subj) {
    // we got signal that either title or 
    // icon pixmap was updated, 
    // so we refresh everything

    XWMHints *hints = XGetWMHints(FbTk::App::instance()->display(), m_win.winClient().window());
    if (hints == 0)
        return;

    if (hints->flags & IconPixmapHint) {
        // setup icon window
        m_icon_window.show();
        m_icon_window.resize(height(), height() - m_icon_window.y());

        m_icon_pixmap.copy(hints->icon_pixmap);
        m_icon_pixmap.scale(m_icon_window.height(), m_icon_window.height());

        m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable());
    } else {
        // no icon pixmap
        m_icon_window.hide();
        m_icon_pixmap = 0;
    }

    if(hints->flags & IconMaskHint) {
        m_icon_mask.copy(hints->icon_mask);
        m_icon_mask.scale(height(), height());
    } else
        m_icon_mask = 0;

    XFree(hints);

#ifdef SHAPE
    //!! TODO! bugs!?!
    /*
      if (m_icon_mask.drawable() != 0) {
        XShapeCombineMask(FbTk::App::instance()->display(),
                          m_icon_window.drawable(),
                          ShapeBounding,
                          0, 0,
                          m_icon_mask.drawable(),
                          ShapeSet);
    }
    */
#endif // SHAPE

    setupWindow();
}

void IconButton::setupWindow() {
    
    m_icon_window.clear();
    setText(m_win.winClient().title());
    // draw with x offset and y offset
    drawText(m_icon_window.x() + m_icon_window.width() + 1);
}

