// Ewmh.cc for fluxbox
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

// $Id: Ewmh.cc,v 1.2 2002/10/11 10:20:33 fluxgen Exp $

#include "Ewmh.hh" 

#include "Screen.hh"
#include "Window.hh"

#include <iostream>
using namespace std;

Ewmh::Ewmh() {
	createAtoms();
}

Ewmh::~Ewmh() {
	while (!m_windows.empty()) {
		XDestroyWindow(BaseDisplay::getXDisplay(), m_windows.back());
		m_windows.pop_back();
	}
}

void Ewmh::initForScreen(const BScreen &screen) {
	Display *disp = BaseDisplay::getXDisplay();

	XSetWindowAttributes attr;
	attr.override_redirect = True;
	Window wincheck = XCreateWindow(disp, screen.getRootWindow(),
		0, 0, 1, 1, 0,
		CopyFromParent, InputOnly, CopyFromParent,
		CWOverrideRedirect, &attr);
	
	if (wincheck != None) {
		m_windows.push_back(wincheck);
		
		XChangeProperty(disp, screen.getRootWindow(), m_net_supporting_wm_check, XA_WINDOW, 32,
			PropModeReplace, (unsigned char *) &wincheck, 1);

		XChangeProperty(disp, wincheck, m_net_wm_name, XA_STRING, 8,
			PropModeReplace, (unsigned char *) "Fluxbox", strlen("Fluxbox"));
	}
	
	//set supported atoms
	Atom atomsupported[] = {
		m_net_wm_state,
		// states that we support:
		m_net_wm_state_sticky,
		m_net_wm_state_shaded,
		
		m_net_client_list,
		m_net_number_of_desktops,
		m_net_current_desktop,
		m_net_desktop_names,
		m_net_supporting_wm_check		
	};

	XChangeProperty(disp, screen.getRootWindow(), 
		m_net_supported, XA_ATOM, 32,
		PropModeReplace, (unsigned char *) &atomsupported, (sizeof atomsupported)/sizeof atomsupported[0]);

	
}
void Ewmh::setupWindow(FluxboxWindow &win) {
/*
	Display *disp = BaseDisplay::getXDisplay();
	Atom ret_type;
	int fmt;
	unsigned long nitems, bytes_after;
	long flags, *data = 0;

	if (XGetWindowProperty(disp, win.getClientWindow(), 
			m_net_wm_state, 0, 1, False, XA_CARDINAL, 
			&ret_type, &fmt, &nitems, &bytes_after, 
			(unsigned char **) &data) ==  Success && data) {
		flags = *data;
		setState(win, flags);
		XFree (data);
	}
*/
}

void Ewmh::updateClientList(const BScreen &screen) {
	size_t num=0;

	BScreen::Workspaces::const_iterator workspace_it = screen.getWorkspacesList().begin();
	BScreen::Workspaces::const_iterator workspace_it_end = screen.getWorkspacesList().end();
	for (; workspace_it != workspace_it_end; ++workspace_it) {
		num += (*workspace_it)->getWindowList().size();
	}
	//int num = getCurrentWorkspace()->getWindowList().size();
	
	Window *wl = new (nothrow) Window[num];
	if (wl == 0) {
		cerr<<"Fatal: Out of memory, can't allocate for Ewmh client list"<<endl;
		return;
	}
	//start the iterator from begining
	workspace_it = screen.getWorkspacesList().begin();
	int win=0;
	for (; workspace_it != workspace_it_end; ++workspace_it) {
	
		// Fill in array of window ID's
		Workspace::Windows::const_iterator it = (*workspace_it)->getWindowList().begin();
		Workspace::Windows::const_iterator it_end = (*workspace_it)->getWindowList().end();		
		for (; it != it_end; ++it) {
			wl[win++] = (*it)->getClientWindow();
		}
	}
	//number of windows to show in client list
	num = win;
	XChangeProperty(BaseDisplay::getXDisplay(), 
		screen.getRootWindow(), 
		m_net_client_list, 
		XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)wl, num);
	
	delete wl;
}

void Ewmh::updateWorkspaceNames(const BScreen &screen) {
	XTextProperty text;
	const size_t number_of_desks = screen.getWorkspaceNames().size();
	
	char *names[number_of_desks];		
	
	for (size_t i = 0; i < number_of_desks; i++) {		
		names[i] = new char[screen.getWorkspaceNames()[i].size()];
		strcpy(names[i], screen.getWorkspaceNames()[i].c_str());
	}
	
	if (XStringListToTextProperty(names, number_of_desks, &text)) {
		XSetTextProperty(BaseDisplay::getXDisplay(), screen.getRootWindow(),
			 &text, m_net_desktop_names);
		XFree(text.value);
	}
	
	for (size_t i = 0; i < number_of_desks; i++)
		delete [] names[i];	
}

