// Toolbar.cc for Blackbox - an X11 Window manager
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
#include "Clientmenu.hh"
#include "Icon.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef		STDC_HEADERS
#	include <string.h>
#endif // STDC_HEADERS

#ifdef		HAVE_STDIO_H
#	include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef		TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else // !TIME_WITH_SYS_TIME
# ifdef		HAVE_SYS_TIME_H
#	include <sys/time.h>
# else // !HAVE_SYS_TIME_H
#	include <time.h>
# endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <iostream>

using namespace std;

Toolbar::Toolbar(BScreen *scrn):
screen(scrn),
image_ctrl(screen->getImageControl())
{

	fluxbox = Fluxbox::instance();
	
	// get the clock updating every minute
	clock_timer = new BTimer(fluxbox, this);
	timeval now;
	gettimeofday(&now, 0);
	clock_timer->setTimeout((60 - (now.tv_sec % 60)) * 1000);
	clock_timer->start();

	hide_handler.toolbar = this;
	hide_timer = new BTimer(fluxbox, &hide_handler);
	hide_timer->setTimeout(fluxbox->getAutoRaiseDelay());
	hide_timer->fireOnce(True);


	editing = False;
	on_top = screen->isToolbarOnTop();
	hidden = do_auto_hide = screen->doToolbarAutoHide();

	frame.grab_x = frame.grab_y = 0;

	toolbarmenu = new Toolbarmenu(this);

	display = fluxbox->getXDisplay();
	XSetWindowAttributes attrib;
	unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
		CWColormap | CWOverrideRedirect | CWEventMask;
		
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
		screen->getBorderColor()->getPixel();
	attrib.colormap = screen->getColormap();
	attrib.override_redirect = True;
	attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
		EnterWindowMask | LeaveWindowMask;

	frame.window =
		XCreateWindow(display, screen->getRootWindow(), 0, 0, 1, 1, 0,
			screen->getDepth(), InputOutput, screen->getVisual(),
			create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.window, this);

	attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
		KeyPressMask | EnterWindowMask;

	frame.workspace_label =
		XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.workspace_label, this);

	frame.window_label =
		XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.window_label, this);

	frame.clock =
		XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.clock, this);

	frame.psbutton =
		XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.psbutton, this);

	frame.nsbutton =
		XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.nsbutton, this);

	frame.pwbutton =
		XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.pwbutton, this);

	frame.nwbutton =
		XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
			InputOutput, screen->getVisual(), create_mask, &attrib);
	fluxbox->saveToolbarSearch(frame.nwbutton, this);

	frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
		frame.pbutton = None;

	if (Fluxbox::instance()->useIconBar())
		iconbar = new IconBar(screen, frame.window_label);
	else
		iconbar = 0;
		
	reconfigure();

	XMapSubwindows(display, frame.window);
	XMapWindow(display, frame.window);
}


Toolbar::~Toolbar(void) {
	XUnmapWindow(display, frame.window);
	 
	if (frame.base) image_ctrl->removeImage(frame.base);
	if (frame.label) image_ctrl->removeImage(frame.label);
	if (frame.wlabel) image_ctrl->removeImage(frame.wlabel);
	if (frame.clk) image_ctrl->removeImage(frame.clk);
	if (frame.button) image_ctrl->removeImage(frame.button);
	if (frame.pbutton) image_ctrl->removeImage(frame.pbutton);

	
	fluxbox->removeToolbarSearch(frame.window);

	fluxbox->removeToolbarSearch(frame.workspace_label);
	fluxbox->removeToolbarSearch(frame.window_label);
	fluxbox->removeToolbarSearch(frame.clock);
	fluxbox->removeToolbarSearch(frame.psbutton);
	fluxbox->removeToolbarSearch(frame.nsbutton);
	fluxbox->removeToolbarSearch(frame.pwbutton);
	fluxbox->removeToolbarSearch(frame.nwbutton);
	
	XDestroyWindow(display, frame.workspace_label);
	XDestroyWindow(display, frame.window_label);
	XDestroyWindow(display, frame.clock);

	XDestroyWindow(display, frame.window);

	delete hide_timer;
	delete clock_timer;
	delete toolbarmenu;
	if (iconbar)
		delete iconbar; 
}

void Toolbar::addIcon(FluxboxWindow *w) {
	if (iconbar)
		Fluxbox::instance()->saveToolbarSearch(iconbar->addIcon(w), this);
}

void Toolbar::delIcon(FluxboxWindow *w) {
	if (iconbar)
		Fluxbox::instance()->removeToolbarSearch(iconbar->delIcon(w));
}
		
