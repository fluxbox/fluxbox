// FocusControl.cc
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "FocusControl.hh"

#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "FbWinFrameTheme.hh"
#include "Debug.hh"

#include "FbTk/EventManager.hh"

#include <string>
#include <iostream>
#include <algorithm>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::endl;
using std::string;

WinClient *FocusControl::s_focused_window = 0;
FluxboxWindow *FocusControl::s_focused_fbwindow = 0;
WinClient *FocusControl::s_expecting_focus = 0;
bool FocusControl::s_reverting = false;

namespace {

bool doSkipWindow(const Focusable &win, const ClientPattern *pat) {
    const FluxboxWindow *fbwin = win.fbwindow();
    if (!fbwin || fbwin->isFocusHidden() || fbwin->isModal())
        return true; // skip if no fbwindow or if focushidden
    if (pat && !pat->match(win))
        return true; // skip if it doesn't match the pattern

    return false; // else don't skip
}

} // end anonymous namespace

FocusControl::FocusControl(BScreen &screen):
    m_screen(screen),
    m_focus_model(screen.resourceManager(),
                  CLICKFOCUS,
                  screen.name()+".focusModel",
                  screen.altName()+".FocusModel"),
    m_tab_focus_model(screen.resourceManager(),
                      CLICKTABFOCUS,
                      screen.name()+".tabFocusModel",
                      screen.altName()+".TabFocusModel"),
    m_focus_new(screen.resourceManager(), true,
                screen.name()+".focusNewWindows",
                screen.altName()+".FocusNewWindows"),
#ifdef XINERAMA
    m_focus_same_head(screen.resourceManager(), false,
                screen.name()+".focusSameHead",
                screen.altName()+".FocusSameHead"),
#endif // XINERAMA
    m_focused_list(screen), m_creation_order_list(screen),
    m_focused_win_list(screen), m_creation_order_win_list(screen),
    m_cycling_list(0),
    m_was_iconic(0),
    m_cycling_last(0),
    m_cycling_next(0),
    m_ignore_mouse_x(-1), m_ignore_mouse_y(-1) {

    m_cycling_window = m_focused_list.clientList().end();

}

void FocusControl::cycleFocus(const FocusableList &window_list,
                              const ClientPattern *pat, bool cycle_reverse) {

    if (!m_cycling_list) {
        if (m_screen.isCycling())
            // only set this when we're waiting for modifiers
            m_cycling_list = &window_list;
        m_was_iconic = 0;
        m_cycling_last = 0;
        m_cycling_next = 0;
    } else if (m_cycling_list != &window_list)
        m_cycling_list = &window_list;

    Focusables::const_iterator it_begin = window_list.clientList().begin();
    Focusables::const_iterator it_end = window_list.clientList().end();

    // too many things can go wrong with remembering this
    m_cycling_window = it_end;
    if (m_cycling_next)
        m_cycling_window = find(it_begin, it_end, m_cycling_next);
    if (m_cycling_window == it_end)
        m_cycling_window = find(it_begin, it_end, s_focused_window);
    if (m_cycling_window == it_end)
        m_cycling_window = find(it_begin, it_end, s_focused_fbwindow);

    Focusables::const_iterator it = m_cycling_window;
    FluxboxWindow *fbwin = 0;
    WinClient *last_client = 0;
    WinClient *was_iconic = 0;

    // find the next window in the list that works
    while (true) {
        if (cycle_reverse && it == it_begin)
            it = it_end;
        else if (!cycle_reverse && it == it_end)
            it = it_begin;
        else
            cycle_reverse ? --it : ++it;
        // give up [do nothing] if we reach the current focused again
        if (it == m_cycling_window)
            return;
        if (it == it_end)
            continue;

        fbwin = (*it)->fbwindow();
        if (!fbwin)
            continue;

        // keep track of the originally selected window in a group
        last_client = &fbwin->winClient();
        was_iconic = (fbwin->isIconic() ? last_client : 0);

        // now we actually try to focus the window
        if (!doSkipWindow(**it, pat) && (m_cycling_next = *it) && (*it)->focus())
            break;
        m_cycling_next = 0;
    }
    m_cycling_window = it;

    // if we're still in the same fbwin, there's nothing else to do
    if (m_cycling_last && m_cycling_last->fbwindow() == fbwin)
        return;

    // if we were already cycling, then restore the old state
    if (m_cycling_last) {
        // set back to originally selected window in that group
        m_cycling_last->fbwindow()->setCurrentClient(*m_cycling_last, false);

        if (m_was_iconic == m_cycling_last) {
            s_reverting = true; // little hack
            m_cycling_last->fbwindow()->iconify();
            s_reverting = false;
        }
    }

    if (!isCycling())
        fbwin->raise();

    m_cycling_last = last_client;
    m_was_iconic = was_iconic;

}

