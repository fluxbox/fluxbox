// ArrowButton.cc for Fluxbox Window Manager
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "ArrowButton.hh"
#include "ButtonTheme.hh"

#include "FbTk/Util.hh"

ArrowButton::ArrowButton(FbTk::FbDrawable::TriangleType arrow_type,
                         const FbTk::FbWindow &parent,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(parent, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0),
    m_arrowscale(250) {

    setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask);
}

ArrowButton::ArrowButton(FbTk::FbDrawable::TriangleType arrow_type,
                         int screen_num,
                         int x, int y,
                         unsigned int width, unsigned int height):
    FbTk::Button(screen_num, x, y, width, height),
    m_arrow_type(arrow_type),
    m_mouse_handler(0),
    m_arrowscale(250) {

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
    if (gc() != 0)
        drawTriangle(gc(), m_arrow_type, 0, 0, width(), height(), m_arrowscale);
}

void ArrowButton::updateTheme(const FbTk::Theme &theme) {
    // it must be a button theme
    const ButtonTheme &btheme = static_cast<const ButtonTheme &>(theme);

    m_arrowscale = btheme.scale();
    if (m_arrowscale == 0) m_arrowscale = 250; // default is 0 => 300

    m_arrowscale = FbTk::Util::clamp(m_arrowscale, 100, 100000);
}
