// Slit.cc for fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at linuxmail.org)
//
// Slit.cc for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Slit.cc,v 1.28 2002/11/27 21:50:09 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef	SLIT

#include "i18n.hh"
#include "fluxbox.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"

#include <algorithm>
#include <iostream>
#include <cassert>

#ifdef HAVE_SYS_STAT_H
#	include <sys/types.h>
#	include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <X11/keysym.h>

// Utility method for extracting name from window
namespace {

void getWMName(BScreen *screen, Window window, std::string& name) {
	name = "";

	if (screen == 0 || window == None)
		return;

	Display *display = BaseDisplay::getXDisplay();

	XTextProperty text_prop;
	char **list;
	int num;
	I18n *i18n = I18n::instance();

	if (XGetWMName(display, window, &text_prop)) {
		if (text_prop.value && text_prop.nitems > 0) {
			if (text_prop.encoding != XA_STRING) {
				
				text_prop.nitems = strlen((char *) text_prop.value);
				
				if ((XmbTextPropertyToTextList(display, &text_prop,
							&list, &num) == Success) &&
						(num > 0) && *list) {
					name = static_cast<char *>(*list);
					XFreeStringList(list);
				} else
					name = (char *)text_prop.value;
					
			} else				
				name = (char *)text_prop.value;
		} else { // default name
			name = i18n->getMessage(
				FBNLS::WindowSet, FBNLS::WindowUnnamed,
				"Unnamed");
		}
	} else {
		// default name
		name = i18n->getMessage(
			FBNLS::WindowSet, FBNLS::WindowUnnamed,
			"Unnamed");
	}

}

}; // End anonymous namespace

Slit::Slit(BScreen *scr):m_screen(scr), timer(this), slitmenu(*this) {
	assert(scr);
	Fluxbox * const fluxbox = Fluxbox::instance();

	on_top = screen()->isSlitOnTop();
	hidden = do_auto_hide = screen()->doSlitAutoHide();

	
	frame.window = frame.pixmap = None;

	
	timer.setTimeout(fluxbox->getAutoRaiseDelay());
	timer.fireOnce(True);

	XSetWindowAttributes attrib;
	unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
		CWColormap | CWOverrideRedirect | CWEventMask;
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
		screen()->getBorderColor()->pixel();
	attrib.colormap = screen()->colormap();
	attrib.override_redirect = True;
	attrib.event_mask = SubstructureRedirectMask | ButtonPressMask |
		EnterWindowMask | LeaveWindowMask;

	frame.x = frame.y = 0;
	frame.width = frame.height = 1;

	frame.window =
		XCreateWindow(BaseDisplay::getXDisplay(), screen()->getRootWindow(), frame.x, frame.y,
			frame.width, frame.height, screen()->getBorderWidth(),
			screen()->getDepth(), InputOutput, screen()->getVisual(),
			create_mask, &attrib);
	fluxbox->saveSlitSearch(frame.window, this);

	// Get client list for sorting purposes
	loadClientList();

	reconfigure();
}


Slit::~Slit() {
	screen()->getImageControl()->removeImage(frame.pixmap);

	Fluxbox::instance()->removeSlitSearch(frame.window);

	XDestroyWindow(BaseDisplay::getXDisplay(), frame.window);
}