void FocusControl::goToWindowNumber(const FocusableList &winlist, int num,
                                    const ClientPattern *pat) {
    Focusables list = winlist.clientList();
    if (num < 0) {
        list.reverse();
        num = -num;
    }
    Focusable *win = 0;
    Focusables::const_iterator it = list.begin(), it_end = list.end();
    for (; num && it != it_end; ++it) {
        if (!doSkipWindow(**it, pat) && (*it)->acceptsFocus()) {
            --num;
            win = *it;
        }
    }
    if (win) {
        win->focus();
        if (win->fbwindow())
            win->fbwindow()->raise();
    }
}

void FocusControl::addFocusBack(WinClient &client) {
    m_focused_list.pushBack(client);
    m_creation_order_list.pushBack(client);
}

void FocusControl::addFocusFront(WinClient &client) {
    m_focused_list.pushFront(client);
    m_creation_order_list.pushBack(client);
}

void FocusControl::addFocusWinBack(Focusable &win) {
    m_focused_win_list.pushBack(win);
    m_creation_order_win_list.pushBack(win);
}

void FocusControl::addFocusWinFront(Focusable &win) {
    m_focused_win_list.pushFront(win);
    m_creation_order_win_list.pushBack(win);
}

// move all clients in given window to back of focused list
void FocusControl::setFocusBack(FluxboxWindow &fbwin) {
    // do nothing if there are no windows open
    // don't change focus order while cycling
    if (m_focused_list.empty() || s_reverting)
        return;

    m_focused_win_list.moveToBack(fbwin);

    // we need to move its clients to the back while preserving their order
    Focusables list = m_focused_list.clientList();
    Focusables::iterator it = list.begin(), it_end = list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() == &fbwin)
            m_focused_list.moveToBack(**it);
    }
}

void FocusControl::stopCyclingFocus() {
    // nothing to do
    if (m_cycling_list == 0)
        return;

    m_cycling_last = 0;
    m_cycling_next = 0;
    m_cycling_list = 0;

    // put currently focused window to top
    if (s_focused_window) {
        // re-focus last window to give the client a chance to redistribute the
        // focus internally (client-side only modality)
        s_focused_window->focus();
        if (s_focused_window)
            setScreenFocusedWindow(*s_focused_window);
        if (s_focused_fbwindow)
            s_focused_fbwindow->raise();
    } else
        revertFocus(m_screen);
}

/**
 * Used to find out which window was last focused on the given workspace
 * If workspace is outside the ID range, then the absolute last focused window
 * is given.
 */
Focusable *FocusControl::lastFocusedWindow(int workspace) {
    if (m_screen.isShuttingdown()) return 0;
    if (workspace < 0 || workspace >= (int) m_screen.numberOfWorkspaces())
        return m_focused_list.clientList().front();

#ifdef XINERAMA
    int cur_head = focusSameHead() ? m_screen.getCurrHead() : (-1);
    if(cur_head != -1) {
      FluxboxWindow *fbwindow = focusedFbWindow();
      if(fbwindow && fbwindow->isMoving()) {
        cur_head = -1;
      }
    }
#endif // XINERAMA

    Focusables::iterator it = m_focused_list.clientList().begin();
    Focusables::iterator it_end = m_focused_list.clientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && (*it)->acceptsFocus() &&
            (*it)->fbwindow()->winClient().validateClient() &&
#ifdef XINERAMA
            ( (cur_head == -1) || ((*it)->fbwindow()->getOnHead() == cur_head) ) &&
#endif // XINERAMA
            ((((int)(*it)->fbwindow()->workspaceNumber()) == workspace ||
             (*it)->fbwindow()->isStuck()) && !(*it)->fbwindow()->isIconic()))
            return *it;
    }
    return 0;
}

