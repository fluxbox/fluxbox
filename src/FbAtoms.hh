// FbAtom.hh
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

// $Id: FbAtoms.hh,v 1.5 2002/08/12 19:25:35 fluxgen Exp $
#ifndef FBATOMS_HH
#define FBATOMS_HH

#include <X11/Xlib.h>
#include <X11/Xatom.h>
/**
	atom handler, should probably be a singleton
*/
class FbAtoms
{
public:
	explicit FbAtoms(Display *display);
	virtual ~FbAtoms();
	void initAtoms(Display *display);

#ifdef GNOME
	inline Atom getGnomeProtAtom() const { return gnome_wm_prot; }
	inline Atom getGnomeClientListAtom() const { return gnome_wm_win_client_list; }
	inline Atom getGnomeSupportingWMCheckAtom() const { return gnome_wm_supporting_wm_check; }
	inline Atom getGnomeWorkspaceAtom() const { return gnome_wm_win_workspace; }
	inline Atom getGnomeWorkspaceCountAtom() const { return gnome_wm_win_workspace_count; }
	inline Atom getGnomeWorkspaceNamesAtom() const { return gnome_wm_win_workspace_names; }
	inline Atom getGnomeStateAtom() const { return gnome_wm_win_state; }
	inline Atom getGnomeHintsAtom() const { return gnome_wm_win_hints; }
	inline Atom getGnomeLayerAtom() const { return gnome_wm_win_layer; }
#endif //GNOME

	inline Atom getWMChangeStateAtom() const { return xa_wm_change_state; }
	inline Atom getWMStateAtom() const { return xa_wm_state; }
	inline Atom getWMDeleteAtom() const { return xa_wm_delete_window; }
	inline Atom getWMProtocolsAtom() const { return xa_wm_protocols; }
	inline Atom getWMTakeFocusAtom() const { return xa_wm_take_focus; }
	inline Atom getWMColormapAtom() const { return xa_wm_colormap_windows; }
	inline Atom getMotifWMHintsAtom() const { return motif_wm_hints; }

	// this atom is for normal app->WM hints about decorations, stacking,
	// starting workspace etc...
	inline Atom getFluxboxHintsAtom() const { return blackbox_hints;}

	// these atoms are for normal app->WM interaction beyond the scope of the
	// ICCCM...
	inline Atom getFluxboxAttributesAtom() const	{ return blackbox_attributes; }
	inline Atom getFluxboxChangeAttributesAtom() const { return blackbox_change_attributes; }

	// these atoms are for window->WM interaction, with more control and
	// information on window "structure"... common examples are
	// notifying apps when windows are raised/lowered... when the user changes
	// workspaces... i.e. "pager talk"
	inline Atom getFluxboxStructureMessagesAtom() const{ return blackbox_structure_messages; }

	// *Notify* portions of the NETStructureMessages protocol
	inline Atom getFluxboxNotifyStartupAtom() const { return blackbox_notify_startup; }
	inline Atom getFluxboxNotifyWindowAddAtom() const { return blackbox_notify_window_add; }
	inline Atom getFluxboxNotifyWindowDelAtom() const { return blackbox_notify_window_del; }
	inline Atom getFluxboxNotifyWindowFocusAtom() const { return blackbox_notify_window_focus; }
	inline Atom getFluxboxNotifyCurrentWorkspaceAtom() const { return blackbox_notify_current_workspace; }
	inline Atom getFluxboxNotifyWorkspaceCountAtom() const { return blackbox_notify_workspace_count; }
	inline Atom getFluxboxNotifyWindowRaiseAtom() const { return blackbox_notify_window_raise; }
	inline Atom getFluxboxNotifyWindowLowerAtom() const { return blackbox_notify_window_lower; }

	// atoms to change that request changes to the desktop environment during
	// runtime... these messages can be sent by any client... as the sending
	// client window id is not included in the ClientMessage event...
	inline Atom getFluxboxChangeWorkspaceAtom() const { return blackbox_change_workspace; }
	inline Atom getFluxboxChangeWindowFocusAtom() const { return blackbox_change_window_focus; }
	inline Atom getFluxboxCycleWindowFocusAtom() const { return blackbox_cycle_window_focus; }

#ifdef NEWWMSPEC

