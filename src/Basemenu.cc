// Basemenu.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Basemenu.cc for blackbox - an X11 Window manager
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

// $Id: Basemenu.cc,v 1.39 2002/12/01 13:41:54 rathnor Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Basemenu.hh"
#include "Screen.hh"
#include "ImageControl.hh"
#include "StringUtil.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>


#ifdef DEBUG
#include <iostream>
using namespace std;
#endif //DEBUG

static Basemenu *shown = 0;

Basemenu::Basemenu(BScreen *screen):
    m_screen(screen),
    m_display(Fluxbox::instance()->getXDisplay()),
    m_parent(0),
    m_image_ctrl(screen->getImageControl()),
    m_alignment(ALIGNDONTCARE) {

    title_vis =
        movable =
        hide_tree = true;

    shifted =
        internal_menu =
        moving =
        torn =
        visible = false;

    menu.x =
        menu.y =
        menu.x_shift =
        menu.y_shift =
        menu.x_move =
        menu.y_move = 0;

    which_sub =
        which_press =
        which_sbl = -1;

    menu.frame_pixmap =
        menu.title_pixmap =
        menu.hilite_pixmap =
        menu.sel_pixmap = None;

    menu.bevel_w = m_screen->getBevelWidth();

    menu.width = menu.title_h = menu.item_w = menu.frame_h =
        m_screen->getMenuStyle()->titlefont.height() + menu.bevel_w * 2;

    menu.sublevels =
        menu.persub =
        menu.minsub = 0;

    menu.item_h = m_screen->getMenuStyle()->framefont.height() + menu.bevel_w;

    menu.height = menu.title_h + m_screen->getBorderWidth() + menu.frame_h;
	
    //set attributes for menu window
    unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
        CWColormap | CWOverrideRedirect | CWEventMask;
    XSetWindowAttributes attrib;
    attrib.background_pixmap = None;
    attrib.background_pixel = attrib.border_pixel =
        m_screen->getBorderColor()->pixel();
    attrib.colormap = m_screen->colormap();
    attrib.override_redirect = True;
    attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask | ExposureMask;

    //create menu window
    menu.window =
        XCreateWindow(m_display, m_screen->getRootWindow(), menu.x, menu.y, menu.width,
                      menu.height, m_screen->getBorderWidth(), m_screen->getDepth(),
                      InputOutput, m_screen->getVisual(), attrib_mask, &attrib);
	
    Fluxbox * const fluxbox = Fluxbox::instance();
    fluxbox->saveMenuSearch(menu.window, this);
	
    //attibutes for title to menuwindow
    attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel | CWEventMask;
    attrib.background_pixel = m_screen->getBorderColor()->pixel();
    attrib.event_mask |= EnterWindowMask | LeaveWindowMask;
    //create menu title
    menu.title =
        XCreateWindow(m_display, menu.window, 0, 0, menu.width, menu.height, 0,
                      m_screen->getDepth(), InputOutput, m_screen->getVisual(),
                      attrib_mask, &attrib);
    fluxbox->saveMenuSearch(menu.title, this);

    attrib.event_mask |= PointerMotionMask;
    menu.frame = XCreateWindow(m_display, menu.window, 0,
                               menu.title_h + m_screen->getBorderWidth(),
                               menu.width, menu.frame_h, 0,
                               m_screen->getDepth(), InputOutput,
                               m_screen->getVisual(), attrib_mask, &attrib);
    fluxbox->saveMenuSearch(menu.frame, this);
	
}


Basemenu::~Basemenu() {
    XUnmapWindow(m_display, menu.window);

    if (shown && shown->windowID() == windowID())
        shown = (Basemenu *) 0;

    //TODO: this looks kind of strange
    int n = menuitems.size() - 1;
    for (int i = 0; i < n; ++i)
        remove(0);

    if (menu.title_pixmap)
        m_image_ctrl->removeImage(menu.title_pixmap);

    if (menu.frame_pixmap)
        m_image_ctrl->removeImage(menu.frame_pixmap);

    if (menu.hilite_pixmap)
        m_image_ctrl->removeImage(menu.hilite_pixmap);

    if (menu.sel_pixmap)
        m_image_ctrl->removeImage(menu.sel_pixmap);

    Fluxbox * fluxbox = Fluxbox::instance();
	
    fluxbox->removeMenuSearch(menu.title);
    XDestroyWindow(m_display, menu.title);

    fluxbox->removeMenuSearch(menu.frame);
    XDestroyWindow(m_display, menu.frame);

    fluxbox->removeMenuSearch(menu.window);
    XDestroyWindow(m_display, menu.window);
}


