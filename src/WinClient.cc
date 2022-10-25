// WinClient.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "WinClient.hh"

#include "Window.hh"
#include "fluxbox.hh"
#include "FocusControl.hh"
#include "Remember.hh"
#include "Screen.hh"
#include "FbAtoms.hh"
#include "Xutil.hh"
#include "Debug.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/MultLayers.hh"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <X11/Xatom.h>

#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::list;
using std::mem_fn;
using std::endl;
using std::cerr;
using std::hex;
using std::dec;

namespace {

void sendMessage(const WinClient& win, Atom atom, Time time) {
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
    ce.xclient.display = win.display();
    ce.xclient.window = win.window();
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = atom;
    ce.xclient.data.l[1] = time;
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    XSendEvent(win.display(), win.window(), false, NoEventMask, &ce);
}

} // end of anonymous namespace

WinClient::TransientWaitMap WinClient::s_transient_wait;

WinClient::WinClient(Window win, BScreen &screen, FluxboxWindow *fbwin):
        Focusable(screen, fbwin), FbTk::FbWindow(win),
                     transient_for(0),
                     window_group(0),
                     old_bw(0),
                     initial_state(0),
                     normal_hint_flags(0),
                     wm_hint_flags(0),
                     m_modal_count(0),
                     m_modal(false),
                     accepts_input(false),
                     send_focus_message(false),
                     send_close_message(false),
                     m_title_override(false),
                     m_icon_override(false),
                     m_window_type(WindowState::TYPE_NORMAL),
                     m_mwm_hint(0),
                     m_strut(0) {

    old_bw = borderWidth();
    updateWMProtocols();
    updateMWMHints();
    updateWMHints();
    updateWMNormalHints();
    updateWMClassHint();
    updateTitle();
    Fluxbox::instance()->saveWindowSearch(win, this);
    if (window_group != None)
        Fluxbox::instance()->saveGroupSearch(window_group, this);

    // search for this in transient waiting list
    if (s_transient_wait.find(win) != s_transient_wait.end()) {
        // Found transients that are waiting for this.
        // For each transient that waits call updateTransientInfo
        for_each(s_transient_wait[win].begin(),
                 s_transient_wait[win].end(),
                 mem_fn(&WinClient::updateTransientInfo));
        // clear transient waiting list for this window
        s_transient_wait.erase(win);
    }

    m_title_update_timer.setTimeout(100 * FbTk::FbTime::IN_MILLISECONDS);
    m_title_update_timer.fireOnce(true);
    FbTk::RefCount<FbTk::Command<void> > ets(new FbTk::SimpleCommand<WinClient>(*this, &WinClient::emitTitleSig));
    m_title_update_timer.setCommand(ets);

    // also check if this window is a transient
    // this needs to be done before creating an fbwindow, so this doesn't get
    // tabbed using the apps file
    updateTransientInfo();
}

WinClient::~WinClient() {
    fbdbg<<__FILE__<<"(~"<<__FUNCTION__<<")[this="<<this<<"]"<<endl;

    FbTk::EventManager::instance()->remove(window());
    Fluxbox *fluxbox = Fluxbox::instance();
    if (window())
        fluxbox->removeWindowSearch(window());

    clearStrut();

    //
    // clear transients and transient_for
    //
    if (transient_for != 0) {
        assert(transient_for != this);
        transient_for->transientList().remove(this);
        if (m_modal)
            transient_for->removeModal();
    }

    while (!transients.empty()) {
        transients.back()->transient_for = 0;
        transients.pop_back();
    }

    accepts_input = send_focus_message = false;
    if (fbwindow() != 0)
        fbwindow()->removeClient(*this);

    // this takes care of any focus issues
    dieSig().emit(*this);

    // This fixes issue 1 (see WinClient.hh):
    // If transients die before the transient_for is created
    transient_for = 0;
    removeTransientFromWaitingList();
    s_transient_wait.erase(window());

    if (window_group != 0) {
        fluxbox->removeGroupSearch(window_group);
        window_group = 0;
    }

    if (m_mwm_hint != 0)
        XFree(m_mwm_hint);

}

