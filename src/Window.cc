// Window.cc for fluxbox
// Copyright (c) 2001	Henrik Kinnunen (fluxgen@linuxmail.org)

// Window.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Icon.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "StringUtil.hh"

#ifdef		SLIT
#	include "Slit.hh"
#endif // SLIT

#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef		STDC_HEADERS
#	include <string.h>
#endif // STDC_HEADERS

#ifdef		DEBUG
#	ifdef		HAVE_STDIO_H
#		include <stdio.h>
#	endif // HAVE_STDIO_H
#endif // DEBUG


#include <iostream>
using namespace std;

FluxboxWindow::FluxboxWindow(Window w, BScreen *s) {

#ifdef		DEBUG
	fprintf(stderr,
		I18n::instance()->
		getMessage(
#	ifdef		NLS
				WindowSet, WindowCreating,
#	else // !NLS
				0, 0,
#	endif // NLS
				"FluxboxWindow::FluxboxWindow(): creating 0x%lx\n"),
		w);
#endif // DEBUG

	Fluxbox *fluxbox = Fluxbox::instance();
	display = fluxbox->getXDisplay();
	
	moving = resizing = shaded = maximized = visible = iconic = false;
	transient = focused = stuck = modal =	send_focus_message = managed = false;

	blackbox_attrib.workspace = workspace_number = window_number = -1;

	blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack = 0l;
	blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
	blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;
	//use tab as default
	decorations.tab = true;
	client.window = w;
	tab = 0;
	frame.window = frame.plate = frame.title = frame.handle = None;
	frame.right_grip = frame.left_grip = None;

	frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
	frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
	frame.pbutton = frame.ugrip = frame.fgrip = None;
	
	//get decorations
	vector<Fluxbox::Titlebar> dir = fluxbox->getTitlebarLeft();
	for (char c=0; c<2; c++) {
		for (unsigned int i=0; i<dir.size(); i++) {
			switch (dir[i]) {
				case Fluxbox::Shade:
					decorations.shade = true;
					break;
				case Fluxbox::Maximize:
					decorations.maximize = true;	
					break;
				case Fluxbox::Minimize:
					decorations.iconify = true;			
				break;
				case Fluxbox::Stick:
					decorations.sticky = true;
					break;
				case Fluxbox::Close:
					decorations.close = true;
					break;
				case Fluxbox::Menu:
					decorations.menu = true;
					break;
				default:
					break;
			}
		}
		//next right
		dir = fluxbox->getTitlebarRight();
	}
	
	decorations.menu = true;	//override menu option
	
	decorations.titlebar = decorations.border = decorations.handle = true;
	functions.resize = functions.move = functions.iconify = functions.maximize = true;
	functions.close = decorations.close = false;

	client.wm_hint_flags = client.normal_hint_flags = 0;
	client.transient_for = client.transient = 0;
	client.title = 0;
	client.title_len = 0;
	client.icon_title = 0;
	client.mwm_hint = (MwmHints *) 0;
	client.blackbox_hint = 0;

	windowmenu = 0;
	lastButtonPressTime = 0;
	timer = 0;
	screen = 0;
	image_ctrl = 0;

	fluxbox->grab();
	if (! validateClient()) 
		return;

	// fetch client size and placement
	XWindowAttributes wattrib;
	if ((! XGetWindowAttributes(display, client.window, &wattrib)) ||
			(! wattrib.screen) || wattrib.override_redirect) {
#ifdef		DEBUG
		fprintf(stderr,
						I18n::instance()->
			getMessage(
#	ifdef		NLS
					 WindowSet, WindowXGetWindowAttributesFail,
#	else // !NLS
					 0, 0,
#	endif // NLS
					 "FluxboxWindow::FluxboxWindow(): XGetWindowAttributes "
					 "failed\n"));
#endif // DEBUG

		delete this;

		fluxbox->ungrab();
		return;
	}

	if (s)
		screen = s;
	else
		screen = fluxbox->searchScreen(RootWindowOfScreen(wattrib.screen));

	if (! screen) {
#ifdef		DEBUG
		fprintf(stderr,
			I18n::instance()->
			getMessage(
#	ifdef		NLS
					WindowSet, WindowCannotFindScreen,
#	else // !NLS
					0, 0,
#	endif // NLS
					"FluxboxWindow::FluxboxWindow(): can't find screen\n"
					"	for root window 0x%lx\n"),
			RootWindowOfScreen(wattrib.screen));
#endif // DEBUG
		delete this;

		fluxbox->ungrab();
		return;
	}

	image_ctrl = screen->getImageControl();

	client.x = wattrib.x;
	client.y = wattrib.y;
	client.width = wattrib.width;
	client.height = wattrib.height;
	client.old_bw = wattrib.border_width;

	timer = new BTimer(fluxbox, this);
	timer->setTimeout(fluxbox->getAutoRaiseDelay());
	timer->fireOnce(true);

	getBlackboxHints();
	if (! client.blackbox_hint)
		getMWMHints();

	// get size, aspect, minimum/maximum size and other hints set by the
	// client
	getWMProtocols();
	getWMHints();
	getWMNormalHints();

#ifdef		SLIT
	if (client.initial_state == WithdrawnState) {
		screen->getSlit()->addClient(client.window);
		delete this;

		fluxbox->ungrab();
		return;
	}
#endif // SLIT

	managed = true;
	fluxbox->saveWindowSearch(client.window, this);

	// determine if this is a transient window
	Window win;
	if (XGetTransientForHint(display, client.window, &win)) {	
		if (win && (win != client.window)) {
			FluxboxWindow *tr;
			if ((tr = fluxbox->searchWindow(win))) {
				
				while (tr->client.transient) 
					tr = tr->client.transient;
				
				client.transient_for = tr;
				tr->client.transient = this;
				stuck = client.transient_for->stuck;
				transient = true;
			} else if (win == client.window_group) {
				if ((tr = fluxbox->searchGroup(win, this))) {
					
					while (tr->client.transient) 
						tr = tr->client.transient;
					
					client.transient_for = tr;
					tr->client.transient = this;
					stuck = client.transient_for->stuck;
					transient = true;
				}
			}
		}

		if (win == screen->getRootWindow()) modal = true;
	}

	// adjust the window decorations based on transience and window sizes
	if (transient) {
		decorations.maximize = decorations.handle =
			decorations.border = functions.maximize = false;
	}	
	
	if ((client.normal_hint_flags & PMinSize) &&
			(client.normal_hint_flags & PMaxSize) &&
			client.max_width <= client.min_width &&
			client.max_height <= client.min_height) {
		decorations.maximize = decorations.handle =
			functions.resize = functions.maximize = false;
			decorations.tab = false; //no tab for this window
	}

	upsize();

	bool place_window = true;
	if (fluxbox->isStartup() || transient ||
			client.normal_hint_flags & (PPosition|USPosition)) {
		setGravityOffsets();

		if (! fluxbox->isStartup()) { // is going to be used when position
			if (decorations.tab) {			// window is cleanly fixed 
				int real_x = frame.x;
				int real_y = frame.y;

				if (screen->getTabPlacement() == Tab::PTOP)
					real_y -= screen->getTabHeight();

				else if (screen->getTabPlacement() == Tab::PLEFT) {
					if (screen->isTabRotateVertical())
						real_x -= screen->getTabHeight();
					else
						real_x -= screen->getTabWidth();
				}

				if (real_x >= 0 && 
						real_y + frame.y_border >= 0 &&
						real_x <= (signed) screen->getWidth() &&
						real_y <= (signed) screen->getHeight())
					place_window = false;

			} else if (frame.x >= 0 && // non tab
					(signed) (frame.y + frame.y_border) >= 0 &&
					frame.x <= (signed) screen->getWidth() &&
					frame.y <= (signed) screen->getHeight())
				place_window = false;
		} else
			place_window = false;

/*		if ((fluxbox->isStartup()) ||
				(frame.x >= 0 &&
				(signed) (frame.y + frame.y_border) >= 0 &&
				frame.x <= (signed) screen->getWidth() &&
				frame.y <= (signed) screen->getHeight()))
			place_window = false; */
			
	}

	frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
							frame.height,
							screen->getBorderWidth());	//create frame window
	
	fluxbox->saveWindowSearch(frame.window, this);	//save frame window

	frame.plate = createChildWindow(frame.window); //Create plate window
	fluxbox->saveWindowSearch(frame.plate, this);	//save plate window

	if (decorations.titlebar) {	//have titlebar decorations?
		frame.title = createChildWindow(frame.window); //create titlebar win
		fluxbox->saveWindowSearch(frame.title, this);	//save titlebar win
		frame.label = createChildWindow(frame.title); //create label win in titlebar
		fluxbox->saveWindowSearch(frame.label, this);	//save label win
	}

	if (decorations.handle) {	//have handle decorations ?
		frame.handle = createChildWindow(frame.window); //create handle win
		fluxbox->saveWindowSearch(frame.handle, this);	//save handle win

		frame.left_grip = // create left handle
			createChildWindow(frame.handle, fluxbox->getLowerLeftAngleCursor());
		fluxbox->saveWindowSearch(frame.left_grip, this); //save left handle

		frame.right_grip = // create right handle
			createChildWindow(frame.handle, fluxbox->getLowerRightAngleCursor());
		fluxbox->saveWindowSearch(frame.right_grip, this); //save right handle
	}

	associateClientWindow();

	if (! (screen->isSloppyFocus() || screen->isSemiSloppyFocus())) {
		XGrabButton(display, Button1, AnyModifier, 
				frame.plate, True, ButtonPressMask,
				GrabModeSync, GrabModeSync, None, None);		
		XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask, frame.plate);
		
	} else 
		XUngrabButton(display, Button1, AnyModifier, frame.plate);
	
	XGrabButton(display, Button1, Mod1Mask, frame.window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, fluxbox->getMoveCursor());

	//----grab with "all" modifiers
	grabButton(display, Button1, frame.window, fluxbox->getMoveCursor());
	
	XGrabButton(display, Button2, Mod1Mask, frame.window, True,
				ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);	
		
	XGrabButton(display, Button3, Mod1Mask, frame.window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, fluxbox->getLowerRightAngleCursor());
	
	//---grab with "all" modifiers
	grabButton(display, Button3, frame.window, fluxbox->getLowerRightAngleCursor());
				
	positionWindows();

	//use tab? delayed this so that tabs wont "flicker" when creating windows
	if (decorations.tab && fluxbox->useTabs())
		tab = new Tab(this, 0, 0);

	XRaiseWindow(display, frame.plate);
	XMapSubwindows(display, frame.plate);
	if (decorations.titlebar)
		XMapSubwindows(display, frame.title);
	
	XMapSubwindows(display, frame.window);

	if (decorations.menu)
		windowmenu = new Windowmenu(this);

	decorate();

	if (workspace_number < 0 || workspace_number >= screen->getCount())
		screen->getCurrentWorkspace()->addWindow(this, place_window);
	else
		screen->getWorkspace(workspace_number)->addWindow(this, place_window);

	configure(frame.x, frame.y, frame.width, frame.height);

	if (shaded) {
		shaded = false;
		shade();
	}

	if (maximized && functions.maximize) {
		int m = maximized;
		maximized = false;

		maximize(m);
	}

	setFocusFlag(false);
/*
#ifdef GNOME
	//tell the creator of this window that we are a gnome compilant windowmanager
	{
	XChangeProperty(display, screen->getRootWindow(),
			screen->getBaseDisplay()->getGnomeSupportingWMCheckAtom(),
			XA_CARDINAL, 32,
      PropModeReplace, (unsigned char *)&client.window, 1);	
  XChangeProperty(display, client.window, 
			screen->getBaseDisplay()->getGnomeSupportingWMCheckAtom(),
			XA_CARDINAL, 32, PropModeReplace,
		 	(unsigned char *)&client.window, 1);
	}
#endif
*/
	fluxbox->ungrab();
	#ifdef DEBUG
	fprintf(stderr, "%s(%d): FluxboxWindow(this=%p)\n", __FILE__, __LINE__, this);
	#endif
}


