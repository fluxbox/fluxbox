// Tab.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen at linuxmail.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Tab.cc,v 1.48 2002/12/07 13:36:03 fluxgen Exp $

#include "Tab.hh"

#include "i18n.hh"
#include "DrawUtil.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "Rootmenu.hh"
#include "ImageControl.hh"

#include <iostream>
using namespace std;

bool Tab::m_stoptabs = false;
Tab::t_tabplacementlist Tab::m_tabplacementlist[] = {
    {PTOP, "Top"},
    {PBOTTOM, "Bottom"},
    {PLEFT, "Left"},
    {PRIGHT, "Right"},
    {PNONE, "none"}
};

Tab::t_tabplacementlist Tab::m_tabalignmentlist[] = {
    {ALEFT, "Left"},
    {ACENTER, "Center"},
    {ARIGHT, "Right"},
    {ARELATIVE, "Relative"},
    {ANONE, "none"}
};

Tab::Tab(FluxboxWindow *win, Tab *prev, Tab *next) { 
    //set default values
	
    m_focus = m_moving = false;
    m_configured = true; // only set to false before Fluxbox::reconfigure
    m_move_x = m_move_y = 0;	
    m_prev = prev; m_next = next; 
    m_win = win;
    m_display = BaseDisplay::getXDisplay();
	
    if ((m_win->getScreen()->getTabPlacement() == PLEFT ||
         m_win->getScreen()->getTabPlacement() == PRIGHT) &&
        m_win->getScreen()->isTabRotateVertical() &&
        !m_win->isShaded()) {
        m_size_w = m_win->getScreen()->getTabHeight();
        m_size_h = m_win->getScreen()->getTabWidth();
    } else {
        m_size_w = m_win->getScreen()->getTabWidth();
        m_size_h = m_win->getScreen()->getTabHeight();
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
        m_win->getScreen()->getWindowStyle()->tab.border_color.pixel();
    attrib.colormap = m_win->getScreen()->colormap();
    attrib.override_redirect = True;
    attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask | ExposureMask | EnterWindowMask;
    //Notice that m_size_w gets the TOTAL width of tabs INCLUDING borders
    m_tabwin = XCreateWindow(m_display, m_win->getScreen()->getRootWindow(), 
                             -30000, -30000, //TODO: So that it wont flicker or
                             // appear before the window do
                             m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
                             m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
                             m_win->getScreen()->getWindowStyle()->tab.border_width,
                             m_win->getScreen()->getDepth(), InputOutput,
                             m_win->getScreen()->getVisual(), attrib_mask, &attrib);

    //set grab
    XGrabButton(m_display, Button1, Mod1Mask, m_tabwin, True,
		ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
		GrabModeAsync, None, Fluxbox::instance()->getMoveCursor());

    //save to tabsearch
    Fluxbox::instance()->saveTabSearch(m_tabwin, this);	

    XMapSubwindows(m_display, m_tabwin);
    // don't show if the window is iconified
    if (!m_win->isIconic())
        XMapWindow(m_display, m_tabwin);

    decorate();
}

//-------------- focus --------------------
// Called when the focus changes in m_win 
// updates pixmap or color and draws the tab
//-----------------------------------------
void Tab::focus() {

    if (m_win->isFocused()) {
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
    Tab *tab = 0;
    //raise tabs
    Workspace::Stack st;
    for (tab = getFirst(this); tab!=0; tab = tab->m_next) {
        st.push_back(tab->m_tabwin);
    }
    m_win->getScreen()->raiseWindows(st);
}

//-------------- lower --------------------
// Lowers the tabs in the tablist AND
// the windows the tabs relate to
//-----------------------------------------
void Tab::lower() {
    Tab *current = this;
    FluxboxWindow *win = 0; //convenience
    //this have to be done in the correct order, otherwise we'll switch the window
    //being ontop in the group
    do { 
        XLowerWindow(m_display, current->m_tabwin); //lower tabwin and tabs window
        win = current->getWindow(); 
        win->getScreen()->getWorkspace(win->getWorkspaceNumber())->lowerWindow(win);

        current = current->next(); //get next
        if (current == 0)
            current = getFirst(this); //there weren't any after, get the first

    } while (current != this);
}

//-------------- loadTheme -----------------
// loads the texture with the correct
// width and height, this is necessary in
// vertical and relative tab modes
// TODO optimize this
//------------------------------------------
void Tab::loadTheme() {
    BImageControl *image_ctrl = m_win->getScreen()->getImageControl();
    Pixmap tmp = m_focus_pm;
    const FbTk::Texture *texture = &(m_win->getScreen()->getWindowStyle()->tab.l_focus);

    if (texture->type() & FbTk::Texture::PARENTRELATIVE ) {
        const FbTk::Texture &pt = m_win->getScreen()->getWindowStyle()->tab.t_focus;
        if (pt.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            m_focus_pm = None;
            m_focus_pixel = pt.color().pixel();
        } else
            m_focus_pm =
                image_ctrl->renderImage(m_size_w, m_size_h, pt);

        if (tmp) image_ctrl->removeImage(tmp);

    } else {
        if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            m_focus_pm = None;
            m_focus_pixel = texture->color().pixel();
        } else
            m_focus_pm =
                image_ctrl->renderImage(m_size_w, m_size_h, *texture);
        if (tmp) image_ctrl->removeImage(tmp);
    }

    tmp = m_unfocus_pm;
    texture = &(m_win->getScreen()->getWindowStyle()->tab.l_unfocus);

    if (texture->type() & FbTk::Texture::PARENTRELATIVE ) {
        const FbTk::Texture &pt = m_win->getScreen()->getWindowStyle()->tab.t_unfocus;
        if (pt.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            m_unfocus_pm = None;
            m_unfocus_pixel = pt.color().pixel();
        } else
            m_unfocus_pm =
                image_ctrl->renderImage(m_size_w, m_size_h, pt);
    } else {
        if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            m_unfocus_pm = None;
            m_unfocus_pixel = texture->color().pixel();
        } else
            m_unfocus_pm =
                image_ctrl->renderImage(m_size_w, m_size_h, *texture);
    }
	
    if (tmp) image_ctrl->removeImage(tmp);
}

