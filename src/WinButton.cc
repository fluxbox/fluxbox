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

/// $Id: WinButton.cc,v 1.3 2003/04/25 17:35:28 fluxgen Exp $

#include "WinButton.hh"
#include "App.hh"
#include "Window.hh"

WinButton::WinButton(const FluxboxWindow &listen_to, 
                     Type buttontype, const FbTk::FbWindow &parent,
                     int x, int y,
                     unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_type(buttontype), m_listen_to(listen_to) {

}

void WinButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawType();
}

void WinButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
    clear();
}

void WinButton::drawType() {
    if (gc() == 0) // must have valid graphic context
        return;

    switch (m_type) {
    case MAXIMIZE:
        window().drawRectangle(gc(),
                               2, 2, width() - 5, height() - 5);
        window().drawLine(gc(),
                          2, 3, width() - 3, 3);
        break;
    case MINIMIZE:
        window().drawRectangle(gc(),
                               2, height() - 5, width() - 5, 2);
        break;
    case STICK:
        if (m_listen_to.isStuck()) {
            window().fillRectangle(gc(),
                                   width()/2 - width()/4, height()/2 - height()/4,
                                   width()/2, height()/2);
        } else {
            window().fillRectangle(gc(),
                           width()/2, height()/2,
                           width()/5, height()/5);
        }

        break;
    case CLOSE:    
        window().drawLine(gc(), 
                          2, 2,
                          width() - 3, height() - 3);
        window().drawLine(gc(), 
                          2, width() - 3, 
                          height() - 3, 2);
        break;
    case SHADE:
        break;
    }
}

void WinButton::clear() {
    FbTk::Button::clear();
    drawType();
}

void WinButton::update(FbTk::Subject *subj) {
    clear();
}