FluxboxWindow::~FluxboxWindow(void) {
	Fluxbox *fluxbox = Fluxbox::instance();
	
	
	if (moving || resizing) {
		screen->hideGeometry();
		XUngrabPointer(display, CurrentTime);
	}
	
	if (workspace_number != -1 && window_number != -1)
		screen->getWorkspace(workspace_number)->removeWindow(this);
	else if (iconic)
		screen->removeIcon(this);
	
	if (timer) {
		if (timer->isTiming()) timer->stop();
		delete timer;
	}

	if (windowmenu)
		delete windowmenu;

	if (client.title)
		delete [] client.title;

	if (client.icon_title)
		delete [] client.icon_title;

	if (tab)
		delete tab;	
	
	if (client.mwm_hint)
		XFree(client.mwm_hint);

	if (client.blackbox_hint)
		XFree(client.blackbox_hint);		

	
	if (client.window_group)
		fluxbox->removeGroupSearch(client.window_group);

	if (transient && client.transient_for)
		client.transient_for->client.transient = client.transient;
	if (client.transient)
		client.transient->client.transient_for = client.transient_for;
	
	while ( !buttonlist.empty()) {	//destroy all buttons on titlebar
		fluxbox->removeWindowSearch(buttonlist.back().win);
		XDestroyWindow(display, buttonlist.back().win);
		buttonlist.pop_back();
	}
 
	if (frame.title) {
		if (frame.ftitle)
			image_ctrl->removeImage(frame.ftitle);

		if (frame.utitle)
			image_ctrl->removeImage(frame.utitle);

		if (frame.flabel)
			image_ctrl->removeImage(frame.flabel);

		if( frame.ulabel)
			image_ctrl->removeImage(frame.ulabel);

		fluxbox->removeWindowSearch(frame.label);
		fluxbox->removeWindowSearch(frame.title);
		XDestroyWindow(display, frame.label);
		XDestroyWindow(display, frame.title);
	}

	if (frame.handle) {
		if (frame.fhandle)
			image_ctrl->removeImage(frame.fhandle);

		if (frame.uhandle)
			image_ctrl->removeImage(frame.uhandle);

		if (frame.fgrip)
			image_ctrl->removeImage(frame.fgrip);

		if (frame.ugrip)
			image_ctrl->removeImage(frame.ugrip);

		fluxbox->removeWindowSearch(frame.handle);
		fluxbox->removeWindowSearch(frame.right_grip);
		fluxbox->removeWindowSearch(frame.left_grip);
		XDestroyWindow(display, frame.right_grip);
		XDestroyWindow(display, frame.left_grip);
		XDestroyWindow(display, frame.handle);
	}

	if (frame.fbutton)
		image_ctrl->removeImage(frame.fbutton);

	if (frame.ubutton)
		image_ctrl->removeImage(frame.ubutton);

	if (frame.pbutton)
		image_ctrl->removeImage(frame.pbutton);

	
	if (frame.plate) {		//NOTE 
		fluxbox->removeWindowSearch(frame.plate);
		XDestroyWindow(display, frame.plate);
	}

	if (frame.window) {
		fluxbox->removeWindowSearch(frame.window);
		XDestroyWindow(display, frame.window);
	}

	if (managed) {
		fluxbox->removeWindowSearch(client.window);
		screen->removeNetizen(client.window);
	}
	//TODO: Move this to Workspace::removeWindow
	if (client.transient_for)	
		fluxbox->setFocusedWindow(client.transient_for);	
	#ifdef DEBUG	
	fprintf(stderr, "%s(%d): ~FluxboxWindow(this=%p) done\n", __FILE__, __LINE__, this);
	#endif
}


Window FluxboxWindow::createToplevelWindow(int x, int y, unsigned int width,
							unsigned int height,
							unsigned int borderwidth)
{
	XSetWindowAttributes attrib_create;
	unsigned long create_mask = CWBackPixmap | CWBorderPixel | CWColormap |
															CWOverrideRedirect | CWEventMask;

	attrib_create.background_pixmap = None;
	attrib_create.colormap = screen->getColormap();
	attrib_create.override_redirect = True;
	attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
														 ButtonMotionMask | EnterWindowMask;

	return (XCreateWindow(display, screen->getRootWindow(), x, y, width, height,
			borderwidth, screen->getDepth(), InputOutput,
			screen->getVisual(), create_mask,
			&attrib_create));
}


Window FluxboxWindow::createChildWindow(Window parent, Cursor cursor) {
	XSetWindowAttributes attrib_create;
	unsigned long create_mask = CWBackPixmap | CWBorderPixel |
															CWEventMask;

	attrib_create.background_pixmap = None;
	attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
														 ButtonMotionMask | ExposureMask |
														 EnterWindowMask | LeaveWindowMask;

	if (cursor) {
		create_mask |= CWCursor;
		attrib_create.cursor = cursor;
	}

	return (XCreateWindow(display, parent, 0, 0, 1, 1, 0,
			screen->getDepth(), InputOutput, screen->getVisual(),
			create_mask, &attrib_create));
}


void FluxboxWindow::associateClientWindow(void) {
	XSetWindowBorderWidth(display, client.window, 0);
	getWMName();
	getWMIconName();

	XChangeSaveSet(display, client.window, SetModeInsert);
	XSetWindowAttributes attrib_set;

	XSelectInput(display, frame.plate, NoEventMask);
	XReparentWindow(display, client.window, frame.plate, 0, 0);
	XSelectInput(display, frame.plate, SubstructureRedirectMask);

	XFlush(display);

	attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask |
													FocusChangeMask;
	attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
																		 ButtonMotionMask;

	XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate,
													&attrib_set);

#ifdef		SHAPE
	if (Fluxbox::instance()->hasShapeExtensions()) {
		XShapeSelectInput(display, client.window, ShapeNotifyMask);

		int foo;
		unsigned int ufoo;

		XShapeQueryExtents(display, client.window, &frame.shaped, &foo, &foo,
							&ufoo, &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);

		if (frame.shaped) {
			XShapeCombineShape(display, frame.window, ShapeBounding,
						frame.mwm_border_w, frame.y_border +
						frame.mwm_border_w, client.window,
						ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
					xrect, num, ShapeUnion, Unsorted);
		}
	}
#endif // SHAPE
	//create the buttons
	if (decorations.iconify) 
		createButton(Fluxbox::Minimize, FluxboxWindow::iconifyPressed_cb, FluxboxWindow::iconifyButton_cb, FluxboxWindow::iconifyDraw_cb);			
	if (decorations.maximize)
		createButton(Fluxbox::Maximize, FluxboxWindow::maximizePressed_cb, FluxboxWindow::maximizeButton_cb, 
				FluxboxWindow::maximizeDraw_cb);
	if (decorations.close) 
		createButton(Fluxbox::Close, FluxboxWindow::closePressed_cb, 
				FluxboxWindow::closeButton_cb, FluxboxWindow::closeDraw_cb);		
	if (decorations.sticky)
		createButton(Fluxbox::Stick, FluxboxWindow::stickyPressed_cb, 
				FluxboxWindow::stickyButton_cb, FluxboxWindow::stickyDraw_cb);

	if (decorations.menu)//TODO
		createButton(Fluxbox::Menu, 0, 0, 0);

	if (decorations.shade)
		createButton(Fluxbox::Shade, 0, FluxboxWindow::shadeButton_cb, FluxboxWindow::shadeDraw_cb);

	if (frame.ubutton) {
		for (unsigned int i=0; i<buttonlist.size(); i++)
			XSetWindowBackgroundPixmap(display, buttonlist[i].win, frame.ubutton);	 
	
	} else {
		for (unsigned int i=0; i<buttonlist.size(); i++)
			XSetWindowBackground(display, buttonlist[i].win, frame.ubutton_pixel);	 
	}
}


void FluxboxWindow::decorate(void) {
		
	if (tab)
		tab->decorate();

	Pixmap tmp = frame.fbutton;
	BTexture *texture = &(screen->getWindowStyle()->b_focus);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.fbutton = None;
		frame.fbutton_pixel = texture->getColor()->getPixel();
	} else
		frame.fbutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.ubutton;
	texture = &(screen->getWindowStyle()->b_unfocus);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.ubutton = None;
		frame.ubutton_pixel = texture->getColor()->getPixel();
	} else
		frame.ubutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.pbutton;
	texture = &(screen->getWindowStyle()->b_pressed);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.pbutton = None;
		frame.pbutton_pixel = texture->getColor()->getPixel();
	} else
		frame.pbutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	if (decorations.titlebar) {
		tmp = frame.ftitle;
		texture = &(screen->getWindowStyle()->t_focus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.ftitle = None;
			frame.ftitle_pixel = texture->getColor()->getPixel();
		} else
			frame.ftitle =
				image_ctrl->renderImage(frame.width, frame.title_h, texture);
		
		if (tmp) 
			image_ctrl->removeImage(tmp);

		tmp = frame.utitle;
		texture = &(screen->getWindowStyle()->t_unfocus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.utitle = None;
			frame.utitle_pixel = texture->getColor()->getPixel();
		} else
			frame.utitle =
				image_ctrl->renderImage(frame.width, frame.title_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		XSetWindowBorder(display, frame.title,
										 screen->getBorderColor()->getPixel());

		decorateLabel();
		
	}

	if (decorations.border) {
		frame.fborder_pixel = screen->getWindowStyle()->f_focus.getPixel();
		frame.uborder_pixel = screen->getWindowStyle()->f_unfocus.getPixel();
	}

	if (decorations.handle) {
		tmp = frame.fhandle;
		texture = &(screen->getWindowStyle()->h_focus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.fhandle = None;
			frame.fhandle_pixel = texture->getColor()->getPixel();
		} else
			frame.fhandle =
				image_ctrl->renderImage(frame.width, frame.handle_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		tmp = frame.uhandle;
		texture = &(screen->getWindowStyle()->h_unfocus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.uhandle = None;
			frame.uhandle_pixel = texture->getColor()->getPixel();
		} else
			frame.uhandle =
				image_ctrl->renderImage(frame.width, frame.handle_h, texture);
		if (tmp)
			image_ctrl->removeImage(tmp);

		tmp = frame.fgrip;
		texture = &(screen->getWindowStyle()->g_focus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.fgrip = None;
			frame.fgrip_pixel = texture->getColor()->getPixel();
		} else
			frame.fgrip =
				image_ctrl->renderImage(frame.grip_w, frame.grip_h, texture);
		if (tmp)
			image_ctrl->removeImage(tmp);

		tmp = frame.ugrip;
		texture = &(screen->getWindowStyle()->g_unfocus);
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			frame.ugrip = None;
			frame.ugrip_pixel = texture->getColor()->getPixel();
		} else
			frame.ugrip =
				image_ctrl->renderImage(frame.grip_w, frame.grip_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		XSetWindowBorder(display, frame.handle,
										 screen->getBorderColor()->getPixel());
		XSetWindowBorder(display, frame.left_grip,
										 screen->getBorderColor()->getPixel());
		XSetWindowBorder(display, frame.right_grip,
										 screen->getBorderColor()->getPixel());
	}
		
	XSetWindowBorder(display, frame.window,
									 screen->getBorderColor()->getPixel());
}


void FluxboxWindow::decorateLabel(void) {
	Pixmap tmp = frame.flabel;
	BTexture *texture = &(screen->getWindowStyle()->l_focus);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.flabel = None;
		frame.flabel_pixel = texture->getColor()->getPixel();		
	} else
		frame.flabel = image_ctrl->renderImage(frame.label_w, frame.label_h, texture);
			
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.ulabel;
	texture = &(screen->getWindowStyle()->l_unfocus);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.ulabel = None;
		frame.ulabel_pixel = texture->getColor()->getPixel();
	} else
		frame.ulabel = image_ctrl->renderImage(frame.label_w, frame.label_h, texture);
		
	if (tmp) image_ctrl->removeImage(tmp);
}

void FluxboxWindow::createButton(int type, ButtonEventProc pressed, ButtonEventProc released, ButtonDrawProc draw) {
	Button b;
	b.win = createChildWindow(frame.title);
	Fluxbox::instance()->saveWindowSearch(b.win, this);
	b.type = type;
	b.used = true;
	b.draw = draw;
	b.pressed = pressed;
	b.released = released;
	buttonlist.push_back(b);
}

Window FluxboxWindow::findTitleButton(int type) {
	for (unsigned int i=0; i<buttonlist.size(); i++)
		if (buttonlist[i].type == type)
			return buttonlist[i].win;

	return 0;
}
void FluxboxWindow::stickyButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->stick();
	FluxboxWindow::stickyDraw_cb(t, be->window, false);
}