int Basemenu::insert(const char *label, int function, const char *exec, int pos) {
    BasemenuItem *item = new BasemenuItem(label, function, exec);
    if (pos == -1) {
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
    }

    return menuitems.size();
}


int Basemenu::insert(const char *label, Basemenu *submenu, int pos) {
    BasemenuItem *item = new BasemenuItem(label, submenu);
    if (pos == -1) {
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
    }

    submenu->m_parent = this;

    return menuitems.size();
}

int Basemenu::remove(unsigned int index) {
    if (index >= menuitems.size()) {
#ifdef DEBUG
        std::cout << "Bad index (" << index << ") given to Basemenu::remove()"
                  << " -- should be between 0 and " << menuitems.size()
                  << " inclusive." << std::endl;
#endif // DEBUG
        return -1;
    }

    Menuitems::iterator it = menuitems.begin() + index;
    BasemenuItem *item = (*it);

    if (item) {
        menuitems.erase(it);
        if ((! internal_menu) && (item->submenu())) {
            Basemenu *tmp = (Basemenu *) item->submenu();

            if (! tmp->internal_menu) {
                delete tmp;				
            } else
                tmp->internal_hide();
        }
		
        delete item;
    }

    if (static_cast<unsigned int>(which_sub) == index)
        which_sub = -1;
    else if (static_cast<unsigned int>(which_sub) > index)
        which_sub--;

    return menuitems.size();
}

void Basemenu::raise() {
    XRaiseWindow(BaseDisplay::getXDisplay(), windowID());
}

void Basemenu::lower() {
    XLowerWindow(BaseDisplay::getXDisplay(), windowID());
}

