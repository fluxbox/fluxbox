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

// $Id$

#include "FocusControl.hh"

#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "FbWinFrameTheme.hh"

#include "FbTk/EventManager.hh"

#include <string>
#include <iostream>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::cerr;
using std::endl;
using std::string;

WinClient *FocusControl::s_focused_window = 0;
FluxboxWindow *FocusControl::s_focused_fbwindow = 0;
bool FocusControl::s_reverting = false;

namespace {

bool doSkipWindow(const Focusable &win, const ClientPattern *pat) {
    const FluxboxWindow *fbwin = win.fbwindow();
    if (!fbwin || fbwin->isFocusHidden())
        return true; // skip if no fbwindow or if focushidden
    if (pat && !pat->match(win))
        return true; // skip if it doesn't match the pattern
    if (fbwin->workspaceNumber() != win.screen().currentWorkspaceID() &&
        !fbwin->isStuck())
        return true; // for now, we only cycle through the current workspace
    return false; // else don't skip
}

}; // end anonymous namespace
    
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
    m_cycling_list(0),
    m_was_iconic(false),
    m_cycling_last(0) {

    m_cycling_window = m_focused_win_list.end();
    
}

void FocusControl::cycleFocus(const Focusables &window_list,
                              const ClientPattern *pat, bool cycle_reverse) {

    if (!m_cycling_list) {
        if (&m_screen == FbTk::EventManager::instance()->grabbingKeyboard())
            // only set this when we're waiting for modifiers
            m_cycling_list = &window_list;
        m_was_iconic = 0;
        m_cycling_last = 0;
    } else if (m_cycling_list != &window_list)
        m_cycling_list = &window_list;

    Focusables::const_iterator it_begin = window_list.begin();
    Focusables::const_iterator it_end = window_list.end();

    // too many things can go wrong with remembering this
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
        if (!doSkipWindow(**it, pat) && (*it)->focus())
            break;
    }
    m_cycling_window = it;

    // if we're still in the same fbwin, there's nothing else to do
    if (m_cycling_last && m_cycling_last->fbwindow() == fbwin)
        return;

    // if we were already cycling, then restore the old state
    if (m_cycling_last) {
        m_screen.layerManager().restack();

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

void FocusControl::goToWindowNumber(const Focusables &winlist, int num,
                                    const ClientPattern *pat) {
    Focusables::const_iterator it = winlist.begin();
    Focusables::const_iterator it_end = winlist.end();
    for (; it != it_end && num; ++it) {
        if (!doSkipWindow(**it, pat) && (*it)->acceptsFocus()) {
            num > 0 ? --num : ++num;
            if (!num) {
                (*it)->focus();
                if ((*it)->fbwindow())
                    (*it)->fbwindow()->raise();
            }
        }
    }
}

void FocusControl::addFocusBack(WinClient &client) {
    m_focused_list.push_back(&client);
    m_creation_order_list.push_back(&client);
}

void FocusControl::addFocusFront(WinClient &client) {
    m_focused_list.push_front(&client);
    m_creation_order_list.push_back(&client);
}

void FocusControl::addFocusWinBack(Focusable &win) {
    m_focused_win_list.push_back(&win);
    m_creation_order_win_list.push_back(&win);
}

void FocusControl::addFocusWinFront(Focusable &win) {
    m_focused_win_list.push_front(&win);
    m_creation_order_win_list.push_back(&win);
}

// move all clients in given window to back of focused list
void FocusControl::setFocusBack(FluxboxWindow *fbwin) {
    // do nothing if there are no windows open
    // don't change focus order while cycling
    if (m_focused_list.empty() || s_reverting)
        return;

    // if the window isn't already in this list, we could accidentally add it
    Focusables::iterator win_begin = m_focused_win_list.begin(),
                         win_end = m_focused_win_list.end();
    Focusables::iterator win_it = find(win_begin, win_end, fbwin);
    if (win_it == win_end)
        return;

    m_focused_win_list.erase(win_it);
    m_focused_win_list.push_back(fbwin);

    Focusables::iterator it = m_focused_list.begin();
    // use back to avoid an infinite loop
    Focusables::iterator it_back = --m_focused_list.end();

    while (it != it_back) {
        if ((*it)->fbwindow() == fbwin) {
            m_focused_list.push_back(*it);
            it = m_focused_list.erase(it);
        } else
            ++it;
    }
    // move the last one, if necessary, in order to preserve focus order
    if ((*it)->fbwindow() == fbwin) {
        m_focused_list.push_back(*it);
        m_focused_list.erase(it);
    }

}
    
void FocusControl::stopCyclingFocus() {
    // nothing to do
    if (m_cycling_list == 0)
        return;

    m_cycling_last = 0;
    m_cycling_list = 0;

    // put currently focused window to top
    if (s_focused_window) {
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
    if (m_focused_list.empty() || m_screen.isShuttingdown()) return 0;
    if (workspace < 0 || workspace >= (int) m_screen.numberOfWorkspaces())
        return m_focused_list.front();

    Focusables::iterator it = m_focused_list.begin();    
    Focusables::iterator it_end = m_focused_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() &&
            ((((int)(*it)->fbwindow()->workspaceNumber()) == workspace ||
             (*it)->fbwindow()->isStuck()) && (*it)->acceptsFocus() &&
             !(*it)->fbwindow()->isIconic()))
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

    Focusables::iterator it = m_focused_list.begin();    
    Focusables::iterator it_end = m_focused_list.end();
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

        // make sure client is in our list, or else we could end up adding it
        Focusables::iterator it_begin = m_focused_list.begin(),
                             it_end = m_focused_list.end();
        Focusables::iterator it = find(it_begin, it_end, &win_client);
        if (it == it_end)
            return;

        m_focused_list.erase(it);
        m_focused_list.push_front(&win_client);

        // also check the fbwindow
        it_begin = m_focused_win_list.begin();
        it_end = m_focused_win_list.end();
        it = find(it_begin, it_end, win_client.fbwindow());

        if (it != it_end) {
            m_focused_win_list.erase(it);
            m_focused_win_list.push_front(win_client.fbwindow());
        }

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
    int borderW = m_screen.winFrameTheme().border().width(),
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

void FocusControl::removeClient(WinClient &client) {
    if (client.screen().isShuttingdown())
        return;

    if (m_cycling_list && m_cycling_window != m_cycling_list->end() &&
        *m_cycling_window == &client) {
        m_cycling_window = m_cycling_list->end();
        stopCyclingFocus();
    } else if (m_cycling_last == &client)
        m_cycling_last = 0;

    m_focused_list.remove(&client);
    m_creation_order_list.remove(&client);
    client.screen().clientListSig().notify();
}

void FocusControl::removeWindow(Focusable &win) {
    if (win.screen().isShuttingdown())
        return;

    if (m_cycling_list && m_cycling_window != m_cycling_list->end() &&
        *m_cycling_window == &win) {
        m_cycling_window = m_cycling_list->end();
        stopCyclingFocus();
    }

    m_focused_win_list.remove(&win);
    m_creation_order_win_list.remove(&win);
    win.screen().clientListSig().notify();
}

void FocusControl::shutdown() {
    // restore windows backwards so they get put back correctly on restart
    Focusables::reverse_iterator it = m_focused_list.rbegin();
    for (; it != m_focused_list.rend(); ++it) {
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

    if (!unfocus_frame) {
        WinClient *last_focus = screen.focusControl().lastFocusedWindow(*fbwin, &client);
        if (last_focus != 0 &&
            fbwin->setCurrentClient(*last_focus, 
                                    s_focused_window == &client)) {
            return;
        }
    }

    if (full_revert && s_focused_window == &client)
        revertFocus(screen);

}


void FocusControl::setFocusedWindow(WinClient *client) {
    BScreen *screen = client ? &client->screen() : 0;
    BScreen *old_screen = 
        FocusControl::focusedWindow() ? 
        &FocusControl::focusedWindow()->screen() : 0;

#ifdef DEBUG
    cerr<<"------------------"<<endl;
    cerr<<"Setting Focused window = "<<client<<endl;
    if (client != 0)
        cerr<<"title: "<<client->title()<<endl;
    cerr<<"Current Focused window = "<<s_focused_window<<endl;
    cerr<<"------------------"<<endl;
#endif // DEBUG

    // Update the old focused client to non focus
    if (s_focused_fbwindow)
        s_focused_fbwindow->setFocusFlag(false);

    if (client && client->fbwindow() && !client->fbwindow()->isIconic()) {
        // screen should be ok
        s_focused_fbwindow = client->fbwindow();        
        s_focused_window = client;     // update focused window
        s_focused_fbwindow->setCurrentClient(*client, 
                              false); // don't set inputfocus
        s_focused_fbwindow->setFocusFlag(true); // set focus flag

    } else {
        s_focused_window = 0;
        s_focused_fbwindow = 0;
    }

    // update AtomHandlers and/or other stuff...
    Fluxbox::instance()->updateFocusedWindow(screen, old_screen);
}

////////////////////// FocusControl RESOURCES
namespace FbTk {

template<>
std::string FbTk::Resource<FocusControl::FocusModel>::getString() const {
    switch (m_value) {
    case FocusControl::MOUSEFOCUS:
        return string("MouseFocus");
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
