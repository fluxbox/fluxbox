// Tab.cc for Fluxbox
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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

#include "Tab.hh"
#include <iostream>
#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H
#include "i18n.hh"

#include "misc.hh"

using namespace std;

bool Tab::m_stoptabs = false;
Tab::t_tabplacementlist Tab::m_tabplacementlist[] = {
		{PTop, "Top"},
		{PBottom, "Bottom"},
		{PLeft, "Left"},
		{PRight, "Right"},
		{pnone, "none"}
	};

Tab::t_tabplacementlist Tab::m_tabalignmentlist[] = {
		{ALeft, "Left"},
		{ACenter, "Center"},
		{ARight, "Right"},
		{ARelative, "Relative"},
		{anone, "none"}
	};

Tab::Tab(FluxboxWindow *win, Tab *prev, Tab *next) { 
	//set default values
	
	m_focus = m_moving = false;
	m_configured = true; // only set to false before Fluxbox::reconfigure
	m_move_x = m_move_y = 0;	
	m_prev = prev; m_next = next; 
	m_win = win;
	m_display = Fluxbox::instance()->getXDisplay();
	
	if ((m_win->screen->getTabPlacement() == PLeft ||
			m_win->screen->getTabPlacement() == PRight) &&
			m_win->screen->isTabRotateVertical()) {
		m_size_w = Fluxbox::instance()->getTabHeight();
		m_size_h = Fluxbox::instance()->getTabWidth();
	} else {
		m_size_w = Fluxbox::instance()->getTabWidth();
		m_size_h = Fluxbox::instance()->getTabHeight();
	}

	createTabWindow();

	calcIncrease();
}

Tab::~Tab() {

	disconnect();	
	
	Fluxbox::instance()->removeTabSearch(m_tabwin);
	XDestroyWindow(m_display, m_tabwin);
}


//----------------  createTabWindow ---------------
// (private)
// Creates the Window for tab to be above the title window.
// This should only be called by the constructor.
//-------------------------------------------------
void Tab::createTabWindow() {
	unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
			      CWColormap | CWOverrideRedirect | CWEventMask;
	XSetWindowAttributes attrib;
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
			m_win->screen->getWindowStyle()->tab.border_color.getPixel();
	attrib.colormap = m_win->screen->getColormap();
	attrib.override_redirect = True;
	attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
						ButtonMotionMask | ExposureMask | EnterWindowMask;
	//Notice that m_size_w gets the TOTAL width of tabs INCLUDING borders
	m_tabwin = XCreateWindow(m_display, m_win->screen->getRootWindow(), 
							-30000, -30000, //TODO: So that it wont flicker or
											// appear before the window do
			m_size_w - m_win->screen->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->screen->getWindowStyle()->tab.border_width_2x,
							m_win->screen->getWindowStyle()->tab.border_width,
							m_win->screen->getDepth(), InputOutput,
							m_win->screen->getVisual(), attrib_mask, &attrib);
	//set grab
	XGrabButton(m_display, Button1, Mod1Mask, m_tabwin, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, Fluxbox::instance()->getMoveCursor());
	
	
	//save to tabsearch
	Fluxbox::instance()->saveTabSearch(m_tabwin, this);	

	XMapSubwindows(m_display, m_tabwin);
		
	XMapWindow(m_display, m_tabwin);

	decorate();
}

//-------------- focus --------------------
// Called when the focus changes in m_win 
// updates pixmap or color and draws the tab
//-----------------------------------------
void Tab::focus() {

	if (m_win->focused) {
		if (m_focus_pm)
			XSetWindowBackgroundPixmap(m_display, m_tabwin, m_focus_pm);
		else
			XSetWindowBackground(m_display, m_tabwin, m_focus_pixel);
	} else {
		if (m_unfocus_pm)
			XSetWindowBackgroundPixmap(m_display, m_tabwin, m_unfocus_pm);
		else
			XSetWindowBackground(m_display, m_tabwin, m_unfocus_pixel);
	}
	XClearWindow(m_display, m_tabwin);	
	draw(false);
}

