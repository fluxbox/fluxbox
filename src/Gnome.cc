// Gnome.cc for fluxbox
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

// $Id: Gnome.cc,v 1.3 2002/09/10 12:23:03 fluxgen Exp $

#include "Gnome.hh"

#include "Window.hh"
#include "Screen.hh"

#include <iostream>
using namespace std;

Gnome::Gnome() {
	createAtoms();
}

Gnome::~Gnome() {
	// destroy gnome windows
	while (!m_gnomewindows.empty()) {
		XDestroyWindow(BaseDisplay::getXDisplay(), m_gnomewindows.back());
		m_gnomewindows.pop_back();		
	}
}

void Gnome::initForScreen(const BScreen &screen) {
	Display *disp = BaseDisplay::getXDisplay();
	// create the GNOME window
	Window gnome_win = XCreateSimpleWindow(disp,
		screen.getRootWindow(), 0, 0, 5, 5, 0, 0, 0);
	// supported WM check
	XChangeProperty(disp, screen.getRootWindow(), 
		m_gnome_wm_supporting_wm_check, 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

	XChangeProperty(disp, gnome_win, 
		m_gnome_wm_supporting_wm_check, 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

	Atom gnomeatomlist[] = {
		m_gnome_wm_supporting_wm_check,
		m_gnome_wm_win_workspace_names,
		m_gnome_wm_win_client_list,
		m_gnome_wm_win_state,
		m_gnome_wm_win_hints
//		m_gnome_wm_win_layer  not supported yet
	};

	//list atoms that we support
	XChangeProperty(disp, screen.getRootWindow(), 
		m_gnome_wm_prot, XA_ATOM, 32, PropModeReplace,
		(unsigned char *)gnomeatomlist, (sizeof gnomeatomlist)/sizeof gnomeatomlist[0]);

	m_gnomewindows.push_back(gnome_win);

	updateClientList(screen);
	updateWorkspaceNames(screen);
	updateWorkspaceCount(screen);
	updateCurrentWorkspace(screen);
	
}

void Gnome::setupWindow(FluxboxWindow &win) {
	// load gnome state atom
	Display *disp = BaseDisplay::getXDisplay();
	Atom ret_type;
	int fmt;
	unsigned long nitems, bytes_after;
	long flags, *data = 0;

	if (XGetWindowProperty(disp, win.getClientWindow(), 
			m_gnome_wm_win_state, 0, 1, False, XA_CARDINAL, 
			&ret_type, &fmt, &nitems, &bytes_after, 
			(unsigned char **) &data) ==  Success && data) {
		flags = *data;
		setState(&win, flags);
		XFree (data);
	}

	// make sure we get right workspace
	updateWorkspace(win);

}

void Gnome::updateClientList(const BScreen &screen) {
	size_t num=0;

	BScreen::Workspaces::const_iterator workspace_it = screen.getWorkspacesList().begin();
	BScreen::Workspaces::const_iterator workspace_it_end = screen.getWorkspacesList().end();
	for (; workspace_it != workspace_it_end; ++workspace_it) {
		num += (*workspace_it)->getWindowList().size();
	}
	//int num = getCurrentWorkspace()->getWindowList().size();
	
	Window *wl = new (nothrow) Window[num];
	if (wl == 0) {
		cerr<<"Fatal: Out of memory, can't allocate for gnome client list"<<endl;
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
			// TODO!
			//check if the window don't want to be visible in the list
			//if (! ( (*it)->getGnomeHints() & WIN_STATE_HIDDEN) ) {
				wl[win++] = (*it)->getClientWindow();
			//}
		}
	}
	//number of windows to show in client list
	num = win;
	XChangeProperty(BaseDisplay::getXDisplay(), 
		screen.getRootWindow(), 
		m_gnome_wm_win_client_list, 
		XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)wl, num);
	
	delete wl;
}