void FluxboxWindow::stickyPressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::stickyDraw_cb(t, be->window, true);
}

void FluxboxWindow::iconifyButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->iconify();
	FluxboxWindow::iconifyDraw_cb(t, be->window, false);
}

void FluxboxWindow::iconifyPressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->screen->getWorkspace(t->workspace_number)->lowerWindow(t);
	FluxboxWindow::iconifyDraw_cb(t, be->window, true);
}

void FluxboxWindow::maximizeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
 FluxboxWindow::maximizeDraw_cb(t, be->window, false);
 t->maximize(be->button); 
}

void FluxboxWindow::maximizePressed_cb(FluxboxWindow *t, XButtonEvent *be) {
 FluxboxWindow::maximizeDraw_cb(t, be->window, true); 
}

void FluxboxWindow::closeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->close();
	FluxboxWindow::closeDraw_cb(t, be->window, false);
}

void FluxboxWindow::closePressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::closeDraw_cb(t, be->window, true);
}

void FluxboxWindow::shadeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::shadeDraw_cb(t, be->window, false);
	t->shade();
	if (t->tab)
		t->tab->shade();
}

void FluxboxWindow::stickyDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	if (t->stuck) {
		XFillRectangle(t->display, w, 
								((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
					t->screen->getWindowStyle()->b_pic_unfocus_gc),
					t->frame.button_w/2-t->frame.button_w/4, t->frame.button_h/2-t->frame.button_h/4,
					t->frame.button_w/2, t->frame.button_h/2);
	} else {
		XFillRectangle(t->display, w, 
								((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
					t->screen->getWindowStyle()->b_pic_unfocus_gc),
					t->frame.button_w/2, t->frame.button_h/2,
					t->frame.button_w/5, t->frame.button_h/5);
	}
}

void FluxboxWindow::iconifyDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawRectangle(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
			t->screen->getWindowStyle()->b_pic_unfocus_gc),
			2, t->frame.button_h - 5, t->frame.button_w - 5, 2);

}

void FluxboxWindow::maximizeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawRectangle(t->display, w,
		((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
			t->screen->getWindowStyle()->b_pic_unfocus_gc),
			2, 2, t->frame.button_w - 5, t->frame.button_h - 5);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc),
			2, 3, t->frame.button_w - 3, 3);
}

void FluxboxWindow::closeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc), 2, 2,
				t->frame.button_w - 3, t->frame.button_h - 3);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc), 2,
				t->frame.button_h - 3,
				t->frame.button_w - 3, 2);
}

void FluxboxWindow::shadeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
}

void FluxboxWindow::grabButton(Display *display, unsigned int button, 
				Window window, Cursor cursor) {

	//numlock
	XGrabButton(display, button, Mod1Mask|Mod2Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	//scrolllock
	XGrabButton(display, button, Mod1Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
	//capslock
	XGrabButton(display, button, Mod1Mask|LockMask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//capslock+numlock
	XGrabButton(display, Button1, Mod1Mask|LockMask|Mod2Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//capslock+scrolllock
	XGrabButton(display, button, Mod1Mask|LockMask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
	//capslock+numlock+scrolllock
	XGrabButton(display, button, Mod1Mask|LockMask|Mod2Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//numlock+scrollLock
	XGrabButton(display, button, Mod1Mask|Mod2Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
}

void FluxboxWindow::drawButtonBase(Window w, bool pressed) {
	if (! pressed) {
		if (focused) {
			if (frame.fbutton)
				XSetWindowBackgroundPixmap(display, w, frame.fbutton);
			else
				XSetWindowBackground(display, w, frame.fbutton_pixel);
		} else {
			if (frame.ubutton)
				XSetWindowBackgroundPixmap(display, w, frame.ubutton);
			else
				XSetWindowBackground(display, w, frame.ubutton_pixel);
		}
	} else {
		if (frame.pbutton)
			XSetWindowBackgroundPixmap(display, w, frame.pbutton);
		else
			XSetWindowBackground(display, w, frame.pbutton_pixel);
	}
	XClearWindow(display, w);

}

void FluxboxWindow::positionButtons(bool redecorate_label) {
	unsigned int bw = frame.button_w + frame.bevel_w + 1,
		by = frame.bevel_w + 1, lx = by, lw = frame.width - by;	
	Fluxbox *fluxbox = Fluxbox::instance();
	vector<Fluxbox::Titlebar> left = fluxbox->getTitlebarLeft();
	vector<Fluxbox::Titlebar> right = fluxbox->getTitlebarRight();

	//left side buttons
	for (unsigned int i=0; i<left.size(); i++) {
		Window w = findTitleButton(left[i]);	//get Window of button
		if (w) {
			XMoveResizeWindow(display, w, lx, by,
					frame.button_w, frame.button_h);
			XMapWindow(display, w);
			XClearWindow(display, w);
			lx += bw;
			lw -= bw;
		}		
	}
	
	int bx = frame.width - bw;

	//right side buttons
	for (int i=right.size()-1; i>=0; i--) {
		Window w = findTitleButton(right[i]);	//get window of button
		if (w) {
			XMoveResizeWindow(display, w, bx, by,
					frame.button_w, frame.button_h);
			XMapWindow(display, w);
			XClearWindow(display, w);
			bx -= bw;
			lw -= bw;
		}
	}
	
	//Draw the label
	frame.label_w = lw - by;
	XMoveResizeWindow(display, frame.label, lx, frame.bevel_w,
										frame.label_w, frame.label_h);
	if (redecorate_label) 
		decorateLabel();
	if (tab) {
		tab->setPosition();
		tab->draw(false);
	}
	redrawLabel();
	redrawAllButtons();
}


void FluxboxWindow::reconfigure(void) {
	upsize();
	
	if (Fluxbox::instance()->useTabs()) {
		//no tab and we allowed to use tab? then create it 
		if (!tab && decorations.tab) {			
			tab = new Tab(this, 0, 0);		
			if (current_state == IconicState)
				tab->iconify();
			else if (current_state == WithdrawnState)
				tab->withdraw();
		}
	}	else {		
		if (tab) { //got a tab? then destroy it
			delete tab;
			tab=0;
		}
	}
	
	client.x = frame.x + frame.mwm_border_w + screen->getBorderWidth();
	client.y = frame.y + frame.y_border + frame.mwm_border_w +
			 screen->getBorderWidth();

	if (client.title) {
		if (I18n::instance()->multibyte()) {
			XRectangle ink, logical;
			XmbTextExtents(screen->getWindowStyle()->font.set,
					client.title, client.title_len, &ink, &logical);
			client.title_text_w = logical.width;
		} else {
			client.title_text_w = XTextWidth(screen->getWindowStyle()->font.fontstruct,
					client.title, client.title_len);
		}

		client.title_text_w += (frame.bevel_w * 4);
	}

		
	positionWindows();
	decorate();

				
	XClearWindow(display, frame.window);
	setFocusFlag(focused);

	configure(frame.x, frame.y, frame.width, frame.height);

	if (! (screen->isSloppyFocus() || screen->isSemiSloppyFocus())) {
		XGrabButton(display, Button1, AnyModifier, frame.plate, True, ButtonPressMask,
				GrabModeSync, GrabModeSync, None, None);
		XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask, frame.plate);
	} else		
		XUngrabButton(display, Button1, AnyModifier, frame.plate);

	if (windowmenu) {
		windowmenu->move(windowmenu->getX(), frame.y + frame.title_h);
		windowmenu->reconfigure();
	}
		
	
}


void FluxboxWindow::positionWindows(void) {
	XResizeWindow(display, frame.window, frame.width,
							((shaded) ? frame.title_h : frame.height));
	XSetWindowBorderWidth(display, frame.window, screen->getBorderWidth());
	XSetWindowBorderWidth(display, frame.plate, screen->getFrameWidth() *
			decorations.border);
	XMoveResizeWindow(display, frame.plate, 0, frame.y_border,
								client.width, client.height);
	XMoveResizeWindow(display, client.window, 0, 0, client.width, client.height);

	if (decorations.titlebar) {
		XSetWindowBorderWidth(display, frame.title, screen->getBorderWidth());
		XMoveResizeWindow(display, frame.title, -screen->getBorderWidth(),
					-screen->getBorderWidth(), frame.width, frame.title_h);

		positionButtons();
	} else if (frame.title)
		XUnmapWindow(display, frame.title);

	if (decorations.handle) {
		XSetWindowBorderWidth(display, frame.handle, screen->getBorderWidth());
		XSetWindowBorderWidth(display, frame.left_grip, screen->getBorderWidth());
		XSetWindowBorderWidth(display, frame.right_grip, screen->getBorderWidth());

		XMoveResizeWindow(display, frame.handle, -screen->getBorderWidth(),
											frame.y_handle - screen->getBorderWidth(),
								frame.width, frame.handle_h);
		XMoveResizeWindow(display, frame.left_grip, -screen->getBorderWidth(),
						-screen->getBorderWidth(), frame.grip_w, frame.grip_h);
		XMoveResizeWindow(display, frame.right_grip,
						frame.width - frame.grip_w - screen->getBorderWidth(),
						-screen->getBorderWidth(), frame.grip_w, frame.grip_h);
		XMapSubwindows(display, frame.handle);
	} else if (frame.handle)
		XUnmapWindow(display, frame.handle);
	if (tab)
		tab->setPosition();
}


void FluxboxWindow::getWMName(void) {
	if (client.title) {
		delete [] client.title;
		client.title = 0;
	}

	XTextProperty text_prop;
	char **list;
	int num;
	I18n *i18n = I18n::instance();
	
	if (XGetWMName(display, client.window, &text_prop)) {
		if (text_prop.value && text_prop.nitems > 0) {
			if (text_prop.encoding != XA_STRING) {
				
				text_prop.nitems = strlen((char *) text_prop.value);
				
				if ((XmbTextPropertyToTextList(display, &text_prop,
							&list, &num) == Success) &&
						(num > 0) && *list) {
					client.title = StringUtil::strdup(*list);
					XFreeStringList(list);
				} else
					client.title = StringUtil::strdup((char *) text_prop.value);
					
			} else
				client.title = StringUtil::strdup((char *) text_prop.value);

			XFree((char *) text_prop.value);
		} else
			client.title = StringUtil::strdup(i18n->getMessage(
#ifdef		NLS
									WindowSet, WindowUnnamed,
#else // !NLS
									0, 0,
#endif //
									"Unnamed"));
	} else {
		client.title = StringUtil::strdup(i18n->getMessage(
#ifdef		NLS
							WindowSet, WindowUnnamed,
#else // !NLS
							0, 0,
#endif //
							"Unnamed"));
	}
	
	client.title_len = strlen(client.title);

	if (i18n->multibyte()) {
		XRectangle ink, logical;
		XmbTextExtents(screen->getWindowStyle()->font.set,
				client.title, client.title_len, &ink, &logical);
		client.title_text_w = logical.width;
	} else {
		client.title_len = strlen(client.title);
		client.title_text_w = XTextWidth(screen->getWindowStyle()->font.fontstruct,
							client.title, client.title_len);
	}

	client.title_text_w += (frame.bevel_w * 4);
}


void FluxboxWindow::getWMIconName(void) {
	if (client.icon_title) {
		delete [] client.icon_title;
		client.icon_title = (char *) 0;
	}

	XTextProperty text_prop;
	char **list;
	int num;

	if (XGetWMIconName(display, client.window, &text_prop)) {
		if (text_prop.value && text_prop.nitems > 0) {
			if (text_prop.encoding != XA_STRING) {
				text_prop.nitems = strlen((char *) text_prop.value);

				if ((XmbTextPropertyToTextList(display, &text_prop,
						&list, &num) == Success) &&
						(num > 0) && *list) {
					client.icon_title = StringUtil::strdup(*list);
					XFreeStringList(list);
				} else
					client.icon_title = StringUtil::strdup((char *) text_prop.value);
			} else
				client.icon_title = StringUtil::strdup((char *) text_prop.value);

			XFree((char *) text_prop.value);
		} else
				client.icon_title = StringUtil::strdup(client.title);
	} else
		client.icon_title = StringUtil::strdup(client.title);
}


void FluxboxWindow::getWMProtocols(void) {
	Atom *proto;
	int num_return = 0;
	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (XGetWMProtocols(display, client.window, &proto, &num_return)) {
		for (int i = 0; i < num_return; ++i) {
			if (proto[i] == fluxbox->getWMDeleteAtom())
				functions.close = decorations.close = true;
			else if (proto[i] == fluxbox->getWMTakeFocusAtom())
				send_focus_message = true;
			else if (proto[i] == fluxbox->getFluxboxStructureMessagesAtom())
				screen->addNetizen(new Netizen(screen, client.window));
		}

		XFree(proto);
	}
}


void FluxboxWindow::getWMHints(void) {
	XWMHints *wmhint = XGetWMHints(display, client.window);
	if (! wmhint) {
		visible = true;
		iconic = false;
		focus_mode = F_Passive;
		client.window_group = None;
		client.initial_state = NormalState;
	} else {
		client.wm_hint_flags = wmhint->flags;
		if (wmhint->flags & InputHint) {
			if (wmhint->input == true) {
				if (send_focus_message)
					focus_mode = F_LocallyActive;
				else
					focus_mode = F_Passive;
			} else {
				if (send_focus_message)
					focus_mode = F_GloballyActive;
				else
					focus_mode = F_NoInput;
			}
		} else
			focus_mode = F_Passive;

		if (wmhint->flags & StateHint)
			client.initial_state = wmhint->initial_state;
		else
			client.initial_state = NormalState;

		if (wmhint->flags & WindowGroupHint) {
			if (! client.window_group) {
				client.window_group = wmhint->window_group;
				Fluxbox::instance()->saveGroupSearch(client.window_group, this);
			}
		} else
			client.window_group = None;

		XFree(wmhint);
	}
}


void FluxboxWindow::getWMNormalHints(void) {
	long icccm_mask;
	XSizeHints sizehint;
	if (! XGetWMNormalHints(display, client.window, &sizehint, &icccm_mask)) {
		client.min_width = client.min_height =
			client.base_width = client.base_height =
			client.width_inc = client.height_inc = 1;
		client.max_width = screen->getWidth();
		client.max_height = screen->getHeight();
		client.min_aspect_x = client.min_aspect_y =
			client.max_aspect_x = client.max_aspect_y = 1;
		client.win_gravity = NorthWestGravity;
	} else {
		client.normal_hint_flags = sizehint.flags;

		if (sizehint.flags & PMinSize) {
			client.min_width = sizehint.min_width;
			client.min_height = sizehint.min_height;
		} else
			client.min_width = client.min_height = 1;

		if (sizehint.flags & PMaxSize) {
			client.max_width = sizehint.max_width;
			client.max_height = sizehint.max_height;
		} else {
			client.max_width = screen->getWidth();
			client.max_height = screen->getHeight();
		}

		if (sizehint.flags & PResizeInc) {
			client.width_inc = sizehint.width_inc;
			client.height_inc = sizehint.height_inc;
		} else
			client.width_inc = client.height_inc = 1;

		if (sizehint.flags & PAspect) {
			client.min_aspect_x = sizehint.min_aspect.x;
			client.min_aspect_y = sizehint.min_aspect.y;
			client.max_aspect_x = sizehint.max_aspect.x;
			client.max_aspect_y = sizehint.max_aspect.y;
		} else
			client.min_aspect_x = client.min_aspect_y =
	client.max_aspect_x = client.max_aspect_y = 1;

		if (sizehint.flags & PBaseSize) {
			client.base_width = sizehint.base_width;
			client.base_height = sizehint.base_height;
		} else
			client.base_width = client.base_height = 0;

		if (sizehint.flags & PWinGravity)
			client.win_gravity = sizehint.win_gravity;
		else
			client.win_gravity = NorthWestGravity;
	}
}


void FluxboxWindow::getMWMHints(void) {
	int format;
	Atom atom_return;
	unsigned long num, len;
	Fluxbox *fluxbox = Fluxbox::instance();
	if (XGetWindowProperty(display, client.window,
												 fluxbox->getMotifWMHintsAtom(), 0,
												 PropMwmHintsElements, false,
												 fluxbox->getMotifWMHintsAtom(), &atom_return,
												 &format, &num, &len,
												 (unsigned char **) &client.mwm_hint) == Success &&
			client.mwm_hint)
		if (num == PropMwmHintsElements) {
			if (client.mwm_hint->flags & MwmHintsDecorations)
				if (client.mwm_hint->decorations & MwmDecorAll)
					decorations.titlebar = decorations.handle = decorations.border =
						decorations.iconify = decorations.maximize =
						decorations.close = decorations.menu = true;
				else {
					decorations.titlebar = decorations.handle = decorations.border =
						decorations.iconify = decorations.maximize =
						decorations.close = decorations.menu = false;

					if (client.mwm_hint->decorations & MwmDecorBorder)
						decorations.border = true;
					if (client.mwm_hint->decorations & MwmDecorHandle)
						decorations.handle = true;
					if (client.mwm_hint->decorations & MwmDecorTitle)
						decorations.titlebar = true;
					if (client.mwm_hint->decorations & MwmDecorMenu)
						decorations.menu = true;
					if (client.mwm_hint->decorations & MwmDecorIconify)
						decorations.iconify = true;
					if (client.mwm_hint->decorations & MwmDecorMaximize)
						decorations.maximize = true;
				}

			if (client.mwm_hint->flags & MwmHintsFunctions)
				if (client.mwm_hint->functions & MwmFuncAll)
					functions.resize = functions.move = functions.iconify =
						functions.maximize = functions.close = true;
				else {
					functions.resize = functions.move = functions.iconify =
						functions.maximize = functions.close = false;

					if (client.mwm_hint->functions & MwmFuncResize)
						functions.resize = true;
					if (client.mwm_hint->functions & MwmFuncMove)
						functions.move = true;
					if (client.mwm_hint->functions & MwmFuncIconify)
						functions.iconify = true;
					if (client.mwm_hint->functions & MwmFuncMaximize)
						functions.maximize = true;
					if (client.mwm_hint->functions & MwmFuncClose)
						functions.close = true;
				}
		}
}


void FluxboxWindow::getBlackboxHints(void) {
	int format;
	Atom atom_return;
	unsigned long num, len;
	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (XGetWindowProperty(display, client.window,
												 fluxbox->getFluxboxHintsAtom(), 0,
												 PropBlackboxHintsElements, false,
												 fluxbox->getFluxboxHintsAtom(), &atom_return,
												 &format, &num, &len,
												 (unsigned char **) &client.blackbox_hint) == Success &&
			client.blackbox_hint)
		if (num == PropBlackboxHintsElements) {
			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_SHADED)
				shaded = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_SHADED);

			if ((client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
					(client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT))
				maximized = ((client.blackbox_hint->attrib &
											(BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?	1 : 0);
			else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT)
				maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
			else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ)
				maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);

			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_OMNIPRESENT)
				stuck = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_OMNIPRESENT);

			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_WORKSPACE)
				workspace_number = client.blackbox_hint->workspace;


			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_DECORATION) {
				switch (client.blackbox_hint->decoration) {
				case BaseDisplay::DECOR_NONE:
					decorations.titlebar = decorations.border = decorations.handle =
						decorations.iconify = decorations.maximize =
						decorations.menu = false;
					functions.resize = functions.move = functions.iconify =
						functions.maximize = false;

					break;

				default:
				case BaseDisplay::DECOR_NORMAL:
					decorations.titlebar = decorations.border = decorations.handle =
						decorations.iconify = decorations.maximize =
						decorations.menu = true;
					functions.resize = functions.move = functions.iconify =
						functions.maximize = true;

					break;

				case BaseDisplay::DECOR_TINY:
					decorations.titlebar = decorations.iconify = decorations.menu =
						functions.move = functions.iconify = true;
					decorations.border = decorations.handle = decorations.maximize =
						functions.resize = functions.maximize = false;

					break;

				case BaseDisplay::DECOR_TOOL:
					decorations.titlebar = decorations.menu = functions.move = true;
					decorations.iconify = decorations.border = decorations.handle =
						decorations.maximize = functions.resize = functions.maximize =
						functions.iconify = false;

					break;
				}

				reconfigure();
			}
		}
}