void Slit::addClient(Window w) {

	//Can't add non existent window
	if (w == None)
		return;

	if (!Fluxbox::instance()->validateWindow(w)) 
		return;

	// Look for slot in client list by name
	SlitClient *client = 0;
	std::string match_name;
	::getWMName(screen(), w, match_name);
	SlitClients::iterator it = clientList.begin();
	SlitClients::iterator it_end = clientList.end();
	bool found_match = false;
	for (; it != it_end; ++it) {
		// If the name matches...
		if ((*it)->match_name == match_name) {
			// Use the slot if no window is assigned
			if ((*it)->window == None) {
				client = (*it);
				client->initialize(screen(), w);
				break;
			}
			// Otherwise keep looking for an unused match or a non-match
			found_match = true;		// Possibly redundant
			
		} else if (found_match) {
			// Insert before first non-match after a previously found match?
			client = new SlitClient(screen(), w);
			clientList.insert(it, client);
			break;
		}
	}
	// Append to client list?
	if (client == 0) {
		client = new SlitClient(screen(), w);
		clientList.push_back(client);
	}
	Display *disp = BaseDisplay::getXDisplay();
	XWMHints *wmhints = XGetWMHints(disp, w);

	if (wmhints) {
		if ((wmhints->flags & IconWindowHint) &&
				(wmhints->icon_window != None)) {
			XMoveWindow(disp, client->client_window, screen()->getWidth() + 10,
				screen()->getHeight() + 10);
			XMapWindow(disp, client->client_window);				
			client->icon_window = wmhints->icon_window;
			client->window = client->icon_window;
		} else {
			client->icon_window = None;
			client->window = client->client_window;
		}

		XFree(wmhints);
	} else {
		client->icon_window = None;
		client->window = client->client_window;
	}
	XWindowAttributes attrib;
#ifdef KDE
	Fluxbox *fluxbox = Fluxbox::instance();
	//Check and see if new client is a KDE dock applet
	//If so force reasonable size
	bool iskdedockapp=false;
	Atom ajunk;
	int ijunk;
	unsigned long *data = (unsigned long *) 0, uljunk;

	// Check if KDE v2.x dock applet
	if (XGetWindowProperty(disp, w,
			fluxbox->getKWM2DockwindowAtom(), 0l, 1l, False,
			fluxbox->getKWM2DockwindowAtom(),
			&ajunk, &ijunk, &uljunk, &uljunk,
			(unsigned char **) &data) == Success) {
		iskdedockapp = (data && data[0] != 0);
		XFree((char *) data);
	}

	// Check if KDE v1.x dock applet
	if (!iskdedockapp) {
		if (XGetWindowProperty(disp, w,
				fluxbox->getKWM1DockwindowAtom(), 0l, 1l, False,
				fluxbox->getKWM1DockwindowAtom(),
				&ajunk, &ijunk, &uljunk, &uljunk,
				(unsigned char **) &data) == Success) {
			iskdedockapp = (data && data[0] != 0);
 		    XFree((char *) data);
  	 	}
	}

	if (iskdedockapp)
		client->width = client->height = 24;
	else
#endif // KDE
	{
		if (XGetWindowAttributes(disp, client->window, &attrib)) {
			client->width = attrib.width;
			client->height = attrib.height;
		} else {
			client->width = client->height = 64;
		}
	}

	XSetWindowBorderWidth(disp, client->window, 0);

	XSelectInput(disp, frame.window, NoEventMask);
	XSelectInput(disp, client->window, NoEventMask);

	XReparentWindow(disp, client->window, frame.window, 0, 0);
	XMapRaised(disp, client->window);
	XChangeSaveSet(disp, client->window, SetModeInsert);

	XSelectInput(disp, frame.window, SubstructureRedirectMask |
		ButtonPressMask | EnterWindowMask | LeaveWindowMask);
	XSelectInput(disp, client->window, StructureNotifyMask |
		SubstructureNotifyMask | EnterWindowMask);
	XFlush(disp);

	Fluxbox::instance()->saveSlitSearch(client->client_window, this);
	Fluxbox::instance()->saveSlitSearch(client->icon_window, this);
	reconfigure();

	saveClientList();

}


void Slit::removeClient(SlitClient *client, bool remap, bool destroy) {
	Fluxbox::instance()->removeSlitSearch(client->client_window);
	Fluxbox::instance()->removeSlitSearch(client->icon_window);

	// Destructive removal?
	if (destroy)
	    clientList.remove(client);
	else // Clear the window info, but keep around to help future sorting?
		client->initialize();

	screen()->removeNetizen(client->window);

	if (remap && Fluxbox::instance()->validateWindow(client->window)) {
		Display *disp = BaseDisplay::getXDisplay();
		XSelectInput(disp, frame.window, NoEventMask);
		XSelectInput(disp, client->window, NoEventMask);
		XReparentWindow(disp, client->window, screen()->getRootWindow(),
			client->x, client->y);
		XChangeSaveSet(disp, client->window, SetModeDelete);
		XSelectInput(disp, frame.window, SubstructureRedirectMask |
			ButtonPressMask | EnterWindowMask | LeaveWindowMask);
		XFlush(disp);
	}

	// Destructive removal?
	if (destroy)
		delete client;
}


