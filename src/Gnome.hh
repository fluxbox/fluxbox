// Gnome.hh for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef GNOME_HH
#define GNOME_HH

#include "AtomHandler.hh"

#include <X11/Xatom.h>
#include <vector>

// Implementes Gnome Window Manager Hints (http://developer.gnome.org/doc/standards/wm/book1.html)
class Gnome:public AtomHandler {
public:
    enum GnomeLayer { 
        WIN_LAYER_DESKTOP = 0,
        WIN_LAYER_BELOW = 2,
        WIN_LAYER_NORMAL = 4,
        WIN_LAYER_ONTOP = 6,
        WIN_LAYER_DOCK = 8,
        WIN_LAYER_ABOVE_DOCK = 10,
        WIN_LAYER_MENU = 12
    };

    enum GnomeState {
        WIN_STATE_STICKY          = (1<<0), // everyone knows sticky
        WIN_STATE_MINIMIZED       = (1<<1), // Reserved - definition is unclear
        WIN_STATE_MAXIMIZED_VERT  = (1<<2), // window in maximized V state
        WIN_STATE_MAXIMIZED_HORIZ = (1<<3), // window in maximized H state
        WIN_STATE_HIDDEN          = (1<<4), // not on taskbar but window visible
        WIN_STATE_SHADED          = (1<<5), // shaded (MacOS / Afterstep style)
        WIN_STATE_HID_WORKSPACE   = (1<<6), // not on current desktop
        WIN_STATE_HID_TRANSIENT   = (1<<7), // owner of transient is hidden
        WIN_STATE_FIXED_POSITION  = (1<<8), // window is fixed in position even
        WIN_STATE_ARRANGE_IGNORE  = (1<<9)  // ignore for auto arranging
    };

    enum GnomeHints {
        WIN_HINTS_SKIP_FOCUS      = (1<<0), // skip this window
        WIN_HINTS_SKIP_WINLIST    = (1<<1), // do not show in window list
        WIN_HINTS_SKIP_TASKBAR    = (1<<2), // do not show on taskbar
        WIN_HINTS_GROUP_TRANSIENT = (1<<3), // Reserved - definition is unclear
        WIN_HINTS_FOCUS_ON_CLICK  = (1<<4)  // app only accepts focus if clicked
    };
	
    Gnome();
    ~Gnome();
    void initForScreen(BScreen &screen);
    void setupFrame(FluxboxWindow &win);
    void setupClient(WinClient &winclient) {}

    void updateWorkarea(BScreen &) { }
    void updateFocusedWindow(BScreen &, Window) { }
    void updateClientList(BScreen &screen);
    void updateClientClose(WinClient &winclient);
    void updateWorkspaceNames(BScreen &screen);
    void updateCurrentWorkspace(BScreen &screen);
    void updateWorkspaceCount(BScreen &screen);

    void updateState(FluxboxWindow &win);
    void updateLayer(FluxboxWindow &win);
    void updateHints(FluxboxWindow &win);
    void updateWorkspace(FluxboxWindow &win);

    bool checkClientMessage(const XClientMessageEvent &ce, BScreen * screen, WinClient * const winclient);
	
    // ignore these ones
    void updateFrameClose(FluxboxWindow &win) {}
    bool propertyNotify(WinClient &winclient, Atom the_property);

private:
    void setLayer(FluxboxWindow *win, int layer);
    void setState(FluxboxWindow *win, int state);
    void setLayer(int layer);
    void createAtoms();
    Atom m_gnome_wm_win_layer, m_gnome_wm_win_state, m_gnome_wm_win_hints,
        m_gnome_wm_win_app_state, m_gnome_wm_win_expanded_size,
        m_gnome_wm_win_icons, m_gnome_wm_win_workspace,
        m_gnome_wm_win_workspace_count, m_gnome_wm_win_workspace_names,
        m_gnome_wm_win_client_list;
    Atom m_gnome_wm_prot;
    Atom m_gnome_wm_supporting_wm_check;	
    std::vector<Window> m_gnomewindows;
};

#endif // GNOME_HH