//-------------- raise --------------------
// Raises the tabs in the tablist
//-----------------------------------------
void Tab::raise() {
	//get first tab
	Tab *first = 0;
	first = getFirst(this);
	//raise tabs
	for (; first!=0; first = first->m_next)
		m_win->screen->raiseWindows(&first->m_tabwin, 1);
}

//-------------- decorate --------------------
// decorates the tab with current theme
// TODO optimize this
//--------------------------------------------
void Tab::decorate() {
	
	BImageControl *image_ctrl = m_win->screen->getImageControl();
	Pixmap tmp = m_focus_pm;
	BTexture *texture = &(m_win->screen->getWindowStyle()->tab.l_focus);

 	if (texture->getTexture() & BImage_ParentRelative ) {
		BTexture *pt = &(m_win->screen->getWindowStyle()->tab.t_focus);
		if (pt->getTexture() == (BImage_Flat | BImage_Solid)) {
  		  m_focus_pm = None;
	    m_focus_pixel = pt->getColor()->getPixel();
  	} else
    	m_focus_pm =
	      image_ctrl->renderImage(m_size_w, m_size_h, pt);
  	if (tmp) image_ctrl->removeImage(tmp);
		
	} else {
		if (texture->getTexture() == (BImage_Flat | BImage_Solid)) {
  		  m_focus_pm = None;
	    m_focus_pixel = texture->getColor()->getPixel();
  	} else
    	m_focus_pm =
	      image_ctrl->renderImage(m_size_w, m_size_h, texture);
  	if (tmp) image_ctrl->removeImage(tmp);
	}
	
	tmp = m_unfocus_pm;
	texture = &(m_win->screen->getWindowStyle()->tab.l_unfocus);
  
	if (texture->getTexture() & BImage_ParentRelative ) {
		BTexture *pt = &(m_win->screen->getWindowStyle()->tab.t_unfocus);
		if (pt->getTexture() == (BImage_Flat | BImage_Solid)) {
			m_unfocus_pm = None;
			m_unfocus_pixel = pt->getColor()->getPixel();
  		} else
			m_unfocus_pm =
			image_ctrl->renderImage(m_size_w, m_size_h, pt);
	} else {
		if (texture->getTexture() == (BImage_Flat | BImage_Solid)) {
			m_unfocus_pm = None;
			m_unfocus_pixel = texture->getColor()->getPixel();
  		} else
			m_unfocus_pm =
				image_ctrl->renderImage(m_size_w, m_size_h, texture);
	}
	
	if (tmp) image_ctrl->removeImage(tmp);
	
	XSetWindowBorderWidth(m_display, m_tabwin,
		m_win->screen->getWindowStyle()->tab.border_width);
	XSetWindowBorder(m_display, m_tabwin,
		m_win->screen->getWindowStyle()->tab.border_color.getPixel());
}


//-------------- deiconify -------------------
// Deiconifies the tab
// Used from FluxboxWindow to deiconify the tab when the window is deiconfied
//--------------------------------------------
void Tab::deiconify() {
	XMapWindow(m_display, m_tabwin);
}

//------------- iconify --------------------
// Iconifies the tab.
// Used from FluxboxWindow to hide tab win when window is iconified
// disconnects itself from the list
//------------------------------------------
void Tab::iconify() {
	disconnect();
	withdraw();
}

//------------ withdraw --------------
// Unmaps the tab from display
//------------------------------------
void Tab::withdraw() {
	XUnmapWindow(m_display, m_tabwin);	
}

//------------ stick --------------------
// Set/reset the the sticky on all windows in the list
//---------------------------------------
void Tab::stick() {
 
	//get first tab
	Tab *first = 0;
	first = getFirst(this);
	
	//now do stick for all windows in the list
	for (; first!=0; first = first->m_next) {
		FluxboxWindow *win = first->m_win; //just for convenient
		if (win->stuck) {
			win->blackbox_attrib.flags ^= AttribOmnipresent;
			win->blackbox_attrib.attrib ^= AttribOmnipresent;
			win->stuck = false;
			if (!win->iconic)
      	win->screen->reassociateWindow(win, -1, true);
				    
		} else {
			win->stuck = true;
			win->blackbox_attrib.flags |= AttribOmnipresent;
			win->blackbox_attrib.attrib |= AttribOmnipresent;
		}
		
		win->setState(win->current_state);
	}	

}

