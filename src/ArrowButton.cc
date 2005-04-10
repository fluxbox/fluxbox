// ArrowButton.cc for Fluxbox Window Manager
// Copyright (c) 2002 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "ArrowButton.hh"
#include "ButtonTheme.hh"

ArrowButton::ArrowButton(ArrowButton::Type arrow_type,
                         const FbTk::FbWindow &parent,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0),
    m_arrowscale(300) {

    setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask);
}

ArrowButton::ArrowButton(ArrowButton::Type arrow_type,
                         int screen_num,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(screen_num, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0),
    m_arrowscale(300) {

    setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
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
    unsigned int w = width();
    unsigned int h = height();

    int arrowscale_n = m_arrowscale;
    int arrowscale_d = 100;
    unsigned int ax = arrowscale_d * w / arrowscale_n;
    unsigned int ay = arrowscale_d * h / arrowscale_n;
    // if these aren't an even number, left and right arrows end up different
    if (( ax % 2 ) == 1) ax++;
    if (( ay % 2 ) == 1) ay++;
    switch (m_arrow_type) {
    case LEFT:
		// start at the tip
        pts[0].x = (w / 2) - (ax / 2); pts[0].y = h / 2;
        pts[1].x = ax; pts[1].y = -ay / 2;
        pts[2].x = 0; pts[2].y = ay;
        break;
    case RIGHT:
        pts[0].x = (w / 2) + (ax / 2); pts[0].y = h / 2;
        pts[1].x = - ax; pts[1].y = ay / 2;
        pts[2].x = 0; pts[2].y = - ay;
        break;
    case UP:
        pts[0].x = (w / 2); pts[0].y = (h / 2) - (ay / 2);
        pts[1].x = ax / 2; pts[1].y = ay;
        pts[2].x = - ax; pts[2].y = 0;
        break;
    case DOWN:
        pts[0].x = (w / 2); pts[0].y = (h / 2) + (ay / 2);
        pts[1].x = ax / 2; pts[1].y = - ay;
        pts[2].x = - ax; pts[2].y = 0;
        break;
    }

    if (gc() != 0) {
        fillPolygon(gc(),
                    pts, 3, 
                    Convex, CoordModePrevious);
    }
}

void ArrowButton::updateTheme(const FbTk::Theme &theme) {
    // it must be a button theme
    const ButtonTheme &btheme = static_cast<const ButtonTheme &>(theme);

    m_arrowscale = btheme.scale();
    if (m_arrowscale == 0) m_arrowscale = 300; // default is 0 => 300
    else if (m_arrowscale < 100) m_arrowscale = 100; // otherwise clamp
    else if (m_arrowscale > 100000) m_arrowscale = 100000; // clamp below overflow when *100
}
