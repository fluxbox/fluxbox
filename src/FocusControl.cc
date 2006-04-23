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

#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "FbWinFrameTheme.hh"

#include <string>
#include <cassert>
#include <iostream>
using std::cerr;
using std::endl;
using std::string;

WinClient *FocusControl::s_focused_window = 0;

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
    m_focus_last(screen.resourceManager(), true, 
                 screen.name()+".focusLastWindow", 
                 screen.altName()+".FocusLastWindow"),
    m_focus_new(screen.resourceManager(), true, 
                screen.name()+".focusNewWindows", 
                screen.altName()+".FocusNewWindows"),
    m_cycling_focus(false),
    m_cycling_last(0) {

    m_cycling_window = m_focused_list.end();
    
}

// true if the windows should be skiped else false
bool doSkipWindow(const WinClient &winclient, int opts) {
    const FluxboxWindow *win = winclient.fbwindow();
    return (!win ||
    // skip if stuck
    (opts & FocusControl::CYCLESKIPSTUCK) != 0 && win->isStuck() || 
    // skip if not active client (i.e. only visit each fbwin once)
    (opts & FocusControl::CYCLEGROUPS) != 0 && win->winClient().window() != winclient.window() ||
    // skip if shaded
    (opts & FocusControl::CYCLESKIPSHADED) != 0 && win->isShaded() || 
    // skip if hidden
    win->isFocusHidden()
    ); 
}

void FocusControl::cycleFocus(int opts, bool cycle_reverse) {
    int num_windows = m_screen.currentWorkspace()->numberOfWindows();
	
    if (num_windows < 1)
        return;

    FocusedWindows *window_list = (opts & CYCLELINEAR) ? &m_creation_order_list : &m_focused_list;
    if (!m_cycling_focus) {
        m_cycling_focus = true;
        if ((opts & CYCLELINEAR) && m_cycling_window != m_focused_list.end()) {
            m_cycling_creation_order = true;
            m_cycling_window = find(window_list->begin(),window_list->end(),*m_cycling_window);
        } else {
            m_cycling_creation_order = (opts & CYCLELINEAR);
            m_cycling_window = window_list->begin();
        }
        m_cycling_last = 0;
    } else {
        // already cycling, so restack to put windows back in their proper order
        m_screen.layerManager().restack();
        if (m_cycling_creation_order ^ (bool)(opts & CYCLELINEAR)) {
            m_cycling_creation_order ^= true;
            if (m_cycling_window != m_focused_list.end() && m_cycling_window != m_creation_order_list.end())
                m_cycling_window = find(window_list->begin(),window_list->end(),*m_cycling_window);
            else
                m_cycling_window = window_list->begin();
        }
    }
    // if it is stacked, we want the highest window in the focused list
    // that is on the same workspace
    FocusedWindows::iterator it = m_cycling_window;
    FocusedWindows::iterator it_begin = window_list->begin();
    FocusedWindows::iterator it_end = window_list->end();

    while (true) {
        if (cycle_reverse && it == it_begin)
            it = it_end;
        cycle_reverse ? --it : ++it;
        if (it == it_end)
            it = it_begin;
        // give up [do nothing] if we reach the current focused again
        if ((*it) == (*m_cycling_window))
            break;

        FluxboxWindow *fbwin = (*it)->fbwindow();
        if (fbwin && !fbwin->isIconic() &&
            (fbwin->isStuck() 
             || fbwin->workspaceNumber() == m_screen.currentWorkspaceID())) {
            // either on this workspace, or stuck

            // keep track of the originally selected window in a set
            WinClient &last_client = fbwin->winClient();

            if (! (doSkipWindow(**it, opts) || !fbwin->setCurrentClient(**it)) ) {
                // moved onto a new fbwin
                if (!m_cycling_last || m_cycling_last->fbwindow() != fbwin) {
                    if (m_cycling_last) {
                        // set back to orig current Client in that fbwin
                        m_cycling_last->fbwindow()->setCurrentClient(*m_cycling_last, false);
                    }
                    m_cycling_last = &last_client;
                }
                fbwin->tempRaise();
                break;
            }
        }
    }
    m_cycling_window = it;
}