/**
 decorates the tab with current theme
*/
void Tab::decorate() {
    loadTheme();

    XSetWindowBorderWidth(m_display, m_tabwin,
                          m_win->getScreen()->getWindowStyle()->tab.border_width);
    XSetWindowBorder(m_display, m_tabwin,
                     m_win->getScreen()->getWindowStyle()->tab.border_color.pixel());
}

/**
 Deiconifies the tab
 Used from FluxboxWindow to deiconify the tab when the window is deiconfied
*/
void Tab::deiconify() {
    XMapWindow(m_display, m_tabwin);
}

/**
 Iconifies the tab.
 Used from FluxboxWindow to hide tab win when window is iconified
 disconnects itself from the list
*/
void Tab::iconify() {
    disconnect();
    withdraw();
    if(!Fluxbox::instance()->useTabs() && !m_next && !m_prev)//if we don't want to use tabs that much
        m_win->setTab(false);//let's get rid of this loner tab
}

/**
 Unmaps the tab from display
*/
void Tab::withdraw() {
    XUnmapWindow(m_display, m_tabwin);	
}

/**
 Set/reset the the sticky on all windows in the list
*/
void Tab::stick() {
    Tab *tab;
 
    bool wasstuck = m_win->isStuck();
 
    //now do stick for all windows in the list
    for (tab = getFirst(this); tab != 0; tab = tab->m_next) {
        FluxboxWindow *win = tab->m_win; //just for convenience
        if (wasstuck) {
            win->blackbox_attrib.flags ^= BaseDisplay::ATTRIB_OMNIPRESENT;
            win->blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_OMNIPRESENT;
            win->stuck = false;

        } else {
            win->stuck = true;
            BScreen *screen = win->getScreen();
            if (!win->isIconic() && !(win->getWorkspaceNumber() !=
                                      screen->getCurrentWorkspaceID())) {
                screen->reassociateWindow(win, screen->getCurrentWorkspaceID(), true);
            }

            win->blackbox_attrib.flags |= BaseDisplay::ATTRIB_OMNIPRESENT;
            win->blackbox_attrib.attrib |= BaseDisplay::ATTRIB_OMNIPRESENT;
        }
		
        win->setState(win->current_state);
    }	

}

