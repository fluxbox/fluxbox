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

// $Id: WinClient.cc,v 1.19 2003/07/21 15:26:56 rathnor Exp $

#include "WinClient.hh"

#include "Window.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "I18n.hh"
#include "FbAtoms.hh"
#include "EventManager.hh"
#include "Xutil.hh"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <cassert>

using namespace std;

WinClient::WinClient(Window win, BScreen &screen, FluxboxWindow *fbwin):FbTk::FbWindow(win),
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
                     send_focus_message(false),
                     m_win(fbwin),
                     m_modal(0),
                     m_title(""), m_icon_title(""),
                     m_class_name(""), m_instance_name(""),
                     m_blackbox_hint(0),
                     m_mwm_hint(0),
                     m_focus_mode(F_PASSIVE),
                     m_diesig(*this), m_screen(screen) {
    updateBlackboxHints();
    updateMWMHints();
    updateWMHints();
    updateWMNormalHints();
    updateWMClassHint();
    updateTitle();
    updateIconTitle();
}

WinClient::~WinClient() {
#ifdef DEBUG
    cerr<<__FILE__<<"(~"<<__FUNCTION__<<")[this="<<this<<"]"<<endl;
#endif // DEBUG

    FbTk::EventManager::instance()->remove(window());

    if (m_win != 0)
        m_win->removeClient(*this);

    // this takes care of any focus issues
    m_diesig.notify();

    Fluxbox *fluxbox = Fluxbox::instance();

    if (transient_for != 0) {
        assert(transient_for != this);
        transient_for->transientList().remove(this);
        transient_for = 0;
    }

    while (!transients.empty()) {
        transients.back()->transient_for = 0;
        transients.pop_back();
    }

    screen().removeNetizen(window());

    if (window_group != 0) {
        fluxbox->removeGroupSearch(window_group);
        window_group = 0;
    }

    if (m_mwm_hint != 0)
        XFree(m_mwm_hint);

    if (m_blackbox_hint != 0)
        XFree(m_blackbox_hint);

    if (window())
        fluxbox->removeWindowSearch(window());

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

bool WinClient::sendFocus() {
    if (!send_focus_message)
        return false;

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
    return true;
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

const std::string &WinClient::getWMClassName() const {
    return m_instance_name;
}

const std::string &WinClient::getWMClassClass() const {
    return m_class_name;
}

void WinClient::updateWMClassHint() {
    XClassHint ch;
    if (XGetClassHint(FbTk::App::instance()->display(), window(), &ch) == 0) {
#ifdef DEBUG
        cerr<<"WinClient: Failed to read class hint!"<<endl;
#endif //DEBUG
    } else {        

        if (ch.res_name != 0) {
            m_instance_name = const_cast<char *>(ch.res_name);
            XFree(ch.res_name);
            ch.res_name = 0;
        } else 
            m_instance_name = "";
        
        if (ch.res_class != 0) {
            m_class_name = const_cast<char *>(ch.res_class);
            XFree(ch.res_class);
            ch.res_class = 0;
        } else
            m_class_name = "";
    }
}

void WinClient::updateTransientInfo() {
    if (m_win == 0)
        return;
    // remove us from parent
    if (transientFor() != 0) {
        transientFor()->transientList().remove(this);
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
	
    if (win != None && m_win->screen().rootWindow() == win) {
        // transient for root window... =  transient for group
        // I don't think we are group-aware yet
        return; 
    }

    FluxboxWindow *transient_win = Fluxbox::instance()->searchWindow(win);
    if (transient_win)
        transient_for = transient_win->findClient(win);

    // make sure we don't have deadlock loop in transient chain
    for (WinClient *w = this; w != 0; w = w->transient_for) {
        if (w == w->transient_for) {
            w->transient_for = 0;
            break;
        }
    }

    if (transientFor() != 0) {
        // we need to add ourself to the right client in
        // the transientFor() window so we search client
        transient_for->transientList().push_back(this);

        if (transientFor()->fbwindow() && transientFor()->fbwindow()->isStuck())
            m_win->stick();
    }
}


void WinClient::updateTitle() {
    m_title = Xutil::getWMName(window());
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
                    m_icon_title = text_prop.value ? (char *)text_prop.value : "";
            } else
                m_icon_title = text_prop.value ? (char *)text_prop.value : "";

            if (text_prop.value)
                XFree((char *) text_prop.value);
        } else
            m_icon_title = title();
    } else
        m_icon_title = title();

}

void WinClient::saveBlackboxAttribs(FluxboxWindow::BlackboxAttributes &blackbox_attribs) {
    changeProperty(FbAtoms::instance()->getFluxboxAttributesAtom(),
                   PropModeReplace, XA_CARDINAL, 32,
                   (unsigned char *)&blackbox_attribs,
                   FluxboxWindow::PropBlackboxAttributesElements
        );
}

