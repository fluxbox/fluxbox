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

/// $Id: WinButton.cc,v 1.12 2003/09/12 23:36:16 fluxgen Exp $

#include "WinButton.hh"
#include "App.hh"
#include "Window.hh"
#include "WinButtonTheme.hh"

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
    updateTransparent();
}

void WinButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
    clear();
}

void WinButton::drawType() {
    bool used = false;

    switch (m_type) {
    case MAXIMIZE:

        if (pressed() && m_theme.maximizePressedPixmap().pixmap().drawable() != 0) { 
            FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                maximizePressedPixmap().
                                                pixmap().drawable());
        } else {
            // check focus 
            if (!m_listen_to.isFocused()) {
                if (m_theme.maximizeUnfocusPixmap().pixmap().drawable() != 0) {
                    // not focused
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        maximizeUnfocusPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            } else if (m_theme.maximizePixmap().pixmap().drawable() != 0) {
                FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                    maximizePixmap().
                                                    pixmap().drawable());
                used = true;
            }
        }
        if (used)
            FbTk::FbWindow::clear();
            

        // if no pixmap was used, use old style
        if (!used) {            
            if (gc() == 0) // must have valid graphic context
                return;
            drawRectangle(gc(),
                          2, 2, width() - 5, height() - 5);
            drawLine(gc(),
                     2, 3, width() - 3, 3);
        }
        break;
    case MINIMIZE:

        if (pressed() && m_theme.iconifyPressedPixmap().pixmap().drawable() != 0) { 
            FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                iconifyPressedPixmap().
                                                pixmap().drawable());
            used = true;
        } else {
            if (m_theme.iconifyPixmap().pixmap().drawable()){
                // check focus 
                if (!m_listen_to.isFocused()) {
                    if (m_theme.iconifyUnfocusPixmap().pixmap().drawable() != 0) {
                        // not focused
                        FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                            iconifyUnfocusPixmap().
                                                            pixmap().drawable());
                        used = true;
                    }
                } else if (m_theme.iconifyPixmap().pixmap().drawable() != 0) {
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        iconifyPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            }

            if (used)
                FbTk::FbWindow::clear();
            
        } 

        if (!used) {
            if (gc() == 0) // must have valid graphic context
                return;
            FbTk::FbWindow::drawRectangle(gc(),
                                          2, height() - 5, width() - 5, 2);
        }
        break;
    case STICK:

        if (m_listen_to.isStuck() && !pressed()) {
            if ( m_theme.stuckPixmap().pixmap().drawable() &&
                 ! pressed()) { // we're using the same pixmap for pressed as in not stuck
                // check focus 
                if (!m_listen_to.isFocused()) {
                    if ( m_theme.stuckUnfocusPixmap().pixmap().drawable() != 0) {
                        // not focused
                        FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                            stuckUnfocusPixmap().
                                                            pixmap().drawable());
                        used = true;
                    }
                } else if (m_theme.stuckPixmap().pixmap().drawable() != 0) {
                    // focused
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        stuckPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            }
        } else { // not stuck and pressed

            if (pressed()) {
                if (m_theme.stickPressedPixmap().pixmap().drawable() == 0) { 
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        stickPressedPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            } else { // not pressed
                // check focus 
                if (!m_listen_to.isFocused()) {
                    if (m_theme.stickUnfocusPixmap().pixmap().drawable() != 0) {
                        // not focused
                        FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                            stickUnfocusPixmap().
                                                            pixmap().drawable());
                        used = true;
                    }
                } else if (m_theme.stickPixmap().pixmap().drawable()) { // focused
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        stickPixmap().
                                                        pixmap().drawable());
                    used = true;
                }

            }
            
        } 

        if (used)
            FbTk::FbWindow::clear();
        else if (gc() != 0) {
            if (m_listen_to.isStuck()) {
                fillRectangle(gc(),
                              width()/2 - width()/4, height()/2 - height()/4,
                              width()/2, height()/2);
            } else {
                fillRectangle(gc(),
                              width()/2 - width()/10, height()/2 - height()/10,
                              width()/5, height()/5);
            }
        }
        break;
    case CLOSE:
        
        if (pressed()) { 
            if (m_theme.closePressedPixmap().pixmap().drawable()) {
                FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                    closePressedPixmap().
                                                    pixmap().drawable());
                used = true;
            }
        } else { // not pressed
            // check focus 
            if (!m_listen_to.isFocused()) {
                if (m_theme.closeUnfocusPixmap().pixmap().drawable() != 0) {
                    // not focused
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        closeUnfocusPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            } else if (m_theme.closePixmap().pixmap().drawable() != 0) { // focused
                FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                    closePixmap().
                                                    pixmap().drawable());
                used = true;
            }
        }

            
        if (used)
            FbTk::FbWindow::clear();            
        else if (gc() != 0) { // must have valid graphic context

            drawLine(gc(), 
                     2, 2,
                     width() - 3, height() - 3);
            drawLine(gc(), 
                     2, width() - 3, 
                     height() - 3, 2);
        }
        break;
    case SHADE:

        if (pressed()) { 
            if (m_theme.shadePressedPixmap().pixmap().drawable()) {
                FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                    shadePressedPixmap().
                                                    pixmap().drawable());
                used = true;
            }
        } else { // not pressed
            // check focus 
            if (!m_listen_to.isFocused()) {
                if ( m_theme.shadeUnfocusPixmap().pixmap().drawable() != 0) {
                    // not focused
                    FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                        shadeUnfocusPixmap().
                                                        pixmap().drawable());
                    used = true;
                }
            } else if (m_theme.shadePixmap().pixmap().drawable() != 0) { // focused
                FbTk::FbWindow::setBackgroundPixmap(m_theme.
                                                    shadePixmap().
                                                    pixmap().drawable());
                used = true;
            }
        }


        FbTk::FbWindow::clear();            

        break;
    }
    updateTransparent();
}

void WinButton::clear() {
    FbTk::Button::clear();
    drawType();
}

void WinButton::update(FbTk::Subject *subj) {
    clear();
    drawType();
}