void FocusControl::addFocusFront(WinClient &client) {
    m_focused_list.push_front(&client);
    m_creation_order_list.push_back(&client);
}

void FocusControl::addFocusBack(WinClient &client) {
    m_focused_list.push_back(&client);
    m_creation_order_list.push_back(&client);
}

void FocusControl::stopCyclingFocus() {
    // nothing to do
    if (!m_cycling_focus)
        return;

    m_cycling_focus = false;
    m_cycling_last = 0;
    // put currently focused window to top
    // the iterator may be invalid if the window died
    // in which case we'll do a proper revert focus
    if (m_cycling_creation_order && m_cycling_window != m_creation_order_list.end())
        m_cycling_window = find(m_focused_list.begin(),m_focused_list.end(),*m_cycling_window);
    if (m_cycling_window != m_focused_list.end() && m_cycling_window != m_creation_order_list.end()) {
        WinClient *client = *m_cycling_window;
        m_focused_list.erase(m_cycling_window);
        m_focused_list.push_front(client);
        client->fbwindow()->raise();
    } else {
        revertFocus(m_screen);
    }

}

/**
 * Used to find out which window was last focused on the given workspace
 * If workspace is outside the ID range, then the absolute last focused window
 * is given.
 */
WinClient *FocusControl::lastFocusedWindow(int workspace) {
    if (m_focused_list.empty()) return 0;
    if (workspace < 0 || workspace >= (int) m_screen.numberOfWorkspaces())
        return m_focused_list.front();

    FocusedWindows::iterator it = m_focused_list.begin();    
    FocusedWindows::iterator it_end = m_focused_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() &&
            (((int)(*it)->fbwindow()->workspaceNumber()) == workspace 
             && !(*it)->fbwindow()->isIconic()
             && (!(*it)->fbwindow()->isStuck() || (*it)->fbwindow()->isFocused())))
            // only give focus to a stuck window if it is currently focused
            // otherwise they tend to override normal workspace focus
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
    if (m_focused_list.empty()) return 0;

    FocusedWindows::iterator it = m_focused_list.begin();    
    FocusedWindows::iterator it_end = m_focused_list.end();
    for (; it != it_end; ++it) {
        if (((*it)->fbwindow() == &group) &&
            (*it) != ignore_client)
            return *it;
    }
    return 0;
}

void FocusControl::raiseFocus() {
    bool have_focused = false;

    // set have_focused if the currently focused window 
    // is on this screen
    if (focusedWindow()) {
        if (focusedWindow()->screen().screenNumber() == m_screen.screenNumber()) {
            have_focused = true;
        }
    }

    // if we have a focused window on this screen and
    // number of windows is greater than one raise the focused window
    if (m_screen.currentWorkspace()->numberOfWindows() > 1 && have_focused)
        focusedWindow()->raise();

}

void FocusControl::setScreenFocusedWindow(WinClient &win_client) {

     // raise newly focused window to the top of the focused list
    if (!m_cycling_focus) { // don't change the order if we're cycling
        m_focused_list.remove(&win_client);
        m_focused_list.push_front(&win_client);
        m_cycling_window = m_focused_list.begin();
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
        top = win.y(), 
        bottom = win.y() + win.height() + 2*borderW,
        left = win.x(),
        right = win.x() + win.width() + 2*borderW;

    Workspace::Windows &wins = m_screen.currentWorkspace()->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it) == &win 
            || (*it)->isIconic() 
            || (*it)->isFocusHidden() 
            || !(*it)->winClient().acceptsFocus()) 
            continue; // skip self
        
        // we check things against an edge, and within the bounds (draw a picture)
        int edge=0, upper=0, lower=0, oedge=0, oupper=0, olower=0;

        int otop = (*it)->y(), 
            // 2 * border = border on each side 
            obottom = (*it)->y() + (*it)->height() + 2*borderW,
            oleft = (*it)->x(),
            // 2 * border = border on each side
            oright = (*it)->x() + (*it)->width() + 2*borderW;

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
        foundwin->setInputFocus();

}