void Toolbar::reconfigure(void) {
	frame.bevel_w = screen->getBevelWidth();
	frame.width = screen->getWidth() * screen->getToolbarWidthPercent() / 100;
	
	I18n *i18n = I18n::instance();
	
	if (i18n->multibyte())
		frame.height =
			screen->getToolbarStyle()->font.set_extents->max_ink_extent.height;
	else
		frame.height = screen->getToolbarStyle()->font.fontstruct->ascent +
			 screen->getToolbarStyle()->font.fontstruct->descent;
	frame.button_w = frame.height;
	frame.height += 2;
	frame.label_h = frame.height;
	frame.height += (frame.bevel_w * 2);
	
	switch (screen->getToolbarPlacement()) {
	case TOPLEFT:
		frame.x = 0;
		frame.y = 0;
		frame.x_hidden = 0;
		frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth() - frame.height;
		break;

	case BOTTOMLEFT:
		frame.x = 0;
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		frame.x_hidden = 0;
		frame.y_hidden = screen->getHeight() - screen->getBevelWidth() - 
			screen->getBorderWidth();
		break;

	case TOPCENTER:
		frame.x = (screen->getWidth() - frame.width) / 2;
		frame.y = 0;
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth() -
			frame.height;
		break;

	case BOTTOMCENTER:
	default:
		frame.x = (screen->getWidth() - frame.width) / 2;
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getHeight() - screen->getBevelWidth() - 
			screen->getBorderWidth();
		break;

	case TOPRIGHT:
		frame.x = screen->getWidth() - frame.width - screen->getBorderWidth2x();
		frame.y = 0;
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth() - 
			frame.height;
		break;

	case BOTTOMRIGHT:
		frame.x = screen->getWidth() - frame.width - screen->getBorderWidth2x();
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getHeight() - screen->getBevelWidth() - 
			screen->getBorderWidth();
		break;
	}

	#ifdef		HAVE_STRFTIME
	time_t ttmp = time(NULL);
	struct tm *tt = 0;

	if (ttmp != -1) {
		tt = localtime(&ttmp);
		if (tt) {
			char t[1024], *time_string = (char *) 0;
			int len = strftime(t, 1024, screen->getStrftimeFormat(), tt);

			time_string = new char[len + 1];

			memset(time_string, '0', len);
			*(time_string + len) = '\0';

			if (i18n->multibyte()) {
				XRectangle ink, logical;
				XmbTextExtents(screen->getToolbarStyle()->font.set, time_string, len,
					&ink, &logical);
				frame.clock_w = logical.width;
			} else
				frame.clock_w = XTextWidth(screen->getToolbarStyle()->font.fontstruct,
					time_string, len);
			
			frame.clock_w += (frame.bevel_w * 4);
			
			delete [] time_string;
		} else
			frame.clock_w = 0;
	} else
		frame.clock_w = 0;
	#else // !HAVE_STRFTIME
	
	frame.clock_w =
		XTextWidth(screen->getToolbarStyle()->font.fontstruct,
				 i18n->getMessage(
		#ifdef NLS
			ToolbarSet, ToolbarNoStrftimeLength,
		#else // !NLS
			0, 0,
		#endif // NLS
			"00:00000"),
			strlen(i18n->getMessage(
		#ifdef		NLS
				 ToolbarSet, ToolbarNoStrftimeLength,
		#else // !NLS
				 0, 0,
		#endif // NLS
			 "00:00000"))) + (frame.bevel_w * 4);
	
	#endif // HAVE_STRFTIME

	int i;
	unsigned int w = 0;
	frame.workspace_label_w = 0;

	for (i = 0; i < screen->getCount(); i++) {
		if (i18n->multibyte()) {
			XRectangle ink, logical;
			XmbTextExtents(screen->getToolbarStyle()->font.set,
				 screen->getWorkspace(i)->getName(),
				 strlen(screen->getWorkspace(i)->getName()),
				 &ink, &logical);
			w = logical.width;
		} else
			w = XTextWidth(screen->getToolbarStyle()->font.fontstruct,
				screen->getWorkspace(i)->getName(),
				strlen(screen->getWorkspace(i)->getName()));

		w += (frame.bevel_w * 4);

		if (w > frame.workspace_label_w)
			frame.workspace_label_w = w;
	}

	if (frame.workspace_label_w < frame.clock_w)
		frame.workspace_label_w = frame.clock_w;
	else if (frame.workspace_label_w > frame.clock_w)
		frame.clock_w = frame.workspace_label_w;

	frame.window_label_w =
		(frame.width - (frame.clock_w + (frame.button_w * 4) +
		frame.workspace_label_w + (frame.bevel_w * 8) + 6));

	if (hidden)
		XMoveResizeWindow(display, frame.window, frame.x_hidden, frame.y_hidden,
			frame.width, frame.height);
	else
		XMoveResizeWindow(display, frame.window, frame.x, frame.y,
			frame.width, frame.height);

	XMoveResizeWindow(display, frame.workspace_label, frame.bevel_w,
		frame.bevel_w, frame.workspace_label_w,
		frame.label_h);
	XMoveResizeWindow(display, frame.psbutton, (frame.bevel_w * 2) +
		frame.workspace_label_w + 1, frame.bevel_w + 1,
		frame.button_w, frame.button_w);
	XMoveResizeWindow(display ,frame.nsbutton, (frame.bevel_w * 3) +
		frame.workspace_label_w + frame.button_w + 2,
		frame.bevel_w + 1, frame.button_w, frame.button_w);
	XMoveResizeWindow(display, frame.window_label, (frame.bevel_w * 4) +
		(frame.button_w * 2) + frame.workspace_label_w + 3,
		frame.bevel_w, frame.window_label_w, frame.label_h);
	XMoveResizeWindow(display, frame.pwbutton, (frame.bevel_w * 5) +
		(frame.button_w * 2) + frame.workspace_label_w +
		frame.window_label_w + 4, frame.bevel_w + 1,
		frame.button_w, frame.button_w);
	XMoveResizeWindow(display, frame.nwbutton, (frame.bevel_w * 6) +
		(frame.button_w * 3) + frame.workspace_label_w +
		frame.window_label_w + 5, frame.bevel_w + 1,
		frame.button_w, frame.button_w);
	XMoveResizeWindow(display, frame.clock, frame.width - frame.clock_w -
		frame.bevel_w, frame.bevel_w, frame.clock_w,
		frame.label_h);

	Pixmap tmp = frame.base;
	BTexture *texture = &(screen->getToolbarStyle()->toolbar);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.base = None;
		XSetWindowBackground(display, frame.window,
			 texture->getColor()->getPixel());
	} else {
		frame.base =
			image_ctrl->renderImage(frame.width, frame.height, texture);
		XSetWindowBackgroundPixmap(display, frame.window, frame.base);
	}
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.label;
	texture = &(screen->getToolbarStyle()->window);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.label = None;
		XSetWindowBackground(display, frame.window_label,
			 texture->getColor()->getPixel());
	} else {
		frame.label =
			image_ctrl->renderImage(frame.window_label_w, frame.label_h, texture);
		XSetWindowBackgroundPixmap(display, frame.window_label, frame.label);
	}
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.wlabel;
	texture = &(screen->getToolbarStyle()->label);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.wlabel = None;
		XSetWindowBackground(display, frame.workspace_label,
			 texture->getColor()->getPixel());
	} else {
		frame.wlabel =
			image_ctrl->renderImage(frame.workspace_label_w, frame.label_h, texture);
		XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.wlabel);
	}
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.clk;
	texture = &(screen->getToolbarStyle()->clock);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.clk = None;
		XSetWindowBackground(display, frame.clock,
			 texture->getColor()->getPixel());
	} else {
		frame.clk =
			image_ctrl->renderImage(frame.clock_w, frame.label_h, texture);
		XSetWindowBackgroundPixmap(display, frame.clock, frame.clk);
	}
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.button;
	texture = &(screen->getToolbarStyle()->button);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.button = None;

		frame.button_pixel = texture->getColor()->getPixel();
		XSetWindowBackground(display, frame.psbutton, frame.button_pixel);
		XSetWindowBackground(display, frame.nsbutton, frame.button_pixel);
		XSetWindowBackground(display, frame.pwbutton, frame.button_pixel);
		XSetWindowBackground(display, frame.nwbutton, frame.button_pixel);
	} else {
		frame.button =
			image_ctrl->renderImage(frame.button_w, frame.button_w, texture);

		XSetWindowBackgroundPixmap(display, frame.psbutton, frame.button);
		XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.button);
		XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.button);
		XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.button);
	}
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.pbutton;
	texture = &(screen->getToolbarStyle()->pressed);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.pbutton = None;
		frame.pbutton_pixel = texture->getColor()->getPixel();
	} else
		frame.pbutton =
			image_ctrl->renderImage(frame.button_w, frame.button_w, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	XSetWindowBorder(display, frame.window,
			 screen->getBorderColor()->getPixel());
	XSetWindowBorderWidth(display, frame.window, screen->getBorderWidth());

	XClearWindow(display, frame.window);
	XClearWindow(display, frame.workspace_label);
	XClearWindow(display, frame.window_label);
	XClearWindow(display, frame.clock);
	XClearWindow(display, frame.psbutton);
	XClearWindow(display, frame.nsbutton);
	XClearWindow(display, frame.pwbutton);
	XClearWindow(display, frame.nwbutton);
	
	redrawWindowLabel();
	redrawWorkspaceLabel();
	redrawPrevWorkspaceButton();
	redrawNextWorkspaceButton();
	redrawPrevWindowButton();
	redrawNextWindowButton();
	checkClock(True);

	toolbarmenu->reconfigure();

	//iconbar, no iconbar?
	if (Fluxbox::instance()->useIconBar()) {
		if (!iconbar) {
			iconbar = new IconBar(screen, frame.window_label);
			if (screen->getIconCount()) {
                BScreen::Icons & l = screen->getIconList();
				BScreen::Icons::iterator it = l.begin();
				BScreen::Icons::iterator it_end = l.end();
				for(; it != it_end; ++it) {
					addIcon(*it);
				}
			}
			
		} else 
			iconbar->reconfigure();
	} else {
		if (iconbar) {
			BScreen::Icons & l = screen->getIconList();
			BScreen::Icons::iterator it = l.begin();
			BScreen::Icons::iterator it_end = l.end();
			for(; it != it_end; ++it) {
				delIcon(*it);
			}
			delete iconbar;
			iconbar = 0;
		}
	}
}