void FluxboxWindow::configure(int dx, int dy,
							 unsigned int dw, unsigned int dh) {
	bool send_event = (frame.x != dx || frame.y != dy);

	if ((dw != frame.width) || (dh != frame.height)) {
		if ((((signed) frame.width) + dx) < 0) dx = 0;
		if ((((signed) frame.height) + dy) < 0) dy = 0;

		frame.x = dx;
		frame.y = dy;
		frame.width = dw;
		frame.height = dh;

		downsize();

#ifdef		SHAPE
		if (Fluxbox::instance()->hasShapeExtensions() && frame.shaped) {
			XShapeCombineShape(display, frame.window, ShapeBounding,
 						 frame.mwm_border_w, frame.y_border +
			 frame.mwm_border_w, client.window,
			 ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
						xrect, num, ShapeUnion, Unsorted);
		}
#endif // SHAPE

		XMoveResizeWindow(display, frame.window, frame.x, frame.y,
							frame.width, frame.height);
		positionWindows();
		decorate();
		setFocusFlag(focused);
		redrawAllButtons();
		shaded = false;
	} else {
		frame.x = dx;
		frame.y = dy;

		XMoveWindow(display, frame.window, frame.x, frame.y);
		//move the tab and the chain		
		if (tab)
			tab->setPosition();
		
		if (! moving) send_event = true;
	}

	if (send_event && ! moving) {
		client.x = dx + frame.mwm_border_w + screen->getBorderWidth();
		client.y = dy + frame.y_border + frame.mwm_border_w +
							 screen->getBorderWidth();

		XEvent event;
		event.type = ConfigureNotify;

		event.xconfigure.display = display;
		event.xconfigure.event = client.window;
		event.xconfigure.window = client.window;
		event.xconfigure.x = client.x;
		event.xconfigure.y = client.y;
		event.xconfigure.width = client.width;
		event.xconfigure.height = client.height;
		event.xconfigure.border_width = client.old_bw;
		event.xconfigure.above = frame.window;
		event.xconfigure.override_redirect = false;

		XSendEvent(display, client.window, True, NoEventMask, &event);

		screen->updateNetizenConfigNotify(&event);
	}
}


bool FluxboxWindow::setInputFocus(void) {
	if (((signed) (frame.x + frame.width)) < 0) {
		if (((signed) (frame.y + frame.y_border)) < 0)
			configure(screen->getBorderWidth(), screen->getBorderWidth(),
								frame.width, frame.height);
		else if (frame.y > (signed) screen->getHeight())
			configure(screen->getBorderWidth(), screen->getHeight() - frame.height,
								frame.width, frame.height);
		else
			configure(screen->getBorderWidth(), frame.y + screen->getBorderWidth(),
								frame.width, frame.height);
	} else if (frame.x > (signed) screen->getWidth()) {
		if (((signed) (frame.y + frame.y_border)) < 0)
			configure(screen->getWidth() - frame.width, screen->getBorderWidth(),
								frame.width, frame.height);
		else if (frame.y > (signed) screen->getHeight())
			configure(screen->getWidth() - frame.width,
					screen->getHeight() - frame.height, frame.width, frame.height);
		else
			configure(screen->getWidth() - frame.width,
								frame.y + screen->getBorderWidth(), frame.width, frame.height);
	}

	Fluxbox *fluxbox = Fluxbox::instance();
	
	fluxbox->grab();
	if (! validateClient()) return false;

	bool ret = false;

	if (client.transient && modal)
		ret = client.transient->setInputFocus();
	else {
		if (! focused) {
			if (focus_mode == F_LocallyActive || focus_mode == F_Passive)
				XSetInputFocus(display, client.window,
											RevertToPointerRoot, CurrentTime);
			else
				XSetInputFocus(display, screen->getRootWindow(),
											RevertToNone, CurrentTime);

			fluxbox->setFocusedWindow(this);
			
			if (send_focus_message) {
				XEvent ce;
				ce.xclient.type = ClientMessage;
				ce.xclient.message_type = fluxbox->getWMProtocolsAtom();
				ce.xclient.display = display;
				ce.xclient.window = client.window;
				ce.xclient.format = 32;
				ce.xclient.data.l[0] = fluxbox->getWMTakeFocusAtom();
				ce.xclient.data.l[1] = fluxbox->getLastTime();
				ce.xclient.data.l[2] = 0l;
				ce.xclient.data.l[3] = 0l;
				ce.xclient.data.l[4] = 0l;
				XSendEvent(display, client.window, false, NoEventMask, &ce);
			}

			if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus())
					&& screen->doAutoRaise())
				timer->start();

			ret = true;
		}
	}

	fluxbox->ungrab();

	return ret;
}