void Basemenu::update() {
    I18n *i18n = I18n::instance();
	
    menu.item_h = m_screen->getMenuStyle()->framefont.height() + menu.bevel_w;
    menu.title_h = m_screen->getMenuStyle()->framefont.height() + menu.bevel_w*2;
	
    if (title_vis) {
        const char *s = (menu.label!="") ? menu.label.c_str() :
            i18n->getMessage(
                FBNLS::BasemenuSet, FBNLS::BasemenuBlackboxMenu,
                "fluxbox Menu");
        int l = strlen(s);
	
        menu.item_w = m_screen->getMenuStyle()->titlefont.textWidth(s, l);
		
        menu.item_w += (menu.bevel_w * 2);
    }	else
        menu.item_w = 1;

    int ii = 0;
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it) {
        BasemenuItem *itmp = (*it);

        const char *s = itmp->label().c_str();
        int l = itmp->label().size();

        ii = screen()->getMenuStyle()->framefont.textWidth(s, l);
			

        ii += (menu.bevel_w * 2) + (menu.item_h * 2);

        menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
    }

    if (menuitems.size()) {
        menu.sublevels = 1;

        while (((menu.item_h * (menuitems.size() + 1) / menu.sublevels) +
                menu.title_h + m_screen->getBorderWidth()) >
               m_screen->getHeight())
            menu.sublevels++;

        if (menu.sublevels < menu.minsub) menu.sublevels = menu.minsub;

        menu.persub = menuitems.size() / menu.sublevels;
        if (menuitems.size() % menu.sublevels) menu.persub++;
    } else {
        menu.sublevels = 0;
        menu.persub = 0;
    }

    menu.width = (menu.sublevels * (menu.item_w));
    if (! menu.width) menu.width = menu.item_w;

    menu.frame_h = (menu.item_h * menu.persub);
    menu.height = ((title_vis) ? menu.title_h + m_screen->getBorderWidth() : 0) +
        menu.frame_h;
    if (! menu.frame_h) menu.frame_h = 1;
    if (menu.height < 1) menu.height = 1;

    Pixmap tmp;
    if (title_vis) {
        tmp = menu.title_pixmap;
        const FbTk::Texture &tex = m_screen->getMenuStyle()->title;
        if (tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            menu.title_pixmap = None;
            XSetWindowBackground(m_display, menu.title, tex.color().pixel());
        } else {
            menu.title_pixmap =
                m_image_ctrl->renderImage(menu.width, menu.title_h, tex);
            XSetWindowBackgroundPixmap(m_display, menu.title, menu.title_pixmap);
        }

        if (tmp) 
            m_image_ctrl->removeImage(tmp);

        XClearWindow(m_display, menu.title);
    }

    tmp = menu.frame_pixmap;
    const FbTk::Texture &frame_tex = m_screen->getMenuStyle()->frame;
    if (frame_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        menu.frame_pixmap = None;
        XSetWindowBackground(m_display, menu.frame, frame_tex.color().pixel());
    } else {
        menu.frame_pixmap =
            m_image_ctrl->renderImage(menu.width, menu.frame_h, frame_tex);
        XSetWindowBackgroundPixmap(m_display, menu.frame, menu.frame_pixmap);
    }

    if (tmp)
        m_image_ctrl->removeImage(tmp);

    tmp = menu.hilite_pixmap;
    const FbTk::Texture &hilite_tex = m_screen->getMenuStyle()->hilite;
    if (hilite_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID))
        menu.hilite_pixmap = None;
    else
        menu.hilite_pixmap =
            m_image_ctrl->renderImage(menu.item_w, menu.item_h, hilite_tex);
    if (tmp)
        m_image_ctrl->removeImage(tmp);

    tmp = menu.sel_pixmap;
    if (hilite_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID))
        menu.sel_pixmap = None;
    else {
        int hw = menu.item_h / 2;
        menu.sel_pixmap =
            m_image_ctrl->renderImage(hw, hw, hilite_tex);
    }
    if (tmp) 
        m_image_ctrl->removeImage(tmp);

    XResizeWindow(m_display, menu.window, menu.width, menu.height);

    if (title_vis)
        XResizeWindow(m_display, menu.title, menu.width, menu.title_h);

    XMoveResizeWindow(m_display, menu.frame, 0,
                      ((title_vis) ? menu.title_h +
                       m_screen->getBorderWidth() : 0), menu.width,
                      menu.frame_h);

    XClearWindow(m_display, menu.window);
    XClearWindow(m_display, menu.title);
    XClearWindow(m_display, menu.frame);

    if (title_vis && visible) redrawTitle();

    unsigned int i = 0;
    for (i = 0; visible && i < menuitems.size(); i++) {
        if (i == (unsigned int)which_sub) {
            drawItem(i, True, 0);
            drawSubmenu(i);
        } else
            drawItem(i, False, 0);
    }

    if (m_parent && visible)
        m_parent->drawSubmenu(m_parent->which_sub);

    XMapSubwindows(m_display, menu.window);
}


void Basemenu::show() {
    XMapSubwindows(m_display, menu.window);
    XMapWindow(m_display, menu.window);
    visible = True;

    if (! m_parent) {
        if (shown && (! shown->torn))
            shown->hide();

        shown = this;
    }

}


void Basemenu::hide() {
    if ((! torn) && hide_tree && m_parent && m_parent->isVisible()) {
        Basemenu *p = m_parent;

        while (p->isVisible() && (! p->torn) && p->m_parent)
            p = p->m_parent;
        p->internal_hide();
    } else
        internal_hide();
}


void Basemenu::internal_hide() {
    if (which_sub >= 0) {
        BasemenuItem *tmp = menuitems[which_sub];
        tmp->submenu()->internal_hide();
    }

    if (m_parent && (! torn)) {
        m_parent->drawItem(m_parent->which_sub, False, True);

        m_parent->which_sub = -1;
    } else if (shown && shown->menu.window == menu.window)
        shown = (Basemenu *) 0;

    torn = visible = false;
    which_sub = which_press = which_sub = -1;

    XUnmapWindow(m_display, menu.window);
}


void Basemenu::move(int x, int y) {
    menu.x = x;
    menu.y = y;
    XMoveWindow(m_display, menu.window, x, y);
    if (which_sub != -1)
        drawSubmenu(which_sub);
}


