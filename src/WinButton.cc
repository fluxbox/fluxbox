// WinButton.cc for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

/// $Id: WinButton.cc,v 1.7 2003/08/04 12:52:39 fluxgen Exp $

#include "WinButton.hh"
#include "App.hh"
#include "Window.hh"
#include "WinButtonTheme.hh"

namespace {

inline void scale(const FbTk::Button &btn, WinButtonTheme::PixmapWithMask &pm) {
    // copy pixmap and scale it
    pm.pixmap_scaled = pm.pixmap;
    pm.mask_scaled = pm.mask;

    if (pm.pixmap_scaled.drawable() != 0)
        pm.pixmap_scaled.scale(btn.width(), btn.height());
    if (pm.mask_scaled.drawable() != 0)
        pm.mask_scaled.scale(btn.width(), btn.height());
}

void updateScale(const FbTk::Button &btn, WinButtonTheme &theme) {

    // we need to scale our pixmaps to right size
    scale(btn, theme.closePixmap());
    scale(btn, theme.closeUnfocusPixmap());
    scale(btn, theme.closePressedPixmap());

    scale(btn, theme.maximizePixmap());
    scale(btn, theme.maximizeUnfocusPixmap());
    scale(btn, theme.maximizePressedPixmap());

    scale(btn, theme.iconifyPixmap());
    scale(btn, theme.iconifyUnfocusPixmap());
    scale(btn, theme.iconifyPressedPixmap());

    scale(btn, theme.shadePixmap());
    scale(btn, theme.shadeUnfocusPixmap());
    scale(btn, theme.shadePressedPixmap());

    scale(btn, theme.stickPixmap());
    scale(btn, theme.stickUnfocusPixmap());
    scale(btn, theme.stickPressedPixmap());

    scale(btn, theme.stuckPixmap());
    scale(btn, theme.stuckUnfocusPixmap());
}

};

WinButton::WinButton(const FluxboxWindow &listen_to, 
                     WinButtonTheme &theme,
                     Type buttontype, const FbTk::FbWindow &parent,
                     int x, int y,
                     unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_type(buttontype), m_listen_to(listen_to), m_theme(theme) {

    theme.reconfigSig().attach(this);
}

void WinButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawType();
    window().updateTransparent();
}

void WinButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
    clear();
}

