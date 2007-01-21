// WinButton.cc for Fluxbox Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

/// $Id$

#include <X11/Xlib.h>

#include "WinButton.hh"
#include "App.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "WinButtonTheme.hh"
#include "FbTk/Color.hh"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE


WinButton::WinButton(const FluxboxWindow &listen_to, 
                     WinButtonTheme &theme,
                     Type buttontype, const FbTk::FbWindow &parent,
                     int x, int y,
                     unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_type(buttontype), m_listen_to(listen_to), m_theme(theme),
    m_icon_pixmap(0), m_icon_mask(0),
    m_override_bg(false) {
    theme.reconfigSig().attach(this);

    update(0);
}

void WinButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawType();
}

void WinButton::buttonPressEvent(XButtonEvent &event) {
    FbTk::Button::buttonPressEvent(event);
    drawType();
}

void WinButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
    drawType();
}
 
// when someone else tries to set the background, we may override it
void WinButton::setBackgroundPixmap(Pixmap pm) {
    if (m_override_bg)
        return;
    FbTk::Button::setBackgroundPixmap(pm);
}
 
void WinButton::setBackgroundColor(const FbTk::Color &color) {
    if (m_override_bg)
        return;
    FbTk::Button::setBackgroundColor(color);
}

void WinButton::setPressedPixmap(Pixmap pm) {
    if (m_override_bg)
        return;
    FbTk::Button::setPressedPixmap(pm);
}

void WinButton::setPressedColor(const FbTk::Color &color) {
    if (m_override_bg)
        return;
    FbTk::Button::setPressedColor(color);
}

// clear is used to force this to clear the window (e.g. called from clear())
void WinButton::drawType() {

    // if it's odd and we're centring, we need to add one
    int oddW = width()%2;
    int oddH = height()%2;

    bool is_pressed = pressed();
    bool focused = m_listen_to.isFocused();
    if (gc() == 0)
        return;

    // check for pixmap in style, otherwise draw old style imagery
    FbTk::PixmapWithMask style_pixmap;
    switch (m_type) {
    case MAXIMIZE:
        if (is_pressed)
            style_pixmap = m_theme.maximizePressedPixmap();
        else if (focused)
            style_pixmap = m_theme.maximizePixmap();
        else
            style_pixmap = m_theme.maximizeUnfocusPixmap();

        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else {
            // if no pixmap was used, use old style
            drawRectangle(gc(),
                          2, 2, width() - 5, height() - 5);
            drawLine(gc(),
                     2, 3, width() - 3, 3);
        }
        break;
    case MINIMIZE:
        if (is_pressed)
            style_pixmap = m_theme.iconifyPressedPixmap();
        else if (focused)
            style_pixmap = m_theme.iconifyPixmap();
        else
            style_pixmap = m_theme.iconifyUnfocusPixmap();

        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else
            drawRectangle(gc(), 2, height() - 5, width() - 5, 2);

        break;
    case STICK:
        if (is_pressed)
            style_pixmap = m_theme.stickPressedPixmap();
        else if (m_listen_to.isStuck()) {
            if (focused)
                style_pixmap = m_theme.stuckPixmap();
            else
                style_pixmap = m_theme.stuckUnfocusPixmap();
        } else {
            if (focused)
                style_pixmap = m_theme.stickPixmap();
            else
                style_pixmap = m_theme.stickUnfocusPixmap();
        }
        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else {
            // width/4 != width/2, so we use /4*2 so that it's properly centred
            if (m_listen_to.isStuck()) {
                fillRectangle(gc(),
                              width()/2 - width()/4, height()/2 - height()/4,
                              width()/4*2 + oddW, height()/4*2 + oddH);
            } else {
                fillRectangle(gc(),
                              width()/2 - width()/10, height()/2 - height()/10,
                              width()/10*2 + oddW, height()/10*2 + oddH);
            }
        }
        break;
    case CLOSE:
        if (is_pressed)
            style_pixmap = m_theme.closePressedPixmap();
        else if (focused)
            style_pixmap = m_theme.closePixmap();
        else
            style_pixmap = m_theme.closeUnfocusPixmap();

        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else {
            drawLine(gc(), 
                     2, 2,
                     width() - 3, height() - 3);
        // I can't figure out why this second one needs a y offset of 1?????
        // but it does - at least on my box:
        //   XFree86 Version 4.2.1.1 (Debian 4.2.1-12.1 20031003005825)
        //   (protocol Version 11, revision 0, vendor release 6600)
        // But not on mine? It's wonky. Put back to the same coords.
        //  was width-2, 1 in the second drawline
        // Perhaps some X versions don't draw the endpoint?
        // Mine:
        // XFree86 Version 4.3.0.1 (Debian 4.3.0.dfsg.1-1 20040428170728)
        // (X Protocol Version 11, Revision 0, Release 6.6)

            drawLine(gc(), 
                     2, height() - 3,
                     width() - 3, 2);
        }
        break;
    case SHADE:
        if (is_pressed) {
            if (m_listen_to.isShaded())
                style_pixmap = m_theme.unshadePressedPixmap();
            else
                style_pixmap = m_theme.shadePressedPixmap();
        } else if (m_listen_to.isShaded()) {
            if (focused)
                style_pixmap = m_theme.unshadePixmap();
            else
                style_pixmap = m_theme.unshadeUnfocusPixmap();
        } else {
            if (focused)
                style_pixmap = m_theme.shadePixmap();
            else
                style_pixmap = m_theme.shadeUnfocusPixmap();
        }
        
        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else {
            int size = width() - 5 - oddW;

            drawRectangle(gc(), 2, 2, size, 2);

            // draw a one-quarter triangle below the rectangle
            drawTriangle(gc(), (m_listen_to.isShaded() ?
                                FbTk::FbDrawable::DOWN:
                                FbTk::FbDrawable::UP),
                         4, 6, 
                         size-2, size/2 - 1, 
                         100);
        }
        break;
    case MENUICON:
        // if we got an icon from the window, use it instead
        if (m_icon_pixmap.drawable()) {
            drawIcon(m_icon_pixmap, m_icon_mask);
            return;
        }

        if (is_pressed)
            style_pixmap = m_theme.menuiconPressedPixmap();
        else if (focused)
            style_pixmap = m_theme.menuiconPixmap();
        else
            style_pixmap = m_theme.menuiconUnfocusPixmap();

        if (style_pixmap.pixmap().drawable())
            drawIcon(style_pixmap.pixmap(), style_pixmap.mask());
        else {
            for (unsigned int y = height()/3; y <= height() - height()/3; y+=3) {
                drawLine(gc(), width()/4, y, width() - width()/4 - oddW - 1, y);
            }
            drawRectangle(gc(),
                      2, 2, width() - 5, height() - 5);
        }
        break;
    }
}

