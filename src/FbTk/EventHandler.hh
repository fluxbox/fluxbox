// EventHandler.cc for Fluxbox Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: EventHandler.hh,v 1.3 2002/12/01 13:42:14 rathnor Exp $

#ifndef FBTK_EVENTHANDLER_HH
#define FBTK_EVENTHANDLER_HH

#include <X11/Xlib.h>

namespace FbTk {

/**
   interface for X events
*/
class EventHandler {
public:
    virtual ~EventHandler() { }

    /**
       Events that don't have an specific event function
    */
    virtual void handleEvent(XEvent &ev) { }
    virtual void buttonPressEvent(XButtonEvent &ev) { }
    virtual void buttonReleaseEvent(XButtonEvent &ev) { }
    virtual void exposeEvent(XExposeEvent &ev) { }
    virtual void motionNotifyEvent(XMotionEvent &ev) { }
    virtual void keyPressEvent(XKeyEvent &ev) { }
    virtual void keyReleaseEvent(XKeyEvent &ev) { }
    virtual void leaveNotifyEvent(XCrossingEvent &ev) { }
    virtual void enterNotifyEvent(XCrossingEvent &ev) { }
};

};

#endif // FBTK_EVENTHANDLER_HH
