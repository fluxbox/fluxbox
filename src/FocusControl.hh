// FocusControl.hh
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

#ifndef FOCUSCONTROL_HH
#define FOCUSCONTROL_HH

#include <list>

#include "FbTk/Resource.hh"

class WinClient;
class FluxboxWindow;
class BScreen;

/**
 * Handles window focus for a specific screen.
 * It also holds the static "global" focused window
 */
class FocusControl {
public:
    typedef std::list<WinClient *> FocusedWindows;

    enum FocusModel { 
        MOUSEFOCUS = 0, ///< focus follows 
        CLICKFOCUS ///< focus on click
    };
    enum TabFocusModel { 
        MOUSETABFOCUS = 0, ///< tab focus follows mouse
        CLICKTABFOCUS  ///< tab focus on click
    };

    enum FocusDir { 
        FOCUSUP,
        FOCUSDOWN,
        FOCUSLEFT, 
        FOCUSRIGHT 
    };

    // prevFocus/nextFocus option bits
    enum { 
        CYCLEGROUPS = 0x01, 
        CYCLESKIPSTUCK = 0x02, 
        CYCLESKIPSHADED = 0x04,
        CYCLELINEAR = 0x08, 
        CYCLESKIPICONIC = 0x10,
        CYCLEDEFAULT = 0x00 
    };

    explicit FocusControl(BScreen &screen);

    void prevFocus() { cycleFocus(0, true); }
    void nextFocus() { cycleFocus(0, false); }
    void prevFocus(int options) { cycleFocus(options, true); }
    void nextFocus(int options) { cycleFocus(options, false); }
    void cycleFocus(int options, bool cycle_reverse);

    void setScreenFocusedWindow(WinClient &win_client);
    void setFocusModel(FocusModel model);
    void setTabFocusModel(TabFocusModel model);

    void stopCyclingFocus();

    void dirFocus(FluxboxWindow &win, FocusDir dir);
    bool isMouseFocus() const { return focusModel() == MOUSEFOCUS; }
    bool isMouseTabFocus() const { return tabFocusModel() == MOUSETABFOCUS; }
    bool isCycling() const { return m_cycling_focus; }
    void addFocusBack(WinClient &client);
    void addFocusFront(WinClient &client);
    void setFocusBack(FluxboxWindow *fbwin);

    FocusModel focusModel() const { return *m_focus_model; }
    TabFocusModel tabFocusModel() const { return *m_tab_focus_model; }
    bool focusNew() const { return *m_focus_new; }

    WinClient *lastFocusedWindow(int workspace);
    WinClient *lastFocusedWindow(FluxboxWindow &group, WinClient *ignore_client);

    FocusedWindows &creationOrderList() { return m_creation_order_list; }
    FocusedWindows &focusedOrderList() { return m_focused_list; }

    void removeClient(WinClient &client);
    void shutdown();

    static void revertFocus(BScreen &screen);
    // like revertFocus, but specifically related to this window (transients etc)
    static void unfocusWindow(WinClient &client, bool full_revert = true, bool unfocus_frame = false);
    static void setFocusedWindow(WinClient *focus_to);
    static void setFocusedFbWindow(FluxboxWindow *focus_to) { s_focused_fbwindow = focus_to; }
    static WinClient *focusedWindow() { return s_focused_window; }
    static FluxboxWindow *focusedFbWindow() { return s_focused_fbwindow; }
private:

    BScreen &m_screen;

    FbTk::Resource<FocusModel> m_focus_model;    
    FbTk::Resource<TabFocusModel> m_tab_focus_model;
    FbTk::Resource<bool> m_focus_new;

    // This list keeps the order of window focusing for this screen
    // Screen global so it works for sticky windows too.
    FocusedWindows m_focused_list;
    FocusedWindows m_creation_order_list;
    FocusedWindows::iterator m_cycling_window;
    bool m_cycling_focus;
    bool m_cycling_creation_order;
    bool m_was_iconic;
    WinClient *m_cycling_last;

    static WinClient *s_focused_window;
    static FluxboxWindow *s_focused_fbwindow;
    static bool s_reverting;
};

#endif // FOCUSCONTROL_HH
