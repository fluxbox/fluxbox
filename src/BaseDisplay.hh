// BaseDisplay.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#ifndef   _BASEDISPLAY_HH_
#define   _BASEDISPLAY_HH_

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// forward declaration
class BaseDisplay;
class ScreenInfo;

#include "LinkedList.hh"
#include "Timer.hh"

#define AttribShaded      (1l << 0)
#define AttribMaxHoriz    (1l << 1)
#define AttribMaxVert     (1l << 2)
#define AttribOmnipresent (1l << 3)
#define AttribWorkspace   (1l << 4)
#define AttribStack       (1l << 5)
#define AttribDecoration  (1l << 6)

#define StackTop          (0)
#define StackNormal       (1)
#define StackBottom       (2)

#define DecorNone         (0)
#define DecorNormal       (1)
#define DecorTiny         (2)
#define DecorTool         (3)

typedef struct _blackbox_hints {
  unsigned long flags, attrib, workspace, stack, decoration;
} BlackboxHints;

typedef struct _blackbox_attributes {
  unsigned long flags, attrib, workspace, stack;
  int premax_x, premax_y;
  unsigned int premax_w, premax_h;
} BlackboxAttributes;

#define PropBlackboxHintsElements      (5)
#define PropBlackboxAttributesElements (8)

#ifndef    __EMX__
void bexec(const char *, char *);
#endif // !__EMX__


// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
// 
// Screen.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Softwar
template <typename Z> inline Z min(Z a, Z b) { return ((a < b) ? a : b); }
template <typename Z> inline Z max(Z a, Z b) { return ((a > b) ? a : b); }

class BaseDisplay {

public:
  BaseDisplay(char *, char * = 0);
  virtual ~BaseDisplay(void);

#ifdef GNOME
	inline Atom *getGnomeListAtoms() { return gnome_atom_list; }
	inline Atom &getGnomeProtAtom() { return gnome_wm_prot; }
	inline Atom &getGnomeClientListAtom() { return gnome_wm_win_client_list; }
	inline Atom &getGnomeSupportingWMCheckAtom() { return gnome_wm_supporting_wm_check; }
	inline Atom &getGnomeWorkspaceAtom() { return gnome_wm_win_workspace; }
	inline Atom &getGnomeWorkspaceCountAtom() { return gnome_wm_win_workspace_count; }
	inline Atom &getGnomeWorkspaceNamesAtom() { return gnome_wm_win_workspace_names; }
#endif //GNOME

  inline const Atom &getWMChangeStateAtom(void) const
    { return xa_wm_change_state; }
  inline const Atom &getWMStateAtom(void) const
    { return xa_wm_state; }
  inline const Atom &getWMDeleteAtom(void) const
    { return xa_wm_delete_window; }
  inline const Atom &getWMProtocolsAtom(void) const
    { return xa_wm_protocols; }
  inline const Atom &getWMTakeFocusAtom(void) const
    { return xa_wm_take_focus; }
  inline const Atom &getWMColormapAtom(void) const
    { return xa_wm_colormap_windows; }
  inline const Atom &getMotifWMHintsAtom(void) const
    { return motif_wm_hints; }

  // this atom is for normal app->WM hints about decorations, stacking,
  // starting workspace etc...
  inline const Atom &getFluxboxHintsAtom(void) const
    { return blackbox_hints;}

  // these atoms are for normal app->WM interaction beyond the scope of the
  // ICCCM...
  inline const Atom &getFluxboxAttributesAtom(void) const
    { return blackbox_attributes; }
  inline const Atom &getFluxboxChangeAttributesAtom(void) const
    { return blackbox_change_attributes; }

  // these atoms are for window->WM interaction, with more control and
  // information on window "structure"... common examples are
  // notifying apps when windows are raised/lowered... when the user changes
  // workspaces... i.e. "pager talk"
  inline const Atom &getFluxboxStructureMessagesAtom(void) const
    { return blackbox_structure_messages; }

  // *Notify* portions of the NETStructureMessages protocol
  inline const Atom &getFluxboxNotifyStartupAtom(void) const
    { return blackbox_notify_startup; }
  inline const Atom &getFluxboxNotifyWindowAddAtom(void) const
    { return blackbox_notify_window_add; }
  inline const Atom &getFluxboxNotifyWindowDelAtom(void) const
    { return blackbox_notify_window_del; }
  inline const Atom &getFluxboxNotifyWindowFocusAtom(void) const
    { return blackbox_notify_window_focus; }
  inline const Atom &getFluxboxNotifyCurrentWorkspaceAtom(void) const
    { return blackbox_notify_current_workspace; }
  inline const Atom &getFluxboxNotifyWorkspaceCountAtom(void) const
    { return blackbox_notify_workspace_count; }
  inline const Atom &getFluxboxNotifyWindowRaiseAtom(void) const
    { return blackbox_notify_window_raise; }
  inline const Atom &getFluxboxNotifyWindowLowerAtom(void) const
    { return blackbox_notify_window_lower; }

  // atoms to change that request changes to the desktop environment during
  // runtime... these messages can be sent by any client... as the sending
  // client window id is not included in the ClientMessage event...
  inline const Atom &getFluxboxChangeWorkspaceAtom(void) const
    { return blackbox_change_workspace; }
  inline const Atom &getFluxboxChangeWindowFocusAtom(void) const
    { return blackbox_change_window_focus; }
  inline const Atom &getFluxboxCycleWindowFocusAtom(void) const
    { return blackbox_cycle_window_focus; }

#ifdef    NEWWMSPEC