void Gnome::updateWorkspaceNames(const BScreen &screen) {
	XTextProperty	text;
	int number_of_desks = screen.getWorkspaceNames().size();
	
	char s[1024];
	char *names[number_of_desks];		
	
	for (int i = 0; i < number_of_desks; i++) {		
		sprintf(s, "Desktop %i", i);
		names[i] = new char[strlen(s) + 1];
		strcpy(names[i], s);
	}
	
	if (XStringListToTextProperty(names, number_of_desks, &text)) {
		XSetTextProperty(BaseDisplay::getXDisplay(), screen.getRootWindow(),
			 &text, m_gnome_wm_win_workspace_names);
		XFree(text.value);
	}
	
	for (int i = 0; i < number_of_desks; i++)
		delete [] names[i];
}

void Gnome::updateCurrentWorkspace(const BScreen &screen) {
	int workspace = screen.getCurrentWorkspaceID();
	XChangeProperty(BaseDisplay::getXDisplay(), 
		screen.getRootWindow(),
		m_gnome_wm_win_workspace, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&workspace, 1);

	updateClientList(screen); // make sure the client list is updated too
}

void Gnome::updateWorkspaceCount(const BScreen &screen) {
	int numworkspaces = screen.getCount();
	XChangeProperty(BaseDisplay::getXDisplay(), screen.getRootWindow(),
		m_gnome_wm_win_workspace_count, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&numworkspaces, 1);
}

void Gnome::updateWorkspace(FluxboxWindow &win) {
	int val = win.getWorkspaceNumber(); 
#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): setting workspace("<<val<<
		") for window("<<&win<<")"<<endl;
#endif // DEBUG
	XChangeProperty(BaseDisplay::getXDisplay(), win.getClientWindow(), 
		m_gnome_wm_win_workspace, 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&val, 1);
}

void Gnome::updateState(FluxboxWindow &win) {
	//translate to gnome win state
	int state=0;
	if (win.isStuck())
		state |= WIN_STATE_STICKY;
	if (win.isIconic())
		state |= WIN_STATE_MINIMIZED;
	if (win.isShaded())
		state |= WIN_STATE_SHADED;
	
	XChangeProperty(BaseDisplay::getXDisplay(), win.getClientWindow(), 
		m_gnome_wm_win_state,
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&state, 1);
}

void Gnome::updateHints(FluxboxWindow &win) {
	//TODO
	
}

bool Gnome::checkClientMessage(const XClientMessageEvent &ce, BScreen *screen, FluxboxWindow *win) {
	if (ce.message_type == m_gnome_wm_win_workspace) {
#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): Got workspace atom="<<ce.data.l[0]<<endl;
#endif//!DEBUG
		if ( win !=0 && // the message sent to client window?
				win->getScreen() && ce.data.l[0] >= 0 &&
				ce.data.l[0] < (signed)win->getScreen()->getCount()) {
			win->getScreen()->changeWorkspaceID(ce.data.l[0]);
					
		} else if (screen!=0 && //the message sent to root window?
				ce.data.l[0] >= 0 &&
				ce.data.l[0] < (signed)screen->getCount())
			screen->changeWorkspaceID(ce.data.l[0]);
		return true;
	} else if (win == 0)
		return false; 
		

	if (ce.message_type == m_gnome_wm_win_state) {
#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_STATE"<<endl;
#endif // DEBUG
			
#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): Mask of members to change:"<<
			hex<<ce.data.l[0]<<dec<<endl; // mask_of_members_to_change
		cerr<<"New members:"<<ce.data.l[1]<<endl;
#endif // DEBUG
	
		//get new states			
		int flag = ce.data.l[0] & ce.data.l[1];
		//don't update this when when we set new state
		disableUpdate();
		// convert to Fluxbox state
		setState(win, flag);
		// enable update of atom states
		enableUpdate();
			
	} else if (ce.message_type == m_gnome_wm_win_hints) {
#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_HINTS"<<endl;
#endif // DEBUG

	} else 
		return false; //the gnome atom wasn't found or not supported

	return true; // we handled the atom
}