void Basemenu::redrawTitle() {
    I18n *i18n = I18n::instance();
    const char *text = menu.label.size() ? menu.label.c_str() :
        i18n->getMessage(
            FBNLS::BasemenuSet, FBNLS::BasemenuBlackboxMenu,
            "fluxbox Menu");
    int dx = menu.bevel_w, len = strlen(text);
    unsigned int l = m_screen->getMenuStyle()->titlefont.textWidth(text, len) + menu.bevel_w*2;

    switch (m_screen->getMenuStyle()->titlefont_justify) {
    case DrawUtil::Font::RIGHT:
        dx += menu.width - l;
        break;

    case DrawUtil::Font::CENTER:
        dx += (menu.width - l) / 2;
        break;
    default:
        break;
    }

    m_screen->getMenuStyle()->titlefont.drawText( 
        menu.title, // drawable
        m_screen->getScreenNumber(),
        m_screen->getMenuStyle()->t_text_gc, // graphic context
        text, len,  // text string with lenght
        dx, m_screen->getMenuStyle()->titlefont.ascent() + menu.bevel_w);  // position


}


void Basemenu::drawSubmenu(unsigned int index) {
    if (which_sub >= 0 && static_cast<unsigned int>(which_sub) != index && 
        static_cast<unsigned int>(which_sub) < menuitems.size()) {
        BasemenuItem *itmp = menuitems[which_sub];

        if (! itmp->submenu()->isTorn())
            itmp->submenu()->internal_hide();
    }

    if (index < menuitems.size()) {
        BasemenuItem *item = menuitems[index];
        if (item->submenu() && visible && (! item->submenu()->isTorn()) &&
            item->isEnabled()) {
			
            if (item->submenu()->m_parent != this)
                item->submenu()->m_parent = this;
			
            int sbl = index / menu.persub, i = index - (sbl * menu.persub),
                x = menu.x +
                ((menu.item_w * (sbl + 1)) + m_screen->getBorderWidth()), y;
		
            if (m_alignment == ALIGNTOP) {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     ((title_vis) ? menu.title_h + m_screen->getBorderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->menu.title_h + m_screen->getBorderWidth() : 0));
            } else {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     (menu.item_h * i) +
                     ((title_vis) ? menu.title_h + m_screen->getBorderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->menu.title_h + m_screen->getBorderWidth() : 0));
            }
			
            if (m_alignment == ALIGNBOTTOM &&
                (y + item->submenu()->menu.height) > ((shifted) ? menu.y_shift :
                                                      menu.y) + menu.height) {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     menu.height - item->submenu()->menu.height);
            }

#ifdef XINERAMA
            int head_x = 0,
                head_y = 0,
                head_w,
                head_h;

            unsigned int head = 0;
            if (m_screen->hasXinerama()) {
                head = m_screen->getHead(menu.x, menu.y);
                head_x = m_screen->getHeadX(head);
                head_y = m_screen->getHeadY(head);
                head_w = m_screen->getHeadWidth(head);
                head_h = m_screen->getHeadHeight(head);
            } else {
                head_w = m_screen->getWidth();
                head_h = m_screen->getHeight();
            }

            if (static_cast<int>(x + item->submenu()->width()) > static_cast<int>(head_x + head_w)) {
                x = ((shifted) ? menu.x_shift : menu.x) -
                    item->submenu()->width() - m_screen->getBorderWidth();
            }
			
            if (x < head_x)
                x = head_x;

            if (static_cast<int>(y + item->submenu()->height()) > static_cast<int>(head_y + head_h)) {
                y = head_y + head_h -
                    item->submenu()->height() - m_screen->getBorderWidth2x();
            }

            if (y < head_y)
                y = head_y;
#else // !XINERAMA

            if ((x + item->submenu()->width()) > m_screen->getWidth()) {
                x = ((shifted) ? menu.x_shift : menu.x) -
                    item->submenu()->width() - m_screen->getBorderWidth();
            }
			
            if (x < 0)
                x = 0;

            if ((y + item->submenu()->height()) > m_screen->getHeight()) {
                y = m_screen->getHeight() - item->submenu()->height() -
                    m_screen->getBorderWidth2x();
            }
			
            if (y < 0)
                y = 0;
#endif // XINERAMA

            item->submenu()->move(x, y);
            if (! moving)
                drawItem(index, True);
		
            if (! item->submenu()->isVisible())
                item->submenu()->show();
			
            item->submenu()->moving = moving;
            which_sub = index;
        } else
            which_sub = -1;
    }
}