void FocusControl::removeClient(WinClient &client) {
    WinClient *cyc = 0;
    if (m_cycling_window != m_focused_list.end() && m_cycling_window != m_creation_order_list.end())
        cyc = *m_cycling_window;

    m_focused_list.remove(&client);
    m_creation_order_list.remove(&client);

    if (cyc == &client)
        stopCyclingFocus();

}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 * last_focused is set to something if we want to make use of the
 * previously focused window (it must NOT be set focused now, it
 *   is probably dying).
 *
 * ignore_event means that it ignores the given event until
 * it gets a focusIn
 */
void FocusControl::revertFocus(BScreen &screen) {

    // Relevant resources:
    // resource.focus_last = whether we focus last focused when changing workspace
    // BScreen::FocusModel = sloppy, click, whatever
    WinClient *next_focus = 
        screen.focusControl().lastFocusedWindow(screen.currentWorkspaceID());

    // if setting focus fails, or isn't possible, fallback correctly
    if (!(next_focus && next_focus->fbwindow() &&
          next_focus->fbwindow()->setCurrentClient(*next_focus, true))) {
        setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
        switch (screen.focusControl().focusModel()) {
        case FocusControl::MOUSEFOCUS:
            XSetInputFocus(screen.rootWindow().display(),
                           PointerRoot, None, CurrentTime);
            break;
        case FocusControl::CLICKFOCUS:
            screen.rootWindow().setInputFocus(RevertToPointerRoot, CurrentTime);
            break;
        }
    }
}

/*
 * Like revertFocus, but specifically related to this window (transients etc)
 * if full_revert, we fallback to a full revertFocus if we can't find anything
 * local to the client.
 * If unfocus_frame is true, we won't focus anything in the same frame
 * as the client.
 *
 * So, we first prefer to choose a transient parent, then the last
 * client in this window, and if no luck (or unfocus_frame), then
 * we just use the normal revertFocus on the screen.
 *
 * assumption: client has focus
 */
void FocusControl::unfocusWindow(WinClient &client,
                                 bool full_revert, 
                                 bool unfocus_frame) {
    // go up the transient tree looking for a focusable window

    FluxboxWindow *fbwin = client.fbwindow();
    if (fbwin == 0)
        unfocus_frame = false;

    WinClient *trans_parent = client.transientFor();
    while (trans_parent) {
        if (trans_parent->fbwindow() && // can't focus if no fbwin
            (!unfocus_frame || trans_parent->fbwindow() != fbwin) && // can't be this window
            trans_parent->fbwindow()->isVisible() &&
            trans_parent->fbwindow()->setCurrentClient(*trans_parent, 
                                                       s_focused_window == &client)) {
            return;
        }
        trans_parent = trans_parent->transientFor();
    }

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
    if (client != 0 && client->fbwindow() != 0)
        cerr<<"title: "<<client->fbwindow()->title()<<endl;
    cerr<<"Current Focused window = "<<s_focused_window<<endl;
    cerr<<"------------------"<<endl;
#endif // DEBUG
    
    WinClient *old_client = 0;

    // Update the old focused client to non focus
    // check if s_focused_window is valid
    if (s_focused_window != 0 &&
        Fluxbox::instance()->validateClient(s_focused_window)) {

        old_client = s_focused_window;
        if (old_client->fbwindow()) {
            FluxboxWindow *old_win = old_client->fbwindow();
            
            if (!client || client->fbwindow() != old_win)
                old_win->setFocusFlag(false);
        }

    } else {
        s_focused_window = 0;
    }


    if (client && client->fbwindow() && !client->fbwindow()->isIconic()) {
        // screen should be ok
        FluxboxWindow *win = client->fbwindow();        
        s_focused_window = client;     // update focused window
        win->setCurrentClient(*client, 
                              false); // don't set inputfocus
        win->setFocusFlag(true); // set focus flag

    } else
        s_focused_window = 0;

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