	// root window properties
	inline Atom getNETSupportedAtom() const { return net_supported; }
	inline Atom getNETClientListAtom() const { return net_client_list; }
	inline Atom getNETClientListStackingAtom() const { return net_client_list_stacking; }
	inline Atom getNETNumberOfDesktopsAtom() const { return net_number_of_desktops; }
	inline Atom getNETDesktopGeometryAtom() const { return net_desktop_geometry; }
	inline Atom getNETDesktopViewportAtom() const { return net_desktop_viewport; }
	inline Atom getNETCurrentDesktopAtom() const { return net_current_desktop; }
	inline Atom getNETDesktopNamesAtom() const { return net_desktop_names; }
	inline Atom getNETActiveWindowAtom() const { return net_active_window; }
	inline Atom getNETWorkareaAtom() const { return net_workarea; }
	inline Atom getNETSupportingWMCheckAtom() const { return net_supporting_wm_check; }
	inline Atom getNETVirtualRootsAtom() const { return net_virtual_roots; }

	// root window messages
	inline Atom getNETCloseWindowAtom() const { return net_close_window; }
	inline Atom getNETWMMoveResizeAtom() const { return net_wm_moveresize; }

	// application window properties
	inline Atom getNETPropertiesAtom() const { return net_properties; }
	inline Atom getNETWMNameAtom() const { return net_wm_name; }
	inline Atom getNETWMDesktopAtom() const { return net_wm_desktop; }
	inline Atom getNETWMWindowTypeAtom() const { return net_wm_window_type; }
	inline Atom getNETWMStateAtom() const { return net_wm_state; }
	inline Atom getNETWMStrutAtom() const { return net_wm_strut; }
	inline Atom getNETWMIconGeometryAtom() const { return net_wm_icon_geometry; }
	inline Atom getNETWMIconAtom() const { return net_wm_icon; }
	inline Atom getNETWMPidAtom() const { return net_wm_pid; }
	inline Atom getNETWMHandledIconsAtom() const { return net_wm_handled_icons; }

	// application protocols
	inline Atom getNETWMPingAtom() const { return net_wm_ping; }

#endif // NEWWMSPEC

private:
// NETAttributes
	Atom blackbox_attributes, blackbox_change_attributes, blackbox_hints;

	// NETStructureMessages
	Atom blackbox_structure_messages, blackbox_notify_startup,
		blackbox_notify_window_add, blackbox_notify_window_del,
		blackbox_notify_window_focus, blackbox_notify_current_workspace,
		blackbox_notify_workspace_count, blackbox_notify_window_raise,
		blackbox_notify_window_lower;

	// message_types for client -> wm messages
	Atom blackbox_change_workspace, blackbox_change_window_focus,
		blackbox_cycle_window_focus;

#ifdef		NEWWMSPEC

	// root window properties
	Atom net_supported, net_client_list, net_client_list_stacking,
		net_number_of_desktops, net_desktop_geometry, net_desktop_viewport,
		net_current_desktop, net_desktop_names, net_active_window, net_workarea,
		net_supporting_wm_check, net_virtual_roots;

	// root window messages
	Atom net_close_window, net_wm_moveresize;

	// application window properties
	Atom net_properties, net_wm_name, net_wm_desktop, net_wm_window_type,
		net_wm_state, net_wm_strut, net_wm_icon_geometry, net_wm_icon, net_wm_pid,
		net_wm_handled_icons;
	
			
	// application protocols
	Atom net_wm_ping;

#endif // NEWWMSPEC

#ifdef GNOME
//	union {
		Atom gnome_wm_win_layer, gnome_wm_win_state, gnome_wm_win_hints,
			gnome_wm_win_app_state, gnome_wm_win_expanded_size,
			gnome_wm_win_icons, gnome_wm_win_workspace,
			gnome_wm_win_workspace_count,	gnome_wm_win_workspace_names,
			gnome_wm_win_client_list;
//		Atom gnome_atom_list[10];	
//	};	
	Atom gnome_wm_prot;
	Atom gnome_wm_supporting_wm_check;	
#endif // GNOME
	Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
		xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
		motif_wm_hints;
	bool m_init;
};

#endif //FBATOMS_HH