//------------ setTab --------------
// Enables or disables the tab on the window
//----------------------------------
void FluxboxWindow::setTab(bool flag) {
	if (flag) {
		if (!tab)
			tab = new Tab(this, 0, 0);
		tab->focus(); // draws the tab with correct texture
		tab->setPosition(); // set tab windows position

	} else if (tab) {
		delete tab;
		tab = 0;		
	}	
	decorations.tab = flag;
}

void FluxboxWindow::iconify(void) {
	if (iconic) return;

	if (windowmenu)
		windowmenu->hide();

	setState(IconicState);

	XSelectInput(display, client.window, NoEventMask);
	XUnmapWindow(display, client.window);
	XSelectInput(display, client.window,
					PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	XUnmapWindow(display, frame.window);
	visible = false;
	iconic = true;
	
	screen->getWorkspace(workspace_number)->removeWindow(this);

	if (transient && client.transient_for) {
		if (! client.transient_for->iconic)
			client.transient_for->iconify();
	}
	screen->addIcon(this);

	if (tab) //if this window got a tab then iconify it too
		tab->iconify();
		
	if (client.transient)
		if (! client.transient->iconic)
			client.transient->iconify();
}


void FluxboxWindow::deiconify(bool reassoc, bool raise) {
	if (iconic || reassoc)
		screen->reassociateWindow(this, -1, false);
	else if (workspace_number != screen->getCurrentWorkspace()->getWorkspaceID())
		return;

	setState(NormalState);

	XSelectInput(display, client.window, NoEventMask);
	XMapWindow(display, client.window);
	XSelectInput(display, client.window,
							 PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	XMapSubwindows(display, frame.window);
	XMapWindow(display, frame.window);

	if (iconic && screen->doFocusNew()) setInputFocus();

	visible = true;
	iconic = false;

	if (reassoc && client.transient) client.transient->deiconify(true, false);
	
	if (tab)
		tab->deiconify();
			
	if (raise) {		
		screen->getWorkspace(workspace_number)->raiseWindow(this);
		if (tab)
			tab->raise();
	}
}


void FluxboxWindow::close(void) {
	Fluxbox *fluxbox = Fluxbox::instance();
	XEvent ce;
	ce.xclient.type = ClientMessage;
	ce.xclient.message_type = fluxbox->getWMProtocolsAtom();
	ce.xclient.display = display;
	ce.xclient.window = client.window;
	ce.xclient.format = 32;
	ce.xclient.data.l[0] = fluxbox->getWMDeleteAtom();
	ce.xclient.data.l[1] = CurrentTime;
	ce.xclient.data.l[2] = 0l;
	ce.xclient.data.l[3] = 0l;
	ce.xclient.data.l[4] = 0l;
	XSendEvent(display, client.window, false, NoEventMask, &ce);
}


void FluxboxWindow::withdraw(void) {
	visible = false;
	iconic = false;

	setState(WithdrawnState);
	XUnmapWindow(display, frame.window);

	XSelectInput(display, client.window, NoEventMask);
	XUnmapWindow(display, client.window);
	XSelectInput(display, client.window,
				PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	if (windowmenu)
		windowmenu->hide();
	
	if (tab)
		tab->withdraw();
}


void FluxboxWindow::maximize(unsigned int button) {
	if (! maximized) {
		int dx, dy;
		unsigned int dw, dh, slitModL = 0, slitModR = 0, slitModT = 0, slitModB = 0;
#ifdef	SLIT
		Slit* mSlt = screen->getSlit();

		if(screen->doMaxOverSlit() && !screen->doFullMax() && (mSlt->getWidth() > 1))
		{
			switch(screen->getSlitDirection())
			{
			case Slit::Vertical:
				switch(screen->getSlitPlacement())
				{
				case Slit::TopRight:
				case Slit::CenterRight:
				case Slit::BottomRight:
					slitModR = mSlt->getWidth() + screen->getBevelWidth();
					break;
				default:
					slitModL = mSlt->getWidth() + screen->getBevelWidth();
					break;
				}
			break;
			case Slit::Horizontal:
				switch(screen->getSlitPlacement())
				{
				case Slit::TopLeft:
				case Slit::TopCenter:
				case Slit::TopRight:
					slitModT = mSlt->getHeight() + screen->getBevelWidth();
					switch (screen->getToolbarPlacement()) {
					case Toolbar::TOPLEFT:
					case Toolbar::TOPCENTER:
					case Toolbar::TOPRIGHT:
						slitModT -= screen->getToolbar()->getExposedHeight() + screen->getBorderWidth();
						break;
					}				
					break;
				default:
					slitModB = mSlt->getHeight() + screen->getBevelWidth();
					switch (screen->getToolbarPlacement()) {
					case Toolbar::BOTTOMLEFT:
					case Toolbar::BOTTOMCENTER:
					case Toolbar::BOTTOMRIGHT:
						slitModB -= screen->getToolbar()->getExposedHeight() + screen->getBorderWidth();
						break;
					}	
					break;
				}	
				break;
			}
		}		
#endif // SLIT
	
		blackbox_attrib.premax_x = frame.x;
		blackbox_attrib.premax_y = frame.y;
		blackbox_attrib.premax_w = frame.width;
		blackbox_attrib.premax_h = frame.height;

		dw = screen->getWidth() - slitModL - slitModR;
		dw -= screen->getBorderWidth2x();
		dw -= frame.mwm_border_w * 2;
		dw -= client.base_width;

		dh = screen->getHeight() - slitModT - slitModB;
		dh -= screen->getBorderWidth2x();
		dh -= frame.mwm_border_w * 2;
		dh -= ((frame.handle_h + screen->getBorderWidth()) * decorations.handle);
		dh -= client.base_height;
		dh -= frame.y_border;

		if (! screen->doFullMax())
			dh -= screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();

		if (dw < client.min_width) dw = client.min_width;
		if (dh < client.min_height) dh = client.min_height;
		if (dw > client.max_width) dw = client.max_width;
		if (dh > client.max_height) dh = client.max_height;

		dw -= (dw % client.width_inc);
		dw += client.base_width;
		dh -= (dh % client.height_inc);
		dh += client.base_height;

		dw += frame.mwm_border_w * 2;

		dh += frame.y_border;
		dh += (frame.handle_h + screen->getBorderWidth());
		dh += frame.mwm_border_w * 2;

		dx = ((screen->getWidth()+ slitModL - slitModR - dw) / 2) - screen->getBorderWidth();

		if (screen->doFullMax()) {
			dy = ((screen->getHeight() - dh) / 2) - screen->getBorderWidth();
		} else {
			dy = (((screen->getHeight() + slitModT - slitModB - (screen->getToolbar()->getExposedHeight()))
			 - dh) / 2) - screen->getBorderWidth2x();

			switch (screen->getToolbarPlacement()) {
			case Toolbar::TOPLEFT:
			case Toolbar::TOPCENTER:
			case Toolbar::TOPRIGHT:
				dy += screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();
				break;
			}
		}

		if (hasTab()) {
			switch(screen->getTabPlacement()) {			
			case Tab::PTOP:
				dy += screen->getTabHeight(); 
				dh -= screen->getTabHeight();
				break;
			case Tab::PLEFT:
				if (screen->isTabRotateVertical()) {
					dx += screen->getTabHeight();
					dw -= screen->getTabHeight();
				} else {
					dx += screen->getTabWidth();
					dw -= screen->getTabWidth();
				}	
				break;
			case Tab::PRIGHT:
				if (screen->isTabRotateVertical())
					dw -= screen->getTabHeight();
				else
					dw -= screen->getTabWidth();	
				break;
			case Tab::PBOTTOM:
				dh -= screen->getTabHeight();
				break;
			default:
				dy += screen->getTabHeight();
				dh -= screen->getTabHeight();
				break;
			}
		}

		if (button == 2) { //expand max width
			dw = frame.width;
			dx = frame.x;
		} else if (button == 3) { //expand max height
			dh = frame.height;
			dy = frame.y;
		}

		switch(button) {
		case 1:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT;

			break;

		case 2:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXVERT;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXVERT;

			break;

		case 3:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXHORIZ;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXHORIZ;

			break;
		}

		if (shaded) {
			blackbox_attrib.flags ^= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_SHADED;
			shaded = false;
		}

		maximized = true;

		configure(dx, dy, dw, dh);
		if (tab)
			tab->raise();
		screen->getWorkspace(workspace_number)->raiseWindow(this);
		setState(current_state);
	} else {
		maximized = false;

		blackbox_attrib.flags &= ! (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT);
		blackbox_attrib.attrib &= ! (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT);

		configure(blackbox_attrib.premax_x, blackbox_attrib.premax_y,
				blackbox_attrib.premax_w, blackbox_attrib.premax_h);

		blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
		blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

		redrawAllButtons();
		setState(current_state);
	}
	if (tab) //resize all the windows in the tab group
		tab->resize();
}


void FluxboxWindow::setWorkspace(int n) {
	workspace_number = n;

	blackbox_attrib.flags |= BaseDisplay::ATTRIB_WORKSPACE;
	blackbox_attrib.workspace = workspace_number;
}


void FluxboxWindow::shade(void) {
	if (decorations.titlebar)
		if (shaded) {
			XResizeWindow(display, frame.window, frame.width, frame.height);
			shaded = false;
			blackbox_attrib.flags ^= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_SHADED;

			setState(NormalState);
		} else {
			XResizeWindow(display, frame.window, frame.width, frame.title_h);
			shaded = true;
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_SHADED;

			setState(IconicState);
		}
}


void FluxboxWindow::stick(void) {

	if (tab) //if it got a tab then do tab's stick on all of the objects in the list
		tab->stick(); //this window will stick too.
	else if (stuck) {
		blackbox_attrib.flags ^= BaseDisplay::ATTRIB_OMNIPRESENT;
		blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_OMNIPRESENT;

		stuck = false;

		if (! iconic)
			screen->reassociateWindow(this, -1, true);

		
	} else {
		stuck = true;

		blackbox_attrib.flags |= BaseDisplay::ATTRIB_OMNIPRESENT;
		blackbox_attrib.attrib |= BaseDisplay::ATTRIB_OMNIPRESENT;

	}
	
	setState(current_state);
}


void FluxboxWindow::setFocusFlag(bool focus) {
	focused = focus;

	if (decorations.titlebar) {
		if (focused) {
			if (frame.ftitle)
				XSetWindowBackgroundPixmap(display, frame.title, frame.ftitle);
			else
				XSetWindowBackground(display, frame.title, frame.ftitle_pixel);
		} else {
			if (frame.utitle)
				XSetWindowBackgroundPixmap(display, frame.title, frame.utitle);
			else
				XSetWindowBackground(display, frame.title, frame.utitle_pixel);
		}
		XClearWindow(display, frame.title);
		
		redrawLabel();
		redrawAllButtons();
	}

	if (decorations.handle) {
		if (focused) {
			if (frame.fhandle)
				XSetWindowBackgroundPixmap(display, frame.handle, frame.fhandle);
			else
				XSetWindowBackground(display, frame.handle, frame.fhandle_pixel);

			if (frame.fgrip) {
				XSetWindowBackgroundPixmap(display, frame.right_grip, frame.fgrip);
				XSetWindowBackgroundPixmap(display, frame.left_grip, frame.fgrip);
			} else {
				XSetWindowBackground(display, frame.right_grip, frame.fgrip_pixel);
				XSetWindowBackground(display, frame.left_grip, frame.fgrip_pixel);
			}
		} else {
			if (frame.uhandle)
				XSetWindowBackgroundPixmap(display, frame.handle, frame.uhandle);
			else
				XSetWindowBackground(display, frame.handle, frame.uhandle_pixel);

			if (frame.ugrip) {
				XSetWindowBackgroundPixmap(display, frame.right_grip, frame.ugrip);
				XSetWindowBackgroundPixmap(display, frame.left_grip, frame.ugrip);
			} else {
				XSetWindowBackground(display, frame.right_grip, frame.ugrip_pixel);
				XSetWindowBackground(display, frame.left_grip, frame.ugrip_pixel);
			}
		}
		XClearWindow(display, frame.handle);
		XClearWindow(display, frame.right_grip);
		XClearWindow(display, frame.left_grip);
	}
	
	if (tab)
		tab->focus();
	
	if (decorations.border) {
		if (focused)
			XSetWindowBorder(display, frame.plate, frame.fborder_pixel);
		else
			XSetWindowBorder(display, frame.plate, frame.uborder_pixel);
	}

	if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus()) &&
			screen->doAutoRaise())
		timer->stop();
}


void FluxboxWindow::installColormap(bool install) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	if (! validateClient()) return;

	int i = 0, ncmap = 0;
	Colormap *cmaps = XListInstalledColormaps(display, client.window, &ncmap);
	XWindowAttributes wattrib;
	if (cmaps) {
		if (XGetWindowAttributes(display, client.window, &wattrib)) {
			if (install) {
				// install the window's colormap
				for (i = 0; i < ncmap; i++) {
					if (*(cmaps + i) == wattrib.colormap) {
						// this window is using an installed color map... do not install
						install = false;
						break; //end for-loop (we dont need to check more)
					}
				}
				// otherwise, install the window's colormap
				if (install)
					XInstallColormap(display, wattrib.colormap);
			} else {				
				for (i = 0; i < ncmap; i++) // uninstall the window's colormap
					if (*(cmaps + i) == wattrib.colormap)						
						XUninstallColormap(display, wattrib.colormap); // we found the colormap to uninstall
			}
		}

		XFree(cmaps);
	}

	fluxbox->ungrab();
}


