// EventManager.hh
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

// $Id: EventManager.hh,v 1.2 2002/12/01 13:42:14 rathnor Exp $

#include "EventHandler.hh"
#include <map>

namespace FbTk {

/**
   singleton mediator for EventHandlers
*/
class EventManager {
public:	
    static EventManager *instance();
	
    void handleEvent(XEvent &ev);

    void registerEventHandler(EventHandler &ev, Window win);
    void unregisterEventHandler(Window win);
private:
    EventManager() { }
    ~EventManager();

    std::map<Window, EventHandler *> m_eventhandlers;
};

}; //end namespace FbTk
