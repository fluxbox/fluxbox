// Ewmh.hh for fluxbox
// Copyright (c) 2002 - 2004 Henrik Kinnunen (fluxgen<at>fluxbox.org)
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

// $Id: Ewmh.hh,v 1.18 2004/06/20 04:49:32 rathnor Exp $

#include "AtomHandler.hh"

#include <X11/Xatom.h>
#include <vector>
#include <map>

/// Implementes Extended Window Manager Hints ( http://www.freedesktop.org/Standards/wm-spec )
class Ewmh:public AtomHandler {
public:

    Ewmh();
    ~Ewmh();
    void initForScreen(BScreen &screen);
    void setupFrame(FluxboxWindow &win);
    void setupClient(WinClient &winclient);
    
    void updateFocusedWindow(BScreen &screen, Window win);
    void updateClientList(BScreen &screen);
    void updateWorkspaceNames(BScreen &screen);
    void updateCurrentWorkspace(BScreen &screen);
    void updateWorkspaceCount(BScreen &screen);
    void updateViewPort(BScreen &screen);
    void updateGeometry(BScreen &screen);
    void updateWorkarea(BScreen &screen);

    void updateState(FluxboxWindow &win);
    void updateLayer(FluxboxWindow &win);
    void updateHints(FluxboxWindow &win);
    void updateWorkspace(FluxboxWindow &win);

    bool checkClientMessage(const XClientMessageEvent &ce, 
                            BScreen * screen, WinClient * const winclient);

    bool propertyNotify(WinClient &winclient, Atom the_property);
    void updateFrameClose(FluxboxWindow &win);

    //ignore this one
    void updateClientClose(WinClient &winclient) {}

    void setFullscreen(FluxboxWindow &win, bool value);

private:
	
    typedef struct WindowState {
        WindowState(int x, int y, unsigned int width, unsigned int height, int layer, unsigned int decor);
        int x, y, layer;
        unsigned int width, height, decor;
    } WindowState;

    enum { STATE_REMOVE = 0, STATE_ADD = 1, STATE_TOGGLE = 2};

    void setState(FluxboxWindow &win, Atom state, bool value);
    void toggleState(FluxboxWindow &win, Atom state);
    void createAtoms();
    void updateStrut(WinClient &winclient);
    void setupState(FluxboxWindow &win);

    // root window properties
    Atom m_net_supported, m_net_client_list, m_net_client_list_stacking,
        m_net_number_of_desktops, m_net_desktop_geometry, m_net_desktop_viewport,
        m_net_current_desktop, m_net_desktop_names, m_net_active_window, m_net_workarea,
        m_net_supporting_wm_check, m_net_virtual_roots, m_net_moveresize_window;

    // root window messages
    Atom m_net_close_window, m_net_wm_moveresize;

    // application window properties
    Atom m_net_properties, m_net_wm_name, m_net_wm_desktop, 
        // types
        m_net_wm_window_type,
        m_net_wm_window_type_dock,
        m_net_wm_window_type_desktop,

        // states
        m_net_wm_state, m_net_wm_state_sticky, m_net_wm_state_shaded,
	m_net_wm_state_maximized_horz, m_net_wm_state_maximized_vert,
        m_net_wm_state_fullscreen,
        m_net_wm_state_hidden,
        m_net_wm_state_skip_taskbar,
        m_net_wm_state_below,
        m_net_wm_state_above,

        m_net_wm_strut, m_net_wm_icon_geometry, m_net_wm_icon, m_net_wm_pid,
        m_net_wm_handled_icons;
			
    // application protocols
    Atom m_net_wm_ping;

    std::vector<Window> m_windows;
    typedef std::map<FluxboxWindow *, WindowState *> SavedState;
    SavedState m_savedstate;

    WindowState *getState(FluxboxWindow &win);
    void clearState(FluxboxWindow &win);
    void saveState(FluxboxWindow &win, WindowState *state);

};