/**
 Resize the window's in the tablist
*/
void Tab::resize() {
    Tab *tab;

    //now move and resize the windows in the list
    for (tab = getFirst(this); tab != 0; tab = tab->m_next) {
        if (tab!=this) {
            tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
                                  m_win->getWidth(), m_win->getHeight());
        }
    }

    // need to resize tabs if in relative mode
    if (m_win->getScreen()->getTabAlignment() == ARELATIVE) {
        calcIncrease();
        setPosition();
    }
}

/**
 Shades the windows in the tablist
*/
void Tab::shade() {
    Tab *tab;

    for(tab = getFirst(this); tab != 0; tab = tab->m_next) {
        if (tab==this)
            continue;
        tab->m_win->shade();
    }

    if (m_win->getScreen()->getTabPlacement() == PLEFT ||
        m_win->getScreen()->getTabPlacement() == PRIGHT) { 
        resizeGroup();
        calcIncrease();
    }

    if (!(m_win->getScreen()->getTabPlacement() == PTOP))
        setPosition();
}

/**
 Draws the tab 
 if pressed = true then it draws the tab in pressed 
 mode else it draws it in normal mode
 TODO: the "draw in pressed mode" 
*/
void Tab::draw(bool pressed) const {	
    XClearWindow(m_display, m_tabwin);
	
    if (m_win->getTitle().size() == 0) // we don't have anything to draw
        return;

    GC gc = ((m_win->isFocused()) ? m_win->getScreen()->getWindowStyle()->tab.l_text_focus_gc :
             m_win->getScreen()->getWindowStyle()->tab.l_text_unfocus_gc);

    Theme::WindowStyle *winstyle = m_win->getScreen()->getWindowStyle();
    size_t dlen = m_win->getTitle().size();
	
    size_t max_width = m_size_w; // special cases in rotated mode
    if (winstyle->tab.font.isRotated() && !m_win->isShaded())
        max_width = m_size_h;

    int dx = DrawUtil::doAlignment(max_width, m_win->frame.bevel_w,
                                   winstyle->tab.justify,
                                   winstyle->tab.font,
                                   m_win->getTitle().c_str(), m_win->getTitle().size(), dlen);
	
    int dy = winstyle->tab.font.ascent() + m_win->frame.bevel_w;
    bool rotate = false;
    // swap dx and dy if we're rotated
    if (winstyle->tab.font.isRotated() && !m_win->isShaded()) {
        int tmp = dy;
        dy = m_size_h - dx; // upside down (reverse direction)
        dx = tmp; 
        rotate = true;
    }
    // draw normal without rotation
    winstyle->tab.font.drawText(
                                m_tabwin,
                                m_win->getScreen()->getScreenNumber(),
                                gc,
                                m_win->getTitle().c_str(), dlen,
                                dx, dy,
                                rotate); 
}

/**
Helper for the Tab::setPosition() call
returns the y position component correctly
according to shading in cases PBOTTOM and 
isShaded()
*/
int Tab::setPositionShadingHelper(bool shaded) {
    if (shaded) {
        return m_win->getYFrame() + m_win->getTitleHeight() + 
            m_win->getScreen()->getBorderWidth2x();
    } else {
        return m_win->getYFrame() + m_win->getHeight() +
            m_win->getScreen()->getBorderWidth2x();
    }
}

/**
Helpers for correct alignment of tabs used
by the setPosition() call
return x/y positions correctly according to
alignment, the 1st for cases PTOP and PBOTTOM
the 2nd for cases PLEFT and PRIGHT
*/
int Tab::setPositionTBAlignHelper(Alignment align) {
    switch(align) {

    case ARELATIVE:
    case ALEFT:
        return m_win->getXFrame(); 
        break;
    case ACENTER:
        return calcCenterXPos(); 
        break;
    case ARIGHT:
        return m_win->getXFrame() + m_win->getWidth() +
            m_win->getScreen()->getBorderWidth2x() - m_size_w;
    default:
#ifdef DEBUG
        cerr << __FILE__ << ":" <<__LINE__ << ": " <<
            "Unsupported Alignment" << endl;
#endif //DEBUG
        return 0;
        break;
    }
}

