// IconBar.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: IconBar.cc,v 1.28 2003/04/09 17:20:02 rathnor Exp $

#include "IconBar.hh"

#include "i18n.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "Window.hh"
#include "ImageControl.hh"
#include "Text.hh"

#include <algorithm>

//!! TODO THIS FILE NEEDS CLEANING

IconBarObj::IconBarObj(FluxboxWindow *fluxboxwin, Window iconwin)
{
    m_fluxboxwin = fluxboxwin;
    m_iconwin = iconwin;
}

IconBarObj::~IconBarObj() {
	
}

/// @return the width of specified icon window
unsigned int IconBarObj::width() const {
    Window root;

    unsigned int width, height;
    unsigned int border_width, depth;	//not used
    int x, y; //not used

    Display *m_display = Fluxbox::instance()->getXDisplay();

    XGetGeometry(m_display, m_iconwin, &root, &x, &y, 
                 &width, &height, &border_width, &depth);

    return width;
}

/// @return the width of specified icon window
unsigned int IconBarObj::height() const {
    Window root;

    unsigned int width, height;
    unsigned int border_width, depth;	//not used
    int x, y; //not used

    Display *m_display = Fluxbox::instance()->getXDisplay();

    XGetGeometry(m_display, m_iconwin, &root, &x, &y, 
                 &width, &height, &border_width, &depth);

    return height;
}


IconBar::IconBar(BScreen *scrn, Window parent, FbTk::Font &font):
    m_screen(scrn),
    m_parent(parent),
    m_focus_pm(None),
    m_vertical(false),
    m_font(font)
{
    m_display = scrn->getBaseDisplay()->getXDisplay();
}

IconBar::~IconBar() {
}

/**
 Adds icon to iconbar and repostions the 
 icons.
 returns window to iconobj
*/
Window IconBar::addIcon(FluxboxWindow *fluxboxwin) {
	
    Window iconwin = createIconWindow(fluxboxwin, m_parent);
    decorate(iconwin);	
    //add window object to list	
    m_iconlist.push_back(new IconBarObj(fluxboxwin, iconwin));
    //reposition all icons to fit windowbar
    repositionIcons();
	
    XMapSubwindows(m_display, iconwin);
    XMapWindow(m_display, iconwin);	
	
    return iconwin;
}

/**
  Removes icon from list and
  repositions the rest of the icons
  Return X Window of the removed iconobj
  returns None if no window was found
*/
Window IconBar::delIcon(FluxboxWindow *fluxboxwin) {
    Window retwin = None;
    IconBarObj *obj = findIcon(fluxboxwin);
    if (obj) {
        IconList::iterator it =
            std::find(m_iconlist.begin(), m_iconlist.end(), obj);
        if (it != m_iconlist.end()) {
            m_iconlist.erase(it);
            retwin = obj->getIconWin();		
            delete obj;
            XDestroyWindow(m_display, retwin);
            repositionIcons();		
        }
    }		
    return retwin;
}

/**
 * Removes all icons from list
 * Return X Windows of the removed iconobjs
 */
IconBar::WindowList *IconBar::delAllIcons() {
    Window retwin = None;
    WindowList *ret = new WindowList();
    while (!m_iconlist.empty()) {
            IconBarObj *obj = m_iconlist.back();
            m_iconlist.pop_back();
            retwin = obj->getIconWin();
            ret->push_back(retwin);
            delete obj;
            XDestroyWindow(m_display, retwin);
    }
    repositionIcons();
    return ret;
}

/**
 renders theme to m_focus_pm
 with the size width * height
*/
void IconBar::loadTheme(unsigned int width, unsigned int height) {
    FbTk::ImageControl *image_ctrl = m_screen->getImageControl();
    Pixmap tmp = m_focus_pm;
    const FbTk::Texture *texture = &(m_screen->getWindowStyle()->tab.l_focus);
	
    //If we are working on a PARENTRELATIVE, change to right focus value
    if (texture->type() & FbTk::Texture::PARENTRELATIVE ) {
        texture = &(m_screen->getWindowStyle()->tab.t_focus);
    }
		
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_focus_pm = None;
        m_focus_pixel = texture->color().pixel();
    } else {
        m_focus_pm =
            image_ctrl->renderImage(width, height, *texture);
    }

    if (tmp)
        image_ctrl->removeImage(tmp);
}

/**
 sets the background pixmap/color, 
 border, border width of the window
*/
void IconBar::decorate(Window win) {

    XSetWindowBorderWidth(m_display, win, m_screen->getWindowStyle()->tab.border_width);
    XSetWindowBorder(m_display, win, m_screen->getWindowStyle()->tab.border_color.pixel());
    if (m_focus_pm)
        XSetWindowBackgroundPixmap(m_display, win, m_focus_pm);
    else
        XSetWindowBackground(m_display, win, m_focus_pixel);
}


/**
  Reconfigures the icons 
  theme, pos and width
*/
void IconBar::reconfigure() {	
    repositionIcons();
	
}

/**
 Handles the expose event
 just redraws all the icons
*/
void IconBar::exposeEvent(XExposeEvent *ee) {
    IconBarObj *obj=0;	
    IconList::iterator it = m_iconlist.begin();
    IconList::iterator it_end = m_iconlist.end();
    for (; it != it_end; ++it) {
        if ((*it)->getIconWin() == ee->window) {
            obj = (*it);
            break;
        }
    }	

    if (obj) {

        Window root;
        unsigned int width, height;
        unsigned int border_width, depth;	//not used
        int x, y;
        XGetGeometry(m_display, m_parent, &root, &x, &y, &width, &height,
                     &border_width, &depth);

        //max width on every icon
        unsigned int icon_width = width / m_iconlist.size();
	
        //load right size of theme
        loadTheme(icon_width, height);
				
        draw(obj, icon_width);
		
    }	
}