bool Basemenu::hasSubmenu(unsigned int index) const {
    if (index >= menuitems.size()) //boundary check
        return false;
	
    if (!menuitems[index]->submenu()) //has submenu?
        return false;
	
    return true;	
}


void Basemenu::drawItem(unsigned int index, bool highlight, bool clear,
                        int x, int y, unsigned int w, unsigned int h)
{
    if (index >= menuitems.size() || menuitems.size() == 0)
        return;

    BasemenuItem *item = menuitems[index];
    if (! item) return;
	
    bool dotext = true, dohilite = true, dosel = true;
    const char *text = item->label().c_str();
    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    int item_x = (sbl * menu.item_w), item_y = (i * menu.item_h);
    int hilite_x = item_x, hilite_y = item_y, hoff_x = 0, hoff_y = 0;
    int text_x = 0, text_y = 0, len = strlen(text), sel_x = 0, sel_y = 0;
    unsigned int hilite_w = menu.item_w, hilite_h = menu.item_h, text_w = 0, text_h = 0;
    unsigned int half_w = menu.item_h / 2, quarter_w = menu.item_h / 4;

    if (text) {		
        text_w = m_screen->getMenuStyle()->framefont.textWidth(text, len);

        text_y = item_y + menu.bevel_w/2 + m_screen->getMenuStyle()->framefont.ascent();

        switch(m_screen->getMenuStyle()->framefont_justify) {
        case DrawUtil::Font::LEFT:
            text_x = item_x + menu.bevel_w + menu.item_h + 1;
            break;
			
        case DrawUtil::Font::RIGHT:
            text_x = item_x + menu.item_w - (menu.item_h + menu.bevel_w + text_w);
            break;			
        default: //center
            text_x = item_x + ((menu.item_w + 1 - text_w) / 2);
            break;
        }

        text_h = menu.item_h - menu.bevel_w;
    }
	
    GC gc =
        ((highlight || item->isSelected()) ? m_screen->getMenuStyle()->h_text_gc :
         m_screen->getMenuStyle()->f_text_gc),
        tgc =
        ((highlight) ? m_screen->getMenuStyle()->h_text_gc :
         ((item->isEnabled()) ? m_screen->getMenuStyle()->f_text_gc :
          m_screen->getMenuStyle()->d_text_gc));
	
    sel_x = item_x;
	
    if (m_screen->getMenuStyle()->bullet_pos == RIGHT)
        sel_x += (menu.item_w - menu.item_h - menu.bevel_w);
	
    sel_x += quarter_w;
    sel_y = item_y + quarter_w;
	
    if (clear) {
        XClearArea(m_display, menu.frame, item_x, item_y, menu.item_w, menu.item_h,
                   False);
    } else if (! (x == y && y == -1 && w == h && h == 0)) {
        // calculate the which part of the hilite to redraw
        if (! (std::max(item_x, x) <= (signed) std::min(item_x + menu.item_w, x + w) &&
               std::max(item_y, y) <= (signed) std::min(item_y + menu.item_h, y + h))) {
            dohilite = False;
        } else {
            hilite_x = std::max(item_x, x);
            hilite_y = std::max(item_y, y);
            hilite_w = std::min(item_x + menu.item_w, x + w) - hilite_x;
            hilite_h = std::min(item_y + menu.item_h, y + h) - hilite_y;
            hoff_x = hilite_x % menu.item_w;
            hoff_y = hilite_y % menu.item_h;
        }
		
        // check if we need to redraw the text		
        int text_ry = item_y + (menu.bevel_w / 2);
        if (! (std::max(text_x, x) <= (signed) std::min(text_x + text_w, x + w) &&
               std::max(text_ry, y) <= (signed) std::min(text_ry + text_h, y + h)))
            dotext = False;
		
        // check if we need to redraw the select pixmap/menu bullet
        if (! (std::max(sel_x, x) <= (signed) std::min(sel_x + half_w, x + w) &&
               std::max(sel_y, y) <= (signed) std::min(sel_y + half_w, y + h)))
            dosel = False;
	
    }
	
    if (dohilite && highlight && (menu.hilite_pixmap != ParentRelative)) {
        if (menu.hilite_pixmap) {
            XCopyArea(m_display, menu.hilite_pixmap, menu.frame,		
                      m_screen->getMenuStyle()->hilite_gc, hoff_x, hoff_y,
                      hilite_w, hilite_h, hilite_x, hilite_y);
        } else {
            XFillRectangle(m_display, menu.frame,
                           m_screen->getMenuStyle()->hilite_gc,
                           hilite_x, hilite_y, hilite_w, hilite_h);
        }
    } 
	
    if (dosel && item->isSelected() &&
        (menu.sel_pixmap != ParentRelative)) {
        if (menu.sel_pixmap) {
            XCopyArea(m_display, highlight ? menu.frame_pixmap : menu.sel_pixmap, menu.frame,		
                      m_screen->getMenuStyle()->hilite_gc, 0, 0,
                      half_w, half_w, sel_x, sel_y);
        } else {
            XFillRectangle(m_display, menu.frame,
                           m_screen->getMenuStyle()->hilite_gc,
                           sel_x, sel_y, half_w, half_w);
        }
    }
	
    if (dotext && text) {
        m_screen->getMenuStyle()->framefont.drawText(			
            menu.frame, // drawable
            m_screen->getScreenNumber(),
            tgc,
            text, len, // text string and lenght
            text_x, text_y); // position

    }

    if (dosel && item->submenu()) {
        switch (m_screen->getMenuStyle()->bullet) {
        case SQUARE:
            XDrawRectangle(m_display, menu.frame, gc, sel_x, sel_y, half_w, half_w);
            break;

        case TRIANGLE:
            XPoint tri[3];

            if (m_screen->getMenuStyle()->bullet_pos == RIGHT) {
                tri[0].x = sel_x + quarter_w - 2;
                tri[0].y = sel_y + quarter_w - 2;
                tri[1].x = 4;
                tri[1].y = 2;
                tri[2].x = -4;
                tri[2].y = 2;
            } else {
                tri[0].x = sel_x + quarter_w - 2;
                tri[0].y = item_y + half_w;
                tri[1].x = 4;
                tri[1].y = 2;
                tri[2].x = 0;
                tri[2].y = -4;
            }
			
            XFillPolygon(m_display, menu.frame, gc, tri, 3, Convex,
                         CoordModePrevious);
            break;
			
        case DIAMOND:
            XPoint dia[4];

            dia[0].x = sel_x + quarter_w - 3;
            dia[0].y = item_y + half_w;
            dia[1].x = 3;
            dia[1].y = -3;
            dia[2].x = 3;
            dia[2].y = 3;
            dia[3].x = -3;
            dia[3].y = 3;

            XFillPolygon(m_display, menu.frame, gc, dia, 4, Convex,
                         CoordModePrevious);
            break;
        }
    }
}