bool WinClient::acceptsFocus() const {
    return ((accepts_input || send_focus_message) &&
            // focusing fbpanel messes up quite a few things
            m_window_type != WindowState::TYPE_DOCK &&
            m_window_type != WindowState::TYPE_SPLASH);
}

bool WinClient::sendFocus() {
    if (accepts_input) {
        setInputFocus(RevertToPointerRoot, CurrentTime);
        FocusControl::setExpectingFocus(this);
        return true;
    }
    if (!send_focus_message)
        return false;

    fbdbg<<"WinClient::"<<__FUNCTION__<<": this = "<<this<<
        " window = 0x"<<hex<<window()<<dec<<endl;

    // setup focus msg
    sendMessage(*this, FbAtoms::instance()->getWMTakeFocusAtom(), Fluxbox::instance()->getLastTime());
    FocusControl::setExpectingFocus(this);
    return true;
}

void WinClient::sendClose(bool forceful) {
    if (forceful || !send_close_message)
        XKillClient(display(), window());
    else {
        // send WM_DELETE message
        sendMessage(*this, FbAtoms::instance()->getWMDeleteAtom(), CurrentTime);
    }
}

bool WinClient::getAttrib(XWindowAttributes &attr) const {
    return XGetWindowAttributes(display(), window(), &attr);
}

bool WinClient::getWMName(XTextProperty &textprop) const {
    return XGetWMName(display(), window(), &textprop);
}

bool WinClient::getWMIconName(XTextProperty &textprop) const {
    return XGetWMIconName(display(), window(), &textprop);
}

string WinClient::getWMRole() const {
    Atom wm_role = XInternAtom(FbTk::App::instance()->display(),
                               "WM_WINDOW_ROLE", False);
    return textProperty(wm_role);
}

void WinClient::updateWMClassHint() {

    m_instance_name = Xutil::getWMClassName(window());
    m_class_name = Xutil::getWMClassClass(window());
}

void WinClient::updateTransientInfo() {
    // remove this from parent
    if (transientFor() != 0) {
        transientFor()->transientList().remove(this);
        if (m_modal)
            transientFor()->removeModal();
    }

    transient_for = 0;
    // determine if this is a transient window
    Window win = 0;
    if (!XGetTransientForHint(display(), window(), &win)) {

        fbdbg<<__FUNCTION__<<": window() = 0x"<<hex<<window()<<dec<<"Failed to read transient for hint."<<endl;
        return;
    }

    // we can't be transient to ourself
    if (win == window()) {
        cerr<<__FUNCTION__<<": transient to ourself"<<endl;
        return;
    }

    if (win != None && screen().rootWindow() == win) {
        // transient for root window... =  transient for group
        // I don't think we are group-aware yet
        return;
    }


    transient_for = Fluxbox::instance()->searchWindow(win);
    // if we did not find a transient WinClient but still
    // have a transient X window, then we have to put the
    // X transient_for window in a waiting list and update this clients transient
    // list later when the transient_for has a Winclient
    if (!transient_for) {
        // We might also already waiting for an old transient_for;
        //
        // this call fixes issue 2:
        // If transients changes to new transient_for before the old transient_for is created.
        // (see comment in WinClient.hh)
        //
        removeTransientFromWaitingList();

        s_transient_wait[win].push_back(this);
    }


    fbdbg<<__FUNCTION__<<": transient_for window = 0x"<<hex<<win<<dec<<endl;
    fbdbg<<__FUNCTION__<<": transient_for = "<<transient_for<<endl;

    // make sure we don't have deadlock loop in transient chain
    for (WinClient *w = this; w != 0; w = w->transient_for) {
        if (this == w->transient_for)
            w->transient_for = 0;
    }

    if (transientFor() != 0) {
        // we need to add ourself to the right client in
        // the transientFor() window so we search client
        transient_for->transientList().push_back(this);
        if (m_modal)
            transient_for->addModal();
    }

}