/**
 Calculates and moves/resizes the icons
*/
void IconBar::repositionIcons() {
    if (m_iconlist.size() == 0)
        return;
		
    Window root;
    unsigned int width, height;
    unsigned int border_width, depth;	//not used
    int x, y;
    XGetGeometry(m_display, m_parent, &root, &x, &y, &width, &height,
                 &border_width, &depth);
	
    //max width on every icon
    unsigned int icon_width = width / m_iconlist.size();
    if (m_vertical)
        icon_width = height / m_iconlist.size();
    //load right size of theme
    loadTheme(icon_width, height);

    IconList::iterator it = m_iconlist.begin();
    IconList::iterator it_end = m_iconlist.end();
    for (x = 0; it != it_end; ++it, x += icon_width) {
        Window iconwin = (*it)->getIconWin();
        if (!m_vertical) {
            XMoveResizeWindow(m_display, iconwin,
                              x, 0,
                              icon_width, height);	
        } else {
            XMoveResizeWindow(m_display, iconwin,
                              0, x,
                              height, icon_width);	
        }
        draw((*it), icon_width);
        decorate(iconwin);
    }
		
}

/**
 creates the X Window of icon
 @return the created X Window
*/
Window IconBar::createIconWindow(FluxboxWindow *fluxboxwin, Window parent) {
    unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
        CWColormap | CWOverrideRedirect | CWEventMask;
    XSetWindowAttributes attrib;
    attrib.background_pixmap = None;
    attrib.background_pixel = attrib.border_pixel =
        fluxboxwin->getScreen()->getWindowStyle()->tab.border_color.pixel();
    attrib.colormap = fluxboxwin->getScreen()->colormap();
    attrib.override_redirect = True;
    attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask | ExposureMask | EnterWindowMask;
											
    //create iconwindow
    Window iconwin = XCreateWindow(m_display, parent, 0, 0, 1, 1, 0,
                                   fluxboxwin->getScreen()->getDepth(), InputOutput, fluxboxwin->getScreen()->getVisual(),
                                   attrib_mask, &attrib);
		
    return iconwin;
}

/**
 Draws theme and string to Window w
*/
void IconBar::draw(const IconBarObj * const obj, int width) const {
    if (!obj)
        return;
	
    const FluxboxWindow * const fluxboxwin = obj->getFluxboxWin();
    Window iconwin = obj->getIconWin();
    unsigned int title_text_w;

    title_text_w = m_font.textWidth(
        fluxboxwin->getIconTitle().c_str(), fluxboxwin->getIconTitle().size());
    int l = title_text_w;
    unsigned int dlen=fluxboxwin->getIconTitle().size();
    unsigned int bevel_w = m_screen->getBevelWidth();
    int dx=bevel_w*2;
    /*	
    for (; dlen >= 0; dlen--) {
        l = m_font.textWidth(
            fluxboxwin->getIconTitle().c_str(), dlen);
        l += (bevel_w * 4);
		
        if (l < width)
            break;
    }

        switch (m_screen->getWindowStyle()->tab.justify) {
    case Text::Font::RIGHT:
        dx += width - l;
        break;
    case Text::CENTER:
        dx += (width - l) / 2;
        break;
    default:
        break;
        }*/
    // center by default
    unsigned int newlen = 0;
    dx = FbTk::doAlignment(m_vertical ? obj->height() : obj->width(), bevel_w*2, FbTk::CENTER, m_font,
                           fluxboxwin->getIconTitle().c_str(), fluxboxwin->getIconTitle().size(),
                           newlen);
    //Draw title to m_iconwin

    XClearWindow(m_display, iconwin);		
    int dy = 1 + m_font.ascent();
    if (m_vertical) {
        int tmp = dy;
        dy = obj->height() - dx;
        dx = tmp;
    }
    //cerr<<"Drawing text: "<<dx<<", "<<dy<<endl;

    m_font.drawText(
        iconwin,
        m_screen->getScreenNumber(),
        m_screen->getWindowStyle()->tab.l_text_focus_gc,
        fluxboxwin->getIconTitle().c_str(), newlen,
        dx, dy,	m_vertical);

}

/**
 Tries to find the FluxboxWindow of the X Window
 in iconbar
 @return the fluxboxwindow on success else 0 on failure
*/
FluxboxWindow *IconBar::findWindow(Window w) {

    IconList::iterator it = m_iconlist.begin();
    IconList::iterator it_end = m_iconlist.end();
    for (; it != it_end; ++it) {
        IconBarObj *tmp = (*it);
        if (tmp)
            if (tmp->getIconWin() == w)
                return tmp->getFluxboxWin();			
    }
	
    return 0;
}

/*
 Tries to find a fluxboxwin icon in the iconlist
 @return pointer to IconBarObj on success else 0 on failure
*/
IconBarObj *IconBar::findIcon(FluxboxWindow *fluxboxwin) {

    IconList::iterator it = m_iconlist.begin();
    IconList::iterator it_end = m_iconlist.end();
    for (; it != it_end; ++it) {
        IconBarObj *tmp = (*it);
        if (tmp)
            if (tmp->getFluxboxWin() == fluxboxwin)
                return tmp;			
    }
	
    return 0;
}

const IconBarObj *IconBar::findIcon(const FluxboxWindow * const fluxboxwin) const {

    IconList::const_iterator it = m_iconlist.begin();
    IconList::const_iterator it_end = m_iconlist.end();
    for (; it != it_end; ++it) {
        IconBarObj *tmp = (*it);
        if (tmp)
            if (tmp->getFluxboxWin() == fluxboxwin)
                return tmp;			
    }
	
    return 0;
}