void FluxboxWindow::setState(unsigned long new_state) {
	current_state = new_state;
	Fluxbox *fluxbox = Fluxbox::instance();
	unsigned long state[2];
	state[0] = (unsigned long) current_state;
	state[1] = (unsigned long) None;
	XChangeProperty(display, client.window, fluxbox->getWMStateAtom(),
			fluxbox->getWMStateAtom(), 32, PropModeReplace,
			(unsigned char *) state, 2);

	XChangeProperty(display, client.window, fluxbox->getFluxboxAttributesAtom(),
					fluxbox->getFluxboxAttributesAtom(), 32, PropModeReplace,
					(unsigned char *) &blackbox_attrib, PropBlackboxAttributesElements);
}


bool FluxboxWindow::getState(void) {
	current_state = 0;

	Atom atom_return;
	bool ret = false;
	int foo;
	unsigned long *state, ulfoo, nitems;
	Fluxbox *fluxbox = Fluxbox::instance();
	if ((XGetWindowProperty(display, client.window, fluxbox->getWMStateAtom(),
				0l, 2l, false, fluxbox->getWMStateAtom(),
				&atom_return, &foo, &nitems, &ulfoo,
				(unsigned char **) &state) != Success) ||
			(! state)) {
			fluxbox->ungrab();
			return false;
	}

	if (nitems >= 1) {
		current_state = static_cast<unsigned long>(state[0]);
		ret = true;
	}

	XFree(static_cast<void *>(state));

	return ret;
}


void FluxboxWindow::setGravityOffsets(void) {
	// translate x coordinate
	switch (client.win_gravity) {
		// handle Westward gravity
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
	default:
		frame.x = client.x;
			break;

		// handle Eastward gravity
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		frame.x = (client.x + client.width) - frame.width;
		break;

		// no x translation desired - default
	case StaticGravity:
	case ForgetGravity:
	case CenterGravity:
		frame.x = client.x - frame.mwm_border_w + screen->getBorderWidth();
	}

	// translate y coordinate
	switch (client.win_gravity) {
		// handle Northbound gravity
	case NorthWestGravity:
	case NorthGravity:
	case NorthEastGravity:
	default:
		frame.y = client.y;
		break;

		// handle Southbound gravity
	case SouthWestGravity:
	case SouthGravity:
	case SouthEastGravity:
		frame.y = (client.y + client.height) - frame.height;
		break;

		// no y translation desired - default
	case StaticGravity:
	case ForgetGravity:
	case CenterGravity:
		frame.y = client.y - frame.y_border - frame.mwm_border_w -
			screen->getBorderWidth();
		break;
	}
}


void FluxboxWindow::restoreAttributes(void) {
	if (! getState()) current_state = NormalState;

	Atom atom_return;
	int foo;
	unsigned long ulfoo, nitems;
	Fluxbox *fluxbox = Fluxbox::instance();
	
	BaseDisplay::BlackboxAttributes *net;
	if (XGetWindowProperty(display, client.window,
			 fluxbox->getFluxboxAttributesAtom(), 0l,
			 PropBlackboxAttributesElements, false,
			 fluxbox->getFluxboxAttributesAtom(), &atom_return, &foo,
			 &nitems, &ulfoo, (unsigned char **) &net) ==
			Success && net && nitems == PropBlackboxAttributesElements) {
		blackbox_attrib.flags = net->flags;
		blackbox_attrib.attrib = net->attrib;
		blackbox_attrib.workspace = net->workspace;
		blackbox_attrib.stack = net->stack;
		blackbox_attrib.premax_x = net->premax_x;
		blackbox_attrib.premax_y = net->premax_y;
		blackbox_attrib.premax_w = net->premax_w;
		blackbox_attrib.premax_h = net->premax_h;

		XFree(static_cast<void *>(net));
	} else
		return;

	if (blackbox_attrib.flags & BaseDisplay::ATTRIB_SHADED &&
			blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) {
		int save_state =
			((current_state == IconicState) ? NormalState : current_state);

		shaded = false;
		shade();
		if (tab)
			tab->shade();
			
		current_state = save_state;
	}

	if (((int) blackbox_attrib.workspace != screen->getCurrentWorkspaceID()) &&
			((int) blackbox_attrib.workspace < screen->getCount())) {
		screen->reassociateWindow(this, blackbox_attrib.workspace, true);

		if (current_state == NormalState) current_state = WithdrawnState;
	} else if (current_state == WithdrawnState)
		current_state = NormalState;

	if (blackbox_attrib.flags & BaseDisplay::ATTRIB_OMNIPRESENT &&
			blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) {
		stuck = false;
		stick();

		current_state = NormalState;
	}

	if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) ||
			(blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)) {
		int x = blackbox_attrib.premax_x, y = blackbox_attrib.premax_y;
		unsigned int w = blackbox_attrib.premax_w, h = blackbox_attrib.premax_h;
		maximized = false;

		int m;
		if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
				(blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT))
			m = ((blackbox_attrib.attrib & (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?
					 1 : 0);
		else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)
			m = ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
		else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ)
			m = ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);
		else
			m = 0;

		if (m) maximize(m);

		blackbox_attrib.premax_x = x;
		blackbox_attrib.premax_y = y;
		blackbox_attrib.premax_w = w;
		blackbox_attrib.premax_h = h;
	}

	setState(current_state);
}


void FluxboxWindow::restoreGravity(void) {
	// restore x coordinate
	switch (client.win_gravity) {
		// handle Westward gravity
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
	default:
		client.x = frame.x;
		break;

		// handle Eastward gravity
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		client.x = (frame.x + frame.width) - client.width;
		break;
	}

	// restore y coordinate
	switch (client.win_gravity) {
		// handle Northbound gravity
	case NorthWestGravity:
	case NorthGravity:
	case NorthEastGravity:
	default:
		client.y = frame.y;
		break;

		// handle Southbound gravity
	case SouthWestGravity:
	case SouthGravity:
	case SouthEastGravity:
		client.y = (frame.y + frame.height) - client.height;
		break;
	}
}


void FluxboxWindow::redrawLabel(void) {
	if (focused) {
		if (frame.flabel)
			XSetWindowBackgroundPixmap(display, frame.label, frame.flabel);
		else
			XSetWindowBackground(display, frame.label, frame.flabel_pixel);
	} else {
		if (frame.ulabel)
			XSetWindowBackgroundPixmap(display, frame.label, frame.ulabel);
		else
			XSetWindowBackground(display, frame.label, frame.ulabel_pixel);
	}
	
	GC gc = ((focused) ? screen->getWindowStyle()->l_text_focus_gc :
			 screen->getWindowStyle()->l_text_unfocus_gc);

	DrawUtil::DrawString(display, frame.label, gc,
			&screen->getWindowStyle()->font, 
			client.title_text_w, frame.label_w,
			frame.bevel_w, client.title);
}


void FluxboxWindow::redrawAllButtons(void) {
	for (unsigned int i=0; i<buttonlist.size(); i++)
		if (buttonlist[i].draw)
			buttonlist[i].draw(this, buttonlist[i].win, false);

}

void FluxboxWindow::mapRequestEvent(XMapRequestEvent *re) {
	if (re->window == client.window) {
#ifdef		DEBUG
		fprintf(stderr,
			I18n::instance()->getMessage(
#ifdef		NLS
					 WindowSet, WindowMapRequest,
#else // !NLS
					 0, 0,
#endif // NLS
					 "FluxboxWindow::mapRequestEvent() for 0x%lx\n"),
						client.window);
#endif // DEBUG
		Fluxbox *fluxbox = Fluxbox::instance();
		
		fluxbox->grab();
		if (! validateClient()) return;

		bool get_state_ret = getState();
		if (! (get_state_ret && fluxbox->isStartup())) {
			if ((client.wm_hint_flags & StateHint) &&
					(! (current_state == NormalState || current_state == IconicState)))
				current_state = client.initial_state;
			else
				current_state = NormalState;
		} else if (iconic)
			current_state = NormalState;

		switch (current_state) {
		case IconicState:
			iconify();

			break;

		case WithdrawnState:
			withdraw();

			break;

		case NormalState:
		case InactiveState:
		case ZoomState:
		default:
			deiconify(false);

			break;
		}

		fluxbox->ungrab();
	}
}


void FluxboxWindow::mapNotifyEvent(XMapEvent *ne) {
	
	if ((ne->window == client.window) && (! ne->override_redirect) && (visible)) {
		Fluxbox *fluxbox = Fluxbox::instance();
		fluxbox->grab();
		if (! validateClient())
			return;

		if (decorations.titlebar)
			positionButtons();

		setState(NormalState);		
			
		redrawAllButtons();

		if (transient || screen->doFocusNew())
			setInputFocus();
		else
			setFocusFlag(false);			
		

		visible = true;
		iconic = false;

		fluxbox->ungrab();
	}
}


void FluxboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
	if (ue->window == client.window) {
#ifdef		DEBUG
		fprintf(stderr,
			I18n::instance()->getMessage(
#ifdef		NLS
					 WindowSet, WindowUnmapNotify,
#else // !NLS
					 0, 0,
#endif // NLS
					 "FluxboxWindow::unmapNotifyEvent() for 0x%lx\n"),
						client.window);
#endif // DEBUG

		Fluxbox *fluxbox = Fluxbox::instance();
		fluxbox->grab();
		if (! validateClient()) return;

		XChangeSaveSet(display, client.window, SetModeDelete);
		XSelectInput(display, client.window, NoEventMask);

		XDeleteProperty(display, client.window, fluxbox->getWMStateAtom());
		XDeleteProperty(display, client.window, fluxbox->getFluxboxAttributesAtom());

		XUnmapWindow(display, frame.window);
		XUnmapWindow(display, client.window);

		XEvent dummy;
		if (! XCheckTypedWindowEvent(display, client.window, ReparentNotify,
				 &dummy)) {
#ifdef		DEBUG
			fprintf(stderr,
				I18n::instance()->getMessage(
#ifdef		NLS
						 WindowSet, WindowUnmapNotifyReparent,
#else // !NLS
						 0, 0,
#endif // NLS
						 "FluxboxWindow::unmapNotifyEvent(): reparent 0x%lx to "
						 "root.\n"), client.window);
#endif // DEBUG

			restoreGravity();
			XReparentWindow(display, client.window, screen->getRootWindow(),
				client.x, client.y);
		}

		XFlush(display);

		fluxbox->ungrab();

		delete this;
	}
}


void FluxboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
	if (de->window == client.window) {
		#ifdef DEBUG
		fprintf(stderr,"%s(%d): DestroyNotifyEvent this=%p\n", __FILE__, __LINE__, this);
		#endif
		XUnmapWindow(display, frame.window);		
		delete this;
	}
}


