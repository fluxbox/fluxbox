// ArrowButton.cc for Fluxbox Window Manager
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ArrowButton.cc,v 1.3 2003/05/17 11:30:59 fluxgen Exp $

#include "ArrowButton.hh"

ArrowButton::ArrowButton(ArrowButton::Type arrow_type,
                         FbTk::FbWindow &parent,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0) {

    window().setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
                          EnterWindowMask | LeaveWindowMask);
}

ArrowButton::ArrowButton(ArrowButton::Type arrow_type,
                         int screen_num,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(screen_num, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0) {

    window().setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
                          EnterWindowMask | LeaveWindowMask);
}

void ArrowButton::clear() {
    FbTk::Button::clear();
    drawArrow();
}

void ArrowButton::exposeEvent(XExposeEvent &event) {
    FbTk::Button::exposeEvent(event);
    drawArrow();
}

void ArrowButton::buttonPressEvent(XButtonEvent &event) {
    FbTk::Button::buttonPressEvent(event);
    drawArrow();
}

void ArrowButton::buttonReleaseEvent(XButtonEvent &event) {
    FbTk::Button::buttonReleaseEvent(event);
    drawArrow();
}

void ArrowButton::enterNotifyEvent(XCrossingEvent &ce) {
    if (m_mouse_handler)
	m_mouse_handler->enterNotifyEvent(ce);
}

void ArrowButton::leaveNotifyEvent(XCrossingEvent &ce) {
    if (m_mouse_handler)
	m_mouse_handler->leaveNotifyEvent(ce);
}

/**
 redraws the arrow button
*/
void ArrowButton::drawArrow() {
    XPoint pts[3];
    unsigned int w = width() / 2;
    unsigned int h = height() / 2;
    switch (m_arrow_type) {
    case LEFT:
        pts[0].x = w - 2; pts[0].y = h;
        pts[1].x = 4; pts[1].y = 2;
        pts[2].x = 0; pts[2].y = -4;
        break;
    case RIGHT:
        pts[0].x = w - 2; pts[0].y = h - 2;
        pts[1].x = 4; pts[1].y = 2;
        pts[2].x = -4; pts[2].y = 2;
        break;
    case UP: // TODO
        break;
    case DOWN: // TODO
        break;
    }

    if (gc() != 0) {
        window().fillPolygon(gc(),
                             pts, 3, 
                             Convex, CoordModePrevious);
    }
}

