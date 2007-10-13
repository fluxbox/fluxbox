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
    overrode_bg(false), overrode_pressed(false) {
    theme.reconfigSig().attach(this);

    if (buttontype == MENUICON)
        update(0);
}

void WinButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawType();
}

void WinButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
}

// when someone else tries to set the background, we may override it
void WinButton::setBackgroundPixmap(Pixmap pm) {
    Pixmap my_pm = getBackgroundPixmap();
    
    if (my_pm != 0) {
        overrode_bg = true;
        pm = my_pm;
    } else {
        overrode_bg = false;
    }

    FbTk::Button::setBackgroundPixmap(pm);
}

void WinButton::setBackgroundColor(const FbTk::Color &color) {
    Pixmap my_pm = getBackgroundPixmap();

    if (my_pm != 0) {
        overrode_bg = true;
        FbTk::Button::setBackgroundPixmap(my_pm);
    } else {
        overrode_bg = false;
        FbTk::Button::setBackgroundColor(color);
    }
}

void WinButton::setPressedPixmap(Pixmap pm) {
    Pixmap my_pm = getPressedPixmap();

    if (my_pm != 0) {
        overrode_pressed = true;
        pm = my_pm;
    } else {
        overrode_pressed = false;
    }

    FbTk::Button::setPressedPixmap(pm);
}

void WinButton::setPressedColor(const FbTk::Color &color) {
    Pixmap my_pm = getPressedPixmap();

    if (my_pm != 0) {
        overrode_pressed = true;
        FbTk::Button::setPressedPixmap(my_pm);
    } else {
        overrode_pressed = false;
        FbTk::Button::setPressedColor(color);
    }
}

Pixmap WinButton::getBackgroundPixmap() const {
    bool focused = m_listen_to.isFocused();
    switch(m_type) {
    case MAXIMIZE:
        if (focused)
            return m_theme.maximizePixmap().pixmap().drawable();
        else
            return m_theme.maximizeUnfocusPixmap().pixmap().drawable();
        break;
    case MINIMIZE:
        if (focused)
            return m_theme.iconifyPixmap().pixmap().drawable();
        else
            return m_theme.iconifyUnfocusPixmap().pixmap().drawable();
        break;
    case STICK:
        if (m_listen_to.isStuck()) {
            if (focused)
                return m_theme.stuckPixmap().pixmap().drawable();
            else
                return m_theme.stuckUnfocusPixmap().pixmap().drawable();
        } else {
            if (focused)
                return m_theme.stickPixmap().pixmap().drawable();
            else
                return m_theme.stickUnfocusPixmap().pixmap().drawable();
        }
        break;
    case CLOSE:
        if (focused)
            return m_theme.closePixmap().pixmap().drawable();
        else
            return m_theme.closeUnfocusPixmap().pixmap().drawable();
        break;
    case SHADE: 
        if (m_listen_to.isShaded()) {
            if (focused)
                return m_theme.unshadePixmap().pixmap().drawable();
            else
                return m_theme.unshadeUnfocusPixmap().pixmap().drawable();
        } else {
            if (focused)
                return m_theme.shadePixmap().pixmap().drawable();
            else
                return m_theme.shadeUnfocusPixmap().pixmap().drawable();
        }
        break;
    case MENUICON:
        if (m_icon_pixmap.drawable()) {
            if (focused)
                return m_theme.titleFocusPixmap().pixmap().drawable();
            else
                return m_theme.titleUnfocusPixmap().pixmap().drawable();
        } else {
            if (focused)
                return m_theme.menuiconPixmap().pixmap().drawable();
            else
                return m_theme.menuiconUnfocusPixmap().pixmap().drawable();
        }
        break;
    }
    return None;
}