void Slit::removeClient(Window w, bool remap) {

	bool reconf = false;

	SlitClients::iterator it = clientList.begin();
	SlitClients::iterator it_end = clientList.end();
	for (; it != it_end; ++it) {
		if ((*it)->window == w) {
			removeClient((*it), remap, false);
			reconf = true;

			break;
		}
	}
	if (reconf)
		reconfigure();

}


void Slit::reconfigure() {
	frame.width = 0;
	frame.height = 0;

	// Need to count windows because not all client list entries
	// actually correspond to mapped windows.
	int num_windows = 0;

	switch (screen()->getSlitDirection()) {
	case VERTICAL: 
		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				//client created window?
				if ((*it)->window != None) {
					num_windows++;
					frame.height += (*it)->height + screen()->getBevelWidth();
					
					//frame width < client window?
					if (frame.width < (*it)->width) 
						frame.width = (*it)->width;
				}
			}
		}

		if (frame.width < 1)
			frame.width = 1;
		else
			frame.width += (screen()->getBevelWidth() * 2);

		if (frame.height < 1)
			frame.height = 1;
		else
			frame.height += screen()->getBevelWidth();

	break;

	case HORIZONTAL: 
		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				//client created window?
				if ((*it)->window != None) {
					num_windows++;
					frame.width += (*it)->width + screen()->getBevelWidth();
					//frame height < client height?
					if (frame.height < (*it)->height)
						frame.height = (*it)->height;
				}
			}
		}

		if (frame.width < 1)
			frame.width = 1;
		else
			frame.width += screen()->getBevelWidth();

		if (frame.height < 1)
			frame.height = 1;
		else
			frame.height += (screen()->getBevelWidth() * 2);

	break;
	}

	reposition();
	Display *disp = BaseDisplay::getXDisplay();

	XSetWindowBorderWidth(disp, frame.window, screen()->getBorderWidth());
	XSetWindowBorder(disp, frame.window,
		screen()->getBorderColor()->pixel());

	//did we actually use slit slots
	if (num_windows == 0)
		XUnmapWindow(disp, frame.window);
	else
		XMapWindow(disp, frame.window);

	Pixmap tmp = frame.pixmap;
	BImageControl *image_ctrl = screen()->getImageControl();
	const FbTk::Texture &texture = screen()->getTheme()->getSlitTexture();
	if (texture.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.pixmap = None;
		XSetWindowBackground(disp, frame.window, texture.color().pixel());
	} else {
		frame.pixmap = image_ctrl->renderImage(frame.width, frame.height,
			texture);
		XSetWindowBackgroundPixmap(disp, frame.window, frame.pixmap);
	}

	if (tmp) 
		image_ctrl->removeImage(tmp);
	XClearWindow(disp, frame.window);

	int x, y;

	switch (screen()->getSlitDirection()) {
	case VERTICAL:
		x = 0;
		y = screen()->getBevelWidth();

		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				//client created window?
				if ((*it)->window == None)
					continue;

				x = (frame.width - (*it)->width) / 2;
				Display *disp = BaseDisplay::getXDisplay();
				XMoveResizeWindow(disp, (*it)->window, x, y,
					(*it)->width, (*it)->height);
				XMapWindow(disp, (*it)->window);

				// for ICCCM compliance
				(*it)->x = x;
				(*it)->y = y;

				XEvent event;
				event.type = ConfigureNotify;

				event.xconfigure.display = disp;
				event.xconfigure.event = (*it)->window;
				event.xconfigure.window = (*it)->window;
				event.xconfigure.x = x;
				event.xconfigure.y = y;
				event.xconfigure.width = (*it)->width;
				event.xconfigure.height = (*it)->height;
				event.xconfigure.border_width = 0;
				event.xconfigure.above = frame.window;
				event.xconfigure.override_redirect = False;

				XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
					&event);

				y += (*it)->height + screen()->getBevelWidth();
			}
		}

		break;

	case HORIZONTAL:
		x = screen()->getBevelWidth();
		y = 0;

		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				//client created window?
				if ((*it)->window == None)
					continue;

				y = (frame.height - (*it)->height) / 2;

				XMoveResizeWindow(disp, (*it)->window, x, y,
					(*it)->width, (*it)->height);
				XMapWindow(disp, (*it)->window);

				// for ICCCM compliance
				(*it)->x = x;
				(*it)->y = y;

				XEvent event;
				event.type = ConfigureNotify;

				event.xconfigure.display = disp;
				event.xconfigure.event = (*it)->window;
				event.xconfigure.window = (*it)->window;
				event.xconfigure.x = frame.x + x + screen()->getBorderWidth();
				event.xconfigure.y = frame.y + y + screen()->getBorderWidth();
				event.xconfigure.width = (*it)->width;
				event.xconfigure.height = (*it)->height;
				event.xconfigure.border_width = 0;
				event.xconfigure.above = frame.window;
				event.xconfigure.override_redirect = False;

				XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
					&event);

				x += (*it)->width + screen()->getBevelWidth();
			}
		}

		break;
	}

	slitmenu.reconfigure();
}