void Basemenu::setLabel(const char *labelstr) {
    //make sure we don't send 0 to std::string
    menu.label = (labelstr ? labelstr : "");
}


void Basemenu::setItemSelected(unsigned int index, bool sel) {
    if (index >= menuitems.size()) return;

    BasemenuItem *item = find(index);
    if (! item) return;

    item->setSelected(sel);
    if (visible) drawItem(index, (index == (unsigned int)which_sub), true);
}


bool Basemenu::isItemSelected(unsigned int index) const{
    if (index >= menuitems.size()) return false;

    const BasemenuItem *item = find(index);
    if (!item)
        return false;

    return item->isSelected();
}


void Basemenu::setItemEnabled(unsigned int index, bool enable) {
    if (index >= menuitems.size()) return;

    BasemenuItem *item = find(index);
    if (! item) return;

    item->setEnabled(enable);
    if (visible) drawItem(index, (index == static_cast<unsigned int>(which_sub)), True);
}


bool Basemenu::isItemEnabled(unsigned int index) const {
    if (index >= menuitems.size()) return false;

    BasemenuItem *item = find(index);
    if (!item)
        return false;

    return item->isEnabled();
}


void Basemenu::buttonPressEvent(XButtonEvent *be) {
    if (be->window == menu.frame) {
        int sbl = (be->x / menu.item_w), i = (be->y / menu.item_h);
        int w = (sbl * menu.persub) + i;

        if (w < static_cast<int>(menuitems.size()) && w >= 0) {
            which_press = i;
            which_sbl = sbl;

            BasemenuItem *item = menuitems[w];

            if (item->submenu())
                drawSubmenu(w);
            else
                drawItem(w, (item->isEnabled()), true);
        }
    } else {
        menu.x_move = be->x_root - menu.x;
        menu.y_move = be->y_root - menu.y;
    }
}


