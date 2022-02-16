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

#ifndef FOCUSCONTROL_HH
#define FOCUSCONTROL_HH

#include <list>

#include "FbTk/Resource.hh"
#include "FocusableList.hh"

class ClientPattern;
class WinClient;
class FluxboxWindow;
class Focusable;
class BScreen;

/**
 * Handles window focus for a specific screen.
 * It also holds the static "global" focused window
 */
class FocusControl {
public:
    typedef std::list<Focusable *> Focusables;
    /// main focus model
    enum FocusModel { 
        MOUSEFOCUS = 0,  ///< focus follows mouse, but only when the mouse is moving
        CLICKFOCUS,      ///< focus on click
        STRICTMOUSEFOCUS ///< focus always follows mouse, even when stationary
    };
    /// focus model for tabs
    enum TabFocusModel { 
        MOUSETABFOCUS = 0, ///< tab focus follows mouse
        CLICKTABFOCUS  ///< tab focus on click
    };

    /// focus direction for windows
    enum FocusDir { 
        FOCUSUP,    ///< window is above
        FOCUSDOWN,  ///< window is down
        FOCUSLEFT,  ///< window is left
        FOCUSRIGHT  ///< window is right
    };

    explicit FocusControl(BScreen &screen);
    /// cycle previous focuable 
    void prevFocus() { cycleFocus(m_focused_list, 0, true); }
    /// cycle next focusable
    void nextFocus() { cycleFocus(m_focused_list, 0, false); }
    /**
     * Cycle focus for a set of windows.
     * @param winlist the windowlist to cycle through
     * @param pat pattern for matching focusables
     * @param reverse reverse the cycle order
     */
    void cycleFocus(const FocusableList &winlist, const ClientPattern *pat = 0,
                    bool reverse = false);
    
    void goToWindowNumber(const FocusableList &winlist, int num,
                          const ClientPattern *pat = 0);
    /// sets the focused window on a screen
    void setScreenFocusedWindow(WinClient &win_client);
    /// sets the main focus model
    void setFocusModel(FocusModel model);
    /// sets tab focus model
    void setTabFocusModel(TabFocusModel model);
    /// stop cycling mode
    void stopCyclingFocus();
    /** 
     * Do directional focus mode.
     * @param win current window
     * @param dir direction from current window to focus.
     */
    void dirFocus(FluxboxWindow &win, FocusDir dir);
    /// @return true if focus mode is mouse focus
    bool isMouseFocus() const { return focusModel() != CLICKFOCUS; }
    /// @return true if tab focus mode is mouse tab focus
    bool isMouseTabFocus() const { return tabFocusModel() == MOUSETABFOCUS; }

    /// Set the "ignore" pointer location to the current pointer location
    /// @param force If true, ignore even in StrictMouseFocus mode
    void ignoreAtPointer(bool force = false);
    /// Set the "ignore" pointer location to the given coordinates
    /// @param x Current X position of the pointer
    /// @param y Current Y position of the pointer
    /// @param force If true, ignore even in StrictMouseFocus mode
    void ignoreAt(int x, int y, bool force = false);
    /// unset the "ignore" pointer location
    void ignoreCancel();
    /// @return true if events at the given X/Y coordinate should be ignored
    /// (ie, they were previously cached via one of the ignoreAt calls)
    bool isIgnored(int x, int y);

    /// @return true if cycling is in progress
    bool isCycling() const { return m_cycling_list != 0; }
    /// Appends a client to the front of the focus list
    void addFocusBack(WinClient &client);
    /// Appends a client to the front of the focus list
    void addFocusFront(WinClient &client);
    void addFocusWinBack(Focusable &win);
    void addFocusWinFront(Focusable &win);
    void setFocusBack(FluxboxWindow &fbwin);
    /// @return main focus model
    FocusModel focusModel() const { return *m_focus_model; }
    /// @return tab focus model
    TabFocusModel tabFocusModel() const { return *m_tab_focus_model; }
    /// @return true if newly created windows are focused
    bool focusNew() const { return *m_focus_new; }
#ifdef XINERAMA
    /// @return true if focus reverts to same head only
    bool focusSameHead() const { return *m_focus_same_head; }
#endif // XINERAMA

    /// @return last focused client in a specific workspace, or NULL.
    Focusable *lastFocusedWindow(int workspace);

    WinClient *lastFocusedWindow(FluxboxWindow &group, WinClient *ignore_client = 0);

    /// @return focus list in creation order
    const FocusableList &creationOrderList() const { return m_creation_order_list; }
    /// @return the focus list in focused order
    const FocusableList &focusedOrderList() const { return m_focused_list; }
    const FocusableList &creationOrderWinList() const { return m_creation_order_win_list; }
    const FocusableList &focusedOrderWinList() const { return m_focused_win_list; }

    /// remove client from focus list
    void removeClient(WinClient &client);
    /// remove window from focus list
    void removeWindow(Focusable &win);
    /// starts terminating this control
    void shutdown();

    /// do fallback focus for screen if normal focus control failed.
    static void revertFocus(BScreen &screen);
    // like revertFocus, but specifically related to this window (transients etc)
    static void unfocusWindow(WinClient &client, bool full_revert = true, bool unfocus_frame = false);
    static void setFocusedWindow(WinClient *focus_to);
    static void setFocusedFbWindow(FluxboxWindow *focus_to) { s_focused_fbwindow = focus_to; }
    static void setExpectingFocus(WinClient *client) { s_expecting_focus = client; }
    static WinClient *focusedWindow() { return s_focused_window; }
    static FluxboxWindow *focusedFbWindow() { return s_focused_fbwindow; }
    static WinClient *expectingFocus() { return s_expecting_focus; }
private:

    BScreen &m_screen;

    FbTk::Resource<FocusModel> m_focus_model;    
    FbTk::Resource<TabFocusModel> m_tab_focus_model;
    FbTk::Resource<bool> m_focus_new;
#ifdef XINERAMA
    FbTk::Resource<bool> m_focus_same_head;
#endif // XINERAMA

    // This list keeps the order of window focusing for this screen
    // Screen global so it works for sticky windows too.
    FocusableList m_focused_list;
    FocusableList m_creation_order_list;
    FocusableList m_focused_win_list;
    FocusableList m_creation_order_win_list;

    Focusables::const_iterator m_cycling_window;
    const FocusableList *m_cycling_list;
    Focusable *m_was_iconic;
    WinClient *m_cycling_last;
    Focusable *m_cycling_next;
    int m_ignore_mouse_x, m_ignore_mouse_y;

    static WinClient *s_focused_window;
    static FluxboxWindow *s_focused_fbwindow;
    static WinClient *s_expecting_focus;
    static bool s_reverting;
};

#endif // FOCUSCONTROL_HH