void Gnome::setState(FluxboxWindow *win, int state) {
#ifdef DEBUG
	cerr<<"Gnome: state=0x"<<hex<<state<<dec<<endl;
#endif // DEBUG

	if (state & WIN_STATE_STICKY) {
#ifdef DEBUG
		cerr<<"Gnome state: Sticky"<<endl;
#endif // DEBUG
		if (!win->isStuck())
			win->stick();
	} else if (win->isStuck())
		win->stick();
			
	if (state & WIN_STATE_MINIMIZED) {
#ifdef DEBUG
		cerr<<"Gnome state: Minimized"<<endl;
#endif // DEBUG
		if (win->isIconic())
			win->iconify();
	} else if (win->isIconic())
		win->deiconify(true, true);

	if (state & WIN_STATE_SHADED) {
#ifdef DEBUG
		cerr<<"Gnome state: Shade"<<endl;
#endif // DEBUG
		if (!win->isShaded())
			win->shade();
	} else if (win->isShaded())
		win->shade();

	/* TODO	
	if (state & WIN_STATE_MAXIMIZED_VERT)
		cerr<<"Maximize Vert"<<endl;
	if (state & WIN_STATE_MAXIMIZED_HORIZ)
		cerr<<"Maximize Horiz"<<endl;
	if (state & WIN_STATE_HIDDEN)
		cerr<<"Hidden"<<endl;
	if (state & WIN_STATE_HID_WORKSPACE)
		cerr<<"HID Workspace"<<endl;
	if (state & WIN_STATE_HID_TRANSIENT)
		cerr<<"HID Transient"<<endl;
	if (state & WIN_STATE_FIXED_POSITION)
		cerr<<"Fixed Position"<<endl;
	if (state & WIN_STATE_ARRANGE_IGNORE)
		cerr<<"Arrange Ignore"<<endl;			
	*/
}

void Gnome::setLayer(GnomeLayer layer) {
	FluxboxWindow::WinLayer winlayer;
	
	switch (layer) {
		case WIN_LAYER_DESKTOP:
			winlayer = FluxboxWindow::LAYER_BOTTOM;
		break;
		case WIN_LAYER_BELOW:
			winlayer = FluxboxWindow::LAYER_BELOW;
		break;
		case WIN_LAYER_NORMAL:
			winlayer = FluxboxWindow::LAYER_NORMAL;
		break;		
		case WIN_LAYER_ONTOP:
		case WIN_LAYER_DOCK:
		case WIN_LAYER_ABOVE_DOCK:
		case WIN_LAYER_MENU:
			winlayer = FluxboxWindow::LAYER_TOP;
		break;
		default:
			winlayer = FluxboxWindow::LAYER_NORMAL;
		break;
	}
}

void Gnome::createAtoms() {
	Display *disp = BaseDisplay::getXDisplay();
	m_gnome_wm_win_layer = XInternAtom(disp, "_WIN_LAYER", False);
	m_gnome_wm_win_state = XInternAtom(disp, "_WIN_STATE", False);
	m_gnome_wm_win_hints = XInternAtom(disp, "_WIN_HINTS", False);
	m_gnome_wm_win_app_state = XInternAtom(disp, "_WIN_APP_STATE", False);
	m_gnome_wm_win_expanded_size = XInternAtom(disp, "_WIN_EXPANDED_SIZE", False);
	m_gnome_wm_win_icons = XInternAtom(disp, "_WIN_ICONS", False);
	m_gnome_wm_win_workspace = XInternAtom(disp, "_WIN_WORKSPACE", False);
	m_gnome_wm_win_workspace_count = XInternAtom(disp, "_WIN_WORKSPACE_COUNT", False);
	m_gnome_wm_win_workspace_names = XInternAtom(disp, "_WIN_WORKSPACE_NAMES", False);
	m_gnome_wm_win_client_list = XInternAtom(disp, "_WIN_CLIENT_LIST", False);
	m_gnome_wm_prot = XInternAtom(disp, "_WIN_PROTOCOLS", False);
	m_gnome_wm_supporting_wm_check = XInternAtom(disp, "_WIN_SUPPORTING_WM_CHECK", False);
}