void Slit::reposition() {
	int head_x = 0,
			head_y = 0,
			head_w,
			head_h;
#ifdef XINERAMA
	if (screen()->hasXinerama()) {
		unsigned int head = screen()->getSlitOnHead();

		head_x = screen()->getHeadX(head);
		head_y = screen()->getHeadY(head);
		head_w = screen()->getHeadWidth(head);
		head_h = screen()->getHeadHeight(head);
	} else {
		head_w = screen()->getWidth();
		head_h = screen()->getHeight();
	}
#else // !XINERAMA
		head_w = screen()->getWidth();
		head_h = screen()->getHeight();
#endif // XINERAMA

	// place the slit in the appropriate place
	switch (screen()->getSlitPlacement()) {
	case TOPLEFT:
		frame.x = head_x;
		frame.y = head_y;
		if (screen()->getSlitDirection() == VERTICAL) {
			frame.x_hidden = screen()->getBevelWidth() -
				screen()->getBorderWidth() - frame.width;
			frame.y_hidden = head_y;
		} else {
			frame.x_hidden = head_x;
			frame.y_hidden = screen()->getBevelWidth() -
				screen()->getBorderWidth() - frame.height;
		}
		break;

	case CENTERLEFT:
		frame.x = head_x;
		frame.y = head_y + (head_h - frame.height) / 2;
		frame.x_hidden = head_x + screen()->getBevelWidth() -
			screen()->getBorderWidth() - frame.width;
		frame.y_hidden = frame.y;
		break;

	case BOTTOMLEFT:
		frame.x = head_x;
		frame.y = head_h - frame.height - screen()->getBorderWidth2x();
		if (screen()->getSlitDirection() == VERTICAL) {
			frame.x_hidden = head_x + screen()->getBevelWidth() -
				screen()->getBorderWidth() - frame.width;
			frame.y_hidden = frame.y;
		} else {
			frame.x_hidden = head_x;
			frame.y_hidden = head_y + head_h -
				screen()->getBevelWidth() - screen()->getBorderWidth();
		}
		break;

	case TOPCENTER:
		frame.x = head_x + ((head_w - frame.width) / 2);
		frame.y = head_y;
		frame.x_hidden = frame.x;
		frame.y_hidden = head_y + screen()->getBevelWidth() -
			screen()->getBorderWidth() - frame.height;
		break;

	case BOTTOMCENTER:
		frame.x = head_x + ((head_w - frame.width) / 2);
		frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
		frame.x_hidden = frame.x;
		frame.y_hidden = head_y + head_h -
			screen()->getBevelWidth() - screen()->getBorderWidth();
		break;

	case TOPRIGHT:
		frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
		frame.y = head_y;
		if (screen()->getSlitDirection() == VERTICAL) {
			frame.x_hidden = head_x + head_w -
				screen()->getBevelWidth() - screen()->getBorderWidth();
			frame.y_hidden = head_y;
		} else {
			frame.x_hidden = frame.x;
			frame.y_hidden = head_y + screen()->getBevelWidth() -
				screen()->getBorderWidth() - frame.height;
		}
		break;

	case CENTERRIGHT:
	default:
		frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
		frame.y = head_y + ((head_h - frame.height) / 2);
		frame.x_hidden = head_x + head_w -
			screen()->getBevelWidth() - screen()->getBorderWidth();
		frame.y_hidden = frame.y;
		break;

	case BOTTOMRIGHT:
		frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
		frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
		if (screen()->getSlitDirection() == VERTICAL) {
			frame.x_hidden = head_x + head_w - 
				screen()->getBevelWidth() - screen()->getBorderWidth();
			frame.y_hidden = frame.y;
		} else {
			frame.x_hidden = frame.x;
			frame.y_hidden = head_y + head_h - 
				screen()->getBevelWidth() - screen()->getBorderWidth();
		}
		break;
	}

	const Toolbar * const tbar = screen()->getToolbar();
	int sw = frame.width + screen()->getBorderWidth2x(),
		sh = frame.height + screen()->getBorderWidth2x(),
		tw = tbar->width() + screen()->getBorderWidth(),
		th = tbar->height() + screen()->getBorderWidth();

	if (tbar->x() < frame.x + sw && tbar->x() + tw > frame.x &&
			tbar->y() < frame.y + sh && tbar->y() + th > frame.y) {
		if (frame.y < th) {
			frame.y += tbar->exposedHeight();
			if (screen()->getSlitDirection() == VERTICAL)
				frame.y_hidden += tbar->exposedHeight();
			else
				frame.y_hidden = frame.y;
		} else {
			frame.y -= tbar->exposedHeight();
			if (screen()->getSlitDirection() == VERTICAL)
				frame.y_hidden -= tbar->exposedHeight();
			else
				frame.y_hidden = frame.y;
		}
	}

	Display *disp = BaseDisplay::getXDisplay();

	if (hidden) {
		XMoveResizeWindow(disp, frame.window, frame.x_hidden,
			frame.y_hidden, frame.width, frame.height);
	} else {
		XMoveResizeWindow(disp, frame.window, frame.x,
			frame.y, frame.width, frame.height);
	}
}


