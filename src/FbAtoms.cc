// BaseDisplay.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// FbAtom.cc
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: FbAtoms.cc,v 1.1 2002/03/18 15:23:08 fluxgen Exp $

#include "FbAtoms.hh"

FbAtoms::FbAtoms(Display *display):m_init(false) {
	if (display)
		initAtoms(display);
}

FbAtoms::~FbAtoms() {

}

void FbAtoms::initAtoms(Display *display) {
	if (m_init) //already done init?
		return;
	else
		m_init = true;

	xa_wm_colormap_windows =
		XInternAtom(display, "WM_COLORMAP_WINDOWS", False);
	xa_wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
	xa_wm_state = XInternAtom(display, "WM_STATE", False);
	xa_wm_change_state = XInternAtom(display, "WM_CHANGE_STATE", False);
	xa_wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
	xa_wm_take_focus = XInternAtom(display, "WM_TAKE_FOCUS", False);
	motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);

	blackbox_hints = XInternAtom(display, "_BLACKBOX_HINTS", False);
	blackbox_attributes = XInternAtom(display, "_BLACKBOX_ATTRIBUTES", False);
	blackbox_change_attributes =
		XInternAtom(display, "_BLACKBOX_CHANGE_ATTRIBUTES", False);

	blackbox_structure_messages =
		XInternAtom(display, "_BLACKBOX_STRUCTURE_MESSAGES", False);
	blackbox_notify_startup =
		XInternAtom(display, "_BLACKBOX_NOTIFY_STARTUP", False);
	blackbox_notify_window_add =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WINDOW_ADD", False);
	blackbox_notify_window_del =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WINDOW_DEL", False);
	blackbox_notify_current_workspace =
		XInternAtom(display, "_BLACKBOX_NOTIFY_CURRENT_WORKSPACE", False);
	blackbox_notify_workspace_count =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WORKSPACE_COUNT", False);
	blackbox_notify_window_focus =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WINDOW_FOCUS", False);
	blackbox_notify_window_raise =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WINDOW_RAISE", False);
	blackbox_notify_window_lower =
		XInternAtom(display, "_BLACKBOX_NOTIFY_WINDOW_LOWER", False);

	blackbox_change_workspace =
		XInternAtom(display, "_BLACKBOX_CHANGE_WORKSPACE", False);
	blackbox_change_window_focus =
		XInternAtom(display, "_BLACKBOX_CHANGE_WINDOW_FOCUS", False);
	blackbox_cycle_window_focus =
		XInternAtom(display, "_BLACKBOX_CYCLE_WINDOW_FOCUS", False);

#ifdef NEWWMSPEC

	net_supported = XInternAtom(display, "_NET_SUPPORTED", False);
	net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);
	net_client_list_stacking = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
	net_number_of_desktops = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
	net_desktop_geometry = XInternAtom(display, "_NET_DESKTOP_GEOMETRY", False);
	net_desktop_viewport = XInternAtom(display, "_NET_DESKTOP_VIEWPORT", False);
	net_current_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
	net_desktop_names = XInternAtom(display, "_NET_DESKTOP_NAMES", False);
	net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
	net_workarea = XInternAtom(display, "_NET_WORKAREA", False);
	net_supporting_wm_check = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	net_virtual_roots = XInternAtom(display, "_NET_VIRTUAL_ROOTS", False);

	net_close_window = XInternAtom(display, "_NET_CLOSE_WINDOW", False);
	net_wm_moveresize = XInternAtom(display, "_NET_WM_MOVERESIZE", False);

	net_properties = XInternAtom(display, "_NET_PROPERTIES", False);
	net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
	net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
	net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
	net_wm_strut = XInternAtom(display, "_NET_WM_STRUT", False);
	net_wm_icon_geometry = XInternAtom(display, "_NET_WM_ICON_GEOMETRY", False);
	net_wm_icon = XInternAtom(display, "_NET_WM_ICON", False);
	net_wm_pid = XInternAtom(display, "_NET_WM_PID", False);
	net_wm_handled_icons = XInternAtom(display, "_NET_WM_HANDLED_ICONS", False);

	net_wm_ping = XInternAtom(display, "_NET_WM_PING", False);
	
#endif // NEWWMSPEC

#ifdef GNOME
	
	gnome_wm_win_layer = XInternAtom(display, "_WIN_LAYER", False);
	gnome_wm_win_state = XInternAtom(display, "_WIN_STATE", False);
	gnome_wm_win_hints = XInternAtom(display, "_WIN_HINTS", False);
	gnome_wm_win_app_state = XInternAtom(display, "_WIN_APP_STATE", False);
	gnome_wm_win_expanded_size = XInternAtom(display, "_WIN_EXPANDED_SIZE", False);
	gnome_wm_win_icons = XInternAtom(display, "_WIN_ICONS", False);
	gnome_wm_win_workspace = XInternAtom(display, "_WIN_WORKSPACE", False);
	gnome_wm_win_workspace_count = XInternAtom(display, "_WIN_WORKSPACE_COUNT", False);
	gnome_wm_win_workspace_names = XInternAtom(display, "_WIN_WORKSPACE_NAMES", False);
	gnome_wm_win_client_list = XInternAtom(display, "_WIN_CLIENT_LIST", False);
	gnome_wm_prot = XInternAtom(display, "_WIN_PROTOCOLS", False);
	gnome_wm_supporting_wm_check = XInternAtom(display, "_WIN_SUPPORTING_WM_CHECK", False);
	
#endif // GNOME
}