void WinClient::updateTitle() {
    // why 512? very very long wmnames seem to either
    // crash fluxbox or to make it have high cpuload
    // see also:
    //    http://www.securityfocus.com/archive/1/382398/2004-11-24/2004-11-30/2
    //
    // TODO: - find out why this mostly happens when using xft-fonts
    //       - why other windowmanagers (pekwm/pwm3/openbox etc) are
    //         also influenced
    //
    // the limitation to 512 chars only avoids running in that trap
    if (m_title_override)
        return;

    m_title.setLogical(FbTk::FbString(Xutil::getWMName(window()), 0, 512));
    m_title_update_timer.start();
}

void WinClient::emitTitleSig() {
    titleSig().emit(m_title.logical(), *this);
}

void WinClient::setTitle(const FbTk::FbString &title) {
    m_title.setLogical(title);
    m_title_override = true;
    m_title_update_timer.start();
}

void WinClient::setIcon(const FbTk::PixmapWithMask& pm) {

    m_icon.pixmap().copy(pm.pixmap());
    m_icon.mask().copy(pm.mask());
    m_icon_override = true;
    titleSig().emit(m_title.logical(), *this);
}

void WinClient::setFluxboxWindow(FluxboxWindow *win) {
    m_fbwin = win;
}

void WinClient::updateMWMHints() {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;

    if (m_mwm_hint) {
        XFree(m_mwm_hint);
        m_mwm_hint = 0;
    }
    Atom motif_wm_hints = FbAtoms::instance()->getMWMHintsAtom();

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
    XWMHints *wmhint = XGetWMHints(display(), window());
    accepts_input = true;
    window_group = None;
    initial_state = NormalState;
    if (wmhint) {
        wm_hint_flags = wmhint->flags;
        /*
         * ICCCM 4.1.7
         *---------------------------------------------
         * Input Model      Input Field   WM_TAKE_FOCUS
         *---------------------------------------------
         * No Input          False         Absent
         * Passive           True          Absent
         * Locally Active    True          Present
         * Globally Active   False         Present
         *---------------------------------------------
         * Here: WM_TAKE_FOCUS = send_focus_message
         *       Input Field   = accepts_input
         */
        if (wmhint->flags & InputHint)
            accepts_input = (bool)wmhint->input;

        if (wmhint->flags & StateHint)
            initial_state = wmhint->initial_state;

        if (wmhint->flags & WindowGroupHint && !window_group)
            window_group = wmhint->window_group;

        if (! m_icon_override) {

            if ((bool)(wmhint->flags & IconPixmapHint) && wmhint->icon_pixmap != 0)
                m_icon.pixmap().copy(wmhint->icon_pixmap, 0, 0);
            else
                m_icon.pixmap().release();

            if ((bool)(wmhint->flags & IconMaskHint) && wmhint->icon_mask != 0)
                m_icon.mask().copy(wmhint->icon_mask, 0, 0);
            else
                m_icon.mask().release();
        }

        if (fbwindow()) {
            if (wmhint->flags & XUrgencyHint) {
                Fluxbox::instance()->attentionHandler().addAttention(*this);
            } else {
                Fluxbox::instance()->attentionHandler().windowFocusChanged(*this);
            }
        }

        XFree(wmhint);
    }
}


void WinClient::updateWMNormalHints() {
    long icccm_mask;
    XSizeHints sizehint;
    if (Remember::instance().isRemembered(*this, Remember::REM_IGNORE_SIZEHINTS) ||
        !XGetWMNormalHints(display(), window(), &sizehint, &icccm_mask))
        sizehint.flags = 0;

    normal_hint_flags = sizehint.flags;
    m_size_hints.reset(sizehint);
}

Window WinClient::getGroupLeftWindow() const {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    static Atom group_left_hint = XInternAtom(display(), "_FLUXBOX_GROUP_LEFT", False);

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
    if (m_screen.isShuttingdown())
        return;
    static Atom group_left_hint = XInternAtom(display(), "_FLUXBOX_GROUP_LEFT", False);
    changeProperty(group_left_hint, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *) &win, 1);
}