int Tab::setPositionLRAlignHelper(Alignment align) {
    switch(align) {
    case ALEFT:
        return m_win->getYFrame() - m_size_h + m_win->getHeight() +
            m_win->getScreen()->getBorderWidth2x();
        break;
    case ACENTER:
        return calcCenterYPos();
        break;
    case ARELATIVE:
    case ARIGHT:
        return m_win->getYFrame();
        break;
    default:
#ifdef DEBUG
        cerr << __FILE__ << ":"<< __LINE__ << ": " <<
            "Unsupported Alignment" << endl;
#endif //DEBUG
        return 0;
        break;
    }
}

/**
 Position tab ( follow the m_win pos ).
 (and resize)
 Set new position of the other tabs in the chain
*/
void Tab::setPosition() {	
    //don't do anything if the tablist is freezed
    if (m_stoptabs)
        return;

    Tab *tab;
    int pos_x = 0, pos_y = 0;
	
    m_stoptabs = true; //freeze tablist
	
    //and check for max tabs

    //Tab placement + alignment
    switch (m_win->getScreen()->getTabPlacement()) {
    case PTOP:
        pos_y = m_win->getYFrame() - m_size_h;
        pos_x = setPositionTBAlignHelper(
            m_win->getScreen()->getTabAlignment());
        break;
    case PBOTTOM:
        pos_y = setPositionShadingHelper(m_win->isShaded());
        pos_x = setPositionTBAlignHelper(
            m_win->getScreen()->getTabAlignment());
        break;
    case PLEFT:
        pos_x = m_win->isShaded() ?
            setPositionTBAlignHelper(m_win->getScreen()->getTabAlignment()) :
                m_win->getXFrame() - m_size_w;
        pos_y = m_win->isShaded() ?
            setPositionShadingHelper(true) :
                setPositionLRAlignHelper(m_win->getScreen()->getTabAlignment());
        break;
    case PRIGHT:
        pos_x = m_win->isShaded() ?
            setPositionTBAlignHelper(m_win->getScreen()->getTabAlignment()) :
                m_win->getXFrame() + m_win->getWidth() +
            m_win->getScreen()->getBorderWidth2x();
        pos_y = m_win->isShaded() ?
            setPositionShadingHelper(true) :
                setPositionLRAlignHelper(m_win->getScreen()->getTabAlignment());
        break;
    default:
        if(m_win->isShaded()) {
            pos_y = setPositionShadingHelper(true);
            pos_x = setPositionTBAlignHelper(
                m_win->getScreen()->getTabAlignment());
        } else {
            setPositionShadingHelper(false);
        }
        break;
    }

    for (tab = getFirst(this);
         tab!=0; 
         pos_x += tab->m_inc_x, pos_y += tab->m_inc_y,
             tab = tab->m_next){
		
        XMoveWindow(m_display, tab->m_tabwin, pos_x, pos_y);
				
        //dont move FluxboxWindow if the iterator = this
        if (tab!=this) {
            tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(), 
                                  m_win->getWidth(), m_win->getHeight());
        }	
    }	
	
    m_stoptabs = false;//thaw tablist
}

//Moves the tab to the left
void Tab::movePrev() {
    insert(m_prev);
}

//Moves the tab to the next tab if m_next != 0
void Tab::moveNext() {
    if(m_next == 0)
        return;
    Tab *tmp = m_next; 
    disconnect();
    tmp->insert(this);
}