#ifdef		HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw) {
#else // !HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw, Bool date) {
#endif // HAVE_STRFTIME
	time_t tmp = 0;
	struct tm *tt = 0;

	if ((tmp = time(NULL)) != -1) {
		if (! (tt = localtime(&tmp))) {
			cerr<<__FILE__<<"("<<__LINE__<<"): ! localtime(&tmp)"<<endl;
			return;
		}
		if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
			frame.hour = tt->tm_hour;
			frame.minute = tt->tm_min;
			XClearWindow(display, frame.clock);
			redraw = True;
		}
	} else
		cerr<<__FILE__<<"("<<__LINE__<<"): time(null)<0"<<endl;
	

	if (redraw) {
		#ifdef HAVE_STRFTIME
		char t[1024];
		if (! strftime(t, 1024, screen->getStrftimeFormat(), tt))
			return;
		#else // !HAVE_STRFTIME
		char t[9];
		if (date) {
			// format the date... with special consideration for y2k ;)
			if (screen->getDateFormat() == Blackbox::B_EuropeanDate) {
				sprintf(t,
					i18n->getMessage(
				#ifdef NLS
				 		ToolbarSet, ToolbarNoStrftimeDateFormatEu,
				#else // !NLS
						0, 0,
				#endif // NLS
					 "%02d.%02d.%02d"),
					tt->tm_mday, tt->tm_mon + 1,
					(tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
			} else {
				sprintf(t,
				i18n->getMessage(
			#ifdef		NLS
				 ToolbarSet, ToolbarNoStrftimeDateFormat,
			#else // !NLS
				 0, 0,
			#endif // NLS
				 "%02d/%02d/%02d"),
				tt->tm_mon + 1, tt->tm_mday,
				(tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
			}
		} else {
			if (screen->isClock24Hour())
	sprintf(t,
		i18n->getMessage(
#ifdef		NLS
				 ToolbarSet, ToolbarNoStrftimeTimeFormat24,
#else // !NLS
				 0, 0,
#endif // NLS
				 "	%02d:%02d "),
		frame.hour, frame.minute);
			else
	sprintf(t,
		i18n->getMessage(
		#ifdef NLS
				ToolbarSet, ToolbarNoStrftimeTimeFormat12,
		#else // !NLS
				0, 0,
		#endif // NLS
				"%02d:%02d %sm"),
		((frame.hour > 12) ? frame.hour - 12 :
		 ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
		((frame.hour >= 12) ?
		 i18n->getMessage(
		#ifdef NLS
				ToolbarSet, ToolbarNoStrftimeTimeFormatP,
		#else // !NLS
				0, 0,
		#endif // NLS
					"p") :
		 i18n->getMessage(
		#ifdef NLS
					ToolbarSet, ToolbarNoStrftimeTimeFormatA,
		#else // !NLS
					0, 0,
			#endif // NLS
					"a")));
		}
		#endif // HAVE_STRFTIME

		int dx = (frame.bevel_w * 2), dlen = strlen(t);
		unsigned int l;
		I18n *i18n = I18n::instance();
		
		if (i18n->multibyte()) {
			XRectangle ink, logical;
			XmbTextExtents(screen->getToolbarStyle()->font.set,
				 t, dlen, &ink, &logical);
			l = logical.width;
		} else
			l = XTextWidth(screen->getToolbarStyle()->font.fontstruct, t, dlen);
		
		l += (frame.bevel_w * 4);
		
		if (l > frame.clock_w) {
			for (; dlen >= 0; dlen--) {
				if (i18n->multibyte()) {
					XRectangle ink, logical;
					XmbTextExtents(screen->getToolbarStyle()->font.set,
						t, dlen, &ink, &logical);
					l = logical.width;
				} else
					l = XTextWidth(screen->getToolbarStyle()->font.fontstruct, t, dlen);
				l += (frame.bevel_w * 4);
	
				if (l < frame.clock_w)
					break;
			}

		}

		switch (screen->getToolbarStyle()->font.justify) {
		case DrawUtil::Font::RIGHT:
			dx += frame.clock_w - l;
			break;

		case DrawUtil::Font::CENTER:
			dx += (frame.clock_w - l) / 2;
			break;
		default: //LEFT
			break;
		}

		if (i18n->multibyte()) {
			XmbDrawString(display, frame.clock,
				screen->getToolbarStyle()->font.set,
				screen->getToolbarStyle()->c_text_gc, dx, 1 -
				screen->getToolbarStyle()->font.set_extents->max_ink_extent.y,
				t, dlen);
		} else {
			XDrawString(display, frame.clock,
				screen->getToolbarStyle()->c_text_gc, dx,
				screen->getToolbarStyle()->font.fontstruct->ascent + 1, t, dlen);
		}
	}
}


void Toolbar::redrawWindowLabel(Bool redraw) {
	if (Fluxbox::instance()->getFocusedWindow()) {
		if (redraw)
			XClearWindow(display, frame.window_label);

		FluxboxWindow *foc = Fluxbox::instance()->getFocusedWindow();
		if (foc->getScreen() != screen)
			return;

		int dx = (frame.bevel_w * 2), dlen = strlen(*foc->getTitle());
		unsigned int l;
		I18n *i18n = I18n::instance();
		
		if (i18n->multibyte()) {
			XRectangle ink, logical;
			XmbTextExtents(screen->getToolbarStyle()->font.set, *foc->getTitle(), dlen,
				 &ink, &logical);
			l = logical.width;
		} else
			l = XTextWidth(screen->getToolbarStyle()->font.fontstruct, *foc->getTitle(), dlen);
		
		l += (frame.bevel_w * 4);

		if (l > frame.window_label_w) {
			for (; dlen >= 0; dlen--) {
				if (i18n->multibyte()) {
					XRectangle ink, logical;
					XmbTextExtents(screen->getToolbarStyle()->font.set,
						 *foc->getTitle(), dlen, &ink, &logical);
					l = logical.width;
				} else
					l = XTextWidth(screen->getToolbarStyle()->font.fontstruct, *foc->getTitle(), dlen);
	
				l += (frame.bevel_w * 4);
	
				if (l < frame.window_label_w)
					break;
			}
		}
		switch (screen->getToolbarStyle()->font.justify) {
		case DrawUtil::Font::RIGHT:
			dx += frame.window_label_w - l;
			break;

		case DrawUtil::Font::CENTER:
			dx += (frame.window_label_w - l) / 2;
			break;
		default:
			break;
		}

		if (i18n->multibyte())
			XmbDrawString(display, frame.window_label,
				screen->getToolbarStyle()->font.set,
				screen->getToolbarStyle()->w_text_gc, dx, 1 -
				screen->getToolbarStyle()->font.set_extents->max_ink_extent.y,
				*foc->getTitle(), dlen);
		else
			XDrawString(display, frame.window_label,
			screen->getToolbarStyle()->w_text_gc, dx,
			screen->getToolbarStyle()->font.fontstruct->ascent	+ 1,
			*foc->getTitle(), dlen);
	} else
		XClearWindow(display, frame.window_label);
}
 
 
void Toolbar::redrawWorkspaceLabel(Bool redraw) {
	if (screen->getCurrentWorkspace()->getName()) {
		
		if (redraw)
			XClearWindow(display, frame.workspace_label);
	/*	DrawString(
					display, frame.label, screen->getToolbarStyle()->l_text_gc, 
					&screen->getToolbarStyle()->font,
					frame.workspace_label_w, frame.width,
				frame.bevel_w, 
				const_cast<char *>(screen->getCurrentWorkspace()->getName()));*/
		
		int dx = (frame.bevel_w * 2), dlen =
						strlen(screen->getCurrentWorkspace()->getName());
		unsigned int l;
		I18n *i18n = I18n::instance();
		
		
		if (i18n->multibyte()) {
			XRectangle ink, logical;
			XmbTextExtents(screen->getToolbarStyle()->font.set,
						screen->getCurrentWorkspace()->getName(), dlen,
						&ink, &logical);
			l = logical.width;
		} else
			l = XTextWidth(screen->getToolbarStyle()->font.fontstruct,
					screen->getCurrentWorkspace()->getName(), dlen);
		
		l += (frame.bevel_w * 4);
		
		if (l > frame.workspace_label_w) {
			for (; dlen >= 0; dlen--) {
				if (i18n->multibyte()) {
					XRectangle ink, logical;
					XmbTextExtents(screen->getToolbarStyle()->font.set,
						screen->getCurrentWorkspace()->getName(), dlen,
						&ink, &logical);
						l = logical.width;
				} else {
					l = XTextWidth(screen->getWindowStyle()->font.fontstruct,
							screen->getCurrentWorkspace()->getName(), dlen);
				}
				
				l += (frame.bevel_w * 4);
	
				if (l < frame.workspace_label_w)
					break;
			}
		}
		
		switch (screen->getToolbarStyle()->font.justify) {
		case DrawUtil::Font::RIGHT:
			dx += frame.workspace_label_w - l;
			break;

		case DrawUtil::Font::CENTER:
			dx += (frame.workspace_label_w - l) / 2;
			break;
		default:
			break;
		}

		if (i18n->multibyte()) {
			XmbDrawString(display, frame.workspace_label,
				screen->getToolbarStyle()->font.set,
				screen->getToolbarStyle()->l_text_gc, dx, 1 -
				screen->getToolbarStyle()->font.set_extents->max_ink_extent.y,
				(char *) screen->getCurrentWorkspace()->getName(), dlen);
		} else {
			XDrawString(display, frame.workspace_label,
				screen->getToolbarStyle()->l_text_gc, dx,
				screen->getToolbarStyle()->font.fontstruct->ascent + 1,
				(char *) screen->getCurrentWorkspace()->getName(), dlen);
		}
	}
}


void Toolbar::redrawPrevWorkspaceButton(Bool pressed, Bool redraw) {
	if (redraw) {
		if (pressed) {
			if (frame.pbutton)
				XSetWindowBackgroundPixmap(display, frame.psbutton, frame.pbutton);
			else
				XSetWindowBackground(display, frame.psbutton, frame.pbutton_pixel);
		} else {
			if (frame.button)
				XSetWindowBackgroundPixmap(display, frame.psbutton, frame.button);
			else
				XSetWindowBackground(display, frame.psbutton, frame.button_pixel);
		}
		XClearWindow(display, frame.psbutton);
	}

	int hh = frame.button_w / 2, hw = frame.button_w / 2;

	XPoint pts[3];
	pts[0].x = hw - 2; pts[0].y = hh;
	pts[1].x = 4; pts[1].y = 2;
	pts[2].x = 0; pts[2].y = -4;

	XFillPolygon(display, frame.psbutton, screen->getToolbarStyle()->b_pic_gc,
		pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWorkspaceButton(Bool pressed, Bool redraw) {
	if (redraw) {
		if (pressed) {
			if (frame.pbutton)
				XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.pbutton);
			else
				XSetWindowBackground(display, frame.nsbutton, frame.pbutton_pixel);
		} else {
			if (frame.button)
				XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.button);
			else
				XSetWindowBackground(display, frame.nsbutton, frame.button_pixel);
		}
		XClearWindow(display, frame.nsbutton);
	}

	int hh = frame.button_w / 2, hw = frame.button_w / 2;

	XPoint pts[3];
	pts[0].x = hw - 2; pts[0].y = hh - 2;
	pts[1].x = 4; pts[1].y =	2;
	pts[2].x = -4; pts[2].y = 2;

	XFillPolygon(display, frame.nsbutton, screen->getToolbarStyle()->b_pic_gc,
		pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawPrevWindowButton(Bool pressed, Bool redraw) {
	if (redraw) {
		if (pressed) {
			if (frame.pbutton)
				XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.pbutton);
			else
				XSetWindowBackground(display, frame.pwbutton, frame.pbutton_pixel);
		} else {
			if (frame.button)
				XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.button);
			else
				XSetWindowBackground(display, frame.pwbutton, frame.button_pixel);
		}
		XClearWindow(display, frame.pwbutton);
	}

	int hh = frame.button_w / 2, hw = frame.button_w / 2;

	XPoint pts[3];
	pts[0].x = hw - 2; pts[0].y = hh;
	pts[1].x = 4; pts[1].y = 2;
	pts[2].x = 0; pts[2].y = -4;

	XFillPolygon(display, frame.pwbutton, screen->getToolbarStyle()->b_pic_gc,
							 pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWindowButton(Bool pressed, Bool redraw) {
	if (redraw) {
		if (pressed) {
			if (frame.pbutton)
				XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.pbutton);
			else
				XSetWindowBackground(display, frame.nwbutton, frame.pbutton_pixel);
		} else {
			if (frame.button)
				XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.button);
			else
				XSetWindowBackground(display, frame.nwbutton, frame.button_pixel);
		}
		XClearWindow(display, frame.nwbutton);
	}

	int hh = frame.button_w / 2, hw = frame.button_w / 2;

	XPoint pts[3];
	pts[0].x = hw - 2; pts[0].y = hh - 2;
	pts[1].x = 4; pts[1].y =	2;
	pts[2].x = -4; pts[2].y = 2;

	XFillPolygon(display, frame.nwbutton, screen->getToolbarStyle()->b_pic_gc,
		pts, 3, Convex, CoordModePrevious);
}


void Toolbar::edit(void) {
	Window window;
	int foo;

	editing = True;	//mark for editing
	
	//workspace labe already got intput focus ?
	if (XGetInputFocus(display, &window, &foo) &&
			window == frame.workspace_label)
		return;

	//set input focus to workspace label
	XSetInputFocus(display, frame.workspace_label,
		((screen->isSloppyFocus() || screen->isSemiSloppyFocus()) ?
		RevertToPointerRoot : RevertToParent), CurrentTime);

	XClearWindow(display, frame.workspace_label);	//clear workspace text

	fluxbox->setNoFocus(True);
	if (fluxbox->getFocusedWindow())	//disable focus on current focused window
		fluxbox->getFocusedWindow()->setFocusFlag(False);

	XDrawRectangle(display, frame.workspace_label,
		screen->getWindowStyle()->l_text_focus_gc,
		frame.workspace_label_w / 2, 0, 1,
		frame.label_h - 1);
}


void Toolbar::buttonPressEvent(XButtonEvent *be) {
	FluxboxWindow *fluxboxwin=0;
	if (be->button == 1) {
		if (be->window == frame.psbutton)
			redrawPrevWorkspaceButton(True, True);
		else if (be->window == frame.nsbutton)
			redrawNextWorkspaceButton(True, True);
		else if (be->window == frame.pwbutton)
			redrawPrevWindowButton(True, True);
		else if (be->window == frame.nwbutton)
			redrawNextWindowButton(True, True);
		else if ( iconbar ) {
			if ( (fluxboxwin = iconbar->findWindow(be->window)) )
				fluxboxwin->deiconify();
		}
#ifndef	 HAVE_STRFTIME
		else if (be->window == frame.clock) {
			XClearWindow(display, frame.clock);
			checkClock(True, True);
		}
#endif // HAVE_STRFTIME
		else if (! on_top) {
			Window w[1] = { frame.window };
			screen->raiseWindows(w, 1);
		}
	} else if (be->button == 2 && (! on_top)) {
		XLowerWindow(display, frame.window);
	} else if (be->button == 3) {
		if (! toolbarmenu->isVisible()) {
			int x, y;

			x = be->x_root - (toolbarmenu->getWidth() / 2);
			y = be->y_root - (toolbarmenu->getHeight() / 2);

			if (x < 0)
				x = 0;
			else if (x + toolbarmenu->getWidth() > screen->getWidth())
				x = screen->getWidth() - toolbarmenu->getWidth();

			if (y < 0)
				y = 0;
			else if (y + toolbarmenu->getHeight() > screen->getHeight())
				y = screen->getHeight() - toolbarmenu->getHeight();

			toolbarmenu->move(x, y);
			toolbarmenu->show();
		} else
			toolbarmenu->hide();
			
	} 
	
}


void Toolbar::buttonReleaseEvent(XButtonEvent *re) {
	if (re->button == 1) {
		if (re->window == frame.psbutton) {
			redrawPrevWorkspaceButton(False, True);

			if (re->x >= 0 && re->x < (signed) frame.button_w &&
					re->y >= 0 && re->y < (signed) frame.button_w)
			 screen->prevWorkspace();
		} else if (re->window == frame.nsbutton) {
			redrawNextWorkspaceButton(False, True);

			if (re->x >= 0 && re->x < (signed) frame.button_w &&
					re->y >= 0 && re->y < (signed) frame.button_w)
				screen->nextWorkspace();
		} else if (re->window == frame.pwbutton) {
			redrawPrevWindowButton(False, True);

			if (re->x >= 0 && re->x < (signed) frame.button_w &&
					re->y >= 0 && re->y < (signed) frame.button_w)
				screen->prevFocus();
		} else if (re->window == frame.nwbutton) {
			redrawNextWindowButton(False, True);

			if (re->x >= 0 && re->x < (signed) frame.button_w &&
					re->y >= 0 && re->y < (signed) frame.button_w)
				screen->nextFocus();
		} else if (re->window == frame.window_label)
			screen->raiseFocus();
#ifndef	 HAVE_STRFTIME
		else if (re->window == frame.clock) {
			XClearWindow(display, frame.clock);
			checkClock(True);
		}
#endif // HAVE_STRFTIME
	} else if (re->button == 4) //mousewheel scroll up
		screen->nextWorkspace();
	else if (re->button == 5)	//mousewheel scroll down
		screen->prevWorkspace();
}


void Toolbar::enterNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (! hide_timer->isTiming()) hide_timer->start();
	} else {
		if (hide_timer->isTiming()) hide_timer->stop();
	}
}

void Toolbar::leaveNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (hide_timer->isTiming()) hide_timer->stop();
	} else if (! toolbarmenu->isVisible()) {
		if (! hide_timer->isTiming()) hide_timer->start();
	}
}


