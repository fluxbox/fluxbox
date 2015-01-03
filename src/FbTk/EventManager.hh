// EventManager.hh
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_EVENTMANAGER_HH
#define FBTK_EVENTMANAGER_HH

#include <map>
#include <X11/Xlib.h>

namespace FbTk {

class FbWindow;
class EventHandler;

/**
   singleton mediator for EventHandlers
*/
class EventManager {
public:	
    static EventManager *instance();

    void handleEvent(XEvent &ev);
    // adds a parent to listen to the childrens events
    void addParent(EventHandler &ev, const FbWindow &parent);
    void add(EventHandler &ev, const FbWindow &win);
    void remove(const FbWindow &win);
    void add(EventHandler &ev, Window win) { registerEventHandler(ev, win); }
    void remove(Window win) { unregisterEventHandler(win); }

    bool grabKeyboard(Window win);
    void ungrabKeyboard();

    EventHandler *find(Window win);

    // Some events have the parent window as the xany.window
    // This function always returns the actual window member of the event structure
    static Window getEventWindow(XEvent &ev);

    void registerEventHandler(EventHandler &ev, Window win);
    void unregisterEventHandler(Window win);

private:
    EventManager() { }
    ~EventManager();
    void dispatch(Window win, XEvent &event, bool parent = false);

    typedef std::map<Window, EventHandler *> EventHandlerMap;
    EventHandlerMap m_eventhandlers;
    EventHandlerMap m_parent;
};

} //end namespace FbTk

#endif // FBTK_EVENTMANAGER_HH