//------------- resize -------------
// Resize the window's in the tablist
//----------------------------------
void Tab::resize() {
	Tab *first = getFirst(this);

	//now move and resize the windows in the list
	for (; first != 0; first = first->m_next) {
		if (first!=this) {
			first->m_win->configure(m_win->frame.x, m_win->frame.y,
								m_win->frame.width, m_win->frame.height);
		}
	}

	// need to resize tabs if in relative mode
	if (m_win->screen->getTabAlignment() == ARelative) {
		calcIncrease();
		setPosition();
	}
}

//----------- shade --------------
// Shades the windows in the tablist
//--------------------------------
void Tab::shade() {
	for(Tab *first = getFirst(this); first != 0; first = first->m_next) {
		if (first==this)
			continue;
			first->m_win->shade();
	}

	if (m_win->screen->getTabPlacement() == PLeft ||
			m_win->screen->getTabPlacement() == PRight) { 
		resizeGroup();
		calcIncrease();
	}

	if (!(m_win->screen->getTabPlacement() == PTop))
		setPosition();
}

//------------ draw -----------------
// Draws the tab 
// if pressed = true then it draws the tab in pressed 
// mode else it draws it in normal mode
// TODO: the "draw in pressed mode" 
//-----------------------------------
void Tab::draw(bool pressed) {	
	unsigned int tabtext_w;

	GC gc = ((m_win->focused) ? m_win->screen->getWindowStyle()->tab.l_text_focus_gc :
		   m_win->screen->getWindowStyle()->tab.l_text_unfocus_gc);

	// Different routines for drawing rotated text
	if ((m_win->screen->getTabPlacement() == PLeft ||
			m_win->screen->getTabPlacement() == PRight) &&
			(!m_win->isShaded() && m_win->screen->isTabRotateVertical())) {

		tabtext_w = Misc::XRotTextWidth(m_win->screen->getWindowStyle()->tab.rot_font,
						m_win->client.title, m_win->client.title_len);
		tabtext_w += (m_win->frame.bevel_w * 4);

		Misc::DrawRotString(m_display, m_tabwin, gc,
				m_win->screen->getWindowStyle()->tab.rot_font,
				m_win->screen->getWindowStyle()->tab.font.justify,
				tabtext_w, m_size_w, m_size_h,
				m_win->frame.bevel_w, m_win->client.title);

	} else {
		if (I18n::instance()->multibyte()) { // TODO: maybe move this out from here?
			XRectangle ink, logical;
			XmbTextExtents(m_win->screen->getWindowStyle()->tab.font.set,
					m_win->client.title, m_win->client.title_len,
					&ink, &logical);
			tabtext_w = logical.width;
		} else {
			tabtext_w = XTextWidth(
					m_win->screen->getWindowStyle()->tab.font.fontstruct,
					m_win->client.title, m_win->client.title_len);
		}
		tabtext_w += (m_win->frame.bevel_w * 4);

		Misc::DrawString(m_display, m_tabwin, gc,
				&m_win->screen->getWindowStyle()->tab.font,
				tabtext_w, m_size_w,
				m_win->frame.bevel_w, m_win->client.title);
	}
}