void WinButton::drawType() {


    switch (m_type) {
    case MAXIMIZE:
        if (m_theme.maximizePixmap().pixmap_scaled.drawable() != 0) {
            if (pressed()) { 
                window().setBackgroundPixmap(m_theme.
                                             maximizePressedPixmap().
                                             pixmap_scaled.drawable());
            } else if (m_theme.maximizePixmap().pixmap_scaled.drawable()) {
                // check focus 
                if (!m_listen_to.isFocused() && 
                    m_theme.maximizeUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                    // not focused
                    window().setBackgroundPixmap(m_theme.
                                                 maximizeUnfocusPixmap().
                                                 pixmap_scaled.drawable());
                } else { // focused
                    window().setBackgroundPixmap(m_theme.
                                                 maximizePixmap().
                                                 pixmap_scaled.drawable());
                }
            }
            
            window().clear();
            
        } else {
            if (gc() == 0) // must have valid graphic context
                return;
            window().drawRectangle(gc(),
                                   2, 2, width() - 5, height() - 5);
            window().drawLine(gc(),
                              2, 3, width() - 3, 3);
        }
        break;
    case MINIMIZE:
        if (m_theme.iconifyPixmap().pixmap_scaled.drawable() != 0) {
            if (pressed()) { 
                window().setBackgroundPixmap(m_theme.
                                             iconifyPressedPixmap().
                                             pixmap_scaled.drawable());
            } else if (m_theme.iconifyPixmap().pixmap_scaled.drawable()){
                // check focus 
                if (!m_listen_to.isFocused() && 
                    m_theme.iconifyUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                    // not focused
                    window().setBackgroundPixmap(m_theme.
                                                 iconifyUnfocusPixmap().
                                                 pixmap_scaled.drawable());
                } else { // focused
                    window().setBackgroundPixmap(m_theme.
                                                 iconifyPixmap().
                                                 pixmap_scaled.drawable());
                }
            }
            
            window().clear();
            
        } else {
            if (gc() == 0) // must have valid graphic context
                return;
            window().drawRectangle(gc(),
                                   2, height() - 5, width() - 5, 2);
        }
        break;
    case STICK:
        if (m_theme.stickPixmap().pixmap_scaled.drawable() != 0) {
            if (m_listen_to.isStuck() && 
                m_theme.stuckPixmap().pixmap_scaled.drawable() &&
                ! pressed()) { // we're using the same pixmap for pressed as in not stuck
                // check focus 
                if (!m_listen_to.isFocused() && 
                    m_theme.stuckUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                    // not focused
                    window().setBackgroundPixmap(m_theme.
                                                 stuckUnfocusPixmap().
                                                 pixmap_scaled.drawable());
                } else { // focused
                    window().setBackgroundPixmap(m_theme.
                                                 stuckPixmap().
                                                 pixmap_scaled.drawable());
                }
            } else { // not stuck

                if (pressed()) { 
                    window().setBackgroundPixmap(m_theme.
                                                 stickPressedPixmap().
                                                 pixmap_scaled.drawable());

                } else if (m_theme.stickPixmap().pixmap_scaled.drawable()) {
                    // check focus 
                    if (!m_listen_to.isFocused() && 
                        m_theme.stickUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                        // not focused
                        window().setBackgroundPixmap(m_theme.
                                                     stickUnfocusPixmap().
                                                     pixmap_scaled.drawable());
                    } else { // focused
                        window().setBackgroundPixmap(m_theme.
                                                     stickPixmap().
                                                     pixmap_scaled.drawable());
                    }

                }
            } // end if stuck

            window().clear();
            
        } else {
            if (m_listen_to.isStuck()) {
                window().fillRectangle(gc(),
                                       width()/2 - width()/4, height()/2 - height()/4,
                                       width()/2, height()/2);
            } else {
                if (gc() == 0) // must have valid graphic context
                    return;
                window().fillRectangle(gc(),
                                       width()/2 - width()/10, height()/2 - height()/10,
                                       width()/5, height()/5);
            }
        }
        break;
    case CLOSE:
        
        if (m_theme.closePixmap().pixmap_scaled.drawable() != 0) {
            if (pressed()) { 
                window().setBackgroundPixmap(m_theme.
                                             closePressedPixmap().
                                             pixmap_scaled.drawable());

            } else if (m_theme.closePixmap().pixmap_scaled.drawable()) {
                // check focus 
                if (!m_listen_to.isFocused() && 
                    m_theme.closeUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                    // not focused
                    window().setBackgroundPixmap(m_theme.
                                                 closeUnfocusPixmap().
                                                 pixmap_scaled.drawable());
                } else { // focused
                    window().setBackgroundPixmap(m_theme.
                                                 closePixmap().
                                                 pixmap_scaled.drawable());
                }
            }
            
            window().clear();
            
        } else {
            if (gc() == 0) // must have valid graphic context
                return;
            window().drawLine(gc(), 
                              2, 2,
                              width() - 3, height() - 3);
            window().drawLine(gc(), 
                              2, width() - 3, 
                              height() - 3, 2);
        }
        break;
    case SHADE:
        if (m_theme.shadePixmap().pixmap_scaled.drawable() != 0) {
            if (pressed()) { 
                window().setBackgroundPixmap(m_theme.
                                             shadePressedPixmap().
                                             pixmap_scaled.drawable());
            } else if (m_theme.shadePixmap().pixmap_scaled.drawable()) {
                // check focus 
                if (!m_listen_to.isFocused() && 
                    m_theme.shadeUnfocusPixmap().pixmap_scaled.drawable() != 0) {
                    // not focused
                    window().setBackgroundPixmap(m_theme.
                                                 shadeUnfocusPixmap().
                                                 pixmap_scaled.drawable());
                } else { // focused
                    window().setBackgroundPixmap(m_theme.
                                                 shadePixmap().
                                                 pixmap_scaled.drawable());
                }
            }
            
            window().clear();            
        }
        break;
    }
}

void WinButton::clear() {
    FbTk::Button::clear();
    drawType();
}

void WinButton::update(FbTk::Subject *subj) {
    clear();

    //!! TODO 
    // We need to optimize this
    // This shouldn't be run on every button in every fluxbox window
    updateScale(*this, m_theme);
    drawType();
}