Pixmap WinButton::getPressedPixmap() const {
    switch(m_type) {
    case MAXIMIZE:
        return m_theme.maximizePressedPixmap().pixmap().drawable();
    case MINIMIZE:
        return m_theme.iconifyPressedPixmap().pixmap().drawable();
    case STICK:
        return m_theme.stickPressedPixmap().pixmap().drawable();
    case CLOSE:
        return m_theme.closePressedPixmap().pixmap().drawable();
    case SHADE:
        if (m_listen_to.isShaded())
            return m_theme.unshadePressedPixmap().pixmap().drawable();
        else
            return m_theme.shadePressedPixmap().pixmap().drawable();
    case MENUICON:
        if (m_icon_pixmap.drawable())
            if (m_listen_to.isFocused())
                return m_theme.titleFocusPixmap().pixmap().drawable();
            else
                return m_theme.titleUnfocusPixmap().pixmap().drawable();
        else
            return m_theme.menuiconPressedPixmap().pixmap().drawable();
    }
    return None;
}

// clear is used to force this to clear the window (e.g. called from clear())
void WinButton::drawType() {

    // if it's odd and we're centring, we need to add one
    int oddW = width()%2;
    int oddH = height()%2;

    bool is_pressed = pressed();
    if (is_pressed && overrode_pressed && !m_icon_pixmap.drawable())
        return;
    if (!is_pressed && overrode_bg && !m_icon_pixmap.drawable())
        return;
    if (gc() == 0)
        return;

    // otherwise draw old style imagery
    switch (m_type) {
    case MAXIMIZE:
        // if no pixmap was used, use old style
        if (gc() == 0) // must have valid graphic context
            return;

        drawRectangle(gc(),
                      2, 2, width() - 5, height() - 5);
        drawLine(gc(),
                 2, 3, width() - 3, 3);
        break;
    case MINIMIZE:
        drawRectangle(gc(), 2, height() - 5, width() - 5, 2);
        break;
    case STICK:
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
        break;
    case CLOSE:
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
        break;
    case SHADE:
        
    {
        int size = width() - 5 - oddW;

        drawRectangle(gc(), 2, 2, size, 2);
        
        // draw a one-quarter triangle below the rectangle
        drawTriangle(gc(), (m_listen_to.isShaded() ?
                            FbTk::FbDrawable::DOWN:
                            FbTk::FbDrawable::UP),
                     4, 6, 
                     size-2, size/2 - 1, 
                     100);

        break;
    }
    case MENUICON:
        if (m_icon_pixmap.drawable()) {

            if (m_icon_mask.drawable()) {
                XSetClipMask(m_listen_to.fbWindow().display(), 
                             gc(), m_icon_mask.drawable());
                XSetClipOrigin(m_listen_to.fbWindow().display(), 
                             gc(), 2, 2);
            }
            
            copyArea(m_icon_pixmap.drawable(),
                     gc(),
                     0, 0, 
                     2, 2, 
                     m_icon_pixmap.width(), m_icon_pixmap.height());

            if (m_icon_mask.drawable())
                XSetClipMask(m_listen_to.fbWindow().display(), gc(), None);
        } else {
            for (unsigned int y = height()/3; y <= height() - height()/3; y+=3) {
                drawLine(gc(), width()/4, y, width() - width()/4 - oddW - 1, y);
            }
            drawRectangle(gc(),
                      2, 2, width() - 5, height() - 5);
        }
        break;
    }
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
        if (m_listen_to.icon().pixmap().drawable() != None) {
             m_icon_pixmap.copy(m_listen_to.icon().pixmap().drawable(), 
                                DefaultDepth(display, screen), screen);
             m_icon_pixmap.scale(width() - 4, height() - 4);
        } else
            m_icon_pixmap.release();
            
        if (m_listen_to.icon().mask().drawable() != None) {
            m_icon_mask.copy(m_listen_to.icon().mask().drawable(), 0, 0);
            m_icon_mask.scale(width() - 4, height() - 4);
        } else
            m_icon_mask.release();
        
    }

    // pressed_pixmap isn't stateful in any current buttons, so no need
    // to potentially override that. Just make sure background pm is ok
    Pixmap my_pm = getBackgroundPixmap();
    if (my_pm != None)
        setBackgroundPixmap(my_pm);

    // incorrect, pressed_pixmap is stateful in shade, so we'll do oneoff for now
    if (m_type == SHADE) {
        Pixmap p_pm = getPressedPixmap();
        if (p_pm != None)
            setPressedPixmap(p_pm);
    }
        

    clear();
}
