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

/// $Id: WinButton.cc,v 1.1 2003/01/05 22:48:54 fluxgen Exp $

#include "WinButton.hh"
#include "App.hh"

WinButton::WinButton(Type buttontype, const FbTk::FbWindow &parent,
                     int x, int y,
                     unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_type(buttontype) {

}

void WinButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawType();
}

void WinButton::drawType() {
    if (gc() == 0) // must have valid graphic context
        return;

    Display *disp = FbTk::App::instance()->display();
    switch (m_type) {
    case MAXIMIZE:
        XDrawRectangle(disp, window().window(),
                       gc(),
                       2, 2, width() - 5, height() - 5);
        XDrawLine(disp, window().window(),
                  gc(),
                  2, 3, width() - 3, 3);
        break;
    case MINIMIZE:
        XDrawRectangle(disp, window().window(),
                       gc(),
                       2, height() - 5, width() - 5, 2);
        break;
    case STICK:
        //        if (m_stuck) {
        XFillRectangle(disp, window().window(), 
                       gc(),
                       width()/2 - width()/4, height()/2 - height()/4,
                       width()/2, height()/2);
        /*                  } else {            
          XFillRectangle(disp, window().window(), 
          gc(),
          width()/2, height()/2,
          width()/5, height()/5);
          }
        */
        break;
    case CLOSE:    
        XDrawLine(disp, window().window(),
                  gc(), 
                  2, 2,
                  width() - 3, height() - 3);
        XDrawLine(disp, window().window(),
                  gc(), 
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