/**
 calculates m_inc_x and m_inc_y for tabs
 used for positioning the tabs.
*/
void Tab::calcIncrease() {
    Tab *tab;
    int inc_x = 0, inc_y = 0;
    unsigned int i = 0, tabs = numObjects();

    if (m_win->getScreen()->getTabPlacement() == PTOP ||
        m_win->getScreen()->getTabPlacement() == PBOTTOM ||
        m_win->isShaded()) {
        inc_y = 0;

        switch(m_win->getScreen()->getTabAlignment()) {
        case ALEFT:
            inc_x = m_size_w;
            break;
        case ACENTER:
            inc_x = m_size_w;
            break;
        case ARIGHT:
            inc_x = -m_size_w;
            break;
        case ARELATIVE:
            inc_x = calcRelativeWidth();
            break;
        default:
            break;
        }
    } else if (m_win->getScreen()->getTabPlacement() == PLEFT ||
               m_win->getScreen()->getTabPlacement() == PRIGHT) {
        inc_x = 0;

        switch(m_win->getScreen()->getTabAlignment()) {
        case ALEFT:
            inc_y = -m_size_h;
            break;
        case ACENTER:
            inc_y = m_size_h;
            break;
        case ARIGHT:
            inc_y = m_size_h;
            break;
        case ARELATIVE:
            inc_y = calcRelativeHeight();
            break;
        default:
            break;
        }
    }

    for (tab = getFirst(this); tab!=0; tab = tab->m_next, i++) {

	//TODO: move this out from here?
        if ((m_win->getScreen()->getTabPlacement() == PTOP ||
             m_win->getScreen()->getTabPlacement() == PBOTTOM ||
             m_win->isShaded()) &&
            m_win->getScreen()->getTabAlignment() == ARELATIVE) {
            if (!((m_win->getWidth() +
                   m_win->getScreen()->getBorderWidth2x()) % tabs) ||
                i >= ((m_win->getWidth() +
                       m_win->getScreen()->getBorderWidth2x()) % tabs)) {
                tab->setTabWidth(inc_x);
                tab->m_inc_x = inc_x;
            } else { // adding 1 extra pixel to get tabs like win width
                tab->setTabWidth(inc_x + 1);
                tab->m_inc_x = inc_x + 1;
            }
            tab->m_inc_y = inc_y;
        } else if (m_win->getScreen()->getTabAlignment() == ARELATIVE) {
            if (!((m_win->getHeight() +
                   m_win->getScreen()->getBorderWidth2x()) % tabs) ||
                i >= ((m_win->getHeight() +
                       m_win->getScreen()->getBorderWidth2x()) % tabs)) {

                tab->setTabHeight(inc_y);
                tab->m_inc_y = inc_y;
            } else {
				// adding 1 extra pixel to get tabs match window width
                tab->setTabHeight(inc_y + 1);
                tab->m_inc_y = inc_y + 1;
            }
            tab->m_inc_x = inc_x;
        } else { // non relative modes
            tab->m_inc_x = inc_x;
            tab->m_inc_y = inc_y;
        }
    }
}

/**
 Handle button press event here.
*/
void Tab::buttonPressEvent(XButtonEvent *be) {	
    //draw in pressed mode
    draw(true);
	
    //invoke root menu with auto-tab?
    if (be->button == 3) {
        BScreen *screen = m_win->getScreen();
        Rootmenu *rootmenu = screen->getRootmenu();
        if (! rootmenu->isVisible()) {
            Fluxbox::instance()->checkMenu();
            screen->getRootmenu()->move(be->x_root, be->y_root-rootmenu->titleHeight());
            rootmenu->setAutoGroupWindow(m_win->getClientWindow());
            rootmenu->show();
        }
    }
    //otherwise let the window handle the event
    else {
        //set window to titlewindow so we can take advantage of drag function
        be->window = m_win->frame.title;
	
        //call windows buttonpress eventhandler
        m_win->buttonPressEvent(be);
    }
}