/**
 * Used to find out which window was last active in the given group
 * If ignore_client is given, it excludes that client.
 * Stuck, iconic etc don't matter within a group
 */
WinClient *FocusControl::lastFocusedWindow(FluxboxWindow &group, WinClient *ignore_client) {
    if (m_focused_list.empty() || m_screen.isShuttingdown())
        return 0;

    Focusables::iterator it = m_focused_list.clientList().begin();
    Focusables::iterator it_end = m_focused_list.clientList().end();
    for (; it != it_end; ++it) {
        if (((*it)->fbwindow() == &group) &&
            (*it) != ignore_client)
            return dynamic_cast<WinClient *>(*it);
    }
    return 0;
}

void FocusControl::setScreenFocusedWindow(WinClient &win_client) {

     // raise newly focused window to the top of the focused list
     // don't change the order if we're cycling or shutting down
    if (!isCycling() && !m_screen.isShuttingdown() && !s_reverting) {
        m_focused_list.moveToFront(win_client);
        if (win_client.fbwindow())
            m_focused_win_list.moveToFront(*win_client.fbwindow());
    }
}

void FocusControl::setFocusModel(FocusModel model) {
    m_focus_model = model;
}

void FocusControl::setTabFocusModel(TabFocusModel model) {
    m_tab_focus_model = model;
}

void FocusControl::dirFocus(FluxboxWindow &win, FocusDir dir) {
    // change focus to the window in direction dir from the given window

    // we scan through the list looking for the window that is "closest"
    // in the given direction

    FluxboxWindow *foundwin = 0;
    int weight = 999999, exposure = 0; // extreme values
    int borderW = win.frame().window().borderWidth(),
        top = win.y() + borderW,
        bottom = win.y() + win.height() + borderW,
        left = win.x() + borderW,
        right = win.x() + win.width() + borderW;

    Workspace::Windows &wins = m_screen.currentWorkspace()->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it) == &win
            || (*it)->isIconic()
            || (*it)->isFocusHidden()
            || !(*it)->acceptsFocus())
            continue; // skip self

        // we check things against an edge, and within the bounds (draw a picture)
        int edge=0, upper=0, lower=0, oedge=0, oupper=0, olower=0;

        int otop = (*it)->y() + borderW,
            // 2 * border = border on each side
            obottom = (*it)->y() + (*it)->height() + borderW,
            oleft = (*it)->x() + borderW,
            // 2 * border = border on each side
            oright = (*it)->x() + (*it)->width() + borderW;

        // check if they intersect
        switch (dir) {
        case FOCUSUP:
            edge = obottom;
            oedge = bottom;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSDOWN:
            edge = top;
            oedge = otop;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSLEFT:
            edge = oright;
            oedge = right;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        case FOCUSRIGHT:
            edge = left;
            oedge = oleft;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        }

        if (oedge < edge)
            continue; // not in the right direction

        if (olower <= upper || oupper >= lower) {
            // outside our horz bounds, get a heavy weight penalty
            int myweight = 100000 + oedge - edge + abs(upper-oupper)+abs(lower-olower);
            if (myweight < weight) {
                foundwin = *it;
                exposure = 0;
                weight = myweight;
            }
        } else if ((oedge - edge) < weight) {
            foundwin = *it;
            weight = oedge - edge;
            exposure = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
        } else if (foundwin && oedge - edge == weight) {
            int myexp = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
            if (myexp > exposure) {
                foundwin = *it;
                // weight is same
                exposure = myexp;
            }
        } // else not improvement
    }

    if (foundwin)
        foundwin->focus();

}

void FocusControl::ignoreAtPointer(bool force)
{
    int ignore_i, ignore_x, ignore_y;
    unsigned int ignore_ui;
    Window ignore_w;

    XQueryPointer(m_screen.rootWindow().display(),
        m_screen.rootWindow().window(), &ignore_w, &ignore_w,
        &ignore_x, &ignore_y,
        &ignore_i, &ignore_i, &ignore_ui);

    this->ignoreAt(ignore_x, ignore_y, force);
}

void FocusControl::ignoreAt(int x, int y, bool force)
{
	if (force || this->focusModel() == MOUSEFOCUS) {
		m_ignore_mouse_x = x; m_ignore_mouse_y = y;
	}
}