//------------- setPosition -----------------
// Position tab ( follow the m_win pos ).
// (and resize)
// Set new position of the other tabs in the chain
//-------------------------------------------
void Tab::setPosition() {	
	//don't do anything if the tablist is freezed
	if (m_stoptabs)
		return;

	Tab *first = 0;
	int pos_x = 0, pos_y = 0;
	
	m_stoptabs = true; //freeze tablist
	
	//and check for max tabs
	//TODO: optimize! There is many ways to implement this...		
	//posible movement to a static member function

	//Tab placement
	if (m_win->screen->getTabPlacement() == PTop) {
		pos_y = m_win->frame.y - m_size_h; 

	} else if (m_win->screen->getTabPlacement() == PBottom ||
				m_win->isShaded()) { 
		if (m_win->isShaded())
			pos_y = m_win->frame.y + m_win->getTitleHeight() +
					m_win->screen->getBorderWidth2x();
		else
			pos_y = m_win->frame.y + m_win->getHeight() +
					m_win->screen->getBorderWidth2x();

	} else if (m_win->screen->getTabPlacement() == PLeft) {
		pos_x = m_win->frame.x - m_size_w;
			
	} else if (m_win->screen->getTabPlacement() == PRight) {
		pos_x = m_win->frame.x + m_win->frame.width +
				m_win->screen->getBorderWidth2x();	
	}

	//Tab alignment
	if (m_win->screen->getTabPlacement() == PTop ||
			m_win->screen->getTabPlacement() == PBottom ||
			m_win->isShaded()) {
		switch(m_win->screen->getTabAlignment()) {
			case ARelative:
			case ALeft:
				pos_x = m_win->frame.x; 
				break;
			case ACenter:
				pos_x = calcCenterXPos(); 
				break;
			case ARight:
				pos_x = m_win->frame.x + m_win->frame.width +
						m_win->screen->getBorderWidth2x() - m_size_w;
				break;
		}
	} else { //PLeft | PRight
		switch(m_win->screen->getTabAlignment()) {
			case ALeft:
				pos_y = m_win->frame.y - m_size_h + m_win->frame.height +
						m_win->screen->getBorderWidth2x();
				break;
			case ACenter:
				pos_y = calcCenterYPos();
				break;
			case ARelative:
			case ARight:
				pos_y = m_win->frame.y;
				break;
		}
	}
	
	for (first = getFirst(this);
			first!=0; 
			pos_x += first->m_inc_x, pos_y += first->m_inc_y,
			first = first->m_next){
		
		XMoveWindow(m_display, first->m_tabwin, pos_x, pos_y);
				
		//dont move fluxboxwindow if the itterator = this
		if (first!=this) {
			first->m_win->configure(m_win->frame.x, m_win->frame.y, 
					m_win->frame.width, m_win->frame.height);
		}	
	}	

	m_stoptabs = false;
}

//------------- calcIncrease ----------------
// calculates m_inc_x and m_inc_y for tabs
// used for positioning the tabs.
//-------------------------------------------
void Tab::calcIncrease(void) {
	#ifdef DEBUG
	cerr << "Calculating tab increase" << endl;
	#endif // DEBUG

	Tab *first;
	int inc_x = 0, inc_y = 0;
	unsigned int tabs = 0, i = 0;

	if (m_win->screen->getTabPlacement() == PTop ||
			m_win->screen->getTabPlacement() == PBottom ||
			m_win->isShaded()) {
		inc_y = 0;

		switch(m_win->screen->getTabAlignment()) {
			case ALeft:
				inc_x = m_size_w;
				break;
			case ACenter:
				inc_x = m_size_w;
				break;
			case ARight:
				inc_x = -m_size_w;
				break;
			case ARelative:
				inc_x = calcRelativeWidth();
				break;
		}
	} else if (m_win->screen->getTabPlacement() == PLeft ||
					m_win->screen->getTabPlacement() == PRight) {
		inc_x = 0;

		switch(m_win->screen->getTabAlignment()) {
			case ALeft:
				inc_y = -m_size_h;
				break;
			case ACenter:
				inc_y = m_size_h;
				break;
			case ARight:
				inc_y = m_size_h;
				break;
			case ARelative:
				inc_y = calcRelativeHeight();
				break;
		}
	}

	for (first = getFirst(this); first!=0; first = first->m_next, tabs++);

	for (first = getFirst(this); first!=0; first = first->m_next, i++){

	//TODO: move this out from here?
		if ((m_win->screen->getTabPlacement() == PTop ||
				m_win->screen->getTabPlacement() == PBottom ||
				m_win->isShaded()) &&
				m_win->screen->getTabAlignment() == ARelative) {
			if (!((m_win->frame.width +
					m_win->screen->getBorderWidth2x()) % tabs) ||
					i >= ((m_win->frame.width +
					m_win->screen->getBorderWidth2x()) % tabs)) {
				first->setTabWidth(inc_x);
				first->m_inc_x = inc_x;
			} else { // adding 1 extra pixel to get tabs like win width
				first->setTabWidth(inc_x + 1);
				first->m_inc_x = inc_x + 1;
			}
			first->m_inc_y = inc_y;
		} else if (m_win->screen->getTabAlignment() == ARelative) {
			if (!((m_win->frame.height +
				m_win->screen->getBorderWidth2x()) % tabs) ||
				i >= ((m_win->frame.height +
				m_win->screen->getBorderWidth2x()) % tabs)) {

				first->setTabHeight(inc_y);
				first->m_inc_y = inc_y;
			} else {
				// adding 1 extra pixel to get tabs match window width
				first->setTabHeight(inc_y + 1);
				first->m_inc_y = inc_y + 1;
			}
			first->m_inc_x = inc_x;
		} else { // non relative modes
			first->m_inc_x = inc_x;
			first->m_inc_y = inc_y;
		}
	}
}