/**
 Handle button release event here.
 If tab is dropped then it should try to find
 the window where the tab where dropped.
*/
void Tab::buttonReleaseEvent(XButtonEvent *be) {		
	
    if (m_moving) {
		
        m_moving = false; 
		
        //erase tabmoving rectangle
        XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(),
                       m_win->getScreen()->getOpGC(),
                       m_move_x, m_move_y, 
                       m_size_w, m_size_h);
		
        Fluxbox::instance()->ungrab();
        XUngrabPointer(m_display, CurrentTime);
		
        //storage of window and pos of window where we dropped the tab
        Window child;
        int dest_x = 0, dest_y = 0;
		
        //find window on coordinates of buttonReleaseEvent
        if (XTranslateCoordinates(m_display, m_win->getScreen()->getRootWindow(), 
                                  m_win->getScreen()->getRootWindow(),
                                  be->x_root, be->y_root, &dest_x, &dest_y, &child)) {
		        
            Tab *tab = Fluxbox::instance()->searchTab(child);
            FluxboxWindow *win = Fluxbox::instance()->searchWindow(child);
            if(win!=0 && m_win->getScreen()->isSloppyWindowGrouping())
                win->setTab(true);
            //search tablist for a tabwindow
            if ( (tab!=0) || (m_win->getScreen()->isSloppyWindowGrouping() &&
                              (win!=0) && (tab = win->getTab())!=0)) {

                if (tab == this) // inserting ourself to ourself causes a disconnect
                    return;

				// do only attach a hole chain if we dropped the
				// first tab in the dropped chain...
                if (m_prev)
                    disconnect();
				
				// attach this tabwindow chain to the tabwindow chain we found.
                tab->insert(this);

            } else {	//Dropped nowhere
                disconnect();

				// convenience
                unsigned int placement = m_win->getScreen()->getTabPlacement();

				// (ab)using dest_x and dest_y
                dest_x = be->x_root;
                dest_y = be->y_root;

                if (placement == PTOP || placement == PBOTTOM || m_win->isShaded()) {
                    if (placement == PBOTTOM && !m_win->isShaded())
                        dest_y -= m_win->getHeight();
                    else if (placement != PTOP && m_win->isShaded())
                        dest_y -= m_win->getTitleHeight();
                    else // PTOP
                        dest_y += m_win->getTitleHeight();

                    switch(m_win->getScreen()->getTabAlignment()) {
                    case ACENTER:
                        dest_x -= (m_win->getWidth() / 2) - (m_size_w / 2);
                        break;
                    case ARIGHT:
                        dest_x -= m_win->getWidth() - m_size_w;
                        break;
                    default:
                        break;
                    }

                } else { // PLEFT & PRIGHT
                    if (placement == PRIGHT)
                        dest_x = be->x_root - m_win->getWidth();

                    switch(m_win->getScreen()->getTabAlignment()) {
                    case ACENTER:
                        dest_y -= (m_win->getHeight() / 2) - (m_size_h / 2);
                        break;
                    case ALEFT:
                        dest_y -= m_win->getHeight() - m_size_h;
                        break;
                    default:
                        break;
                    }
                }
				//TODO: this causes an calculate increase event, even if we
				// only are moving a window
                m_win->configure(dest_x, dest_y, m_win->getWidth(), m_win->getHeight());
                if(!Fluxbox::instance()->useTabs())
                    m_win->setTab(0);//Remove tab from window, as it is now alone...
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
				
            XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(), 
                           m_win->getScreen()->getOpGC(),
                           m_move_x, m_move_y,
                           m_size_w, m_size_h);
	
        } else {
		
            int dx = me->x_root - 1, dy = me->y_root - 1;

            dx -= m_win->getScreen()->getBorderWidth();
            dy -= m_win->getScreen()->getBorderWidth();

            if (m_win->getScreen()->getEdgeSnapThreshold()) {
                int drx = m_win->getScreen()->getWidth() - (dx + 1);

                if (dx > 0 && dx < drx && dx < m_win->getScreen()->getEdgeSnapThreshold()) 
                    dx = 0;
                else if (drx > 0 && drx < m_win->getScreen()->getEdgeSnapThreshold())
                    dx = m_win->getScreen()->getWidth() - 1;

                int dtty, dbby, dty, dby;
		
                switch (m_win->getScreen()->getToolbarPlacement()) {
                case Toolbar::TOPLEFT:
                case Toolbar::TOPCENTER:
                case Toolbar::TOPRIGHT:
                    dtty = m_win->getScreen()->getToolbar()->exposedHeight() +
                        m_win->getScreen()->getBorderWidth();
                    dbby = m_win->getScreen()->getHeight();
                    break;

                default:
                    dtty = 0;
                    dbby = m_win->getScreen()->getToolbar()->y();
                    break;
                }
		
                dty = dy - dtty;
                dby = dbby - (dy + 1);

                if (dy > 0 && dty < m_win->getScreen()->getEdgeSnapThreshold())
                    dy = dtty;
                else if (dby > 0 && dby < m_win->getScreen()->getEdgeSnapThreshold())
                    dy = dbby - 1;
		
            }
		
            //erase rectangle
            XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(),
                           m_win->getScreen()->getOpGC(),
                           m_move_x, m_move_y, 
                           m_size_w, m_size_h);

            //redraw rectangle at new pos
            m_move_x = dx;
            m_move_y = dy;			
            XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(), 
                           m_win->getScreen()->getOpGC(),
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
    if (!current)
        return 0;
		
    Tab *i=current;
	
    for (; i->m_prev != 0; i = i->m_prev);
    return i;
}