void FocusControl::ignoreCancel()
{
	m_ignore_mouse_x = m_ignore_mouse_y = -1;
}

bool FocusControl::isIgnored(int x, int y)
{
    return x == m_ignore_mouse_x && y == m_ignore_mouse_y;
}

void FocusControl::removeClient(WinClient &client) {
    if (client.screen().isShuttingdown())
        return;

    if (isCycling() && m_cycling_window != m_cycling_list->clientList().end() &&
        *m_cycling_window == &client) {
        m_cycling_window = m_cycling_list->clientList().end();
        stopCyclingFocus();
    } else if (m_cycling_last == &client) {
        m_cycling_last = 0;
    } else if (m_cycling_next == &client) {
        m_cycling_next = 0;
    }

    m_focused_list.remove(client);
    m_creation_order_list.remove(client);
    client.screen().clientListSig().emit(client.screen());
}

void FocusControl::removeWindow(Focusable &win) {
    if (win.screen().isShuttingdown())
        return;

    if (isCycling() && m_cycling_window != m_cycling_list->clientList().end() &&
        *m_cycling_window == &win) {
        m_cycling_window = m_cycling_list->clientList().end();
        stopCyclingFocus();
    }

    m_focused_win_list.remove(win);
    m_creation_order_win_list.remove(win);
    win.screen().clientListSig().emit(win.screen());
}

void FocusControl::shutdown() {
    // restore windows backwards so they get put back correctly on restart
    Focusables::reverse_iterator it = m_focused_list.clientList().rbegin();
    for (; it != m_focused_list.clientList().rend(); ++it) {
        WinClient *client = dynamic_cast<WinClient *>(*it);
        if (client && client->fbwindow())
            client->fbwindow()->restore(client, true);
    }
}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 */
void FocusControl::revertFocus(BScreen &screen) {
    if (s_reverting || screen.isShuttingdown())
        return;

    Focusable *next_focus =
        screen.focusControl().lastFocusedWindow(screen.currentWorkspaceID());

    if (next_focus && next_focus->fbwindow() &&
        next_focus->fbwindow()->isStuck())
        FocusControl::s_reverting = true;

    // if setting focus fails, or isn't possible, fallback correctly
    if (!(next_focus && next_focus->focus())) {

        setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
        // if there's a menu open, focus it
        if (FbTk::Menu::shownMenu())
            FbTk::Menu::shownMenu()->grabInputFocus();
        else {
            switch (screen.focusControl().focusModel()) {
            case FocusControl::MOUSEFOCUS:
            case FocusControl::STRICTMOUSEFOCUS:
                XSetInputFocus(screen.rootWindow().display(),
                               PointerRoot, None, CurrentTime);
                break;
            case FocusControl::CLICKFOCUS:
                screen.rootWindow().setInputFocus(RevertToPointerRoot,
                                                  CurrentTime);
                break;
            }
        }
    }

    FocusControl::s_reverting = false;
}

/*
 * Like revertFocus, but specifically related to this window (transients etc)
 * if full_revert, we fallback to a full revertFocus if we can't find anything
 * local to the client.
 * If unfocus_frame is true, we won't focus anything in the same frame
 * as the client.
 *
 * So, we first prefer to choose the last client in this window, and if no luck
 * (or unfocus_frame), then we just use the normal revertFocus on the screen.
 *
 * assumption: client has focus
 */
void FocusControl::unfocusWindow(WinClient &client,
                                 bool full_revert,
                                 bool unfocus_frame) {
    // go up the transient tree looking for a focusable window

    FluxboxWindow *fbwin = client.fbwindow();
    if (fbwin == 0)
        return; // nothing more we can do

    BScreen &screen = fbwin->screen();

    if (client.isTransient() && client.transientFor()->focus())
        return;

    if (!unfocus_frame) {
        WinClient *last_focus = screen.focusControl().lastFocusedWindow(*fbwin, &client);
        if (last_focus && last_focus->focus())
            return;
    }

    if (full_revert && s_focused_window == &client)
        revertFocus(screen);

}