void FluxboxWindow::propertyNotifyEvent(Atom atom) {
 	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	if (! validateClient()) return;

	switch(atom) {
	case XA_WM_CLASS:
	case XA_WM_CLIENT_MACHINE:
	case XA_WM_COMMAND:
		break;

	case XA_WM_TRANSIENT_FOR:
		// determine if this is a transient window
		Window win;
		if (XGetTransientForHint(display, client.window, &win)) {
			if (win && (win != client.window))
				if ((client.transient_for = fluxbox->searchWindow(win))) {
					client.transient_for->client.transient = this;
					stuck = client.transient_for->stuck;
					transient = true;
				} else if (win == client.window_group) {
		//jr This doesn't look quite right...
					if ((client.transient_for = fluxbox->searchGroup(win, this))) {
						client.transient_for->client.transient = this;
						stuck = client.transient_for->stuck;
						transient = true;
					}
				}

			if (win == screen->getRootWindow()) modal = true;
		}

		// adjust the window decorations based on transience
		if (transient)
			decorations.maximize = decorations.handle =
				decorations.border = functions.maximize = false;

		reconfigure();

		break;

	case XA_WM_HINTS:
		getWMHints();
		break;

	case XA_WM_ICON_NAME:
		getWMIconName();
		if (iconic) screen->iconUpdate();
		break;

	case XA_WM_NAME:
		getWMName();

		if (decorations.titlebar)
			redrawLabel();

		if (hasTab()) // update tab
			getTab()->draw(false);

		if (! iconic)
			screen->getWorkspace(workspace_number)->update();
		else if (Fluxbox::instance()->useIconBar()) {
			IconBar *iconbar = 0;
			IconBarObj *icon = 0;
			if ((iconbar = screen->getToolbar()->getIconBar()) != 0) {
				if ((icon = iconbar->findIcon(this)) != 0)
					iconbar->draw(icon, icon->getWidth());
#ifdef DEBUG
				else
					cerr<<__FILE__<<"("<<__LINE__<<"): can't find icon!"<<endl;
#endif //DEBUG	
			}
#ifdef DEBUG
			else
				cerr<<__FILE__<<"("<<__LINE__<<"): can't find iconbar!"<<endl;
#endif //DEBUG
		}

		break;

	case XA_WM_NORMAL_HINTS: {
		getWMNormalHints();

		if ((client.normal_hint_flags & PMinSize) &&
				(client.normal_hint_flags & PMaxSize)) {
			if (client.max_width <= client.min_width &&
					client.max_height <= client.min_height)
				decorations.maximize = decorations.handle =
			functions.resize = functions.maximize = false;
			else
				decorations.maximize = decorations.handle =
			functions.resize = functions.maximize = true;
		}

		int x = frame.x, y = frame.y;
		unsigned int w = frame.width, h = frame.height;

		upsize();

		if ((x != frame.x) || (y != frame.y) ||
				(w != frame.width) || (h != frame.height))
			reconfigure();

		break; 
	}

	default:
		if (atom == fluxbox->getWMProtocolsAtom()) {
			getWMProtocols();
	
		}

		break;
	}

	fluxbox->ungrab();
}


void FluxboxWindow::exposeEvent(XExposeEvent *ee) {
	if (frame.label == ee->window && decorations.titlebar)
		redrawLabel();
	else 
		redrawAllButtons();

}


void FluxboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {
	if (cr->window == client.window) {
		Fluxbox *fluxbox = Fluxbox::instance();
		fluxbox->grab();
		if (! validateClient())
			return;

		int cx = frame.x, cy = frame.y;
		unsigned int cw = frame.width, ch = frame.height;

		if (cr->value_mask & CWBorderWidth)
			client.old_bw = cr->border_width;

		if (cr->value_mask & CWX)
			cx = cr->x - frame.mwm_border_w - screen->getBorderWidth();

		if (cr->value_mask & CWY)
			cy = cr->y - frame.y_border - frame.mwm_border_w -
				screen->getBorderWidth();

		if (cr->value_mask & CWWidth)
			cw = cr->width + (frame.mwm_border_w * 2);

		if (cr->value_mask & CWHeight)
			ch = cr->height + frame.y_border + (frame.mwm_border_w * 2) +
				(screen->getBorderWidth() * decorations.handle) + frame.handle_h;

		if (frame.x != cx || frame.y != cy ||
			frame.width != cw || frame.height != ch) {
			configure(cx, cy, cw, ch);
			if (tab)
				tab->resize();
		}
		if (cr->value_mask & CWStackMode) {
			switch (cr->detail) {
			case Above:
			case TopIf:
			default:
				if (iconic)
					deiconify();
//!!TODO check this and the line below..
//				if (tab)
//					tab->raise();
				screen->getWorkspace(workspace_number)->raiseWindow(this);
				break;

			case Below:
			case BottomIf:
				if (iconic)
					deiconify();

//				if (tab)
//					tab->raise();
				screen->getWorkspace(workspace_number)->lowerWindow(this);
				break;
			}
		}

		fluxbox->ungrab();
	}
}


void FluxboxWindow::buttonPressEvent(XButtonEvent *be) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	
	if (! validateClient())	
		return;

		
	if (be->button == 1 || (be->button == 3 && be->state == Mod1Mask)) {
		if ((! focused) && (! screen->isSloppyFocus()))	 //check focus
			setInputFocus(); 
		
		//Redraw buttons
		for (unsigned int i=0; i<buttonlist.size(); i++) {
			if (be->window == buttonlist[i].win && buttonlist[i].draw)
					buttonlist[i].draw(this, be->window, true);
		}
		
		if (frame.plate == be->window) {
			
			if (windowmenu && windowmenu->isVisible()) //hide menu if its visible
				windowmenu->hide();
			//raise tab first, if there is any, so the focus on windows get 
			//right and dont "hide" the tab behind other windows
			if (tab)
				tab->raise();
			
			screen->getWorkspace(workspace_number)->raiseWindow(this);

			XAllowEvents(display, ReplayPointer, be->time);
			
		} else {
		
			if (frame.title == be->window || frame.label == be->window) {
				if (((be->time - lastButtonPressTime) <=
			 		fluxbox->getDoubleClickInterval()) ||
			 	 			(be->state & ControlMask)) {
					lastButtonPressTime = 0;
					shade();
					if (tab) //shade windows in the tablist too
						tab->shade();
				} else
					lastButtonPressTime = be->time;			
			}
			
			
			frame.grab_x = be->x_root - frame.x - screen->getBorderWidth();
			frame.grab_y = be->y_root - frame.y - screen->getBorderWidth();

			if (windowmenu && windowmenu->isVisible())
				windowmenu->hide();
			//raise tab first if there is any
			if (tab)
				tab->raise();
		
			screen->getWorkspace(workspace_number)->raiseWindow(this);
		}
	} else if (be->button == 2 && be->window == frame.label) {
		screen->getWorkspace(workspace_number)->lowerWindow(this);

	} else if (windowmenu && be->button == 3 &&
			(frame.title == be->window || frame.label == be->window ||
			frame.handle == be->window)) {

		int mx = 0, my = 0;

		if (frame.title == be->window || frame.label == be->window) {
			mx = be->x_root - (windowmenu->getWidth() / 2);
			my = frame.y + frame.title_h;
		} else if (frame.handle == be->window) {
			mx = be->x_root - (windowmenu->getWidth() / 2);
			my = frame.y + frame.y_handle - windowmenu->getHeight();
		} else {
			bool buttonproc=false;
			
			if (buttonproc)
				mx = be->x_root - (windowmenu->getWidth() / 2);

			if (be->y <= (signed) frame.bevel_w)
				my = frame.y + frame.y_border;
			else
				my = be->y_root - (windowmenu->getHeight() / 2);
		}

		if (mx > (signed) (frame.x + frame.width - windowmenu->getWidth()))
			mx = frame.x + frame.width - windowmenu->getWidth();
		if (mx < frame.x)
			mx = frame.x;

		if (my > (signed) (frame.y + frame.y_handle - windowmenu->getHeight()))
			my = frame.y + frame.y_handle - windowmenu->getHeight();
		if (my < (signed) (frame.y + ((decorations.titlebar) ? frame.title_h :
							frame.y_border)))
			my = frame.y +
			((decorations.titlebar) ? frame.title_h : frame.y_border);

		if (windowmenu) {
			if (! windowmenu->isVisible()) { // if not window menu is visible then show it
				windowmenu->move(mx, my);
				windowmenu->show();		
				XRaiseWindow(display, windowmenu->getWindowID());
				XRaiseWindow(display, windowmenu->getSendToMenu()->getWindowID());
				XRaiseWindow(display, windowmenu->getSendGroupToMenu()->getWindowID());
			} else //else hide menu
				windowmenu->hide(); 
		}
		
	} else if (be->button == 4) { //scroll to tab right
		if (tab && tab->next()) {
			tab->next()->getWindow()->setInputFocus();
			screen->getWorkspace(workspace_number)->raiseWindow(tab->next()->getWindow());
		}
	} else if (be->button == 5) { //scroll to tab left
		if (tab && tab->prev()) {
			tab->prev()->getWindow()->setInputFocus();
			screen->getWorkspace(workspace_number)->raiseWindow(tab->prev()->getWindow());
		}
	}
	
	fluxbox->ungrab();
}


void FluxboxWindow::buttonReleaseEvent(XButtonEvent *re) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();

	if (! validateClient())
		return;

	if (moving) {
		moving = false;
						
		fluxbox->maskWindowEvents(0, (FluxboxWindow *) 0);

		if (! screen->doOpaqueMove()) {
			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		 		 frame.move_x, frame.move_y, frame.resize_w,
			 	 frame.resize_h);

			configure(frame.move_x, frame.move_y, frame.width, frame.height);
			fluxbox->ungrab();
		} else
			configure(frame.x, frame.y, frame.width, frame.height);

		screen->hideGeometry();
		XUngrabPointer(display, CurrentTime);
	} else if (resizing) {
		XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
			frame.resize_x, frame.resize_y,
			frame.resize_w, frame.resize_h);

		screen->hideGeometry();

		if (re->window == frame.left_grip)
			left_fixsize();
		else
			right_fixsize();

		resizing = false;
		configure(frame.resize_x, frame.resize_y,
				frame.resize_w - screen->getBorderWidth2x(),
				frame.resize_h - screen->getBorderWidth2x());
	
		if (tab)
			tab->resize();
			
		fluxbox->ungrab();
		XUngrabPointer(display, CurrentTime);
	} else if (re->window == frame.window) {
		if (re->button == 2 && re->state == Mod1Mask)
			XUngrabPointer(display, CurrentTime);
	} else {
		if ((re->x >= 0) && ((unsigned) re->x <= frame.button_w) &&
			(re->y >= 0) && ((unsigned) re->y <= frame.button_h)) {
			for (unsigned int i=0; i<buttonlist.size(); i++) {
				if (re->window == buttonlist[i].win &&
						buttonlist[i].released) {
					buttonlist[i].released(this, re);	
					break;					
				}
			}
		} 
		redrawAllButtons();
	}

	fluxbox->ungrab();
}