//-------------- getLast() ---------
// Returns the last Tab in the chain 
// of currentchain. 
//-----------------------------------
Tab *Tab::getLast(Tab *current) {
    if (!current)
        return 0;
    Tab *i=current;
	
    for (; i->m_next != 0; i = i->m_next);
    return i;
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
    if (m_win->isStuck() != tab->m_win->isStuck()) {
        tab->m_win->stuck = !m_win->stuck; // it will toggle
        tab->stick(); //this will set all the m_wins in the list
    }
	
    //connect the tab to this chain
	
    if (m_next)	
        m_next->m_prev = last;
    tab->m_prev = this; 
    last->m_next = m_next; 

    m_next = tab;	

    bool resize_tabs = false;

    //TODO: cleanup and optimize
    //move and resize all windows in the tablist we inserted
    //only from first tab of the inserted chain to the last
    for (; tab!=last->m_next; tab=tab->m_next) {
        if (m_win->isShaded() != tab->m_win->isShaded()) {
            tab->m_stoptabs = true; // we don't want any actions performed on the
            // tabs, just the tab windows!
            if (m_win->getScreen()->getTabPlacement() == PLEFT ||
                m_win->getScreen()->getTabPlacement() == PRIGHT)
                resize_tabs = true;

            // if the window we are grouping to, we need to shade the tab window
            // _after_ reconfigure
            if(m_win->isShaded()) {
                tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
                                      m_win->getWidth(), m_win->getHeight());
                tab->m_win->shade();
            } else {
                tab->m_win->shade(); // switch to correct shade state
                tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
                                      m_win->getWidth(), m_win->getHeight());
            }

            tab->m_stoptabs = false;

            // both window have the same shaded state and have different sizes,
            // checking this so that I'll only do shade on windows if configure did
            // anything.
        } else if ((m_win->getWidth() != tab->m_win->getWidth()) ||
                   (m_win->getHeight() != tab->m_win->getHeight())) {

            tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
                                  m_win->getWidth(), m_win->getHeight());

            // need to shade the tab window as configure will mess it up
            if (m_win->isShaded())
                tab->m_win->shade();
        }
    }

    // resize if in relative mode or resize_tabs is true
    if(m_win->getScreen()->getTabAlignment() == ARELATIVE ||
       resize_tabs) {
        resizeGroup();
        calcIncrease();
    }
    // reposition tabs
    setPosition();
}

//---------- disconnect() --------------
// Disconnects the tab from any chain
//--------------------------------------
void Tab::disconnect() {
    Tab *tmp = 0;

    Fluxbox *fluxbox = Fluxbox::instance();
    if (m_prev) {	//if this have a chain to "the left" (previous tab) then set it's next to this next
        m_prev->m_next = m_next;
        if(!m_next && !fluxbox->useTabs())//Only two tabs in list, remove tab from remaining window
            m_prev->m_win->setTab(false);
        else
            tmp = m_prev;
    } 
    if (m_next) {	//if this have a chain to "the right" (next tab) then set it's prev to this prev
        m_next->m_prev = m_prev;
        if(!m_prev && !fluxbox->useTabs())//Only two tabs in list, remove tab from remaining window
            m_next->m_win->setTab(false);
        else
            tmp = m_next;
    }

    //mark as no chain, previous and next.
    m_prev = 0;
    m_next = 0;
	
    //reposition the tabs
    if (tmp) {
        if (m_win->getScreen()->getTabAlignment() == ARELATIVE)
            tmp->calcIncrease();
        tmp->setPosition();
    }

    if (m_win->getScreen()->getTabAlignment() == ARELATIVE)
        calcIncrease();
	
    setPosition();
}

// ------------ setTabWidth --------------
// Sets Tab width _including_ borders
// ---------------------------------------
void Tab::setTabWidth(unsigned int w) {
    if (w > m_win->getScreen()->getWindowStyle()->tab.border_width_2x &&
        w != m_size_w) {
        m_size_w = w;
        XResizeWindow(m_display, m_tabwin,
                      m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
                      m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x);

        loadTheme(); // rerender themes to right size
        focus(); // redraw the window
    }
}