//------------- buttonPressEvent -----------
// Handle button press event here.
//------------------------------------------
void Tab::buttonPressEvent(XButtonEvent *be) {	
	//draw in pressed mode
	draw(true);
	
	//set window to titlewindow so we can take advatage of drag function
	be->window = m_win->frame.title;
	
	//call windows buttonpress eventhandler
	m_win->buttonPressEvent(be);
}

//----------- buttonReleaseEvent ----------
// Handle button release event here.
// If tab is dropped then it should try to find
// the window where the tab where dropped.
//-----------------------------------------
void Tab::buttonReleaseEvent(XButtonEvent *be) {		
	
	if (m_moving) {
		
		m_moving = false; 
		
		//erase tabmoving rectangle
		XDrawRectangle(m_display, m_win->screen->getRootWindow(),
			m_win->screen->getOpGC(),
			m_move_x, m_move_y, 
			m_size_w, m_size_h);
		
		Fluxbox::instance()->ungrab();
		XUngrabPointer(m_display, CurrentTime);
		
		//storage of window and pos of window where we dropped the tab
		Window child;
		int dest_x = 0, dest_y = 0;
		
		//find window on coordinates of buttonReleaseEvent
		if (XTranslateCoordinates(m_display, m_win->screen->getRootWindow(), 
							m_win->screen->getRootWindow(),
							be->x_root, be->y_root, &dest_x, &dest_y, &child)) {
			
			Tab *tab = 0;
			//search tablist for a tabwindow
			if ((tab = Fluxbox::instance()->searchTab(child))!=0)  {
				// do only attach a hole chain if we dropped the
				// first tab in the dropped chain...

				if (m_prev)
					disconnect();
				
				// attach this tabwindow chain to the tabwindow chain we found.								
				tab->insert(this);

			} else {	
				disconnect();

				// (ab)using dest_x and dest_y
				switch(m_win->screen->getTabPlacement()) {
					case PTop:
						dest_x = be->x_root;
						dest_y = be->y_root;
						switch(m_win->screen->getTabAlignment()) {
						case ACenter:
							dest_x -= (m_win->frame.width / 2) - (m_size_w / 2);
							break;
						case ARight:
							dest_x -=  m_win->frame.width - m_size_w;
						break;
						}
					break;
					case PBottom:
						dest_x = be->x_root;
						dest_y = be->y_root - m_win->frame.height;
						switch(m_win->screen->getTabAlignment()) {
						case ACenter:
							dest_x -= (m_win->frame.width / 2) - (m_size_w / 2);
							break;
						case ARight:
							dest_x -=  m_win->frame.width - m_size_w;
						break;
						}
					break;
					case PLeft:
						dest_x = be->x_root;
						dest_y = be->y_root;
						switch(m_win->screen->getTabAlignment()) {
						case ACenter:
							dest_y -= (m_win->frame.height / 2) - (m_size_h / 2);
							break;
						case ALeft:
							dest_y -= m_win->frame.height + m_size_h;
							break;
						}
					break;
					case PRight:
						dest_x = be->x_root - m_win->frame.width;
						dest_y = be->y_root;
						switch(m_win->screen->getTabAlignment()) {
						case ACenter:
							dest_y -= (m_win->frame.height / 2) - (m_size_h / 2);
							break;
						case ALeft:
							dest_y -= m_win->frame.height + m_size_h;
							break;
						}
					break;
				}
				//TODO: this causes an calculate increase event, even if
				// only moving a tab!
				m_win->configure(dest_x, dest_y, m_win->frame.width, m_win->frame.height);
			}
		}
	} else {
		
		//raise this tabwindow
		raise();
		
		//set window to title window soo we can use m_win handler for menu
		be->window = m_win->frame.title;
		
		//call windows buttonrelease event handler so it can popup a menu if needed
		m_win->buttonReleaseEvent(be);
	}
	
}