void WinClient::updateBlackboxHints() {
    int format;
    Atom atom_return;
    unsigned long num, len;
    FbAtoms *atoms = FbAtoms::instance();

    if (m_blackbox_hint) {
        XFree(m_blackbox_hint);
        m_blackbox_hint = 0;
    }

    if (property(atoms->getFluxboxHintsAtom(), 0,
                 PropBlackboxHintsElements, False,
                 atoms->getFluxboxHintsAtom(), &atom_return,
                 &format, &num, &len,
                 (unsigned char **) &m_blackbox_hint) &&
        m_blackbox_hint) {

        if (num != (unsigned)PropBlackboxHintsElements) {
            XFree(m_blackbox_hint);
            m_blackbox_hint = 0;
        }
    }
}

void WinClient::updateMWMHints() {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    Atom  motif_wm_hints = XInternAtom(FbTk::App::instance()->display(), "_MOTIF_WM_HINTS", False);

    if (m_mwm_hint) {
        XFree(m_mwm_hint);
        m_mwm_hint = 0;
    }

    if (!(property(motif_wm_hints, 0,
                   PropMwmHintsElements, false,
                   motif_wm_hints, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &m_mwm_hint) &&
          m_mwm_hint)) {
        if (num != static_cast<unsigned int>(PropMwmHintsElements)) {
            XFree(m_mwm_hint);
            m_mwm_hint = 0;
            return;
        }
    }
}

void WinClient::updateWMHints() {
    XWMHints *wmhint = XGetWMHints(FbTk::App::instance()->display(), window());
    if (! wmhint) {
        m_focus_mode = F_PASSIVE;
        window_group = None;
        initial_state = NormalState;
    } else {
        wm_hint_flags = wmhint->flags;
        if (wmhint->flags & InputHint) {
            if (wmhint->input) {
                if (send_focus_message)
                    m_focus_mode = F_LOCALLYACTIVE;
                else
                    m_focus_mode = F_PASSIVE;
            } else {
                if (send_focus_message)
                    m_focus_mode = F_GLOBALLYACTIVE;
                else
                    m_focus_mode = F_NOINPUT;
            }
        } else
            m_focus_mode = F_PASSIVE;

        if (wmhint->flags & StateHint)
            initial_state = wmhint->initial_state;
        else
            initial_state = NormalState;

        if (wmhint->flags & WindowGroupHint) {
            if (! window_group)
                window_group = wmhint->window_group;
        } else
            window_group = None;

        XFree(wmhint);
    }
}


void WinClient::updateWMNormalHints() {
    long icccm_mask;
    XSizeHints sizehint;
    if (! XGetWMNormalHints(FbTk::App::instance()->display(), window(), &sizehint, &icccm_mask)) {
        min_width = min_height =
            base_width = base_height =
            width_inc = height_inc = 1;
        max_width = 0; // unbounded
        max_height = 0;
        min_aspect_x = min_aspect_y =
            max_aspect_x = max_aspect_y = 1;
        win_gravity = NorthWestGravity;
    } else {
        normal_hint_flags = sizehint.flags;

        if (sizehint.flags & PMinSize) {
            min_width = sizehint.min_width;
            min_height = sizehint.min_height;
        } else
            min_width = min_height = 1;

        if (sizehint.flags & PMaxSize) {
            max_width = sizehint.max_width;
            max_height = sizehint.max_height;
        } else {
            max_width = 0; // unbounded
            max_height = 0;
        }

        if (sizehint.flags & PResizeInc) {
            width_inc = sizehint.width_inc;
            height_inc = sizehint.height_inc;
        } else
            width_inc = height_inc = 1;

        if (sizehint.flags & PAspect) {
            min_aspect_x = sizehint.min_aspect.x;
            min_aspect_y = sizehint.min_aspect.y;
            max_aspect_x = sizehint.max_aspect.x;
            max_aspect_y = sizehint.max_aspect.y;
        } else
            min_aspect_x = min_aspect_y =
                max_aspect_x = max_aspect_y = 1;

        if (sizehint.flags & PBaseSize) {
            base_width = sizehint.base_width;
            base_height = sizehint.base_height;
        } else
            base_width = base_height = 0;

        if (sizehint.flags & PWinGravity)
            win_gravity = sizehint.win_gravity;
        else
            win_gravity = NorthWestGravity;
    }
}

Window WinClient::getGroupLeftWindow() const {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);

    Window *data = 0;
    if (property(group_left_hint, 0,
                   1, false,
                   XA_WINDOW, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &data) &&
        data) {
        if (num != 1) {
            XFree(data);
            return None;
        } else {
            Window ret = *data;
            XFree(data);
            return ret;
        }
    }
    return None;
}


void WinClient::setGroupLeftWindow(Window win) {
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);
    changeProperty(group_left_hint, XA_WINDOW, 32, 
                   PropModeReplace, (unsigned char *) &win, 1);
}

bool WinClient::hasGroupLeftWindow() const {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);

    Window *data = 0;
    if (property(group_left_hint, 0,
                   1, false,
                   XA_WINDOW, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &data) &&
        data) {
        if (num != 1) {
            XFree(data);
            return false;
        } else {
            XFree(data);
            return true;
        }
    }
    return false;
}

void WinClient::addModal() {
    ++m_modal;
    if (transient_for)
        transient_for->addModal();
}

void WinClient::removeModal() {
    --m_modal;
    if (transient_for)
        transient_for->removeModal();
}