// ------------ setTabHeight ---------
// Sets Tab height _including_ borders
// ---------------------------------------
void Tab::setTabHeight(unsigned int h) {
    if (h > m_win->getScreen()->getWindowStyle()->tab.border_width_2x &&
        h != m_size_h) {
        m_size_h = h;
        XResizeWindow(m_display, m_tabwin, 
                      m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
                      m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x);

        loadTheme(); // rerender themes to right size
        focus(); // redraw the window
    } 
}

// ------------ resizeGroup --------------
// This function is used when (un)shading
// to get right size/width of tabs when
// PLeft || PRight && isTabRotateVertical
// ---------------------------------------
void Tab::resizeGroup() {

    Tab *first;
    for (first = getFirst(this); first != 0; first = first->m_next) {
        if ((m_win->getScreen()->getTabPlacement() == PLEFT ||
             m_win->getScreen()->getTabPlacement() == PRIGHT) &&
            m_win->getScreen()->isTabRotateVertical() &&
            !m_win->isShaded()) {
            first->setTabWidth(m_win->getScreen()->getTabHeight());
            first->setTabHeight(m_win->getScreen()->getTabWidth());
        } else {
            first->setTabWidth(m_win->getScreen()->getTabWidth());
            first->setTabHeight(m_win->getScreen()->getTabHeight());
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

    return ((m_win->getWidth() + m_win->getScreen()->getBorderWidth2x())/num);
}

/**
 Returns the number of objects in
 the TabGroup. 
*/
unsigned int Tab::numObjects() {
    unsigned int num = 0;
    for (Tab *tab = getFirst(this); tab != 0; tab = tab->m_next, num++);
    return num;
}

/**
 Returns: Calculated height for relative 
 alignment
*/
unsigned int Tab::calcRelativeHeight() {
    return ((m_win->getHeight() + 
             m_win->getScreen()->getBorderWidth2x())/numObjects());
}

//------------- calcCenterXPos -----------
// Returns: Calculated x position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterXPos() {
    return (m_win->getXFrame() + ((m_win->getWidth() - 
                                   (m_size_w * numObjects())) / 2));
}

//------------- calcCenterYPos -----------
// Returns: Calculated y position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterYPos() {
    return (m_win->getYFrame() + ((m_win->getHeight() - 
                                   (m_size_h * numObjects())) / 2));
}


//------- getTabPlacementString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else 0.
//----------------------------------------
const char *Tab::getTabPlacementString(Tab::Placement placement) {	
    for (int i=0; i<(PNONE / 5); i++) {
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
Tab::Placement Tab::getTabPlacementNum(const char *string) {
    for (int i=0; i<(PNONE / 5); i ++) {
        if (m_tabplacementlist[i] == string) {
            return static_cast<Tab::Placement>(m_tabplacementlist[i].tp);
        }
    }
    return PNONE;
}

//------- getTabAlignmentString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else 0.
//----------------------------------------
const char *Tab::getTabAlignmentString(Tab::Alignment alignment) {	
    for (int i=0; i<ANONE; i++) {
        if (m_tabalignmentlist[i] == alignment)
            return m_tabalignmentlist[i].string;
    }
    return 0;
}

//------- getTabAlignmentNum -------------
// Returns the tabplacement number of the 
// tabplacement string on success else
// the type none on failure.
//----------------------------------------
Tab::Alignment Tab::getTabAlignmentNum(const char *string) {
    for (int i=0; i<ANONE; i++) {
        if (m_tabalignmentlist[i] == string) {
            return static_cast<Tab::Alignment>(m_tabalignmentlist[i].tp);
        }
    }
    return ANONE;
}

//---------- addWindowToGroup ------------
// Add a window the the tabbed group
//----------------------------------------
bool Tab::addWindowToGroup(FluxboxWindow *otherWindow)
{
    if (!otherWindow || otherWindow == m_win)
        return false;
    Tab *otherTab = otherWindow->getTab();
    if (!otherTab)
        return false;
    insert(otherTab);
    return true;
}