//------------- exposeEvent ------------
// Handle expose event here.
// Draws the tab unpressed
//--------------------------------------
void Tab::exposeEvent(XExposeEvent *ee) {
	draw(false);
}

//----------- motionNotifyEvent --------
// Handles motion event here
// Draws the rectangle of moving tab
//--------------------------------------
void Tab::motionNotifyEvent(XMotionEvent *me) {
	
	Fluxbox *fluxbox = Fluxbox::instance();
	
	//if mousebutton 2 is pressed
	if (me->state & Button2Mask) {
		if (!m_moving) {
			m_moving = true; 
			
			XGrabPointer(m_display, me->window, False, Button2MotionMask |
								ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
								None, fluxbox->getMoveCursor(), CurrentTime);

			fluxbox->grab();

			m_move_x = me->x_root - 1;
			m_move_y = me->y_root - 1;
				
			XDrawRectangle(m_display, m_win->screen->getRootWindow(), 
					m_win->screen->getOpGC(),
					m_move_x, m_move_y,
					m_size_w, m_size_h);
	
		} else {
		
			int dx = me->x_root - 1, dy = me->y_root - 1;

			dx -= m_win->screen->getBorderWidth();
			dy -= m_win->screen->getBorderWidth();

			if (m_win->screen->getEdgeSnapThreshold()) {
				int drx = m_win->screen->getWidth() - (dx + 1);

				if (dx > 0 && dx < drx && dx < m_win->screen->getEdgeSnapThreshold()) 
					dx = 0;
				else if (drx > 0 && drx < m_win->screen->getEdgeSnapThreshold())
					dx = m_win->screen->getWidth() - 1;

				int dtty, dbby, dty, dby;
		
				switch (m_win->screen->getToolbarPlacement()) {
				case Toolbar::TopLeft:
				case Toolbar::TopCenter:
				case Toolbar::TopRight:
					dtty = m_win->screen->getToolbar()->getExposedHeight() +
							m_win->screen->getBorderWidth();
					dbby = m_win->screen->getHeight();
					break;

				default:
					dtty = 0;
					dbby = m_win->screen->getToolbar()->getY();
					break;
				}
		
				dty = dy - dtty;
				dby = dbby - (dy + 1);

				if (dy > 0 && dty < m_win->screen->getEdgeSnapThreshold())
					dy = dtty;
				else if (dby > 0 && dby < m_win->screen->getEdgeSnapThreshold())
					dy = dbby - 1;
		
			}
		
			//erase rectangle
			XDrawRectangle(m_display, m_win->screen->getRootWindow(),
					m_win->screen->getOpGC(),
					m_move_x, m_move_y, 
					m_size_w, m_size_h);

			//redraw rectangle at new pos
			m_move_x = dx;
			m_move_y = dy;			
			XDrawRectangle(m_display, m_win->screen->getRootWindow(), 
					m_win->screen->getOpGC(),
					m_move_x, m_move_y,
					m_size_w, m_size_h);					
			
		}
	} 
}

//-------------- getFirst() ---------
// Returns the first Tab in the chain 
// of currentchain. 
//-----------------------------------
Tab *Tab::getFirst(Tab *current) {
	if (current==0)
		return 0;
	for (; current->m_prev != 0; current = current->m_prev);
	return current;
}

