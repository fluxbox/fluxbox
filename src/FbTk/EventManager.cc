// EventManager.cc
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at linuxmail.org)
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

// $Id: EventManager.cc,v 1.4 2003/02/23 14:30:18 fluxgen Exp $

#include "EventManager.hh"
#include "FbWindow.hh"

#include <iostream>
using namespace std;

namespace FbTk {

EventManager *EventManager::instance() {
    static EventManager ev;
    return &ev;
}

EventManager::~EventManager() {
    if (m_eventhandlers.size() != 0)
        cerr<<"FbTk::EventManager: Warning: unregistered eventhandlers!"<<endl;
}

void EventManager::handleEvent(XEvent &ev) {
    // find eventhandler for event window
    if (m_eventhandlers.find(ev.xany.window) == m_eventhandlers.end()) {
        //cerr<<"Can't find window="<<ev.xany.window<<endl;
        return;
    }
    EventHandler *evhand = m_eventhandlers[ev.xany.window];
    if (evhand == 0) {
        cerr<<"FbTk::EventManager: Warning: evhand == 0!"<<endl;
        return;
    }
		
    switch (ev.xany.type) {
    case KeyPress:
        evhand->keyPressEvent(ev.xkey);
	break;
    case KeyRelease:
        evhand->keyReleaseEvent(ev.xkey);
	break;
    case ButtonPress:
        evhand->buttonPressEvent(ev.xbutton);
	break;
    case ButtonRelease:
        evhand->buttonReleaseEvent(ev.xbutton);
	break;
    case MotionNotify:
        evhand->motionNotifyEvent(ev.xmotion);
	break;
    case Expose:
        evhand->exposeEvent(ev.xexpose);
	break;
    case EnterNotify:
        evhand->enterNotifyEvent(ev.xcrossing);
	break;
    case LeaveNotify:
        evhand->leaveNotifyEvent(ev.xcrossing);
	break;
    default:
        evhand->handleEvent(ev);
	break;
    };
}

void EventManager::add(EventHandler &ev, const FbWindow &win) {
    registerEventHandler(ev, win.window());
}

void EventManager::remove(const FbWindow &win) {
    unregisterEventHandler(win.window());
}

void EventManager::registerEventHandler(EventHandler &ev, Window win) {
    m_eventhandlers[win] = &ev;
}

void EventManager::unregisterEventHandler(Window win) {
    m_eventhandlers.erase(win);
}

};