void FocusControl::setFocusedWindow(WinClient *client) {
    if (client == s_focused_window &&
        (!client || client->fbwindow() == s_focused_fbwindow))
        return;

    BScreen *screen = client ? &client->screen() : 0;
    if (client && screen && screen->focusControl().isCycling()) {
        Focusable *next = screen->focusControl().m_cycling_next;
        WinClient *nextClient = dynamic_cast<WinClient*>(next);
        FluxboxWindow *nextWindow = nextClient ? 0 : dynamic_cast<FluxboxWindow*>(next);
        if (next && nextClient != client && nextWindow != client->fbwindow() &&
                screen->focusControl().m_cycling_list->contains(*next)) {
            // if we're currently cycling and the client tries to juggle around focus
            // on FocusIn events to provide client-side modality - don't let him
            next->focus();
            if (nextClient)
                setFocusedWindow(nextClient); // doesn't happen automatically while cycling, 1148
            return;
        }
    }

    if (client && client != expectingFocus() && s_focused_window &&
        (!(screen && screen->focusControl().isCycling())) &&
        ((s_focused_fbwindow->focusProtection() & Focus::Lock) ||
        (client && client->fbwindow() && (client->fbwindow()->focusProtection() & Focus::Deny)))) {
        s_focused_window->focus();
        return;
    }

    BScreen *old_screen =
        FocusControl::focusedWindow() ?
        &FocusControl::focusedWindow()->screen() : 0;

    fbdbg<<"------------------"<<endl;
    fbdbg<<"Setting Focused window = "<<client<<endl;
    if (client != 0)
        fbdbg<<"title: "<<client->title().logical()<<endl;
    fbdbg<<"Current Focused window = "<<s_focused_window<<endl;
    fbdbg<<"------------------"<<endl;


    // Update the old focused client to non focus
    if (s_focused_fbwindow &&
        (!client || client->fbwindow() != s_focused_fbwindow))
        s_focused_fbwindow->setFocusFlag(false);

    if (client && client->fbwindow() && !client->fbwindow()->isIconic()) {
        // screen should be ok
        s_focused_fbwindow = client->fbwindow();
        s_focused_window = client;     // update focused window
        s_expecting_focus = 0;
        s_focused_fbwindow->setCurrentClient(*client,
                              false); // don't set inputfocus
        s_focused_fbwindow->setFocusFlag(true); // set focus flag

    } else {
        s_focused_window = 0;
        s_focused_fbwindow = 0;
    }

    // update AtomHandlers and/or other stuff...
    if (screen)
        screen->focusedWindowSig().emit(*screen, s_focused_fbwindow, s_focused_window);
    if (old_screen && screen != old_screen)
        old_screen->focusedWindowSig().emit(*old_screen, s_focused_fbwindow, s_focused_window);
}

////////////////////// FocusControl RESOURCES
namespace FbTk {

template<>
std::string FbTk::Resource<FocusControl::FocusModel>::getString() const {
    switch (m_value) {
    case FocusControl::MOUSEFOCUS:
        return string("MouseFocus");
    case FocusControl::STRICTMOUSEFOCUS:
        return string("StrictMouseFocus");
    case FocusControl::CLICKFOCUS:
        return string("ClickFocus");
    }
    // default string
    return string("ClickFocus");
}

template<>
void FbTk::Resource<FocusControl::FocusModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "MouseFocus") == 0)
        m_value = FocusControl::MOUSEFOCUS;
    else if (strcasecmp(strval, "StrictMouseFocus") == 0)
        m_value = FocusControl::STRICTMOUSEFOCUS;
    else if (strcasecmp(strval, "ClickToFocus") == 0)
        m_value = FocusControl::CLICKFOCUS;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<FocusControl::TabFocusModel>::getString() const {
    switch (m_value) {
    case FocusControl::MOUSETABFOCUS:
        return string("SloppyTabFocus");
    case FocusControl::CLICKTABFOCUS:
        return string("ClickToTabFocus");
    }
    // default string
    return string("ClickToTabFocus");
}

template<>
void FbTk::Resource<FocusControl::TabFocusModel>::
setFromString(char const *strval) {

    if (strcasecmp(strval, "SloppyTabFocus") == 0 )
        m_value = FocusControl::MOUSETABFOCUS;
    else if (strcasecmp(strval, "ClickToTabFocus") == 0)
        m_value = FocusControl::CLICKTABFOCUS;
    else
        setDefaultValue();
}

} // end namespace FbTk

