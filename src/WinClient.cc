// WinClient.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: WinClient.cc,v 1.4 2003/04/25 11:21:17 fluxgen Exp $

#include "WinClient.hh"

#include "Window.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "i18n.hh"
#include "FbAtoms.hh"
#include "EventManager.hh"

#include <iostream>
#include <algorithm>
#include <iterator>
using namespace std;

WinClient::WinClient(Window win, FluxboxWindow &fbwin):FbTk::FbWindow(win),
                     transient_for(0),
                     window_group(0),
                     x(0), y(0), old_bw(0),
                     min_width(1), min_height(1),
                     max_width(1), max_height(1),
                     width_inc(1), height_inc(1),
                     min_aspect_x(1), min_aspect_y(1),
                     max_aspect_x(1), max_aspect_y(1),
                     base_width(1), base_height(1),
                     win_gravity(0),
                     initial_state(0),
                     normal_hint_flags(0),
                     wm_hint_flags(0),
                     mwm_hint(0),
                     blackbox_hint(0),
                     m_win(&fbwin),
                     m_title(""), m_icon_title(""),
                     m_diesig(*this) { }

WinClient::~WinClient() {
#ifdef DEBUG
    cerr<<__FILE__<<"(~"<<__FUNCTION__<<")[this="<<this<<"]"<<endl;
#endif // DEBUG

    m_diesig.notify();

    Fluxbox *fluxbox = Fluxbox::instance();

    if (transient_for != 0) {
        if (transientFor() == m_win) {
            transient_for = 0;
        }

        fluxbox->setFocusedWindow(transient_for);

        if (transient_for != 0) {
            FluxboxWindow::ClientList::iterator client_it = 
                transientFor()->clientList().begin();
            FluxboxWindow::ClientList::iterator client_it_end = 
                transientFor()->clientList().end();
            for (; client_it != client_it_end; ++client_it) {
                (*client_it)->transientList().remove(m_win);
            }

            transient_for->setInputFocus();
            transient_for = 0;
        }
    }
	
    while (!transients.empty()) {
        FluxboxWindow::ClientList::iterator it = 
            transients.back()->clientList().begin();
        FluxboxWindow::ClientList::iterator it_end = 
            transients.back()->clientList().end();
        for (; it != it_end; ++it) {
            if ((*it)->transientFor() == m_win)
                (*it)->transient_for = 0;
        }

        transients.pop_back();
    }
	
    if (window_group != 0) {
        fluxbox->removeGroupSearch(window_group);
        window_group = 0;
    }

    if (mwm_hint != 0)
        XFree(mwm_hint);

    if (blackbox_hint != 0)
        XFree(blackbox_hint);

    if (window())
        fluxbox->removeWindowSearch(window());

    if (m_win != 0)
        m_win->removeClient(*this);
    FbTk::EventManager::instance()->remove(window());
    m_win = 0;

}

void WinClient::updateRect(int x, int y, 
                        unsigned int width, unsigned int height) {
    Display *disp = FbTk::App::instance()->display();
    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = disp;
    event.xconfigure.event = window();
    event.xconfigure.window = window();
    event.xconfigure.x = x;
    event.xconfigure.y = y;
    event.xconfigure.width = width;
    event.xconfigure.height = height;
    //!! TODO
    event.xconfigure.border_width = 1;//client.old_bw;
    //!! TODO
    event.xconfigure.above = None; //m_frame.window().window();
    event.xconfigure.override_redirect = false;

    XSendEvent(disp, window(), False, StructureNotifyMask, &event);

}

void WinClient::sendFocus() {
    Display *disp = FbTk::App::instance()->display();
    // setup focus msg
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
    ce.xclient.display = disp;
    ce.xclient.window = window();
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = FbAtoms::instance()->getWMTakeFocusAtom();
    ce.xclient.data.l[1] = Fluxbox::instance()->getLastTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    // send focus msg
    XSendEvent(disp, window(), false, NoEventMask, &ce);
}