void Basemenu::buttonReleaseEvent(XButtonEvent *re) {
    if (re->window == menu.title) {
        if (moving) {
            moving = false;
			
            if (which_sub >= 0)
                drawSubmenu(which_sub);
        }

        if (re->x >= 0 && re->x <= (signed) menu.width &&
            re->y >= 0 && re->y <= (signed) menu.title_h &&
            re->button == 3)
            hide();
			
    } else if (re->window == menu.frame &&
               re->x >= 0 && re->x < (signed) menu.width &&
               re->y >= 0 && re->y < (signed) menu.frame_h) {
			
        if (re->button == 3) {
            hide();
        } else {
            int sbl = (re->x / menu.item_w), i = (re->y / menu.item_h),
                ix = sbl * menu.item_w, iy = i * menu.item_h,
                w = (sbl * menu.persub) + i,
                p = (which_sbl * menu.persub) + which_press;

            if (w < static_cast<int>(menuitems.size()) && w >= 0) {
                drawItem(p, (p == which_sub), True);

                if	(p == w && isItemEnabled(w)) {
                    if (re->x > ix && re->x < (signed) (ix + menu.item_w) &&
                        re->y > iy && re->y < (signed) (iy + menu.item_h)) {
                        itemSelected(re->button, w);
                    }
                }
            } else
                drawItem(p, False, True);
				
        }
    }
}


void Basemenu::motionNotifyEvent(XMotionEvent *me) {
    if (me->window == menu.title && (me->state & Button1Mask)) {
        if (movable) {
            if (! moving) {
                if (m_parent && (! torn)) {
                    m_parent->drawItem(m_parent->which_sub, False, True);
                    m_parent->which_sub = -1;
                }

                moving = torn = True;

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            } else {
                menu.x = me->x_root - menu.x_move,
                    menu.y = me->y_root - menu.y_move;
	
                XMoveWindow(m_display, menu.window, menu.x, menu.y);

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            }
        }
    } else if ((! (me->state & Button1Mask)) && me->window == menu.frame &&
               me->x >= 0 && me->x < (signed) menu.width &&
               me->y >= 0 && me->y < (signed) menu.frame_h) {
        int sbl = (me->x / menu.item_w), i = (me->y / menu.item_h),
            w = (sbl * menu.persub) + i;

        if ((i != which_press || sbl != which_sbl) &&
            (w < static_cast<int>(menuitems.size()) && w >= 0)) {
            if (which_press != -1 && which_sbl != -1) {
                int p = (which_sbl * menu.persub) + which_press;
                BasemenuItem *item = menuitems[p];

                drawItem(p, False, True);
                if (item->submenu()) {
                    if (item->submenu()->isVisible() &&
                        (! item->submenu()->isTorn())) {
                        item->submenu()->internal_hide();
                        which_sub = -1;
                    }
                }
            }

            which_press = i;
            which_sbl = sbl;

            BasemenuItem *itmp = menuitems[w];

            if (itmp->submenu())
                drawSubmenu(w);
            else
                drawItem(w, (itmp->isEnabled()), True);
        }
    }
}


void Basemenu::exposeEvent(XExposeEvent *ee) {
    if (ee->window == menu.title) {
        redrawTitle();
    } else if (ee->window == menu.frame) {
        // this is a compilicated algorithm... lets do it step by step...
        // first... we see in which sub level the expose starts... and how many
        // items down in that sublevel

        int sbl = (ee->x / menu.item_w), id = (ee->y / menu.item_h),
            // next... figure out how many sublevels over the redraw spans
            sbl_d = ((ee->x + ee->width) / menu.item_w),
            // then we see how many items down to redraw
            id_d = ((ee->y + ee->height) / menu.item_h);

        if (id_d > menu.persub) id_d = menu.persub;

        // draw the sublevels and the number of items the exposure spans
        int i, ii;
        for (i = sbl; i <= sbl_d; i++) {
            // set the iterator to the first item in the sublevel needing redrawing
            int index = id + i * menu.persub;
            if (index < static_cast<int>(menuitems.size()) && index >= 0) {
                Menuitems::iterator it = menuitems.begin() + index;
                Menuitems::iterator it_end = menuitems.end();
                for (ii = id; ii <= id_d && it != it_end; ++it, ii++) {
                    int index = ii + (i * menu.persub);
                    // redraw the item
                    drawItem(index, (which_sub == index), true,
                             ee->x, ee->y, ee->width, ee->height);
                }
            }
        }
    }
}


