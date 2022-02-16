// EventManager.cc
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

#include "EventManager.hh"
#include "EventHandler.hh"
#include "FbWindow.hh"
#include "App.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

namespace FbTk {

EventManager *EventManager::instance() {
    static EventManager ev;
    return &ev;
}

EventManager::~EventManager() {
#ifdef DEBUG
    if (m_eventhandlers.size() != 0)
        cerr<<"FbTk::EventManager: Warning: unregistered eventhandlers!"<<endl;
#endif // DEBUG
}

void EventManager::handleEvent(XEvent &ev) {
    dispatch(ev.xany.window, ev);
}

void EventManager::add(EventHandler &ev, const FbWindow &win) {
    registerEventHandler(ev, win.window());
}

void EventManager::addParent(EventHandler &ev, const FbWindow &win) {
    if (win.window() != 0)
        m_parent[win.window()] = &ev;
}

void EventManager::remove(const FbWindow &win) {
    unregisterEventHandler(win.window());
}

EventHandler *EventManager::find(Window win) {
    return m_eventhandlers[win];
}

bool EventManager::grabKeyboard(Window win) {
    int ret = XGrabKeyboard(App::instance()->display(), win, False,
                            GrabModeAsync, GrabModeAsync, CurrentTime);
    return (ret == Success);
}

void EventManager::ungrabKeyboard() {
    XUngrabKeyboard(App::instance()->display(), CurrentTime);
}

Window EventManager::getEventWindow(XEvent &ev) {
    // we only have cases for events that differ from xany
    switch (ev.type) {
    case CreateNotify:
        // XCreateWindowEvent
        return ev.xcreatewindow.window;
        break;
    case DestroyNotify:
        // XDestroyWindowEvent
        return ev.xdestroywindow.window;
        break;
    case UnmapNotify:
        // XUnmapEvent
        return ev.xunmap.window;
        break;
    case MapNotify:
        // XMapEvent
        return ev.xmap.window;
        break;
    case MapRequest:
        // XMapRequestEvent
        return ev.xmaprequest.window;
        break;
    case ReparentNotify:
        // XReparentEvent
        return ev.xreparent.window;
        break;
    case ConfigureNotify:
        // XConfigureNotify
        return ev.xconfigure.window;
        break;
    case GravityNotify:
        // XGravityNotify
        return ev.xgravity.window;
        break;
    case ConfigureRequest:
        // XConfigureRequestEvent
        return ev.xconfigurerequest.window;
        break;
    case CirculateNotify:
        // XCirculateEvent
        return ev.xcirculate.window;
        break;
    case CirculateRequest:
        // XCirculateRequestEvent
        return ev.xcirculaterequest.window;
        break;
    default:
        return ev.xany.window;
    }
}

void EventManager::registerEventHandler(EventHandler &ev, Window win) {
    if (win != None)
        m_eventhandlers[win] = &ev;
}

void EventManager::unregisterEventHandler(Window win) {
    if (win != None) {
        m_eventhandlers.erase(win);
        m_parent.erase(win);
    }
}

void EventManager::dispatch(Window win, XEvent &ev, bool parent) {
    EventHandler *evhand = 0;
    if (parent) {
        EventHandlerMap::iterator it = m_parent.find(win);
        if (it == m_parent.end())
            return;
        else
            evhand = it->second;
    } else {
        win = getEventWindow(ev);
        EventHandlerMap::iterator it = m_eventhandlers.find(win);
        if (it == m_eventhandlers.end())
            return;
        else
            evhand = it->second;
    }

    if (evhand == 0)
        return;

    switch (ev.type) {
    case KeyPress:
        if (!XFilterEvent(&ev, win))
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
        if (ev.xcrossing.mode != NotifyGrab &&
            ev.xcrossing.mode != NotifyUngrab)
            evhand->leaveNotifyEvent(ev.xcrossing);
    break;
    default:
        evhand->handleEvent(ev);
    break;
    };

    // find out which window is the parent and
    // dispatch event
    Window root, parent_win, *children = 0;
    unsigned int num_children;
    if (XQueryTree(FbTk::App::instance()->display(), win,
                   &root, &parent_win, &children, &num_children) != 0) {
        if (children != 0)
            XFree(children);

        if (parent_win != 0 &&
            parent_win != root) {
            if (m_parent[parent_win] == 0)
                return;

            // dispatch event to parent
            dispatch(parent_win, ev, true);
        }
    }

}

} // end namespace FbTk