void Ewmh::updateCurrentWorkspace(const BScreen &screen) {
	size_t workspace = screen.getCurrentWorkspaceID();
	XChangeProperty(BaseDisplay::getXDisplay(), 
		screen.getRootWindow(),
		m_net_current_desktop, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&workspace, 1);

}

void Ewmh::updateWorkspaceCount(const BScreen &screen) {
	size_t numworkspaces = screen.getCount();
	XChangeProperty(BaseDisplay::getXDisplay(), screen.getRootWindow(),
		m_net_number_of_desktops, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&numworkspaces, 1);
}

void Ewmh::updateState(FluxboxWindow &win) {

}

void Ewmh::updateHints(FluxboxWindow &win) {

}

void Ewmh::updateWorkspace(FluxboxWindow &win) {

}

bool Ewmh::checkClientMessage(const XClientMessageEvent &ce, BScreen *screen, FluxboxWindow *win) {
	if (win != 0) {
		if (ce.message_type == m_net_wm_state) {
			if (ce.data.l[0] == STATE_REMOVE) {
				setState(*win, ce.data.l[1], false);
				setState(*win, ce.data.l[2], false);
			} else if (ce.data.l[0] == STATE_ADD) {
				setState(*win, ce.data.l[1], true);
				setState(*win, ce.data.l[2], true);
			} else if (ce.data.l[0] == STATE_TOGGLE) {
				toggleState(*win, ce.data.l[1]);
				toggleState(*win, ce.data.l[2]);
			}
		}
	}
	return false;
}


void Ewmh::createAtoms() {
	Display *disp = BaseDisplay::getXDisplay();
	m_net_supported = XInternAtom(disp, "_NET_SUPPORTED", False);
	m_net_client_list = XInternAtom(disp, "_NET_CLIENT_LIST", False);
	m_net_client_list_stacking = XInternAtom(disp, "_NET_CLIENT_LIST_STACKING", False);
	m_net_number_of_desktops = XInternAtom(disp, "_NET_NUMBER_OF_DESKTOPS", False);
	m_net_desktop_geometry = XInternAtom(disp, "_NET_DESKTOP_GEOMETRY", False);
	m_net_desktop_viewport = XInternAtom(disp, "_NET_DESKTOP_VIEWPORT", False);
	m_net_current_desktop = XInternAtom(disp, "_NET_CURRENT_DESKTOP", False);
	m_net_desktop_names = XInternAtom(disp, "_NET_DESKTOP_NAMES", False);
	m_net_active_window = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
	m_net_workarea = XInternAtom(disp, "_NET_WORKAREA", False);
	m_net_supporting_wm_check = XInternAtom(disp, "_NET_SUPPORTING_WM_CHECK", False);
	m_net_virtual_roots = XInternAtom(disp, "_NET_VIRTUAL_ROOTS", False);

	m_net_close_window = XInternAtom(disp, "_NET_CLOSE_WINDOW", False);
	m_net_wm_moveresize = XInternAtom(disp, "_NET_WM_MOVERESIZE", False);

	m_net_properties = XInternAtom(disp, "_NET_PROPERTIES", False);
	m_net_wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
	m_net_wm_desktop = XInternAtom(disp, "_NET_WM_DESKTOP", False);
	m_net_wm_window_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
	
	// state atom and the supported state atoms
	m_net_wm_state = XInternAtom(disp, "_NET_WM_STATE", False);
	m_net_wm_state_sticky = XInternAtom(disp, "_NET_WM_STATE_STICKY", False);
	m_net_wm_state_shaded = XInternAtom(disp, "_NET_WM_STATE_SHADED", False);

	m_net_wm_strut = XInternAtom(disp, "_NET_WM_STRUT", False);
	m_net_wm_icon_geometry = XInternAtom(disp, "_NET_WM_ICON_GEOMETRY", False);
	m_net_wm_icon = XInternAtom(disp, "_NET_WM_ICON", False);
	m_net_wm_pid = XInternAtom(disp, "_NET_WM_PID", False);
	m_net_wm_handled_icons = XInternAtom(disp, "_NET_WM_HANDLED_ICONS", False);

	m_net_wm_ping = XInternAtom(disp, "_NET_WM_PING", False);
}

// set window state
void Ewmh::setState(FluxboxWindow &win, Atom state, bool value) const {

	if (state == m_net_wm_state_sticky) { // STICKY
		if (value && !win.isStuck() ||
			(!value && win.isStuck()))
			win.stick();
	} else if (state == m_net_wm_state_shaded) { // SHADED
		if ((value && !win.isShaded()) ||
			(!value && win.isShaded()))
			win.shade();
	} 
	
}

// toggle window state
void Ewmh::toggleState(FluxboxWindow &win, Atom state) const {
	if (state == m_net_wm_state_sticky) {
		win.stick();
	} else if (state == m_net_wm_state_shaded)
		win.shade();
}