bool WinClient::hasGroupLeftWindow() const {
    // try to find _FLUXBOX_GROUP_LEFT atom in window
    // if we have one then we have a group left window
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    static Atom group_left_hint = XInternAtom(display(), "_FLUXBOX_GROUP_LEFT", False);

    Window *data = 0;
    if (property(group_left_hint, 0,
                   1, false,
                   XA_WINDOW, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &data) &&
        data) {
            XFree(data);
            if (num != 1)
                return false;
            else
                return true;
    }

    return false;
}

void WinClient::setStateModal(bool state) {
    if (state == m_modal)
        return;

    m_modal = state;
    if (transient_for) {
        if (state)
            transient_for->addModal();
        else
            transient_for->removeModal();
    }

    // TODO: we're not implementing the following part of EWMH spec:
    // "if WM_TRANSIENT_FOR is not set or set to the root window the dialog is
    //  modal for its window group."
}

bool WinClient::validateClient() const {
    FbTk::App::instance()->sync(false);

    XEvent e;
    if (( XCheckTypedWindowEvent(display(), window(), DestroyNotify, &e) ||
          XCheckTypedWindowEvent(display(), window(), UnmapNotify, &e))
        && XPutBackEvent(display(), &e)) {
        Fluxbox::instance()->ungrab();
        return false;
    }

    return true;
}

void WinClient::setStrut(Strut *strut) {
    clearStrut();
    m_strut = strut;
}

void WinClient::clearStrut() {
    if (m_strut != 0) {
        screen().clearStrut(m_strut);
        screen().updateAvailableWorkspaceArea();
        m_strut = 0;
    }
}

bool WinClient::focus() {
    if (fbwindow() == 0)
        return false;
    else
        return fbwindow()->setCurrentClient(*this, true);
}

bool WinClient::isFocused() const {
    return (fbwindow() ?
        fbwindow()->isFocused() && &fbwindow()->winClient() == this :
        false);
}

void WinClient::setAttentionState(bool value) {
    Focusable::setAttentionState(value);
    if (fbwindow() && !fbwindow()->isFocused())
        fbwindow()->setAttentionState(value);
}

void WinClient::updateWMProtocols() {
    Atom *proto = 0;
    int num_return = 0;
    FbAtoms *fbatoms = FbAtoms::instance();

    if (XGetWMProtocols(display(), window(), &proto, &num_return)) {

        // defaults
        send_focus_message = false;
        send_close_message = false;
        for (int i = 0; i < num_return; ++i) {
            if (proto[i] == fbatoms->getWMDeleteAtom())
                send_close_message = true;
            else if (proto[i] == fbatoms->getWMTakeFocusAtom())
                send_focus_message = true;
        }

        XFree(proto);
        if (fbwindow())
            fbwindow()->updateFunctions();

    } else {
        fbdbg<<"Warning: Failed to read WM Protocols. "<<endl;
    }

}

void WinClient::removeTransientFromWaitingList() {

    // holds the windows that dont have empty
    // transient waiting list
    list<Window> remove_list;

    // The worst case complexity is huge, but since we usually do not (virtualy never)
    // have a large transient waiting list the time spent here is neglectable
    TransientWaitMap::iterator t_it = s_transient_wait.begin();
    TransientWaitMap::iterator t_it_end = s_transient_wait.end();
    for (; t_it != t_it_end; ++t_it) {
        (*t_it).second.remove(this);
        // if the list is empty, add it to remove list
        // so we can erase it later
        if ((*t_it).second.empty())
            remove_list.push_back((*t_it).first);
    }

    // erase empty waiting lists
    list<Window>::iterator it = remove_list.begin();
    list<Window>::iterator it_end = remove_list.end();
    for (; it != it_end; ++it)
        s_transient_wait.erase(*it);
}