void FluxboxWindow::motionNotifyEvent(XMotionEvent *me) {
	Fluxbox *fluxbox = Fluxbox::instance();
	if ((me->state & Button1Mask) && functions.move &&
			(frame.title == me->window || frame.label == me->window ||
			 frame.handle == me->window || frame.window == me->window) && !resizing) {
			 
		if (! moving) {
			XGrabPointer(display, me->window, False, Button1MotionMask |
								ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
								None, fluxbox->getMoveCursor(), CurrentTime);

			if (windowmenu && windowmenu->isVisible())
				windowmenu->hide();

			moving = true;

			fluxbox->maskWindowEvents(client.window, this);

			if (! screen->doOpaqueMove()) {
				fluxbox->grab();

				frame.move_x = frame.x;
				frame.move_y = frame.y;
				frame.resize_w = frame.width + screen->getBorderWidth2x();
				frame.resize_h = ((shaded) ? frame.title_h : frame.height) +
						screen->getBorderWidth2x();

				screen->showPosition(frame.x, frame.y);

				XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
				frame.move_x, frame.move_y,
					frame.resize_w, frame.resize_h);
			}
		} else {
			int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;

			dx -= screen->getBorderWidth();
			dy -= screen->getBorderWidth();

			if (screen->getEdgeSnapThreshold()) {
				int drx = screen->getWidth() - (dx + frame.snap_w);

				if (dx > 0 && dx < drx && dx < screen->getEdgeSnapThreshold()) 
					dx = 0;
				else if (drx > 0 && drx < screen->getEdgeSnapThreshold())
					dx = screen->getWidth() - frame.snap_w;

				int dtty, dbby, dty, dby;
				switch (screen->getToolbarPlacement()) {
				case Toolbar::TOPLEFT:
				case Toolbar::TOPCENTER:
				case Toolbar::TOPRIGHT:
					dtty = screen->getToolbar()->getExposedHeight() +
							screen->getBorderWidth();
					dbby = screen->getHeight();
					break;

				default:
					dtty = 0;
					dbby = screen->getToolbar()->getY();
					break;
				}

				dty = dy - dtty;
				dby = dbby - (dy + frame.snap_h);

				if (dy > 0 && dty < screen->getEdgeSnapThreshold())
					dy = dtty;
				else if (dby > 0 && dby < screen->getEdgeSnapThreshold())
					dy = dbby - frame.snap_h;
			}

			if (! screen->doOpaqueMove()) {
				XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
						frame.move_x, frame.move_y, frame.resize_w,
						frame.resize_h);

				frame.move_x = dx;
				frame.move_y = dy;

				XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
					 frame.move_x, frame.move_y, frame.resize_w,
					 frame.resize_h);
			} else
				configure(dx, dy, frame.width, frame.height);

				screen->showPosition(dx, dy);
		}
	} else if (functions.resize &&
			(((me->state & Button1Mask) && (me->window == frame.right_grip ||
							 me->window == frame.left_grip)) ||
									 me->window == frame.window)) {
		bool left = (me->window == frame.left_grip);

		if (! resizing) {
				XGrabPointer(display, me->window, false, ButtonMotionMask |
					ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
						((left) ? fluxbox->getLowerLeftAngleCursor() :
						fluxbox->getLowerRightAngleCursor()),
						CurrentTime);

			resizing = true;

			fluxbox->grab();

			int gx, gy;
			frame.grab_x = me->x - screen->getBorderWidth();
			frame.grab_y = me->y - screen->getBorderWidth2x();
			frame.resize_x = frame.x;
			frame.resize_y = frame.y;
			frame.resize_w = frame.width + screen->getBorderWidth2x();
			frame.resize_h = frame.height + screen->getBorderWidth2x();

			if (left)
				left_fixsize(&gx, &gy);
			else
				right_fixsize(&gx, &gy);

			screen->showGeometry(gx, gy);

			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
				frame.resize_x, frame.resize_y,
				frame.resize_w, frame.resize_h);
		} else if (resizing) {
			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
				frame.resize_x, frame.resize_y,
				frame.resize_w, frame.resize_h);

			int gx, gy;

			frame.resize_h = frame.height + (me->y - frame.grab_y);
			if (frame.resize_h < 1)
				frame.resize_h = 1;

			if (left) {
				frame.resize_x = me->x_root - frame.grab_x;
				if (frame.resize_x > (signed) (frame.x + frame.width))
					frame.resize_x = frame.resize_x + frame.width - 1;

				left_fixsize(&gx, &gy);
			} else {
				frame.resize_w = frame.width + (me->x - frame.grab_x);
				if (frame.resize_w < 1)
					frame.resize_w = 1;

				right_fixsize(&gx, &gy);
			}

			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
					frame.resize_x, frame.resize_y,
					frame.resize_w, frame.resize_h);

			screen->showGeometry(gx, gy);
		}
	}
}


#ifdef		SHAPE
void FluxboxWindow::shapeEvent(XShapeEvent *) {
	Fluxbox *fluxbox = Fluxbox::instance();
	if (fluxbox->hasShapeExtensions()) {
		if (frame.shaped) {			
			fluxbox->grab();
			if (! validateClient())
				return;
			XShapeCombineShape(display, frame.window, ShapeBounding,
				frame.mwm_border_w, frame.y_border +
				frame.mwm_border_w, client.window,
				ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
					xrect, num, ShapeUnion, Unsorted);
			fluxbox->ungrab();
		}
	}
}
#endif // SHAPE


bool FluxboxWindow::validateClient(void) {
	XSync(display, false);

	XEvent e;
	if (XCheckTypedWindowEvent(display, client.window, DestroyNotify, &e) ||
			XCheckTypedWindowEvent(display, client.window, UnmapNotify, &e)) {
		XPutBackEvent(display, &e);
		Fluxbox::instance()->ungrab();

		return false;
	}

	return true;
}


void FluxboxWindow::restore(void) {
	XChangeSaveSet(display, client.window, SetModeDelete);
	XSelectInput(display, client.window, NoEventMask);

	restoreGravity();

	XUnmapWindow(display, frame.window);
	XUnmapWindow(display, client.window);

	XSetWindowBorderWidth(display, client.window, client.old_bw);
	XReparentWindow(display, client.window, screen->getRootWindow(),
									client.x, client.y);
	XMapWindow(display, client.window);
		
	XFlush(display);
}


void FluxboxWindow::timeout(void) {
	if (tab)
		tab->raise();
	screen->getWorkspace(workspace_number)->raiseWindow(this);
}


void FluxboxWindow::changeBlackboxHints(BaseDisplay::BlackboxHints *net) {
	if ((net->flags & BaseDisplay::ATTRIB_SHADED) &&
			((blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) !=
			(net->attrib & BaseDisplay::ATTRIB_SHADED)))
		shade();

	if ((net->flags & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) &&
			((blackbox_attrib.attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) !=
				(net->attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)))) {
		if (maximized) {
			maximize(0);
		} else {
			int m = 0;

			if ((net->flags & BaseDisplay::ATTRIB_MAXHORIZ) && (net->flags & BaseDisplay::ATTRIB_MAXVERT))
				m = ((net->attrib & (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?	1 : 0);
			else if (net->flags & BaseDisplay::ATTRIB_MAXVERT)
				m = ((net->attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
			else if (net->flags & BaseDisplay::ATTRIB_MAXHORIZ)
				m = ((net->attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);

			maximize(m);
		}
	}

	if ((net->flags & BaseDisplay::ATTRIB_OMNIPRESENT) &&
			((blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) !=
			(net->attrib & BaseDisplay::ATTRIB_OMNIPRESENT)))
		stick();

	if ((net->flags & BaseDisplay::ATTRIB_WORKSPACE) &&
			(workspace_number != (signed) net->workspace)) {
		screen->reassociateWindow(this, net->workspace, true);

		if (screen->getCurrentWorkspaceID() != (signed) net->workspace)
			withdraw();
		else 
			deiconify();
	}

	if (net->flags & BaseDisplay::ATTRIB_DECORATION) {
		switch (net->decoration) {
		case BaseDisplay::DECOR_NONE:
			decorations.titlebar = decorations.border = decorations.handle =
				decorations.iconify = decorations.maximize =
				decorations.menu = false;
			functions.resize = functions.move = functions.iconify =
				functions.maximize = false;

			break;

		default:
		case BaseDisplay::DECOR_NORMAL:
			decorations.titlebar = decorations.border = decorations.handle =
				decorations.iconify = decorations.maximize =
				decorations.menu = true;
			functions.resize = functions.move = functions.iconify =
				functions.maximize = true;

			break;

		case BaseDisplay::DECOR_TINY:
			decorations.titlebar = decorations.iconify = decorations.menu =
				functions.move = functions.iconify = true;
			decorations.border = decorations.handle = decorations.maximize =
				functions.resize = functions.maximize = false;

			break;

		case BaseDisplay::DECOR_TOOL:
			decorations.titlebar = decorations.menu = functions.move = true;
			decorations.iconify = decorations.border = decorations.handle =
				decorations.maximize = functions.resize = functions.maximize =
				functions.iconify = false;

			break;
		}

		reconfigure();
	}
}


void FluxboxWindow::upsize(void) {
	// convert client.width/height into frame sizes

	frame.bevel_w = screen->getBevelWidth();
	frame.mwm_border_w = screen->getFrameWidth() * decorations.border;
	
	if (I18n::instance()->multibyte()) {
		frame.title_h = (screen->getWindowStyle()->font.set_extents->
				max_ink_extent.height +
				(frame.bevel_w * 2) + 2) * decorations.titlebar;
	} else
		frame.title_h = (screen->getWindowStyle()->font.fontstruct->ascent +
				screen->getWindowStyle()->font.fontstruct->descent +
				(frame.bevel_w * 2) + 2) * decorations.titlebar;

	frame.label_h = (frame.title_h - (frame.bevel_w * 2)) * decorations.titlebar;
	frame.button_w = frame.button_h = frame.label_h - 2;

	frame.y_border = (frame.title_h + screen->getBorderWidth()) * decorations.titlebar;
	frame.border_h = client.height + (frame.mwm_border_w * 2);

	frame.y_handle = frame.y_border + frame.border_h +
							(screen->getBorderWidth() * decorations.handle);
	frame.handle_h = decorations.handle * screen->getHandleWidth();

	frame.grip_w = (frame.button_h * 2) * decorations.handle;
	frame.grip_h = screen->getHandleWidth() * decorations.handle;

	frame.width = client.width + (frame.mwm_border_w * 2);
	frame.height = frame.y_handle + frame.handle_h;

	frame.snap_w = frame.width + screen->getBorderWidth2x();
	frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void FluxboxWindow::downsize(void) {
	// convert frame.width/height into client sizes

	frame.y_handle = frame.height - frame.handle_h;
	frame.border_h = frame.y_handle - frame.y_border -
							(screen->getBorderWidth() * decorations.handle);

	client.x = frame.x + frame.mwm_border_w + screen->getBorderWidth();
	client.y = frame.y + frame.y_border + frame.mwm_border_w +
							screen->getBorderWidth();

	client.width = frame.width - (frame.mwm_border_w * 2);
	client.height = frame.height - frame.y_border -
						(frame.mwm_border_w * 2) - frame.handle_h -
						(screen->getBorderWidth() * decorations.handle);

	frame.y_handle = frame.border_h + frame.y_border + screen->getBorderWidth();

	frame.snap_w = frame.width + screen->getBorderWidth2x();
	frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void FluxboxWindow::right_fixsize(int *gx, int *gy) {
	// calculate the size of the client window and conform it to the
	// size specified by the size hints of the client window...
	int dx = frame.resize_w - client.base_width - (frame.mwm_border_w * 2) -
		screen->getBorderWidth2x();
	int dy = frame.resize_h - frame.y_border - client.base_height -
		frame.handle_h - (screen->getBorderWidth() * 3) - (frame.mwm_border_w * 2);

	if (dx < (signed) client.min_width)
		dx = client.min_width;
	if (dy < (signed) client.min_height)
		dy = client.min_height;
	if ((unsigned) dx > client.max_width)
		dx = client.max_width;
	if ((unsigned) dy > client.max_height)
		dy = client.max_height;

	dx /= client.width_inc;
	dy /= client.height_inc;

	if (gx) *gx = dx;
	if (gy) *gy = dy;

	dx = (dx * client.width_inc) + client.base_width;
	dy = (dy * client.height_inc) + client.base_height;

	frame.resize_w = dx + (frame.mwm_border_w * 2) + screen->getBorderWidth2x();
	frame.resize_h = dy + frame.y_border + frame.handle_h +
								(frame.mwm_border_w * 2) +	(screen->getBorderWidth() * 3);
}


void FluxboxWindow::left_fixsize(int *gx, int *gy) {
	// calculate the size of the client window and conform it to the
	// size specified by the size hints of the client window...
	int dx = frame.x + frame.width - frame.resize_x - client.base_width -
		(frame.mwm_border_w * 2);
	int dy = frame.resize_h - frame.y_border - client.base_height -
		frame.handle_h - (screen->getBorderWidth() * 3) - (frame.mwm_border_w * 2);

	if (dx < (signed) client.min_width) dx = client.min_width;
	if (dy < (signed) client.min_height) dy = client.min_height;
	if ((unsigned) dx > client.max_width) dx = client.max_width;
	if ((unsigned) dy > client.max_height) dy = client.max_height;

	dx /= client.width_inc;
	dy /= client.height_inc;

	if (gx) *gx = dx;
	if (gy) *gy = dy;

	dx = (dx * client.width_inc) + client.base_width;
	dy = (dy * client.height_inc) + client.base_height;

	frame.resize_w = dx + (frame.mwm_border_w * 2) + screen->getBorderWidth2x();
	frame.resize_x = frame.x + frame.width - frame.resize_w +
									 screen->getBorderWidth2x();
	frame.resize_h = dy + frame.y_border + frame.handle_h +
									 (frame.mwm_border_w * 2) + (screen->getBorderWidth() * 3);
}