void WinClient::sendClose() {
    Display *disp = FbTk::App::instance()->display();
    // fill in XClientMessage structure for delete message
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
    ce.xclient.display = disp;
    ce.xclient.window = window();
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = FbAtoms::instance()->getWMDeleteAtom();
    ce.xclient.data.l[1] = CurrentTime;
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    // send event delete message to client window
    XSendEvent(disp, window(), false, NoEventMask, &ce);
}

void WinClient::reparent(Window win, int x, int y) {
    XReparentWindow(FbTk::App::instance()->display(), window(), win, x, y);
}

bool WinClient::getAttrib(XWindowAttributes &attr) const {
    return XGetWindowAttributes(FbTk::App::instance()->display(), window(), &attr);
}

bool WinClient::getWMName(XTextProperty &textprop) const {
    return XGetWMName(FbTk::App::instance()->display(), window(), &textprop);
}

bool WinClient::getWMIconName(XTextProperty &textprop) const {
    return XGetWMName(FbTk::App::instance()->display(), window(), &textprop);
}

void WinClient::updateTransientInfo() {
    if (m_win == 0)
        return;
    // remove us from parent
    if (transientFor() != 0) {
        //!! TODO
        // since we don't know which client in transientFor()
        // that we're transient for then we just remove us 
        // from every client in transientFor() clientlist
        FluxboxWindow::ClientList::iterator client_it = 
            transientFor()->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end = 
            transientFor()->clientList().end();
        for (; client_it != client_it_end; ++client_it) {
            (*client_it)->transientList().remove(m_win);
        }
    }
    
    transient_for = 0;
    Display *disp = FbTk::App::instance()->display();
    // determine if this is a transient window
    Window win;
    if (!XGetTransientForHint(disp, window(), &win))
        return;

    // we can't be transient to ourself
    if (win == window())
        return;
	
    if (win != 0 && m_win->getScreen().getRootWindow() == win) {
        m_win->modal = true;
        return;
    }

    transient_for = Fluxbox::instance()->searchWindow(win);
    if (transient_for != 0 &&
        window_group != None && win == window_group) {
        transient_for = Fluxbox::instance()->searchGroup(win, m_win);
    }
	
    // make sure we don't have deadlock loop in transient chain
    for (FluxboxWindow *w = m_win; w != 0; w = w->m_client->transient_for) {
        if (w == w->m_client->transient_for) {
            w->m_client->transient_for = 0;
            break;
        }
    }

    if (transientFor() != 0) {
        // we need to add ourself to the right client in
        // the transientFor() window so we search client
        WinClient *client = transientFor()->findClient(win);
        assert(client != 0);
        client->transientList().push_back(m_win);
        // make sure we only have on instance of this
        client->transientList().unique(); 
        if (transientFor()->isStuck())
            m_win->stick();       
    }
}


void WinClient::updateTitle() {
    XTextProperty text_prop;
    char **list = 0;
    int num = 0;
    I18n *i18n = I18n::instance();

    if (getWMName(text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if (XmbTextPropertyToTextList(FbTk::App::instance()->display(), &text_prop,
                                              &list, &num) == Success &&
                    num > 0 && *list) {
                    m_title = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    m_title = (char *)text_prop.value;
					
            } else
                m_title = (char *)text_prop.value;
            XFree((char *) text_prop.value);
        } else { // ok, we don't have a name, set default name
            m_title = i18n->getMessage(
                                       FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                       "Unnamed");
        }
    } else {
        m_title = i18n->getMessage(
                                   FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                   "Unnamed");
    }

}

void WinClient::updateIconTitle() {
    XTextProperty text_prop;
    char **list = 0;
    int num = 0;
 
    if (getWMIconName(text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
                text_prop.nitems = strlen((char *) text_prop.value);

                if (XmbTextPropertyToTextList(FbTk::App::instance()->display(), &text_prop,
                                               &list, &num) == Success &&
                    num > 0 && *list) {
                    m_icon_title = (char *)*list;
                    XFreeStringList(list);
                } else
                    m_icon_title = (char *)text_prop.value;
            } else
                m_icon_title = (char *)text_prop.value;

            XFree((char *) text_prop.value);
        } else
            m_icon_title = title();
    } else
        m_icon_title = title();

}