void Slit::shutdown() {
	saveClientList();
	while (clientList.size() != 0)
		removeClient(clientList.front(), true, true);
}


void Slit::buttonPressEvent(XButtonEvent *e) {
	if (e->window != frame.window) 
		return;

	if (e->button == Button1 && (! on_top)) {
		Workspace::Stack st;
		st.push_back(frame.window);
		screen()->raiseWindows(st);
	} else if (e->button == Button2 && (! on_top)) {
		XLowerWindow(BaseDisplay::getXDisplay(), frame.window);
	} else if (e->button == Button3) {
		if (! slitmenu.isVisible()) {
			int x = e->x_root - (slitmenu.width() / 2),
				y = e->y_root - (slitmenu.height() / 2); 

			if (x < 0)
				x = 0;
			else if (x + slitmenu.width() > screen()->getWidth())
				x = screen()->getWidth() - slitmenu.width();

			if (y < 0)
				y = 0;
			else if (y + slitmenu.height() > screen()->getHeight())
				y = screen()->getHeight() - slitmenu.height();

			slitmenu.move(x, y);
			slitmenu.show();
		} else
			slitmenu.hide();
	}
}


void Slit::enterNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (! timer.isTiming()) 
			timer.start();
	} else {
		if (timer.isTiming()) 
			timer.stop();
	}
}


void Slit::leaveNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (timer.isTiming()) 
			timer.stop();
	} else if (! slitmenu.isVisible()) {
		if (! timer.isTiming()) 
			timer.start();
	}
}


void Slit::configureRequestEvent(XConfigureRequestEvent *e) {

	if (!Fluxbox::instance()->validateWindow(e->window))
		return;

	bool reconf = false;
	XWindowChanges xwc;

	xwc.x = e->x;
	xwc.y = e->y;
	xwc.width = e->width;
	xwc.height = e->height;
	xwc.border_width = 0;
	xwc.sibling = e->above;
	xwc.stack_mode = e->detail;

	XConfigureWindow(BaseDisplay::getXDisplay(), e->window, e->value_mask, &xwc);

	SlitClients::iterator it = clientList.begin();
	SlitClients::iterator it_end = clientList.end();
	for (; it != it_end; ++it) {
		if ((*it)->window == e->window) {
			if ((*it)->width != ((unsigned) e->width) ||
					(*it)->height != ((unsigned) e->height)) {
				(*it)->width = (unsigned) e->width;
				(*it)->height = (unsigned) e->height;

				reconf = true; //requires reconfiguration

				break;
			}
		}
	}

	if (reconf) 
		reconfigure();
}


