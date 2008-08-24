// Button.cc for FbTk - fluxbox toolkit
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

#include "Button.hh"

#include "Command.hh"
#include "EventManager.hh"
#include "App.hh"

namespace FbTk {

Button::Button(int screen_num, int x, int y,
               unsigned int width, unsigned int height):
    FbWindow(screen_num, x, y, width, height,
             ExposureMask | ButtonPressMask | EnterWindowMask |
             LeaveWindowMask | ButtonReleaseMask),
    m_background_pm(0),
    m_pressed_pm(0),
    m_pressed_color(),
    m_gc(DefaultGC(FbTk::App::instance()->display(), screen_num)),
    m_pressed(false),
    mark_if_deleted(0) {

    // add this to eventmanager
    FbTk::EventManager::instance()->add(*this, *this); 
}

Button::Button(const FbWindow &parent, int x, int y, 
               unsigned int width, unsigned int height):
    FbWindow(parent, x, y, width, height,
             ExposureMask | ButtonPressMask | ButtonReleaseMask |
             EnterWindowMask | LeaveWindowMask),
    m_background_pm(0),
    m_pressed_pm(0),
    m_pressed_color(),
    m_gc(DefaultGC(FbTk::App::instance()->display(), screenNumber())),
    m_pressed(false),
    mark_if_deleted(0) {
    // add this to eventmanager
    FbTk::EventManager::instance()->add(*this, *this);
}

Button::~Button() {
    if (mark_if_deleted) {
        *mark_if_deleted = true;
    }
}

void Button::setOnClick(RefCount<Command<void> > &cmd, int button) {
    // we only handle buttons 1 to 5
    if (button > 5 || button == 0)
        return;
    //set on click command for the button
    m_onclick[button - 1] = cmd;
}

void Button::setPressedPixmap(Pixmap pm) {
    m_pressed_pm = pm;
}

void Button::setPressedColor(const FbTk::Color &color) {
    m_pressed_pm = None;
    m_pressed_color = color;
}

void Button::setBackgroundColor(const Color &color) {
    m_background_pm = 0; // we're using background color now
    m_background_color = color;    
    FbTk::FbWindow::setBackgroundColor(color);
}

void Button::setBackgroundPixmap(Pixmap pm) {
    m_background_pm = pm;
    FbTk::FbWindow::setBackgroundPixmap(pm);
}


void Button::enterNotifyEvent(XCrossingEvent &ce){
		
}
void Button::leaveNotifyEvent(XCrossingEvent &ce){
		
}

void Button::buttonPressEvent(XButtonEvent &event) {
    bool update = false;
    if (m_pressed_pm != 0) {
        update = true;
        FbTk::FbWindow::setBackgroundPixmap(m_pressed_pm);
    } else if (m_pressed_color.isAllocated()) {
        update = true;
        FbTk::FbWindow::setBackgroundColor(m_pressed_color);
    }
        
    m_pressed = true;    
    if (update) {
        clear();
    }
}

void Button::buttonReleaseEvent(XButtonEvent &event) {
    if (!m_pressed) // we don't want to pick up clicks from other widgets
        return;
    m_pressed = false;
    bool update = false;
    bool been_deleted = false;
    mark_if_deleted = &been_deleted;

    // This command may result in this object being deleted
    // hence the mark_if_deleted mechanism so that we can
    // update our state after the command
    if (event.button > 0 && event.button <= 5 &&
        event.x >= -static_cast<signed>(borderWidth()) &&
        event.x <= static_cast<signed>(width()+borderWidth()) &&
        event.y >= -static_cast<signed>(borderWidth()) &&
        event.y <= static_cast<signed>(height()+borderWidth()) &&
        m_onclick[event.button -1].get() != 0)
        m_onclick[event.button - 1]->execute();

    if (!been_deleted) {
        mark_if_deleted = 0;
        if (m_background_pm) {
            if (m_pressed_pm != 0 || m_pressed_color.isAllocated()) {
                update = true;
                setBackgroundPixmap(m_background_pm);
            }
        } else if (m_pressed_pm != 0 || m_pressed_color.isAllocated()) {
            update = true;
            setBackgroundColor(m_background_color);
        }

        if (update)
            clear(); // clear background
    }

}

void Button::exposeEvent(XExposeEvent &event) {
    clearArea(event.x, event.y, event.width, event.height);
}

} // end namespace FbTk