  // root window properties
  inline const Atom &getNETSupportedAtom(void) const
    { return net_supported; }
  inline const Atom &getNETClientListAtom(void) const
    { return net_client_list; }
  inline const Atom &getNETClientListStackingAtom(void) const
    { return net_client_list_stacking; }
  inline const Atom &getNETNumberOfDesktopsAtom(void) const
    { return net_number_of_desktops; }
  inline const Atom &getNETDesktopGeometryAtom(void) const
    { return net_desktop_geometry; }
  inline const Atom &getNETDesktopViewportAtom(void) const
    { return net_desktop_viewport; }
  inline const Atom &getNETCurrentDesktopAtom(void) const
    { return net_current_desktop; }
  inline const Atom &getNETDesktopNamesAtom(void) const
    { return net_desktop_names; }
  inline const Atom &getNETActiveWindowAtom(void) const
    { return net_active_window; }
  inline const Atom &getNETWorkareaAtom(void) const
    { return net_workarea; }
  inline const Atom &getNETSupportingWMCheckAtom(void) const
    { return net_supporting_wm_check; }
  inline const Atom &getNETVirtualRootsAtom(void) const
    { return net_virtual_roots; }

  // root window messages
  inline const Atom &getNETCloseWindowAtom(void) const
    { return net_close_window; }
  inline const Atom &getNETWMMoveResizeAtom(void) const
    { return net_wm_moveresize; }

  // application window properties
  inline const Atom &getNETPropertiesAtom(void) const
    { return net_properties; }
  inline const Atom &getNETWMNameAtom(void) const
    { return net_wm_name; }
  inline const Atom &getNETWMDesktopAtom(void) const
    { return net_wm_desktop; }
  inline const Atom &getNETWMWindowTypeAtom(void) const
    { return net_wm_window_type; }
  inline const Atom &getNETWMStateAtom(void) const
    { return net_wm_state; }
  inline const Atom &getNETWMStrutAtom(void) const
    { return net_wm_strut; }
  inline const Atom &getNETWMIconGeometryAtom(void) const
    { return net_wm_icon_geometry; }
  inline const Atom &getNETWMIconAtom(void) const
    { return net_wm_icon; }
  inline const Atom &getNETWMPidAtom(void) const
    { return net_wm_pid; }
  inline const Atom &getNETWMHandledIconsAtom(void) const
    { return net_wm_handled_icons; }

  // application protocols
  inline const Atom &getNETWMPingAtom(void) const
    { return net_wm_ping; }

#endif // NEWWMSPEC

  inline ScreenInfo *getScreenInfo(int s)
    { return (ScreenInfo *) screenInfoList->find(s); }

  inline const Bool &hasShapeExtensions(void) const
    { return shape.extensions; }
  inline const Bool &doShutdown(void) const
    { return _shutdown; }
  inline const Bool &isStartup(void) const
    { return _startup; }

  inline const Cursor &getSessionCursor(void) const
    { return cursor.session; }
  inline const Cursor &getMoveCursor(void) const
    { return cursor.move; }
  inline const Cursor &getLowerLeftAngleCursor(void) const
    { return cursor.ll_angle; }
  inline const Cursor &getLowerRightAngleCursor(void) const
    { return cursor.lr_angle; }

  inline Display *getXDisplay(void) { return display; }

  inline const char *getXDisplayName(void) const
    { return (const char *) display_name; }
  inline const char *getApplicationName(void) const
    { return (const char *) application_name; }

  inline const int &getNumberOfScreens(void) const
    { return number_of_screens; }
  inline const int &getShapeEventBase(void) const
    { return shape.event_basep; }

  inline void shutdown(void) { _shutdown = True; }
  inline void run(void) { _startup = _shutdown = False; }

  const Bool validateWindow(Window);

  void grab(void);
  void ungrab(void);
  void eventLoop(void);
  void addTimer(BTimer *);
  void removeTimer(BTimer *);

  // another pure virtual... this is used to handle signals that BaseDisplay
  // doesn't understand itself
  virtual Bool handleSignal(int) = 0;
	
private:
  struct cursor {
    Cursor session, move, ll_angle, lr_angle;
  } cursor;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;

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

#ifdef    NEWWMSPEC

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
	union {
		Atom gnome_wm_win_layer, gnome_wm_win_state, gnome_wm_win_hints,
			gnome_wm_win_app_state, gnome_wm_win_expanded_size,
			gnome_wm_win_icons, gnome_wm_win_workspace,
			gnome_wm_win_workspace_count,	gnome_wm_win_workspace_names,
			gnome_wm_win_client_list;
		Atom gnome_atom_list[10];	
	};	
	Atom gnome_wm_prot;
	Atom gnome_wm_supporting_wm_check;	
#endif // GNOME


  Bool _startup, _shutdown;
  Display *display;
  LinkedList<ScreenInfo> *screenInfoList;
  LinkedList<BTimer> *timerList;

  char *display_name, *application_name;
  int number_of_screens, server_grabs, colors_per_channel;

protected:
  
  virtual void process_event(XEvent *) = 0;


};


class ScreenInfo {
public:
  ScreenInfo(BaseDisplay *, int);

  inline BaseDisplay *getBaseDisplay(void) { return basedisplay; }

  inline Visual *getVisual(void) { return visual; }
  inline const Window &getRootWindow(void) const { return root_window; }
  inline const Colormap &getColormap(void) const { return colormap; }

  inline const int &getDepth(void) const { return depth; }
  inline const int &getScreenNumber(void) const { return screen_number; }

  inline const unsigned int &getWidth(void) const { return width; }
  inline const unsigned int &getHeight(void) const { return height; }

private:
  BaseDisplay *basedisplay;
  Visual *visual;
  Window root_window;
  Colormap colormap;

  int depth, screen_number;
  unsigned int width, height;


};


#endif // __BaseDisplay_hh
