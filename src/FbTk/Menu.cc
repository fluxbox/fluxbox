// Menu.cc for FbTk - Fluxbox Toolkit 
// Copyright (c) 2001 - 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Basemenu.cc for blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

// $Id: Menu.cc,v 1.71 2004/07/14 18:30:37 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include "Menu.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#include "MenuItem.hh"
#include "ImageControl.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "EventManager.hh"
#include "Transparent.hh"
#include "SimpleCommand.hh"
#include "I18n.hh"

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

namespace FbTk {

static Menu *shown = 0;

Menu *Menu::s_focused = 0;

static Pixmap getRootPixmap(int screen_num) {
    Pixmap root_pm = 0;
    // get root pixmap for transparency
    Display *disp = FbTk::App::instance()->display();
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned int *data;
    if (XGetWindowProperty(disp, RootWindow(disp, screen_num), 
                           XInternAtom(disp, "_XROOTPMAP_ID", false),
                           0L, 1L, 
                           false, XA_PIXMAP, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) { 
        root_pm = (Pixmap) (*data);                  
        XFree(data);
    }

    return root_pm; 
}

Menu::Menu(MenuTheme &tm, ImageControl &imgctrl):
    m_theme(tm),
    m_parent(0),
    m_image_ctrl(imgctrl),
    m_screen_width(DisplayWidth(FbTk::App::instance()->display(), tm.screenNum())),
    m_screen_height(DisplayHeight(FbTk::App::instance()->display(), tm.screenNum())),
    m_alignment(ALIGNDONTCARE),
    m_active_index(-1),
    m_need_update(true) {

    // setup timers

    RefCount<Command> show_cmd(new SimpleCommand<Menu>(*this, &Menu::openSubmenu));
    m_submenu_timer.setCommand(show_cmd);
    m_submenu_timer.fireOnce(true);


    RefCount<Command> hide_cmd(new SimpleCommand<Menu>(*this, &Menu::closeMenu));
    m_hide_timer.setCommand(hide_cmd);
    m_hide_timer.fireOnce(true);

    // make sure we get updated when the theme is reloaded
    tm.reconfigSig().attach(this);

    title_vis = true;

    shifted =
        internal_menu =
        moving =
        torn =
        visible = false;



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


    menu.item_w = menu.frame_h =
        theme().titleFont().height() + theme().bevelWidth() * 2;

    menu.sublevels =
        menu.persub =
        menu.minsub = 0;

    long event_mask = ButtonPressMask | ButtonReleaseMask | 
        ButtonMotionMask | KeyPressMask | ExposureMask | FocusChangeMask;
    // create menu window
    menu.window = FbTk::FbWindow(tm.screenNum(),
                                 0, 0, 10, 10,
                                 event_mask,
                                 true,  // override redirect
                                 true); // save_under

    // strip focus change mask from attrib, since we should only use it with main window
    event_mask ^= FocusChangeMask;

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(*this, menu.window);
	

    event_mask |= EnterWindowMask | LeaveWindowMask;
    //create menu title
    menu.title = FbTk::FbWindow(menu.window, // parent
                                0, 0, width(), theme().titleHeight(), // pos and size
                                event_mask, // mask
                                false, // override redirect
                                true); // save under

    evm.add(*this, menu.title);

    event_mask |= PointerMotionMask;
    menu.frame = FbTk::FbWindow(menu.window, // parent
                                0, theme().titleHeight(), // pos
                                width(), menu.frame_h ? menu.frame_h : 1, // size
                                event_mask,  // mask
                                false,  // override redirect
                                true); // save under
    evm.add(*this, menu.frame);

    menu.title.raise();

}

Menu::~Menu() {

    menu.window.hide();
   
    if (shown && shown->window() == window())
        shown = 0;

    removeAll();

    if (menu.title_pixmap)
        m_image_ctrl.removeImage(menu.title_pixmap);

    if (menu.frame_pixmap)
        m_image_ctrl.removeImage(menu.frame_pixmap);

    if (menu.hilite_pixmap)
        m_image_ctrl.removeImage(menu.hilite_pixmap);

    if (menu.sel_pixmap)
        m_image_ctrl.removeImage(menu.sel_pixmap);

    FbTk::EventManager &evm = *FbTk::EventManager::instance();

    if (s_focused == this)
        s_focused = 0;
}

int Menu::insert(const char *label, RefCount<Command> &cmd, int pos) {
    return insert(new MenuItem(label, cmd), pos);
}

int Menu::insert(const char *label, int pos) {
    return insert(new MenuItem(label), pos);
}

int Menu::insert(const char *label, Menu *submenu, int pos) {
    submenu->m_parent = this;
    return insert(new MenuItem(label, submenu), pos);
}

int Menu::insert(MenuItem *item, int pos) {
    if (pos == -1) {
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
    }
    m_need_update = true; // we need to redraw the menu
    return menuitems.size();
}

int Menu::remove(unsigned int index) {
    if (index >= menuitems.size()) {
#ifdef DEBUG
        std::cout << "Bad index (" << index << ") given to Menu::remove()"
                  << " -- should be between 0 and " << menuitems.size()
                  << " inclusive." << std::endl;
#endif // DEBUG
        return -1;
    }

    Menuitems::iterator it = menuitems.begin() + index;
    MenuItem *item = (*it);

    if (item) {
        menuitems.erase(it);
        if (!internal_menu && item->submenu() != 0) {
            Menu *tmp = item->submenu();
            // if menu is interal we should just hide it instead
            // if destroying it
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

    m_need_update = true; // we need to redraw the menu

    return menuitems.size();
}

void Menu::removeAll() {
    while (!menuitems.empty()) {
        remove(0);
    }
    m_need_update = true;
}

void Menu::raise() {
    menu.window.raise();
}

void Menu::lower() {
    menu.window.lower();
}

void Menu::nextItem() {
    int old_which_press = which_press;
    m_active_index = -1;
    if (validIndex(old_which_press) && 
        menuitems[old_which_press] != 0) {
        if (menuitems[old_which_press]->submenu()) {
            // we need to do this explicitly on the menu.window
            // since it might hide the parent if we use Menu::hide
            menuitems[old_which_press]->submenu()->internal_hide();
        }
        drawItem(old_which_press, 
                 true,  // clear
                 true); // transp
    }

    // restore old in case we changed which_press
    which_press = old_which_press;
    if (!validIndex(which_press))
        which_press = 0;
    else
        which_press++;


    if (menuitems[which_press] == 0) {
        m_active_index = -1;
        return;
    }

    m_active_index = which_press;    

    drawItem(which_press, 
             true,  // clear
             true); // transp

}

void Menu::prevItem() {

    int old_which_press = which_press;
    m_active_index = -1;
    if (validIndex(old_which_press)) {
        if (menuitems[old_which_press]->submenu()) {
            // we need to do this explicitly on the menu.window
            // since it might hide the parent if we use Menu::hide
            menuitems[old_which_press]->submenu()->internal_hide();            
        }
        drawItem(old_which_press, 
                 true,  // clear
                 true); // transp
    }
    // restore old in case we changed which_press
    which_press = old_which_press;

    if (!validIndex(which_press))
        which_press = menuitems.size() - 1;
    else if (which_press - 1 >= 0)
        which_press--;

    if (menuitems[which_press] == 0) {
        m_active_index = -1;
        return;
    }

    m_active_index = which_press;

    drawItem(which_press, 
             true,  // clear
             true); // transp

}

void Menu::enterSubmenu() {
    if (!validIndex(which_press))
        return;

    Menu *submenu = menuitems[which_press]->submenu();
    if (submenu == 0)
        return;

    if (submenu->menuitems.size() == 0)
        return;

    drawSubmenu(which_press);
    submenu->grabInputFocus();
    submenu->which_press = -1; // so we land on 0 after nextItem()
    submenu->nextItem();
}

void Menu::enterParent() {
    if (!validIndex(which_press) || parent() == 0)
        return;

    Menu *submenu = menuitems[which_press]->submenu();
    if (submenu)
        submenu->internal_hide();

    drawItem(which_press, 
             true,  // clear
             true); // transp
    which_press = -1; // dont select any in this 
    // hide self
    visible = false;
    menu.window.hide();
    // return focus to parent
    parent()->grabInputFocus();
}

void Menu::disableTitle() {
    setTitleVisibility(false);
}

void Menu::enableTitle() {
    setTitleVisibility(true);
}

void Menu::update(int active_index) {
    if (title_vis) {
        menu.item_w = theme().titleFont().textWidth(menu.label.c_str(),
                                                    menu.label.size());
		
        menu.item_w += (theme().bevelWidth() * 2);
    }	else
        menu.item_w = 1;

    unsigned int ii = 0;
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it) {
        ii = (*it)->width(theme());
        menu.item_w = (ii > menu.item_w ? ii : menu.item_w);
    }

    if (!menuitems.empty()) {
        menu.sublevels = 1;

        while (theme().itemHeight() * (menuitems.size() + 1) / menu.sublevels +
               theme().titleHeight() + theme().borderWidth() > m_screen_height) {
            menu.sublevels++;
        }

        if (menu.sublevels < menu.minsub) 
            menu.sublevels = menu.minsub;

        menu.persub = menuitems.size() / menu.sublevels;
        if (menuitems.size() % menu.sublevels) menu.persub++;
    } else {
        menu.sublevels = 0;
        menu.persub = 0;
    }

    int itmp = (theme().itemHeight() * menu.persub);
    menu.frame_h = itmp < 0 ? 0 : itmp;

    int new_width = (menu.sublevels * menu.item_w);
    int new_height = menu.frame_h;

    if (title_vis)
        new_height += theme().titleHeight() + ((menu.frame_h > 0)?menu.title.borderWidth():0);


    if (new_width < 1) 
        new_width = menu.item_w;

    if (new_height < 1)
        new_height = 1;

    menu.window.resize(new_width, new_height);

    Pixmap tmp = 0;
    if (title_vis && m_need_update) {
        tmp = menu.title_pixmap;
        const FbTk::Texture &tex = theme().titleTexture();
        if (!tex.usePixmap()) {
            menu.title_pixmap = None;
        } else {
            menu.title_pixmap =
                m_image_ctrl.renderImage(width(), theme().titleHeight(), tex);
        }

        if (tmp) 
            m_image_ctrl.removeImage(tmp);

        // if new size of title doesn't match our
        // buffer pixmap -> resize buffer pixmap
        if (m_title_pm.width() != width() || 
            m_title_pm.height() != theme().titleHeight()) {
            m_title_pm = FbPixmap(menu.title.window(),
                                  width(), theme().titleHeight(),
                                  menu.title.depth());
            m_real_title_pm = FbPixmap(menu.title.window(),
                                       width(), theme().titleHeight(),
                                       menu.title.depth());
            // set pixmap that we have as real face to the user
            menu.title.setBackgroundPixmap(m_real_title_pm.drawable());
            menu.title.setBufferPixmap(m_real_title_pm.drawable());
            //!! TODO: error checking?
            GContext def_gc(menu.title);
            if (menu.title_pixmap == 0) {
                def_gc.setForeground(theme().titleTexture().color());
                m_title_pm.fillRectangle(def_gc.gc(),
                                         0, 0,
                                         m_title_pm.width(), m_title_pm.height());
                m_real_title_pm.fillRectangle(def_gc.gc(),
                                              0, 0,
                                              m_title_pm.width(), m_title_pm.height());
            } else {
                m_title_pm.copyArea(menu.title_pixmap, 
                                    def_gc.gc(),
                                    0, 0,
                                    0, 0,
                                    m_title_pm.width(), m_title_pm.height());

                m_real_title_pm.copyArea(menu.title_pixmap, 
                                         def_gc.gc(),
                                         0, 0,
                                         0, 0,
                                         m_title_pm.width(), m_title_pm.height());

            }

        }
    }

    tmp = menu.frame_pixmap;
    const FbTk::Texture &frame_tex = theme().frameTexture();
    if (m_need_update) {
        if (!frame_tex.usePixmap()) {
            menu.frame_pixmap = None;
        } else {
            menu.frame_pixmap =
                m_image_ctrl.renderImage(width(), menu.frame_h, frame_tex);        
        }

        if (tmp)
            m_image_ctrl.removeImage(tmp);

    }
    tmp = menu.hilite_pixmap;
    const FbTk::Texture &hilite_tex = theme().hiliteTexture();
    if (!hilite_tex.usePixmap()) {
        menu.hilite_pixmap = None;
    } else
        menu.hilite_pixmap =
            m_image_ctrl.renderImage(menu.item_w, theme().itemHeight(), hilite_tex);
    if (tmp)
        m_image_ctrl.removeImage(tmp);

    tmp = menu.sel_pixmap;
    if (!hilite_tex.usePixmap()) {
        menu.sel_pixmap = None;
    } else {
        int hw = theme().itemHeight() / 2;
        menu.sel_pixmap =
            m_image_ctrl.renderImage(hw, hw, hilite_tex);
    }
    if (tmp) 
        m_image_ctrl.removeImage(tmp);



    if (title_vis) {
        menu.title.moveResize(-menu.title.borderWidth(), -menu.title.borderWidth(), 
                              width() + menu.title.borderWidth(), theme().titleHeight());
    }

    menu.frame.moveResize(0, ((title_vis) ? menu.title.y() + menu.title.height() + 
                              menu.title.borderWidth()*2 : 0), 
                          width(), menu.frame_h);


    if (m_need_update || m_frame_pm.width() != menu.frame.width() ||
        m_frame_pm.height() != menu.frame.height()){

        m_frame_pm = FbTk::FbPixmap(menu.frame.window(),
                                    menu.frame.width(), menu.frame.height(),
                                    menu.frame.depth());

        m_real_frame_pm = FbTk::FbPixmap(menu.frame.window(),
                                         menu.frame.width(), menu.frame.height(),
                                         menu.frame.depth());

        menu.frame.setBackgroundPixmap(m_real_frame_pm.drawable());
        GContext def_gc(menu.frame);
        if (m_frame_pm.drawable() == 0) {
            _FB_USES_NLS;
            cerr<<"FbTk::Menu: "<<_FBTKTEXT(Error, CreatePixmap, "Error creating pixmap", "Couldn't create a pixmap - image - for some reason")<<" ("<<
                menu.frame.window()<<", "<<menu.frame.width()<<", "<<
                menu.frame.height()<<
                ", "<<menu.frame.depth()<<") !"<<endl;
        } else {

            if (menu.frame_pixmap == 0) {
                def_gc.setForeground(theme().frameTexture().color());
                m_frame_pm.fillRectangle(def_gc.gc(),
                                         0, 0,
                                         width(), menu.frame_h);
            } else {
                m_frame_pm.copyArea(menu.frame_pixmap, def_gc.gc(),
                                    0, 0,
                                    0, 0,
                                    width(), menu.frame_h);
            }
            

        }

        m_real_frame_pm.copyArea(m_frame_pm.drawable(),
                                 def_gc.gc(),
                                 0, 0,
                                 0, 0,
                                 m_frame_pm.width(), m_frame_pm.height());
            
    }

    // if menu visible and title visible
    if (title_vis && visible) 
        redrawTitle();

    if (active_index >= 0 || m_need_update) {

        renderTransp(0, 0,
                     m_real_frame_pm.width(), m_real_frame_pm.height());
        for (unsigned int i = 0; i < menuitems.size(); i++) {
            drawItem(i,      // index
                     true,   // clear
                     false); // render_trans
        }
        
    }

    m_need_update = false;
}


void Menu::show() {

    if (m_need_update)
        update();

    menu.window.showSubwindows();
    menu.window.show();
    raise();
    visible = true;

    if (! m_parent && shown != this) {
        if (shown && (! shown->torn))
            shown->hide();

        shown = this;
    }
    
}


void Menu::hide() {

    if (!isVisible())
        return;


    // if not torn and parent is visible, go to first parent
    // and hide it
    if (!torn && m_parent && m_parent->isVisible()) {
        Menu *p = m_parent;

        while (p->isVisible() && (! p->torn) && p->m_parent)
            p = p->m_parent;
        p->internal_hide();
    } else // if we dont have a parent then do hide here
        internal_hide();

}

void Menu::grabInputFocus() {
    s_focused = this;

    // grab input focus
    menu.window.setInputFocus(RevertToPointerRoot, CurrentTime);

}


void Menu::clearWindow() {
    redrawTitle();
 
    if (alpha() < 255) {
        renderTransp(0, 0,
                     menu.frame.width(), menu.frame.height());
        update();
    }

    menu.title.clear();
    menu.frame.clear();
}

void Menu::internal_hide() {

    if (validIndex(which_sub)) {
        MenuItem *tmp = menuitems[which_sub];
        tmp->submenu()->internal_hide();
    }

    if (shown && shown->menu.window == menu.window)
        shown = (Menu *) 0;

    torn = visible = false;
    which_sub = which_press = which_sub = -1;

    // if we have an active index we need to redraw it 
    // as non active
    int old = m_active_index;
    m_active_index = -1;
    drawItem(old, true); // clear old area from highlight
    
    menu.window.hide();
}


void Menu::move(int x, int y) {
    if (x == this->x() && y == this->y())
        return;

    // if we're not visible and we do transparency
    // we need to update transparency when we call
    // show() next time
    if (alpha() < 255)
        m_need_update = true;

    menu.window.move(x, y);

    if (!isVisible())
        return;

    if (which_sub != -1)
        drawSubmenu(which_sub);

    if (alpha() < 255) {
        redrawTitle();
        menu.title.clear();
        renderTransp(0, 0,
                     m_real_frame_pm.width(), m_real_frame_pm.height());
        for (size_t i=0; i < menuitems.size(); ++i) {
            drawItem(i,
                     true, // clear
                     false); // transparent
        }
        m_need_update = false;
    }

}


void Menu::redrawTitle() {
    const char *text = menu.label.c_str();

    const FbTk::Font &font = theme().titleFont();
    int dx = theme().bevelWidth(), len = menu.label.size();
    unsigned int l = font.textWidth(text, len) + theme().bevelWidth()*2;

    switch (theme().titleFontJustify()) {
    case FbTk::RIGHT:
        dx += width() - l;
        break;

    case FbTk::CENTER:
        dx += (width() - l) / 2;
        break;
    default:
        break;
    }

    if (menu.title.alpha() != alpha())
        menu.title.setAlpha(alpha());

    FbTk::GContext def_gc(menu.title);

    m_real_title_pm.copyArea(m_title_pm.drawable(),
                             def_gc.gc(),
                             0, 0,
                             0, 0,
                             m_title_pm.width(), m_title_pm.height());

    menu.title.updateTransparent();
    font.drawText(m_real_title_pm.drawable(), // drawable
                  screenNumber(),
                  theme().titleTextGC().gc(), // graphic context
                  text, len,  // text string with lenght
                  dx, theme().titleHeight()/2 + (font.ascent() - theme().bevelWidth())/2);  // position
}


void Menu::drawSubmenu(unsigned int index) {
    if (validIndex(which_sub) && static_cast<unsigned int>(which_sub) != index) {
        MenuItem *itmp = menuitems[which_sub];

        if (! itmp->submenu()->isTorn())
            itmp->submenu()->internal_hide();
    }

    if (index >= menuitems.size())
        return;

    MenuItem *item = menuitems[index];
    if (item->submenu() && visible && (! item->submenu()->isTorn()) &&
        item->isEnabled()) {
			
        if (item->submenu()->m_parent != this)
            item->submenu()->m_parent = this;
			
        int sbl = index / menu.persub, i = index - (sbl * menu.persub);
        int new_x = x() + ((menu.item_w * (sbl + 1)) + menu.window.borderWidth());
        int new_y;
		
        if (m_alignment == ALIGNTOP) {
            new_y = (((shifted) ? menu.y_shift : y()) +
                     ((title_vis) ? theme().titleHeight() + menu.title.borderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->theme().titleHeight() + menu.window.borderWidth() : 0));
        } else {
            new_y = (((shifted) ? menu.y_shift : y()) +
                     (theme().itemHeight() * i) +
                     ((title_vis) ? theme().titleHeight() + menu.window.borderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->theme().titleHeight() + menu.window.borderWidth() : 0));
        }
			
        if (m_alignment == ALIGNBOTTOM &&
            (new_y + item->submenu()->height()) > ((shifted) ? menu.y_shift :
                                                   y()) + height()) {
            new_y = (((shifted) ? menu.y_shift : y()) +
                     height() - item->submenu()->height());
        }

        if ((new_x + item->submenu()->width()) > m_screen_width) {
            new_x = ((shifted) ? menu.x_shift : x()) -
                item->submenu()->width() - menu.window.borderWidth();
        }
			
        if (new_x < 0)
            new_x = 0;

        if ((new_y + item->submenu()->height()) > m_screen_height) {
            new_y = m_screen_height - item->submenu()->height() -
                menu.window.borderWidth() * 2;
        }
			
        item->submenu()->moving = moving;
        which_sub = index;

        if (new_y < 0)
            new_y = 0;

        item->submenu()->move(new_x, new_y);
        if (! moving)
            drawItem(index);
		
        if (! item->submenu()->isVisible()) {
            item->submenu()->show();
            item->submenu()->raise();
        }
			

    } else
        which_sub = -1;

}


bool Menu::hasSubmenu(unsigned int index) const {
    if (index >= menuitems.size()) //boundary check
        return false;
	
    if (!menuitems[index]->submenu()) //has submenu?
        return false;
	
    return true;	
}


int Menu::drawItem(unsigned int index, bool clear, bool render_trans,
                   int x, int y, unsigned int w, unsigned int h) {
    if (index >= menuitems.size() || menuitems.size() == 0 ||
        menu.persub == 0)
        return 0;

    MenuItem *item = menuitems[index];
    if (! item) return 0;

    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    int item_x = (sbl * menu.item_w), item_y = (i * theme().itemHeight());
    int hilite_x = item_x, hilite_y = item_y, hoff_x = 0, hoff_y = 0;
    int sel_x = 0, sel_y = 0;
    unsigned int hilite_w = menu.item_w, hilite_h = theme().itemHeight();
    unsigned int half_w = theme().itemHeight() / 2, quarter_w = theme().itemHeight() / 4;
    bool highlight = (index == m_active_index);
    GC gc =
        ((highlight || item->isSelected()) ? theme().hiliteTextGC().gc() :
         theme().frameTextGC().gc());
	
    sel_x = item_x;
	
    if (theme().bulletPos() == FbTk::RIGHT)
        sel_x += (menu.item_w - theme().itemHeight() - theme().bevelWidth());
	
    sel_x += quarter_w;
    sel_y = item_y + quarter_w;

    if (clear) {
        GContext def_gc(menu.frame);
        if (menu.frame_pixmap == 0) {
            def_gc.setForeground(theme().frameTexture().color());
            m_frame_pm.fillRectangle(def_gc.gc(), item_x, item_y, menu.item_w, theme().itemHeight());

        } else {

            m_frame_pm.copyArea(menu.frame_pixmap, def_gc.gc(),
                                item_x, item_y,
                                item_x, item_y,
                                menu.item_w, theme().itemHeight());
        }    
    } else if (! (x == y && y == -1 && w == h && h == 0)) {
        // calculate the which part of the hilite to redraw
        if (!(std::max(item_x, x) <= (signed) std::min(item_x + menu.item_w, x + w) &&
              std::max(item_y, y) <= (signed) std::min(item_y + theme().itemHeight(), y + h))) {
            hilite_x = std::max(item_x, x);
            hilite_y = std::max(item_y, y);
            hilite_w = std::min(item_x + menu.item_w, x + w) - hilite_x;
            hilite_h = std::min(item_y + theme().itemHeight(), y + h) - hilite_y;
            hoff_x = hilite_x % menu.item_w;
            hoff_y = hilite_y % theme().itemHeight();
        }
		
    }
    
    if (highlight && (menu.hilite_pixmap != ParentRelative)) {
        if (menu.hilite_pixmap) {
            m_frame_pm.copyArea(menu.hilite_pixmap,
                                theme().hiliteGC().gc(), hoff_x, hoff_y,
                                hilite_x, hilite_y,
                                hilite_w, hilite_h);
        } else {            
            m_frame_pm.fillRectangle(theme().hiliteGC().gc(),
                                     hilite_x, hilite_y, hilite_w, hilite_h);
        }
        
    } 
	

    if (render_trans) {
        renderTransp(item_x, item_y,
                     width(), theme().itemHeight()); 
    }
    //!!
    //!! TODO: Move this out to MenuItem
    //!! current problem: menu.sel_pixmap needs a image control instance
    //!! to be generated :(
    //!!
    if (item->isToggleItem() && item->isSelected()) {
        Display *disp = FbTk::App::instance()->display();
        if (theme().selectedPixmap().pixmap().drawable()) {

            // enable clip mask
            XSetClipMask(disp,
                         gc,
                         theme().selectedPixmap().mask().drawable());
            XSetClipOrigin(disp,
                           gc, sel_x, item_y);
            // copy bullet pixmap to frame
            m_real_frame_pm.copyArea(theme().selectedPixmap().pixmap().drawable(),
                                     gc,
                                     0, 0,
                                     sel_x, item_y,
                                     theme().selectedPixmap().width(),
                                     theme().selectedPixmap().height());
            // disable clip mask
            XSetClipMask(disp,
                         gc,
                         None);
        } else {
            if (menu.sel_pixmap) {
                m_real_frame_pm.copyArea(highlight ? menu.frame_pixmap : menu.sel_pixmap,
                                         theme().hiliteGC().gc(), 
                                         0, 0,                                 
                                         sel_x, sel_y,
                                         half_w, half_w);
            } else {
                m_real_frame_pm.fillRectangle(theme().hiliteGC().gc(),
                                              sel_x, sel_y, half_w, half_w);
            }
        }
        
    }

    item->draw(m_real_frame_pm, theme(), highlight, 
               item_x, item_y, 
               menu.item_w, theme().itemHeight());


    if (clear) {
        frameWindow().clearArea(item_x, item_y,
                                menu.item_w, theme().itemHeight(), False);
    }


    return item_y;
}

void Menu::setLabel(const char *labelstr) {
    //make sure we don't send 0 to std::string
    menu.label = (labelstr ? labelstr : "");
    reconfigure();
}


void Menu::setItemSelected(unsigned int index, bool sel) {
    if (index >= menuitems.size()) return;

    MenuItem *item = find(index);
    if (! item) return;

    item->setSelected(sel);

}


bool Menu::isItemSelected(unsigned int index) const{
    if (index >= menuitems.size()) return false;

    const MenuItem *item = find(index);
    if (!item)
        return false;

    return item->isSelected();
}


void Menu::setItemEnabled(unsigned int index, bool enable) {
    if (index >= menuitems.size()) return;

    MenuItem *item = find(index);
    if (! item) return;

    item->setEnabled(enable);

}


bool Menu::isItemEnabled(unsigned int index) const {
    if (index >= menuitems.size()) return false;

    const MenuItem *item = find(index);
    if (!item)
        return false;

    return item->isEnabled();
}

void Menu::handleEvent(XEvent &event) {
    if (event.type == FocusOut) {
        if (s_focused == this)
            s_focused = 0;
    } else if (event.type == FocusIn) {
        if (s_focused != this)
            s_focused = this; 
    }
}

void Menu::buttonPressEvent(XButtonEvent &be) {
    if (be.window == menu.title)
        grabInputFocus();

    if (be.window == menu.frame && menu.item_w != 0) {

        int sbl = (be.x / menu.item_w), i = (be.y / theme().itemHeight());
        int w = (sbl * menu.persub) + i;

        if (validIndex(w)) {
            which_press = i;
            which_sbl = sbl;

            MenuItem *item = menuitems[w];

            if (item->submenu()) {
                if (!item->submenu()->isVisible())
                    drawSubmenu(w);
            } else
                drawItem(w, 
                         true,  // clear
                         true); // render transparency

        }
    } else {
        menu.x_move = be.x_root - x();
        menu.y_move = be.y_root - y();
    }
}


void Menu::buttonReleaseEvent(XButtonEvent &re) {
    if (re.window == menu.title) {
        if (moving) {
            moving = false;

            if (which_sub != -1)
                drawSubmenu(which_sub);

            if (alpha() < 255) {
                redrawTitle();
                menu.title.clear();
                renderTransp(0, 0,
                             m_real_frame_pm.width(), m_real_frame_pm.height());
                for (size_t i=0; i < menuitems.size(); ++i) {
                    drawItem(i, 
                             true, // clear
                             false); // transparent
                }

            }
        }

        if (re.x >= 0 && re.x <= (signed) width() &&
            re.y >= 0 && re.y <= (signed) theme().titleHeight() &&
            re.button == 3)
            hide();
			
    } else if (re.window == menu.frame &&
               re.x >= 0 && re.x < (signed) width() &&
               re.y >= 0 && re.y < (signed) menu.frame_h) {
			
        int sbl = (re.x / menu.item_w), i = (re.y / theme().itemHeight()),
            ix = sbl * menu.item_w, iy = i * theme().itemHeight(),
            w = (sbl * menu.persub) + i,
            p = (which_sbl * menu.persub) + which_press;

        if (validIndex(w)) {
            if (p == w && isItemEnabled(w)) {
                if (re.x > ix && re.x < (signed) (ix + menu.item_w) &&
                    re.y > iy && re.y < (signed) (iy + theme().itemHeight())) {
                    menuitems[w]->click(re.button, re.time);
                    itemSelected(re.button, w);
                    // just redraw this item
                    drawItem(w, 
                             true,  // clear
                             true); // transparent
                }
            } else {
                drawItem(p, 
                         true,  // clear
                         true); // transparent
            }
            
        } else {
            drawItem(p,
                     true,  // clear
                     true); // transparent
        }
    }
}


void Menu::motionNotifyEvent(XMotionEvent &me) {
    // if draging the with the titlebar:
    if (me.window == menu.title && (me.state & Button1Mask)) {
        stopHide();

        if (! moving) {
            // if not moving: start moving operation
            if (m_parent && (! torn)) {
                m_parent->drawItem(m_parent->which_sub, 
                                   true,  // clear
                                   true); // render transparency
                m_parent->which_sub = -1;
            }

            moving = torn = true;

            if (which_sub >= 0)
                drawSubmenu(which_sub);
        } else {
            // we dont call ::move here 'cause we dont want to update transparency
            // while draging the menu (which is slow)
            menu.window.move(me.x_root - menu.x_move, me.y_root - menu.y_move);
        }

    } else if (!(me.state & Button1Mask) && me.window == menu.frame) {
        stopHide();
        int sbl = (me.x / menu.item_w), 
            i = (me.y / theme().itemHeight()),
            w = (sbl * menu.persub) + i;
       
        if (validIndex(m_active_index) && w != m_active_index) {

            int old = m_active_index;
            m_active_index = -1;
            MenuItem *item = menuitems[old];

            if (item != 0) {

                drawItem(old,
                         true,  // clear
                         true); // transparent

                if (item->submenu()) {

                    if (item->submenu()->isVisible() &&
                        !item->submenu()->isTorn()) {
                        // setup hide timer for submenu
                        item->submenu()->startHide();
                    }
                }                

            }

        }


        which_press = i;
        which_sbl = sbl;

        m_active_index = -1;

        if (!validIndex(w))
            return;


        MenuItem *itmp = menuitems[w];

        m_active_index = w;

        if (itmp == 0)
            return;

        if (itmp->submenu()) {
            // if submenu,
            // draw item highlighted and
            // start submenu open delay

            drawItem(w, 
                     true); // clear
            
            if (theme().menuMode() == MenuTheme::DELAY_OPEN) {
                // setup show menu timer
                timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = theme().delayOpen() * 1000; // transformed to usec
                m_submenu_timer.setTimeout(timeout);
                m_submenu_timer.start();

            }

        } else { 
            // else normal menu item
            // draw highlighted
            m_submenu_timer.stop();
            if (itmp->isEnabled()) {
                drawItem(w, 
                         true,  // clear
                         true); // transp
            }
        }
        
    }
}


void Menu::exposeEvent(XExposeEvent &ee) {
    if (ee.window == menu.title) {
        redrawTitle();
        menu.title.clearArea(ee.x, ee.y, ee.width, ee.height);
    } else if (ee.window == menu.frame)
        menu.frame.clearArea(ee.x, ee.y, ee.width, ee.height);
}


void Menu::enterNotifyEvent(XCrossingEvent &ce) {

    if (menu.frame != ce.window)
        return;

    menu.x_shift = x(), menu.y_shift = y();
    if (x() + width() > m_screen_width) {
        menu.x_shift = m_screen_width - width() - 2*theme().borderWidth();
        shifted = true;
    } else if (x() < 0) {
        menu.x_shift = 0; //-theme().borderWidth();
        shifted = true;
    }

    if (y() + height() + 2*theme().borderWidth() > m_screen_height) {
        menu.y_shift = m_screen_height - height() - 2*theme().borderWidth();
        shifted = true;
    } else if (y() + (signed) theme().titleHeight() < 0) {
        menu.y_shift = 0; // -theme().borderWidth();;
        shifted = true;
    }


    if (shifted)
        menu.window.move(menu.x_shift, menu.y_shift);
    
    if (validIndex(which_sub)) {
        MenuItem *tmp = menuitems[which_sub];
        if (tmp->submenu()->isVisible()) {
            int sbl = (ce.x / menu.item_w), i = (ce.y / theme().itemHeight()),
                w = (sbl * menu.persub) + i;

            if (w != which_sub && (! tmp->submenu()->isTorn())) {
                tmp->submenu()->internal_hide();

                drawItem(which_sub, 
                         true,  // clear
                         true); // transp
                which_sub = -1;
            }
        }
    }
}

void Menu::leaveNotifyEvent(XCrossingEvent &ce) {
    if (menu.frame != ce.window)
        return;

    if (which_press != -1 && which_sbl != -1 && menuitems.size() > 0) {
        int p = (which_sbl * menu.persub) + which_press;

        drawItem(p, 
                 true,  // clear
                 true); // transp

        which_sbl = which_press = -1;
    }

    if (shifted) {
        //        menu.window.move(menu.x, menu.y);
        shifted = false;
    }
}

void Menu::keyPressEvent(XKeyEvent &event) {
    KeySym ks;
    char keychar[1];
    XLookupString(&event, keychar, 1, &ks, 0);
    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks)) 
        return;

    switch (ks) {
    case XK_Up:
        prevItem();
        break;
    case XK_Down:
        nextItem();
        break;
    case XK_Left: // enter parent if we have one
        enterParent(); 
        break;
    case XK_Right: // enter submenu if we have one
        enterSubmenu();
        break;
    case XK_Escape: // close menu
        hide();
        break;
    case XK_Return:
        // send fake button 1 click
        if (validIndex(which_press) && 
            isItemEnabled(which_press)) {
            menuitems[which_press]->click(1, event.time);
            itemSelected(1, which_press);
            m_need_update = true;
            update();
        }
        break;
    default:
        break;
    }
}

void Menu::reconfigure() {


    if (alpha() == 255 && m_transp.get() != 0) {
        m_transp.reset(0);
    } else if (alpha () < 255) {

        if (m_transp.get() == 0) {
            m_transp.reset(new Transparent(getRootPixmap(screenNumber()),
                                           m_real_frame_pm.drawable(), alpha(),
                                           screenNumber()));
        } else
            m_transp->setAlpha(alpha());
    }

    m_need_update = true; // redraw items


    menu.title.setAlpha(alpha());

    menu.window.setBorderColor(theme().borderColor());
    menu.title.setBorderColor(theme().borderColor());
    menu.frame.setBorderColor(theme().borderColor());

    menu.window.setBorderWidth(theme().borderWidth());
    menu.title.setBorderWidth(theme().borderWidth());
    

    update();

}
    

void Menu::openSubmenu() {
    if (!isVisible() || ! validIndex(which_press) ||
        ! validIndex(which_sbl))
        return;

    int item = which_sbl * menu.persub + which_press;
    if (!validIndex(item))
        return;

    drawItem(item, 
             true); // clear
    if (menuitems[item]->submenu() != 0 && !menuitems[item]->submenu()->isVisible())
        drawSubmenu(item);

}

void Menu::closeMenu() {
    if (isVisible() && !isTorn())
        internal_hide();
}

void Menu::startHide() {
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = theme().delayClose() * 1000; // transformed to usec
    m_hide_timer.setTimeout(timeout);
    m_hide_timer.start(); 
}

void Menu::stopHide() {
    m_hide_timer.stop();
}

void Menu::update(FbTk::Subject *subj) {

    m_need_update = true;
    
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it)
        (*it)->updateTheme(theme());
    reconfigure();
}

                  
void Menu::renderTransp(int x, int y,
                        unsigned int width, unsigned int height) {
    // even though we dont render transparency
    // we do need to copy the style background
    GContext def_gc(menu.frame);
    m_real_frame_pm.copyArea(m_frame_pm.drawable(),
                             def_gc.gc(),
                             x, y,
                             x, y,
                             width, height);

    // no need to render transparent unless visible
    // but we do need to render it if we marked it as
    // need update
    if (!isVisible() && !m_need_update || m_transp.get() == 0)
        return;


    // render the root background
#ifdef HAVE_XRENDER

    Pixmap root = getRootPixmap(screenNumber());
    if (m_transp->source() != root)
        m_transp->setSource(root, screenNumber());


    if (m_transp->dest() != m_real_frame_pm.drawable())
        m_transp->setDest(m_real_frame_pm.drawable(), screenNumber());

    if (m_transp->alpha() != alpha())
        m_transp->setAlpha(alpha());

    const FbWindow *root_parent = menu.frame.parent();
    // our position in parent ("root")
    int root_x = menu.frame.x() - menu.frame.borderWidth(), 
        root_y = menu.frame.y() - menu.frame.borderWidth();
    if (root_parent != 0) {
        root_x += root_parent->x() + root_parent->borderWidth();
        root_y += root_parent->y() + root_parent->borderWidth();
        while (root_parent->parent() != 0) {
            root_parent = root_parent->parent();
            root_x += root_parent->x() + root_parent->borderWidth();
            root_y += root_parent->y() + root_parent->borderWidth();
        }

    } // else toplevel window so we already have x, y set

    // render background image from root pos to our window
    m_transp->render(root_x + x, root_y + y,
                     x, y,
                     width, height);
        
#endif // HAVE_XRENDER
        
}

}; // end namespace FbTk