//-------------- insert ------------
// (private)
// Inserts a tab in the chain
//----------------------------------
void Tab::insert(Tab *tab) {
	
	if (!tab || tab == this) //dont insert if the tab = 0 or the tab = this
		return;

	Tab *first = getFirst(this);
	
	//if the tab already in chain then disconnect it
	for (; first!=0; first = first->m_next) {
		if (first==tab) {
			#ifdef DEBUG
			cerr<<"Tab already in chain. Disconnecting!"<<endl;			
			#endif // DEBUG
			tab->disconnect();
			break;
		}
	}

	//get last tab in the chain to be inserted
	Tab *last = tab;
	for (; last->m_next!=0; last=last->m_next); 
	//do sticky before we connect it to the chain
	//sticky bit on window
	if (m_win->isStuck() && !tab->m_win->isStuck() ||
			!m_win->isStuck() && tab->m_win->isStuck())
			tab->m_win->stick(); //this will set all the m_wins in the list
	
	//connect the tab to this chain
	
	if (m_next)	
		m_next->m_prev = last;
	tab->m_prev = this; 
	last->m_next = m_next; 

	m_next = tab;	

	//TODO: cleanup and optimize
	//move and resize all windows in the tablist we inserted
	//only from first tab of the inserted chain to the last
	for (; tab!=last->m_next; tab=tab->m_next) {
		if (m_win->isShaded() != tab->m_win->isShaded()) {
			if (m_win->screen->getTabPlacement() == PLeft ||
					m_win->screen->getTabPlacement() == PRight) {
				// if window were grouping to, we need to shade the tab window
				// _after_ reconfigure
				if(m_win->isShaded()) {
						tab->m_win->configure(m_win->frame.x, m_win->frame.y,
							m_win->frame.width, m_win->frame.height);
						tab->m_win->shade();
				// don't need unshading as configure will fix that for me
				} else {
					if ((m_win->frame.width != tab->m_win->frame.width) ||
							(m_win->frame.height != tab->m_win->frame.height)) {
						tab->m_win->configure(m_win->frame.x, m_win->frame.y,
							m_win->frame.width, m_win->frame.height);
					} else // need to change shade state as configure _won't_
							// do the trick if the new and old size is the same
						tab->m_win->shade();
				}

				tab->resizeGroup();
				tab->calcIncrease();

			} else { // PTop & PBottom
				if(m_win->isShaded()) {

					tab->m_win->configure(m_win->frame.x, m_win->frame.y,
							m_win->frame.width, m_win->frame.height);
					tab->m_win->shade();
				// don't need unshading as configure will fix that for me
				} else {
					if ((m_win->frame.width != tab->m_win->frame.width) ||
							(m_win->frame.height != tab->m_win->frame.height)) {

						tab->m_win->configure(m_win->frame.x, m_win->frame.y,
							m_win->frame.width, m_win->frame.height);
					} else
						tab->m_win->shade();
				}
			}

		// both window have the same shaded state
		} else {
			if ((m_win->frame.width != tab->m_win->frame.width) ||
					(m_win->frame.height != tab->m_win->frame.height)) {

				tab->m_win->configure(m_win->frame.x, m_win->frame.y,
					m_win->frame.width, m_win->frame.height);

				// need to shade the tab window as configure will mess it up
				if (m_win->isShaded())
					tab->m_win->shade();
			}
		}

		// TODO: should check if alignemnt is left or right,
		// cus then resize is allready done resize tabs 
		if(m_win->screen->getTabAlignment() == ARelative) {
			tab->resizeGroup();
			tab->calcIncrease();
		}
		m_win->tab->setPosition();
	}	
}

//---------- disconnect() --------------
// Disconnects the tab from any chain
//--------------------------------------
void Tab::disconnect() {
	Tab *tmp = 0;
	if (m_prev) {	//if this have a chain to "the left" (previous tab) then set it's next to this next
		m_prev->m_next = m_next;
		tmp = m_prev;
	} 
	if (m_next) {	//if this have a chain to "the right" (next tab) then set it's prev to this prev
		m_next->m_prev = m_prev;
		tmp = m_next;
	}

	//mark as no chain, previous and next.
	m_prev = 0;
	m_next = 0;
	
	//reposition the tabs
	if (tmp) {
		if (m_win->screen->getTabAlignment() == ARelative)
			tmp->calcIncrease();
		tmp->setPosition();
	}

	if (m_win->screen->getTabAlignment() == ARelative)
		calcIncrease();
	
	setPosition();
}