void Slit::timeout() {
	hidden = ! hidden;
	Display *disp = BaseDisplay::getXDisplay();
	if (hidden)
		XMoveWindow(disp, frame.window, frame.x_hidden, frame.y_hidden);
	else
		XMoveWindow(disp, frame.window, frame.x, frame.y);
}

void Slit::loadClientList() {
	const std::string &filename = Fluxbox::instance()->getSlitlistFilename();
	struct stat buf;
	if (!stat(filename.c_str(), &buf)) {
		std::ifstream file(Fluxbox::instance()->getSlitlistFilename().c_str());
		std::string name;
		while (! file.eof()) {
			name = "";
			std::getline(file, name); // get the entire line
			if (name.size() > 0) {
				SlitClient *client = new SlitClient(name.c_str());
				clientList.push_back(client);
			}
		}
	}
}

void Slit::saveClientList() {
	const std::string &filename = Fluxbox::instance()->getSlitlistFilename();
	std::ofstream file(filename.c_str());
	SlitClients::iterator it = clientList.begin();
	SlitClients::iterator it_end = clientList.end();
	std::string prevName;
	std::string name;
	for (; it != it_end; ++it) {
		name = (*it)->match_name;
		if (name != prevName)
			file << name.c_str() << std::endl;

		prevName = name;
	}
}
void Slit::setOnTop(bool val) {
	on_top = val;
	screen()->saveSlitOnTop(val);
			
	if (isOnTop())
		screen()->raiseWindows(Workspace::Stack());

}


void Slit::setAutoHide(bool val) {
	do_auto_hide = val;
	screen()->saveSlitAutoHide(val);
}

Slitmenu::Slitmenu(Slit &sl) : Basemenu(sl.screen()),
slit(sl),
#ifdef XINERAMA
m_headmenu(0),
#endif // XINERAMA 
m_placementmenu(*this),
m_directionmenu(*this) {

	I18n *i18n = I18n::instance();
	using namespace FBNLS;
	setLabel(i18n->getMessage(
		SlitSet, SlitSlitTitle,
		"Slit"));
	setInternalMenu();


#ifdef XINERAMA
	if (screen()->hasXinerama()) { // only create if we need
		m_headmenu.reset(new Headmenu(*this));
	}
#endif // XINERAMA

	insert(i18n->getMessage(
		CommonSet, CommonDirectionTitle,
		"Direction"),
	 &m_directionmenu);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTitle,
		"Placement"),
	 &m_placementmenu);

#ifdef XINERAMA
	if (screen()->hasXinerama()) {
		insert(i18n->getMessage(0, 0, "Place on Head"), m_headmenu.get());
	}
#endif // XINERAMA

	insert(i18n->getMessage(
		CommonSet, CommonAlwaysOnTop,
		"Always on top"), 1);
	insert(i18n->getMessage(
		CommonSet, CommonAutoHide,
		"Auto hide"), 2);

	setItemSelected(2, slit.isOnTop());
	setItemSelected(3, slit.doAutoHide());
	
	update();

}


Slitmenu::~Slitmenu() {

}


void Slitmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		switch (item->function()) {
		case 1:
			// toggle on top
			slit.setOnTop(slit.isOnTop() ? false : true);
			setItemSelected(2, slit.isOnTop());
		break;
		case 2: // auto hide
			slit.setAutoHide(slit.doAutoHide() ? false : true);
			setItemSelected(3, slit.doAutoHide());
		break;
		}
		
		//save the new configuration
		Fluxbox::instance()->save_rc();
		update();
	}
}


void Slitmenu::internal_hide() {
	Basemenu::internal_hide();
	if (slit.doAutoHide())
		slit.timeout();
}


