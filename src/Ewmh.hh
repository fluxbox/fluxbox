// Ewmh.hh for fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@fluxbox.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Ewmh.hh,v 1.7 2003/06/18 13:33:15 fluxgen Exp $

#include "AtomHandler.hh"

#include <X11/Xatom.h>
#include <vector>

class Ewmh:public AtomHandler {
public:

    Ewmh();
    ~Ewmh();
    void initForScreen(BScreen &screen);
    void setupWindow(FluxboxWindow &win);
	
    void updateClientList(BScreen &screen);
    void updateWorkspaceNames(BScreen &screen);
    void updateCurrentWorkspace(BScreen &screen);
    void updateWorkspaceCount(BScreen &screen);

    void updateState(FluxboxWindow &win);
    void updateLayer(FluxboxWindow &win);
    void updateHints(FluxboxWindow &win);
    void updateWorkspace(FluxboxWindow &win);


    bool checkClientMessage(const XClientMessageEvent &ce, 
                            BScreen * screen, FluxboxWindow * const win);

    bool propertyNotify(FluxboxWindow &win, Atom the_property);
    //ignore these ones
    void updateWindowClose(FluxboxWindow &win) {}

private:
	
    enum { STATE_REMOVE = 0, STATE_ADD = 1, STATE_TOGGLE = 2};
	
    void setState(FluxboxWindow &win, Atom state, bool value) const;
    void toggleState(FluxboxWindow &win, Atom state) const;
    void createAtoms();
    void updateStrut(FluxboxWindow &win);

    // root window properties
    Atom m_net_supported, m_net_client_list, m_net_client_list_stacking,
        m_net_number_of_desktops, m_net_desktop_geometry, m_net_desktop_viewport,
        m_net_current_desktop, m_net_desktop_names, m_net_active_window, m_net_workarea,
        m_net_supporting_wm_check, m_net_virtual_roots, m_net_moveresize_window;

    // root window messages
    Atom m_net_close_window, m_net_wm_moveresize;

    // application window properties
    Atom m_net_properties, m_net_wm_name, m_net_wm_desktop, m_net_wm_window_type,
        m_net_wm_state, m_net_wm_state_sticky, m_net_wm_state_shaded,
        m_net_wm_strut, m_net_wm_icon_geometry, m_net_wm_icon, m_net_wm_pid,
        m_net_wm_handled_icons;
			
    // application protocols
    Atom m_net_wm_ping;

    std::vector<Window> m_windows;
};
