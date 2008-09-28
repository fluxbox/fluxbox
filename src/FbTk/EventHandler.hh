// EventHandler.hh for Fluxbox Window Manager
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

/// @file EventHandler.hh holds EventHandler interface for X events

#ifndef FBTK_EVENTHANDLER_HH
#define FBTK_EVENTHANDLER_HH

#include <X11/Xlib.h>

namespace FbTk {


/// interface for X events
/**
 * Use this class to catch events from X windows and FbWindows \n
 * Register instance of this class to EventManager \n
 * example: \n
 * EventManager::instance()->add(your_eventhandler, your_window); \n
 * Don't forget to unregister it: \n
 * EventManager::instance()->remove(your_window);
 * @see EventManager
 */
class EventHandler {
public:
    virtual ~EventHandler() { }

    /**
       Events that don't have an specific event function
    */
    virtual void handleEvent(XEvent &) { }
    virtual void buttonPressEvent(XButtonEvent &) { }
    virtual void buttonReleaseEvent(XButtonEvent &) { }
    virtual void exposeEvent(XExposeEvent &) { }
    virtual void motionNotifyEvent(XMotionEvent &) { }
    virtual void keyPressEvent(XKeyEvent &) { }
    virtual void keyReleaseEvent(XKeyEvent &) { }
    virtual void leaveNotifyEvent(XCrossingEvent &) { }
    virtual void enterNotifyEvent(XCrossingEvent &) { }

    virtual void grabButtons() { }
};

} // end namespace FbTk

#endif // FBTK_EVENTHANDLER_HH