void Toolbar::exposeEvent(XExposeEvent *ee) {
	if (ee->window == frame.clock) checkClock(True);
	else if (ee->window == frame.workspace_label && (! editing))
		redrawWorkspaceLabel();
	else if (ee->window == frame.window_label) redrawWindowLabel();
	else if (ee->window == frame.psbutton) redrawPrevWorkspaceButton();
	else if (ee->window == frame.nsbutton) redrawNextWorkspaceButton();
	else if (ee->window == frame.pwbutton) redrawPrevWindowButton();
	else if (ee->window == frame.nwbutton) redrawNextWindowButton();
	else if (iconbar)//try iconbar
		iconbar->exposeEvent(ee);
}


void Toolbar::keyPressEvent(XKeyEvent *ke) {
	if (ke->window == frame.workspace_label && editing) {
		BaseDisplay::GrabGuard gg(*fluxbox);
		fluxbox->grab();

		KeySym ks;
		char keychar[1];
		XLookupString(ke, keychar, 1, &ks, 0);

		if (ks == XK_Return || ks == XK_Escape) {
			

			editing = False;
			
			fluxbox->setNoFocus(False);
			if (fluxbox->getFocusedWindow()) {
				fluxbox->getFocusedWindow()->setInputFocus();
				fluxbox->getFocusedWindow()->setFocusFlag(True);
			} else
				XSetInputFocus(display, PointerRoot, None, CurrentTime);
			
			if (ks == XK_Return)	//change workspace name if keypress = Return
				screen->getCurrentWorkspace()->setName(const_cast<char *>(new_workspace_name.c_str()));

			new_workspace_name.erase(); //erase temporary workspace name
			
			screen->getCurrentWorkspace()->getMenu()->hide();
			screen->getWorkspacemenu()->
				remove(screen->getCurrentWorkspace()->getWorkspaceID() + 2);
			screen->getWorkspacemenu()->
				insert(screen->getCurrentWorkspace()->getName(),
			screen->getCurrentWorkspace()->getMenu(),
			screen->getCurrentWorkspace()->getWorkspaceID() + 1);
			screen->getWorkspacemenu()->update();

			reconfigure();
		} else if (! (ks == XK_Shift_L || ks == XK_Shift_R ||
			ks == XK_Control_L || ks == XK_Control_R ||
			ks == XK_Caps_Lock || ks == XK_Shift_Lock ||
			ks == XK_Meta_L || ks == XK_Meta_R ||
			ks == XK_Alt_L || ks == XK_Alt_R ||
			ks == XK_Super_L || ks == XK_Super_R ||
			ks == XK_Hyper_L || ks == XK_Hyper_R)) {

			if (ks == XK_BackSpace && new_workspace_name.size())
				new_workspace_name.erase(new_workspace_name.size()-1);
			else
				new_workspace_name += keychar[0];


			XClearWindow(display, frame.workspace_label);
			int l = new_workspace_name.size(), tw, x;
			I18n *i18n = I18n::instance();
			
			if (i18n->multibyte()) {
				XRectangle ink, logical;
				XmbTextExtents(screen->getToolbarStyle()->font.set,
					new_workspace_name.c_str(), l, &ink, &logical);
				tw = logical.width;
			} else
				tw = XTextWidth(screen->getToolbarStyle()->font.fontstruct,
					new_workspace_name.c_str(), l);
			
				x = (frame.workspace_label_w - tw) / 2;

			if (x < (signed) frame.bevel_w) x = frame.bevel_w;

			if (i18n->multibyte())
				XmbDrawString(display, frame.workspace_label,
					screen->getWindowStyle()->font.set,
					screen->getWindowStyle()->l_text_focus_gc, x, 1 -
					screen->getWindowStyle()->font.set_extents->max_ink_extent.y,
					new_workspace_name.c_str(), l);
			else
				XDrawString(display, frame.workspace_label,
					screen->getWindowStyle()->l_text_focus_gc, x,
					screen->getToolbarStyle()->font.fontstruct->ascent + 1,
					new_workspace_name.c_str(), l);
			
			XDrawRectangle(display, frame.workspace_label,
				screen->getWindowStyle()->l_text_focus_gc, x + tw, 0, 1,
				frame.label_h - 1);
		}
		
		fluxbox->ungrab();
	}
}


