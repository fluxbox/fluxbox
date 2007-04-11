// IconButton.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "IconButton.hh"
#include "IconbarTool.hh"
#include "IconbarTheme.hh"

#include "Screen.hh"
#include "Focusable.hh"

#include "FbTk/App.hh"
#include "FbTk/Command.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/Menu.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE


IconButton::IconButton(const FbTk::FbWindow &parent, IconbarTheme &theme,
                       Focusable &win):
    FbTk::TextButton(parent, theme.focusedText().font(), win.title()),
    m_win(win), 
    m_icon_window(*this, 1, 1, 1, 1, 
                  ExposureMask | ButtonPressMask | ButtonReleaseMask),
    m_use_pixmap(true),
    m_theme(theme),
    m_focused_pm(win.screen().imageControl()),
    m_unfocused_pm(win.screen().imageControl()) {

    m_win.titleSig().attach(this);
    m_win.focusSig().attach(this);
    m_win.attentionSig().attach(this);
    
    FbTk::EventManager::instance()->add(*this, m_icon_window);

    reconfigTheme();
    update(0);
}

IconButton::~IconButton() {
    // ~FbWindow cleans event manager
}


void IconButton::exposeEvent(XExposeEvent &event) {
    if (m_icon_window == event.window)
        m_icon_window.clear();
    else
        FbTk::TextButton::exposeEvent(event);
}

void IconButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {

    FbTk::TextButton::moveResize(x, y, width, height);

    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height()) {
        update(0); // update icon window
    }
}

void IconButton::resize(unsigned int width, unsigned int height) {
    FbTk::TextButton::resize(width, height);
    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height()) {
        update(0); // update icon window
    }
}

void IconButton::clear() {
    setupWindow();
}

void IconButton::clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposure) {
    FbTk::TextButton::clearArea(x, y,
                                width, height, exposure);
}

void IconButton::setPixmap(bool use) {
    if (m_use_pixmap != use) {
        m_use_pixmap = use;
        update(0);
    }
}

void IconButton::reconfigTheme() {

    if (m_theme.focusedTexture().usePixmap())
        m_focused_pm.reset(m_win.screen().imageControl().renderImage(
                            width(), height(), m_theme.focusedTexture(),
                            orientation()));
    else
        m_focused_pm.reset(0);

    if (m_theme.unfocusedTexture().usePixmap())
        m_unfocused_pm.reset(m_win.screen().imageControl().renderImage(
                              width(), height(), m_theme.unfocusedTexture(),
                              orientation()));
    else
        m_unfocused_pm.reset(0);

    setAlpha(parent()->alpha());

    if (m_win.isFocused() || m_win.getAttentionState()) {
        if (m_focused_pm != 0)
            setBackgroundPixmap(m_focused_pm);
        else
            setBackgroundColor(m_theme.focusedTexture().color());

        setGC(m_theme.focusedText().textGC());
        setFont(m_theme.focusedText().font());
        setJustify(m_theme.focusedText().justify());
        setBorderWidth(m_theme.focusedBorder().width());
        setBorderColor(m_theme.focusedBorder().color());

    } else {
        if (m_unfocused_pm != 0)
            setBackgroundPixmap(m_unfocused_pm);
        else
            setBackgroundColor(m_theme.unfocusedTexture().color());

        setGC(m_theme.unfocusedText().textGC());
        setFont(m_theme.unfocusedText().font());
        setJustify(m_theme.unfocusedText().justify());
        setBorderWidth(m_theme.unfocusedBorder().width());
        setBorderColor(m_theme.unfocusedBorder().color());

    }

}

void IconButton::update(FbTk::Subject *subj) {
    // if the window's focus state changed, we need to update the background
    if (subj == &m_win.focusSig() || subj == &m_win.attentionSig()) {
        reconfigTheme();
        clear();
        return;
    }

    // we got signal that either title or 
    // icon pixmap was updated, 
    // so we refresh everything

    Display *display = FbTk::App::instance()->display();
    int screen = m_win.screen().screenNumber();

    if (m_use_pixmap && m_win.icon().pixmap().drawable() != None) {
        // setup icon window
        m_icon_window.show();
        unsigned int w = width();
        unsigned int h = height();
        FbTk::translateSize(orientation(), w, h);
        int iconx = 1, icony = 1;
        unsigned int neww = w, newh = h;
        if (newh > 2*static_cast<unsigned>(icony))
            newh -= 2*icony;
        else
            newh = 1;
        neww = newh;

        FbTk::translateCoords(orientation(), iconx, icony, w, h);
        FbTk::translatePosition(orientation(), iconx, icony, neww, newh, 0);
        
        m_icon_window.moveResize(iconx, icony, neww, newh);

        m_icon_pixmap.copy(m_win.icon().pixmap().drawable(),
                           DefaultDepth(display, screen), screen);
        m_icon_pixmap.scale(m_icon_window.width(), m_icon_window.height());

        // rotate the icon or not?? lets go not for now, and see what they say...
        // need to rotate mask too if we do do this
        m_icon_pixmap.rotate(orientation());

        m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable());
    } else {
        // no icon pixmap
        m_icon_window.move(0, 0);
        m_icon_window.hide();
        m_icon_pixmap = 0;
    }

    if(m_icon_pixmap.drawable() && m_win.icon().mask().drawable() != None) {
        m_icon_mask.copy(m_win.icon().mask().drawable(), 0, 0);
        m_icon_mask.scale(m_icon_pixmap.width(), m_icon_pixmap.height());
        m_icon_mask.rotate(orientation());
    } else
        m_icon_mask = 0;

#ifdef SHAPE

    XShapeCombineMask(display,
                      m_icon_window.drawable(),
                      ShapeBounding,
                      0, 0,
                      m_icon_mask.drawable(),
                      ShapeSet);

#endif // SHAPE

    if (subj != 0) {
        setupWindow();
    } else {
        m_icon_window.clear();
    }
}

void IconButton::setupWindow() {
    m_icon_window.clear();
    setText(m_win.title());
    FbTk::TextButton::clear();
}

void IconButton::drawText(int x, int y, FbTk::FbDrawable *drawable) {
    // offset text
    if (m_icon_pixmap.drawable() != 0)
        FbTk::TextButton::drawText(m_icon_window.x() + m_icon_window.width() + 1, y, drawable);
    else
        FbTk::TextButton::drawText(1, y, drawable);
}
                          
bool IconButton::setOrientation(FbTk::Orientation orient) {
    if (orientation() == orient)
        return true;

    if (FbTk::TextButton::setOrientation(orient)) {
        int iconx = 1, icony = 1;
        unsigned int tmpw = width(), tmph = height();
        FbTk::translateSize(orient, tmpw, tmph);
        FbTk::translateCoords(orient, iconx, icony, tmpw, tmph);
        FbTk::translatePosition(orient, iconx, icony, m_icon_window.width(), m_icon_window.height(), 0);
        m_icon_window.move(iconx, icony);
        return true;
    } else {
        return false;
    }
}

