// Button.cc for FbTk - fluxbox toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Button.cc,v 1.11 2003/08/13 09:25:16 fluxgen Exp $

#include "Button.hh"

#include "Command.hh"
#include "EventManager.hh"
#include "App.hh"

namespace FbTk {

Button::Button(int screen_num, int x, int y,
               unsigned int width, unsigned int height):
    FbWindow(screen_num, x, y, width, height,
             ExposureMask | ButtonPressMask | ButtonReleaseMask),
    m_foreground_pm(0),
    m_background_pm(0),
    m_pressed_pm(0),
    m_gc(DefaultGC(FbTk::App::instance()->display(), screen_num)),
    m_pressed(false) {

    // add this to eventmanager
    FbTk::EventManager::instance()->add(*this, *this); 
}

Button::Button(const FbWindow &parent, int x, int y, 
               unsigned int width, unsigned int height):
    FbWindow(parent, x, y, width, height,
             ExposureMask | ButtonPressMask | ButtonReleaseMask),
    m_foreground_pm(0),
    m_background_pm(0),
    m_pressed_pm(0),
    m_gc(DefaultGC(FbTk::App::instance()->display(), screenNumber())),
    m_pressed(false) {
    // add this to eventmanager
    FbTk::EventManager::instance()->add(*this, *this);
}

Button::~Button() {
    FbTk::EventManager::instance()->remove(*this);
}

void Button::setOnClick(RefCount<Command> &cmd, int button) {
    // we only handle buttons 1 to 5
    if (button > 5 || button == 0)
        return;
    //set on click command for the button
    m_onclick[button - 1] = cmd;
}

void Button::setPixmap(Pixmap pm) {
    m_foreground_pm = pm;
}

void Button::setPressedPixmap(Pixmap pm) {
    m_pressed_pm = pm;
}

void Button::setBackgroundColor(const Color &color) {
    m_background_color = color;    
    FbTk::FbWindow::setBackgroundColor(color);
}

void Button::setBackgroundPixmap(Pixmap pm) {
    m_background_pm = pm;
    FbTk::FbWindow::setBackgroundPixmap(pm);
}

void Button::buttonPressEvent(XButtonEvent &event) {
    if (m_pressed_pm != 0)
        FbWindow::setBackgroundPixmap(m_pressed_pm);
    m_pressed = true;    
    clear();
    FbWindow::updateTransparent();
}

void Button::buttonReleaseEvent(XButtonEvent &event) {
    m_pressed = false;
    if (m_background_pm)
        FbWindow::setBackgroundPixmap(m_background_pm);
    else
        FbWindow::setBackgroundColor(m_background_color);

    clear(); // clear background

    if (m_foreground_pm) { // draw foreground pixmap
        Display *disp = App::instance()->display();

        if (m_gc == 0) // get default gc if we dont have one
            m_gc = DefaultGC(disp, screenNumber());

        XCopyArea(disp, m_foreground_pm, window(), m_gc, 0, 0, width(), height(), 0, 0);
    }

    if (event.button > 0 && event.button <= 5 &&
        m_onclick[event.button -1].get() != 0)
        m_onclick[event.button - 1]->execute();

    FbWindow::updateTransparent();
}

void Button::exposeEvent(XExposeEvent &event) {
    if (m_background_pm)
        FbWindow::setBackgroundPixmap(m_background_pm);
    else
        FbWindow::setBackgroundColor(m_background_color);

    clear();
    FbWindow::updateTransparent(event.x, event.y, event.width, event.height);
}

}; // end namespace FbTk