void Toolbar::timeout(void) {
	checkClock(True);

	timeval now;
	gettimeofday(&now, 0);
	clock_timer->setTimeout((60 - (now.tv_sec % 60)) * 1000);
}


void Toolbar::HideHandler::timeout(void) {
	toolbar->hidden = ! toolbar->hidden;
	if (toolbar->hidden)
		XMoveWindow(toolbar->display, toolbar->frame.window,
		toolbar->frame.x_hidden, toolbar->frame.y_hidden);
	else
		XMoveWindow(toolbar->display, toolbar->frame.window,
		toolbar->frame.x, toolbar->frame.y);
}


Toolbarmenu::Toolbarmenu(Toolbar *tb) : Basemenu(tb->screen) {
	toolbar = tb;
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
				ToolbarSet, ToolbarToolbarTitle,
#else // !NLS
				0, 0,
#endif // NLS
				"Toolbar"));
	setInternalMenu();

	placementmenu = new Placementmenu(this);

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTitle,
#else // !NLS
				0, 0,
#endif // NLS
				"Placement"),
				placementmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonAlwaysOnTop,
#else // !NLS
				0, 0,
#endif // NLS
				"Always on top"),
				1);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonAutoHide,
#else // !NLS
				0, 0,
#endif // NLS
				"Auto hide"),
				2);
	insert(i18n->getMessage(
#ifdef		NLS
				ToolbarSet, ToolbarEditWkspcName,
#else // !NLS
				0, 0,
#endif // NLS
				"Edit current workspace name"),
				3);

	update();

	if (toolbar->isOnTop())
		setItemSelected(1, True);
	if (toolbar->doAutoHide())
		setItemSelected(2, True);
}