void Basemenu::enterNotifyEvent(XCrossingEvent *ce) {
#ifdef XINERAMA
    int head = m_screen->hasXinerama() ? m_screen->getCurrHead() : 0;

    if (ce->window == menu.frame) {
        menu.x_shift = menu.x, menu.y_shift = menu.y;
        if (menu.x + menu.width >
            (m_screen->getHeadX(head) + m_screen->getHeadWidth(head))) {
            menu.x_shift = m_screen->getHeadX(head) +  m_screen->getHeadWidth(head) -
                menu.width - m_screen->getBorderWidth2x();
            shifted = True;
        } else if (menu.x < m_screen->getHeadX(head)) {
            menu.x_shift = m_screen->getHeadX(head);
            shifted = True;
        }

        if (menu.y + menu.height >
            (m_screen->getHeadY(head) + m_screen->getHeadHeight(head))) {
            menu.y_shift = m_screen->getHeadY(head) + m_screen->getHeadHeight(head) -
                menu.height - m_screen->getBorderWidth2x();
            shifted = True;
        } else if (menu.y + (signed) menu.title_h < m_screen->getHeadY(head)) {
            menu.y_shift = m_screen->getHeadY(head);
            shifted = True;
        }

#else // !XINERAMA

	if (ce->window == menu.frame) {
            menu.x_shift = menu.x, menu.y_shift = menu.y;
            if (menu.x + menu.width > m_screen->getWidth()) {
                menu.x_shift = m_screen->getWidth() - menu.width -
                    m_screen->getBorderWidth();
                shifted = True;
            } else if (menu.x < 0) {
                menu.x_shift = -m_screen->getBorderWidth();
                shifted = True;
            }

            if (menu.y + menu.height > m_screen->getHeight()) {
                menu.y_shift = m_screen->getHeight() - menu.height -
                    m_screen->getBorderWidth();
                shifted = True;
            } else if (menu.y + (signed) menu.title_h < 0) {
                menu.y_shift = -m_screen->getBorderWidth();
                shifted = True;
            }

#endif // XINERAMA

            if (shifted) {
#ifdef XINERAMA
                menu.x = menu.x_shift; // need to do this to avoid jumping beetween heads
                menu.y = menu.y_shift;
#endif // XINERAMA
                XMoveWindow(m_display, menu.window, menu.x_shift, menu.y_shift);
            }

            if (which_sub >= 0 && static_cast<size_t>(which_sub) < menuitems.size()) {
                BasemenuItem *tmp = menuitems[which_sub];
                if (tmp->submenu()->isVisible()) {
                    int sbl = (ce->x / menu.item_w), i = (ce->y / menu.item_h),
                        w = (sbl * menu.persub) + i;

                    if (w != which_sub && (! tmp->submenu()->isTorn())) {
                        tmp->submenu()->internal_hide();

                        drawItem(which_sub, False, True);
                        which_sub = -1;
                    }
                }
            }
	}
    }


    void Basemenu::leaveNotifyEvent(XCrossingEvent *ce) {
	if (ce->window == menu.frame) {
            if (which_press != -1 && which_sbl != -1 && menuitems.size() > 0) {
                int p = (which_sbl * menu.persub) + which_press;

                drawItem(p, (p == which_sub), true);

                which_sbl = which_press = -1;
            }

            if (shifted) {
                XMoveWindow(m_display, menu.window, menu.x, menu.y);
                shifted = false;

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            }
	}
    }


    void Basemenu::reconfigure(void) {
	XSetWindowBackground(m_display, menu.window,
                             m_screen->getBorderColor()->pixel());
	XSetWindowBorder(m_display, menu.window,
			 m_screen->getBorderColor()->pixel());
	XSetWindowBorderWidth(m_display, menu.window, m_screen->getBorderWidth());

	menu.bevel_w = m_screen->getBevelWidth();
	update();
    }