void WinButton::drawIcon(FbTk::FbPixmap icon, FbTk::FbPixmap mask) {
    if (mask.drawable()) {
        XSetClipMask(m_listen_to.fbWindow().display(), 
                     gc(), mask.drawable());
        XSetClipOrigin(m_listen_to.fbWindow().display(), 
                       gc(), 0, 0);
    }

    copyArea(icon.drawable(),
             gc(),
             0, 0, 
             0, 0, 
             icon.width(), icon.height());

    if (mask.drawable())
        XSetClipMask(m_listen_to.fbWindow().display(), gc(), None);
}

void WinButton::clear() {
    FbTk::Button::clear();
    drawType();
}

void WinButton::update(FbTk::Subject *subj) {

    // update the menu icon
    if (m_type == MENUICON && !m_listen_to.empty()) {
       
        Display* display = m_listen_to.fbWindow().display();
        int screen = m_listen_to.screen().screenNumber();
        if (m_listen_to.usePixmap()) {
             m_icon_pixmap.copy(m_listen_to.iconPixmap().drawable(), 
                                DefaultDepth(display, screen), screen);
             m_icon_pixmap.scale(width(), height());
        } else
            m_icon_pixmap.release();
            
        if (m_listen_to.useMask()) {
            m_icon_mask.copy(m_listen_to.iconMask().drawable(), 0, 0);
            m_icon_mask.scale(width(), height());
        } else
            m_icon_mask.release();
        
    }

    // pressed_pixmap isn't stateful in any current buttons, so no need
    // to potentially override that. Just make sure background pm is ok
    Pixmap my_pm;
    if (m_listen_to.isFocused())
        my_pm = m_theme.titleFocusPixmap().pixmap().drawable();
    else
        my_pm = m_theme.titleUnfocusPixmap().pixmap().drawable();

    if (my_pm == None)
        m_override_bg = false;
    else {
        FbTk::Button::setPressedPixmap(my_pm);
        FbTk::Button::setBackgroundPixmap(my_pm);
        m_override_bg = true;
    }

    clear();
}