Toolbarmenu::~Toolbarmenu(void) {
	delete placementmenu;
}


void Toolbarmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		switch (item->function()) {
		case 1: // always on top
			{
			Bool change = ((toolbar->isOnTop()) ? False : True);
			toolbar->on_top = change;
				setItemSelected(1, change);

			if (toolbar->isOnTop()) toolbar->screen->raiseWindows((Window *) 0, 0);
			break;
			}

		case 2: // auto hide
			{
	Bool change = ((toolbar->doAutoHide()) ?	False : True);
	toolbar->do_auto_hide = change;
	setItemSelected(2, change);

#ifdef		SLIT
	toolbar->screen->getSlit()->reposition();
#endif // SLIT
	break;
			}

		case 3: // edit current workspace name
			toolbar->edit();	//set edit mode
			hide();	//dont show menu while editing name

			break;
		}
	}
}


void Toolbarmenu::internal_hide(void) {
	Basemenu::internal_hide();
	if (toolbar->doAutoHide() && ! toolbar->isEditing())
		toolbar->hide_handler.timeout();
}


void Toolbarmenu::reconfigure(void) {
	placementmenu->reconfigure();

	Basemenu::reconfigure();
}


Toolbarmenu::Placementmenu::Placementmenu(Toolbarmenu *tm)
	: Basemenu(tm->toolbar->screen) {
	toolbarmenu = tm;
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
					ToolbarSet, ToolbarToolbarPlacement,
#else // !NLS
					0, 0,
#endif // NLS
					"Toolbar Placement"));
	setInternalMenu();
	setMinimumSublevels(3);

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Left"),
				Toolbar::TOPLEFT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Left"),
				Toolbar::BOTTOMLEFT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Center"),
				Toolbar::TOPCENTER);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Center"),
				Toolbar::BOTTOMCENTER);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Right"),
				Toolbar::TOPRIGHT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Right"),
	Toolbar::BOTTOMRIGHT);

	update();
}


void Toolbarmenu::Placementmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item)
			return;

		toolbarmenu->toolbar->screen->saveToolbarPlacement(
			static_cast<Toolbar::Placement>(item->function()));
		hide();
		toolbarmenu->toolbar->reconfigure();

#ifdef		SLIT
		// reposition the slit as well to make sure it doesn't intersect the
		// toolbar
		toolbarmenu->toolbar->screen->getSlit()->reposition();
#endif // SLIT

	}
}