void Slitmenu::reconfigure() {
	m_directionmenu.reconfigure();
	m_placementmenu.reconfigure();
#ifdef XINERAMA
	if (m_headmenu.get() != 0) {
		m_headmenu->reconfigure();
	}
#endif // XINERAMA
	setItemSelected(2, slit.isOnTop());
	setItemSelected(3, slit.doAutoHide());

	Basemenu::reconfigure();
}


Slitmenu::Directionmenu::Directionmenu(Slitmenu &sm) : Basemenu(sm.screen()),
slitmenu(sm) {

	I18n *i18n = I18n::instance();
	using namespace FBNLS;	
	setLabel(i18n->getMessage(
		SlitSet, SlitSlitDirection,
		"Slit Direction"));
	setInternalMenu();

	insert(i18n->getMessage(
		CommonSet, CommonDirectionHoriz,
		"Horizontal"),
	 Slit::HORIZONTAL);
	insert(i18n->getMessage(
		CommonSet, CommonDirectionVert,
		"Vertical"),
	 Slit::VERTICAL);

	update();

	if (screen()->getSlitDirection() == Slit::HORIZONTAL)
		setItemSelected(0, true);
	else
		setItemSelected(1, true);
}


void Slitmenu::Directionmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (item == 0)
			return;

		screen()->saveSlitDirection(item->function());

		if (item->function() == Slit::HORIZONTAL) {
			setItemSelected(0, true);
			setItemSelected(1, false);
		} else {
			setItemSelected(0, false);
			setItemSelected(1, true);
		}
		Fluxbox::instance()->save_rc();
		hide();		
		slitmenu.slit.reconfigure();
	}
}


Slitmenu::Placementmenu::Placementmenu(Slitmenu &sm) : Basemenu(sm.screen()),
slitmenu(sm) {

	I18n *i18n = I18n::instance();
	using namespace FBNLS;	
	setLabel(i18n->getMessage(
		SlitSet, SlitSlitPlacement,
		"Slit Placement"));
	setMinimumSublevels(3);
	setInternalMenu();

	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopLeft,
		"Top Left"),
	 Slit::TOPLEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementCenterLeft,
		"Center Left"),
	 Slit::CENTERLEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomLeft,
		"Bottom Left"),
	 Slit::BOTTOMLEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopCenter,
		"Top Center"),
	 Slit::TOPCENTER);
	insert("");
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomCenter,
		"Bottom Center"),
	 Slit::BOTTOMCENTER);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopRight,
		"Top Right"),
	 Slit::TOPRIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementCenterRight,
		"Center Right"),
	 Slit::CENTERRIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomRight,
		"Bottom Right"),
	 Slit::BOTTOMRIGHT);

	update();
}


void Slitmenu::Placementmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		if (item->function()) {
			screen()->saveSlitPlacement(item->function());
			hide();
			slitmenu.slit.reconfigure();
			Fluxbox::instance()->save_rc();
		}
	}
}

#ifdef XINERAMA

Slitmenu::Headmenu::Headmenu(Slitmenu &sm): Basemenu(sm.screen()),
slitmenu(sm) {

	I18n *i18n = I18n::instance();

	setLabel(i18n->getMessage(0, 0, "Place on Head")); //TODO: NLS
	setInternalMenu();

	int numHeads = screen()->getNumHeads();
	// fill menu with head entries
	for (int i = 0; i < numHeads; i++) {
		char headName[32];
		sprintf(headName, "Head %i", i+1); //TODO: NLS
		insert(i18n->getMessage(0, 0, headName), i);
	}

	update();
}

void Slitmenu::Headmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item)
			return;

		screen()->saveSlitOnHead(item->function());
		hide();
		slitmenu.slit.reconfigure();
		Fluxbox::instance()->save_rc();
	}
}

#endif // XINERAMA

Slit::SlitClient::SlitClient(const char *name) {
	initialize();
	match_name = (name == 0 ? "" : name);
}

Slit::SlitClient::SlitClient(BScreen *screen, Window w) {
	initialize(screen, w);
}

void Slit::SlitClient::initialize(BScreen *screen, Window w) {
	client_window = w;
	window = icon_window = None;
	x = y = 0;
	width = height = 0;
	if (match_name.size() == 0)
		getWMName(screen, client_window, match_name);
}

#endif // SLIT