// ------------ setTabWidth --------------
// Sets Tab width _including_ borders
// ---------------------------------------
void Tab::setTabWidth(unsigned int w) {
	if (w > m_win->screen->getWindowStyle()->tab.border_width_2x) {
		m_size_w = w;
		XResizeWindow(m_display, m_tabwin,
			m_size_w - m_win->screen->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->screen->getWindowStyle()->tab.border_width_2x);
	}
}

// ------------ setTabHeight ---------
// Sets Tab height _including_ borders
// ---------------------------------------
void Tab::setTabHeight(unsigned int h) {
	if (h > m_win->screen->getWindowStyle()->tab.border_width_2x) {
		m_size_h = h;
		XResizeWindow(m_display, m_tabwin, 
			m_size_w - m_win->screen->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->screen->getWindowStyle()->tab.border_width_2x);
	} 
}

// ------------ resizeGroup --------------
// This function is used when (un)shading
// to get right size/width of tabs when
// PLeft || PRight && isTabRotateVertical
// ---------------------------------------
void Tab::resizeGroup(void) {
	#ifdef DEBUG
	cerr << "Resising group" << endl;
	#endif //DEBUG
	Tab *first;
	for (first = getFirst(this); first != 0; first = first->m_next) {
		if ((m_win->screen->getTabPlacement() == PLeft ||
				m_win->screen->getTabPlacement() == PRight) &&
				m_win->screen->isTabRotateVertical() &&
				!m_win->isShaded()) {
			first->setTabWidth(Fluxbox::instance()->getTabHeight());
			first->setTabHeight(Fluxbox::instance()->getTabWidth());
		} else {
			first->setTabWidth(Fluxbox::instance()->getTabWidth());
			first->setTabHeight(Fluxbox::instance()->getTabHeight());
		}
		//TODO: do I have to set this all the time?
		first->m_configured = true; //used in Fluxbox::reconfigure()
	}
}

//------------- calcRelativeWidth --------
// Returns: Calculated width for relative 
// alignment
//----------------------------------------
unsigned int Tab::calcRelativeWidth() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return ((m_win->frame.width + m_win->screen->getBorderWidth2x())/num);
}

//------------- calcRelativeHeight -------
// Returns: Calculated height for relative 
// alignment
//----------------------------------------
unsigned int Tab::calcRelativeHeight() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return ((m_win->frame.height + m_win->screen->getBorderWidth2x())/num);
}

//------------- calcCenterXPos -----------
// Returns: Calculated x position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterXPos() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return (m_win->frame.x + ((m_win->frame.width - (m_size_w * num)) / 2));
}

//------------- calcCenterYPos -----------
// Returns: Calculated y position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterYPos() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return (m_win->frame.y + ((m_win->frame.height - (m_size_h * num)) / 2));
}


//------- getTabPlacementString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else
// 0.
//----------------------------------------
const char *Tab::getTabPlacementString(int placement) {	
	for (int i=0; i<(pnone / 5); i++) {
		if (m_tabplacementlist[i] == placement)
			return m_tabplacementlist[i].string;
	}
	return 0;
}

//------- getTabPlacementNum -------------
// Returns the tabplacement number of the 
// tabplacement string on success else
// the type none on failure.
//----------------------------------------
int Tab::getTabPlacementNum(const char *string) {
	for (int i=0; i<(pnone / 5); i ++) {
		if (m_tabplacementlist[i] == string) {
			return m_tabplacementlist[i].tp;
		}
	}
	return pnone;
}

//------- getTabAlignmentString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else
// 0.
//----------------------------------------
const char *Tab::getTabAlignmentString(int placement) {	
	for (int i=0; i<anone; i++) {
		if (m_tabalignmentlist[i] == placement)
			return m_tabalignmentlist[i].string;
	}
	return 0;
}

//------- getTabAlignmentNum -------------
// Returns the tabplacement number of the 
// tabplacement string on success else
// the type none on failure.
//----------------------------------------
int Tab::getTabAlignmentNum(const char *string) {
	for (int i=0; i<anone; i++) {
		if (m_tabalignmentlist[i] == string) {
			return m_tabalignmentlist[i].tp;
		}
	}
	return anone;
}
