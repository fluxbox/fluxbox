// Menu.cc for FbTk - Fluxbox Toolkit 
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Menu.cc,v 1.52 2004/01/08 22:07:00 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include "Menu.hh"

#include "MenuItem.hh"
#include "ImageControl.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "EventManager.hh"
#include "Transparent.hh"
#include "SimpleCommand.hh"

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

Menu::Menu(MenuTheme &tm, ImageControl &imgctrl):
    m_theme(tm),
    m_parent(0),
    m_image_ctrl(imgctrl),
    m_screen_width(DisplayWidth(FbTk::App::instance()->display(), tm.screenNum())),
    m_screen_height(DisplayHeight(FbTk::App::instance()->display(), tm.screenNum())),
    m_alignment(ALIGNDONTCARE),
    m_border_width(0),
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

    title_vis =
        movable =
        hide_tree = true;

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

    menu.bevel_w = 2;

    menu.title_h = menu.item_w = menu.frame_h =
        m_theme.titleFont().height() + menu.bevel_w * 2;

    menu.sublevels =
        menu.persub =
        menu.minsub = 0;

    menu.item_h = m_theme.frameFont().height() + menu.bevel_w;

    long event_mask = ButtonPressMask | ButtonReleaseMask | 
        ButtonMotionMask | KeyPressMask | ExposureMask | FocusChangeMask;
    // create menu window
    menu.window = FbTk::FbWindow(tm.screenNum(),
                                 0, 0, 10, 10,
                                 event_mask,
                                 true); // override redirect

    // strip focus change mask from attrib, since we should only use it with main window
    event_mask ^= FocusChangeMask;

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(*this, menu.window);
	

    event_mask |= EnterWindowMask | LeaveWindowMask;
    //create menu title
    menu.title = FbTk::FbWindow(menu.window,
                                0, 0, width(), menu.title_h,
                                event_mask);
                                
    evm.add(*this, menu.title);

    event_mask |= PointerMotionMask;
    menu.frame = FbTk::FbWindow(menu.window,
                                0, menu.title_h,
                                width(), menu.frame_h ? menu.frame_h : 1, 
                                event_mask);
    evm.add(*this, menu.frame);
    // update style 
    reconfigure();
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
    evm.remove(menu.title);
    evm.remove(menu.frame);
    evm.remove(menu.window);
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
        if ((! internal_menu) && (item->submenu())) {
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
    if (which_press >= 0 && which_press == static_cast<signed>(menuitems.size() - 1))
        return;

    int old_which_press = which_press;

    if (old_which_press >= 0 && 
        old_which_press < static_cast<signed>(menuitems.size()) && 
        menuitems[old_which_press] != 0) {
        if (menuitems[old_which_press]->submenu()) {
            // we need to do this explicitly on the menu.window
            // since it might hide the parent if we use Menu::hide
            menuitems[old_which_press]->submenu()->menu.window.hide();
        }
        drawItem(old_which_press, false, true, true);
    }

    // restore old in case we changed which_press
    which_press = old_which_press;
    if (which_press < 0 || which_press >= static_cast<signed>(menuitems.size()))
        which_press = 0;
    else if (which_press > 0 && which_press < static_cast<signed>(menuitems.size() - 1))
        which_press++;


    if (menuitems[which_press] == 0)
        return;

    if (menuitems[which_press]->submenu())
        drawSubmenu(which_press);
    else
        drawItem(which_press, true, true, true);

}

void Menu::prevItem() {

    int old_which_press = which_press;

    if (old_which_press >= 0 && old_which_press < static_cast<signed>(menuitems.size())) {
        if (menuitems[old_which_press]->submenu()) {
            // we need to do this explicitly on the menu.window
            // since it might hide the parent if we use Menu::hide
            menuitems[old_which_press]->submenu()->menu.window.hide();            
        }
        drawItem(old_which_press, false, true, true);
    }
    // restore old in case we changed which_press
    which_press = old_which_press;

    if (which_press < 0 || which_press >= static_cast<signed>(menuitems.size()))
        which_press = 0;
    else if (which_press - 1 >= 0)
        which_press--;

    if (menuitems[which_press] != 0) {
        if (menuitems[which_press]->submenu())
            drawSubmenu(which_press);
        else
            drawItem(which_press, true, true, true);
    }

}

void Menu::enterSubmenu() {
    if (which_press < 0 || which_press >= static_cast<signed>(menuitems.size()))
        return;

    Menu *submenu = menuitems[which_press]->submenu();
    if (submenu == 0)
        return;

    submenu->grabInputFocus();
    submenu->which_press = -1; // so we land on 0 after nextItem()
    submenu->nextItem();
}

void Menu::enterParent() {
    if (which_press < 0 || which_press >= static_cast<signed>(menuitems.size()) || parent() == 0)
        return;

    Menu *submenu = menuitems[which_press]->submenu();
    if (submenu)
        submenu->menu.window.hide();

    drawItem(which_press, false, true, true);
    which_press = -1; // dont select any in this 
    // return focus to parent but keep this window open
    parent()->grabInputFocus();
}

void Menu::disableTitle() {
    setTitleVisibility(false);
}

void Menu::enableTitle() {
    setTitleVisibility(true);
}

void Menu::update(int active_index) {

    if (menu.bevel_w > 10) // clamp to "normal" size
        menu.bevel_w = 10;
    if (m_border_width > 20)
        m_border_width = 20;

    menu.item_h = m_theme.frameFont().height() + menu.bevel_w;
    menu.title_h = m_theme.titleFont().height() + menu.bevel_w*2;

    if (title_vis) {
        menu.item_w = m_theme.frameFont().textWidth(menu.label.c_str(), menu.label.size());
		
        menu.item_w += (menu.bevel_w * 2);
    }	else
        menu.item_w = 1;

    int ii = 0;
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it) {
        MenuItem *itmp = (*it);

        const char *s = itmp->label().c_str();
        int l = itmp->label().size();

        ii = m_theme.frameFont().textWidth(s, l);
			

        ii += (menu.bevel_w * 2) + (menu.item_h * 2);

        menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
    }

    if (menuitems.size()) {
        menu.sublevels = 1;

        while (menu.item_h * (menuitems.size() + 1) / menu.sublevels +
               menu.title_h + m_border_width > m_screen_height) {
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

    int itmp = (menu.item_h * menu.persub);
    menu.frame_h = itmp < 0 ? 0 : itmp;

    int new_width = (menu.sublevels * menu.item_w);
    int new_height = menu.frame_h;

    if (title_vis)
        new_height += menu.title_h + ((menu.frame_h>0)?menu.title.borderWidth():0);


    if (new_width < 1) 
        new_width = menu.item_w;

    if (new_height < 1)
        new_height = 1;

    menu.window.resize(new_width, new_height);

    Pixmap tmp;
    if (title_vis) {
        tmp = menu.title_pixmap;
        const FbTk::Texture &tex = m_theme.titleTexture();
        if (!tex.usePixmap()) {
            menu.title_pixmap = None;
            menu.title.setBackgroundColor(tex.color());
        } else {
            menu.title_pixmap =
                m_image_ctrl.renderImage(width(), menu.title_h, tex);
            menu.title.setBackgroundPixmap(menu.title_pixmap);
        }

        if (tmp) 
            m_image_ctrl.removeImage(tmp);

    }

    tmp = menu.frame_pixmap;
    const FbTk::Texture &frame_tex = m_theme.frameTexture();
    if (!frame_tex.usePixmap()) {
        menu.frame_pixmap = None;
    } else {
        menu.frame_pixmap =
            m_image_ctrl.renderImage(width(), menu.frame_h, frame_tex);        
    }

    if (tmp)
        m_image_ctrl.removeImage(tmp);

    tmp = menu.hilite_pixmap;
    const FbTk::Texture &hilite_tex = m_theme.hiliteTexture();
    if (!hilite_tex.usePixmap()) {
        menu.hilite_pixmap = None;
    } else
        menu.hilite_pixmap =
            m_image_ctrl.renderImage(menu.item_w, menu.item_h, hilite_tex);
    if (tmp)
        m_image_ctrl.removeImage(tmp);

    tmp = menu.sel_pixmap;
    if (!hilite_tex.usePixmap()) {
        menu.sel_pixmap = None;
    } else {
        int hw = menu.item_h / 2;
        menu.sel_pixmap =
            m_image_ctrl.renderImage(hw, hw, hilite_tex);
    }
    if (tmp) 
        m_image_ctrl.removeImage(tmp);



    if (title_vis) {
        menu.title.moveResize(-menu.title.borderWidth(), -menu.title.borderWidth(), 
                              width() + menu.title.borderWidth(), menu.title_h);
    }

    menu.frame.moveResize(0, ((title_vis) ? menu.title.y() + menu.title.height() + 
                              menu.title.borderWidth()*2 : 0), 
                          menu.window.width(), menu.frame_h);

    // render pixmaps
    Display *disp = FbTk::App::instance()->display();

    XWindowAttributes attr;

    if (m_need_update && (m_frame_pm.width() != menu.frame.width() ||
                          m_frame_pm.height() != menu.frame.height() )){
        XGetWindowAttributes(disp, menu.frame.window(), &attr);
        m_frame_pm = FbTk::FbPixmap(menu.frame.window(),
                                    menu.frame.width(), menu.frame.height(),
                                    attr.depth);

        if (m_frame_pm.drawable() == 0) {
            cerr<<"FbTk::Menu: Warning: Failed to create pixmap ("<<
                menu.frame.window()<<", "<<menu.frame.width()<<", "<<
                menu.frame.height()<<
                ", "<<attr.depth<<") !"<<endl;
        }

       
    }

    menu.frame.setBackgroundPixmap(m_frame_pm.drawable());

    clearWindow();

    if (title_vis && visible) 
        redrawTitle();

    if (active_index >= 0) {
        for (unsigned int i = 0; visible && i < menuitems.size(); i++) {
            if (i == (unsigned int)which_sub) {
                drawItem(i, true, true, false);
            } else
                drawItem(i, (static_cast<signed>(i) == active_index && isItemEnabled(i)), true, false);
        }

        if (m_parent && visible)
            m_parent->drawSubmenu(m_parent->which_sub);
    }

    menu.window.clear();
    renderTransFrame();
                    
    m_need_update = false;
    menu.window.showSubwindows();
}


void Menu::show() {
    if (m_need_update)
        update();
    menu.window.showSubwindows();
    menu.window.show();
    //!! TODO, this should probably be done explicit if one don't want to raise
    raise();
    visible = true;

    if (! m_parent && shown != this) {
        if (shown && (! shown->torn))
            shown->hide();

        shown = this;
    }

}


void Menu::hide() {
    if ((! torn) && hide_tree && m_parent && m_parent->isVisible()) {
        Menu *p = m_parent;

        while (p->isVisible() && (! p->torn) && p->m_parent)
            p = p->m_parent;
        p->internal_hide();
    } else
        internal_hide();
    
}

void Menu::grabInputFocus() {
    s_focused = this;

    // grab input focus
    menu.window.setInputFocus(RevertToPointerRoot, CurrentTime);

}


void Menu::clearWindow() {
    menu.window.clear();
    menu.title.clear();
    menu.frame.clear();
}

void Menu::internal_hide() {
    if (which_sub >= 0) {
        MenuItem *tmp = menuitems[which_sub];
        tmp->submenu()->internal_hide();
    }

    if (m_parent && (! torn)) {
        m_parent->drawItem(m_parent->which_sub, false, true);

        m_parent->which_sub = -1;
    } else if (shown && shown->menu.window == menu.window)
        shown = (Menu *) 0;

    torn = visible = false;
    which_sub = which_press = which_sub = -1;

    menu.window.hide();
}


void Menu::move(int x, int y) {

    menu.window.move(x, y);

    if (which_sub != -1)
        drawSubmenu(which_sub);

    if (!(m_parent && m_parent->moving) && !torn) {
        redrawTitle();
        renderTransFrame();
    }
}


void Menu::redrawTitle() {
    const char *text = menu.label.c_str();

    const FbTk::Font &font = m_theme.titleFont();
    int dx = menu.bevel_w, len = menu.label.size();
    unsigned int l = font.textWidth(text, len) + menu.bevel_w*2;

    switch (m_theme.titleFontJustify()) {
    case FbTk::RIGHT:
        dx += width() - l;
        break;

    case FbTk::CENTER:
        dx += (width() - l) / 2;
        break;
    default:
        break;
    }
    menu.title.clear();
    font.drawText(menu.title.window(), // drawable
                  screenNumber(),
                  m_theme.titleTextGC().gc(), // graphic context
                  text, len,  // text string with lenght
                  dx, font.ascent() + menu.bevel_w);  // position

    menu.title.updateTransparent();
    
}


void Menu::drawSubmenu(unsigned int index) {
    if (which_sub >= 0 && static_cast<unsigned int>(which_sub) != index && 
        static_cast<unsigned int>(which_sub) < menuitems.size()) {
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
                     ((title_vis) ? menu.title_h + menu.title.borderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->menu.title_h + menu.window.borderWidth() : 0));
        } else {
            new_y = (((shifted) ? menu.y_shift : y()) +
                     (menu.item_h * i) +
                     ((title_vis) ? menu.title_h + menu.window.borderWidth() : 0) -
                     ((item->submenu()->title_vis) ?
                  item->submenu()->menu.title_h + menu.window.borderWidth() : 0));
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
			
        if (new_y < 0)
            new_y = 0;

        item->submenu()->move(new_x, new_y);
        if (! moving)
            drawItem(index, true);
		
        if (! item->submenu()->isVisible()) {
            item->submenu()->show();
            item->submenu()->raise();
        }
			
        item->submenu()->moving = moving;
        which_sub = index;
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


void Menu::drawItem(unsigned int index, bool highlight, bool clear, bool render_trans,
                    int x, int y, unsigned int w, unsigned int h) {
    if (index >= menuitems.size() || menuitems.size() == 0 || 
        menu.persub == 0)
        return;

    MenuItem *item = menuitems[index];
    if (! item) return;

    bool dotext = true, dohilite = true, dosel = true;
    const char *text = item->label().c_str();
    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    int item_x = (sbl * menu.item_w), item_y = (i * menu.item_h);
    int hilite_x = item_x, hilite_y = item_y, hoff_x = 0, hoff_y = 0;
    int text_x = 0, text_y = 0, len = strlen(text), sel_x = 0, sel_y = 0;
    unsigned int hilite_w = menu.item_w, hilite_h = menu.item_h, text_w = 0, text_h = 0;
    unsigned int half_w = menu.item_h / 2, quarter_w = menu.item_h / 4;
    const FbTk::Font &font = m_theme.frameFont();
    if (text) {		
        text_w = font.textWidth(text, len);

        text_y = item_y + menu.bevel_w/2 + font.ascent();

        switch(m_theme.frameFontJustify()) {
        case FbTk::LEFT:
            text_x = item_x + menu.bevel_w + menu.item_h + 1;
            break;
			
        case FbTk::RIGHT:
            text_x = item_x + menu.item_w - (menu.item_h + menu.bevel_w + text_w);
            break;			
        default: //center
            text_x = item_x + ((menu.item_w + 1 - text_w) / 2);
            break;
        }

        text_h = menu.item_h - menu.bevel_w;
    }
	
    GC gc =
        ((highlight || item->isSelected()) ? m_theme.hiliteTextGC().gc() :
         m_theme.frameTextGC().gc());
    const GContext &tgc =
        (highlight ? m_theme.hiliteTextGC() :
         (item->isEnabled() ? m_theme.frameTextGC() : m_theme.disableTextGC() ) );
	
    sel_x = item_x;
	
    if (m_theme.bulletPos() == FbTk::RIGHT)
        sel_x += (menu.item_w - menu.item_h - menu.bevel_w);
	
    sel_x += quarter_w;
    sel_y = item_y + quarter_w;

    if (clear) {
        FbTk::GContext def_gc(menu.frame.window());
        if (menu.frame_pixmap == 0) {
            def_gc.setForeground(m_theme.frameTexture().color());
            m_frame_pm.fillRectangle(def_gc.gc(), item_x, item_y, menu.item_w, menu.item_h);

        } else {

            m_frame_pm.copyArea(menu.frame_pixmap, def_gc.gc(),
                                item_x, item_y,
                                item_x, item_y,
                                menu.item_w, menu.item_h);
        }    
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
            dotext = false;
		
        // check if we need to redraw the select pixmap/menu bullet
        if (! (std::max(sel_x, x) <= (signed) std::min(sel_x + half_w, x + w) &&
               std::max(sel_y, y) <= (signed) std::min(sel_y + half_w, y + h)))
            dosel = false;
	
    }
    
    if (dohilite && highlight && (menu.hilite_pixmap != ParentRelative)) {
        if (menu.hilite_pixmap) {
            m_frame_pm.copyArea(menu.hilite_pixmap,
                                m_theme.hiliteGC().gc(), hoff_x, hoff_y,
                                hilite_x, hilite_y,
                                hilite_w, hilite_h);
        } else {            
            m_frame_pm.fillRectangle(m_theme.hiliteGC().gc(),
                                     hilite_x, hilite_y, hilite_w, hilite_h);
        }
        
    } 
	
    
    if (item->isToggleItem() && item->isSelected() &&
        menu.sel_pixmap != ParentRelative) {
        if (m_theme.selectedPixmap().pixmap().drawable()) {
            // enable clip mask
            XSetClipMask(FbTk::App::instance()->display(),
                         gc,
                         m_theme.selectedPixmap().mask().drawable());
            XSetClipOrigin(FbTk::App::instance()->display(),
                           gc, sel_x, item_y);
            // copy bullet pixmap to frame
            m_frame_pm.copyArea(m_theme.selectedPixmap().pixmap().drawable(),
                                gc,
                                0, 0,
                                sel_x, item_y,
                                m_theme.selectedPixmap().width(),
                                m_theme.selectedPixmap().height());
            // disable clip mask
            XSetClipMask(FbTk::App::instance()->display(),
                         gc,
                         None);
        } else {
            if (menu.sel_pixmap) {
                m_frame_pm.copyArea(highlight ? menu.frame_pixmap : menu.sel_pixmap,
                                    m_theme.hiliteGC().gc(), 
                                    0, 0,                                 
                                    sel_x, sel_y,
                                    half_w, half_w);
            } else {
                m_frame_pm.fillRectangle(m_theme.hiliteGC().gc(),
                                         sel_x, sel_y, half_w, half_w);
            }
        }
        
    } else if (item->isToggleItem() && m_theme.unselectedPixmap().pixmap().drawable() != 0) {
        // enable clip mask
        XSetClipMask(FbTk::App::instance()->display(),
                     gc,
                     m_theme.unselectedPixmap().mask().drawable());
        XSetClipOrigin(FbTk::App::instance()->display(),
                       gc, sel_x, item_y);
        // copy bullet pixmap to frame
        m_frame_pm.copyArea(m_theme.unselectedPixmap().pixmap().drawable(),
                            gc,
                            0, 0,
                            sel_x, item_y,
                            m_theme.unselectedPixmap().width(),
                            m_theme.unselectedPixmap().height());
        // disable clip mask
        XSetClipMask(FbTk::App::instance()->display(),
                     gc,
                     None);
    }
    
    if (dotext && text) {
        //!! TODO: this is just temporarly and will be removed
        // once we've cleaned up the menu code this will be somewhere else...
        if (strcmp(text, "---") == 0){ // draw separator
            m_frame_pm.drawRectangle(tgc.gc(),
                                     item_x + menu.bevel_w + menu.item_h + 1, item_y + (menu.item_h / 2),
                                     menu.item_w - ((menu.bevel_w + menu.item_h) * 2) - 1, 0);
        } else { // draw normal text
            m_theme.frameFont().drawText(m_frame_pm.drawable(), // drawable
                                         screenNumber(),
                                         tgc.gc(),
                                         text, len, // text string and lenght
                                         text_x, text_y); // position
        }
    }

    if (dosel && item->submenu()) {
        if (m_theme.bulletPixmap().pixmap().drawable() != 0) {
            // enable clip mask
            XSetClipMask(FbTk::App::instance()->display(),
                         gc,
                         m_theme.bulletPixmap().mask().drawable());
            XSetClipOrigin(FbTk::App::instance()->display(),
                           gc, sel_x, item_y);
            // copy bullet pixmap to frame
            m_frame_pm.copyArea(m_theme.bulletPixmap().pixmap().drawable(),
                                gc,
                                0, 0,
                                sel_x, item_y,
                                m_theme.bulletPixmap().width(),
                                m_theme.bulletPixmap().height());
            // disable clip mask
            XSetClipMask(FbTk::App::instance()->display(),
                         gc,
                         None);
        } else {
            switch (m_theme.bullet()) {
            case MenuTheme::SQUARE:
                m_frame_pm.drawRectangle(gc, sel_x, sel_y, half_w, half_w);
                break;

            case MenuTheme::TRIANGLE:
                XPoint tri[3];

                if (m_theme.bulletPos() == FbTk::RIGHT) {
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
			
                m_frame_pm.fillPolygon(gc, tri, 3, Convex,
                                       CoordModePrevious);
                break;
			
            case MenuTheme::DIAMOND:
                XPoint dia[4];

                dia[0].x = sel_x + quarter_w - 3;
                dia[0].y = item_y + half_w;
                dia[1].x = 3;
                dia[1].y = -3;
                dia[2].x = 3;
                dia[2].y = 3;
                dia[3].x = -3;
                dia[3].y = 3;

                m_frame_pm.fillPolygon(gc, dia, 4, Convex,
                                       CoordModePrevious);
                break;
            default:
                break;
            }
        }
    }

    menu.frame.clearArea(item_x, item_y,
                         menu.item_w, menu.item_h, False);

    
    menu.frame.updateTransparent(item_x, item_y,
                                 menu.item_w, menu.item_h);
    
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

    if (be.window == menu.frame && menu.item_h != 0 && menu.item_w != 0) {

        int sbl = (be.x / menu.item_w), i = (be.y / menu.item_h);
        int w = (sbl * menu.persub) + i;

        if (w < static_cast<int>(menuitems.size()) && w >= 0) {
            which_press = i;
            which_sbl = sbl;

            MenuItem *item = menuitems[w];

            if (item->submenu()) {
                if (!item->submenu()->isVisible())
                    drawSubmenu(w);
            } else
                drawItem(w, item->isEnabled(), true, true);
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
			
            if (which_sub >= 0)
                drawSubmenu(which_sub);
            update();
        }

        if (re.x >= 0 && re.x <= (signed) width() &&
            re.y >= 0 && re.y <= (signed) menu.title_h &&
            re.button == 3)
            hide();
			
    } else if (re.window == menu.frame &&
               re.x >= 0 && re.x < (signed) width() &&
               re.y >= 0 && re.y < (signed) menu.frame_h) {
			
        int sbl = (re.x / menu.item_w), i = (re.y / menu.item_h),
            ix = sbl * menu.item_w, iy = i * menu.item_h,
            w = (sbl * menu.persub) + i,
            p = (which_sbl * menu.persub) + which_press;

        if (w < static_cast<int>(menuitems.size()) && w >= 0) {
            if (p == w && isItemEnabled(w)) {
                if (re.x > ix && re.x < (signed) (ix + menu.item_w) &&
                    re.y > iy && re.y < (signed) (iy + menu.item_h)) {
                    menuitems[w]->click(re.button, re.time);
                    itemSelected(re.button, w);
                    // redraw whole menu as enableds for any item
                    // may have changed
                    update(w);
                }
            } else {
                drawItem(p, isItemEnabled(p) && (p == which_sub), true, true);
            }
        } else
            drawItem(p, false, true, true);
    }
}


void Menu::motionNotifyEvent(XMotionEvent &me) {

    if (me.window == menu.title && (me.state & Button1Mask)) {
        stopHide();
        if (movable) {
            if (! moving) {
                if (m_parent && (! torn)) {
                    m_parent->drawItem(m_parent->which_sub, false, true, true);
                    m_parent->which_sub = -1;
                }

                moving = torn = true;

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            } else {
                menu.window.move(me.x_root - menu.x_move, me.y_root - menu.y_move);

                // if (which_sub >= 0)
                //     drawSubmenu(which_sub);
            }
        }
    } else if ((! (me.state & Button1Mask)) && me.window == menu.frame &&
               me.x >= 0 && me.x < (signed) width() &&
               me.y >= 0 && me.y < (signed) menu.frame_h) {
        stopHide();
        int sbl = (me.x / menu.item_w), i = (me.y / menu.item_h),
            w = (sbl * menu.persub) + i;

        if ((i != which_press || sbl != which_sbl) &&
            (w < static_cast<int>(menuitems.size()) && w >= 0)) {

            if (which_press != -1 && which_sbl != -1) {

                int p = which_sbl * menu.persub + which_press;
                MenuItem *item = menuitems[p];
                // don't redraw disabled items on enter/leave
                if (item != 0 && item->isEnabled()) {

                    drawItem(p, false, true, true);

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

            MenuItem *itmp = menuitems[w];

            if (itmp->submenu()) {

                drawItem(w, true);

                if (theme().menuMode() == MenuTheme::DELAY_OPEN) {
                    // setup show menu timer
                    timeval timeout;
                    timeout.tv_sec = 0;
                    timeout.tv_usec = theme().delayOpen() * 1000; // transformed to usec
                    m_submenu_timer.setTimeout(timeout);
                    m_submenu_timer.start();

                }

            } else {
                m_submenu_timer.stop();
                if (itmp->isEnabled())
                    drawItem(w, true, true, true);
            }
        }
    }
}


void Menu::exposeEvent(XExposeEvent &ee) {
    if (ee.window == menu.title) {
        redrawTitle();
    } else if (ee.window == menu.frame) {
        // this is a compilicated algorithm... lets do it step by step...
        // first... we see in which sub level the expose starts... and how many
        // items down in that sublevel
        if (menu.item_w == 0)
            menu.item_w = 1;
        if (menu.item_h == 0)
            menu.item_h = 1;
        unsigned int sbl = (ee.x / menu.item_w), id = (ee.y / menu.item_h),
            // next... figure out how many sublevels over the redraw spans
            sbl_d = ((ee.x + ee.width) / menu.item_w),
            // then we see how many items down to redraw
            id_d = ((ee.y + ee.height) / menu.item_h);
        if (static_cast<signed>(id_d) > menu.persub) 
            id_d = menu.persub;

        // draw the sublevels and the number of items the exposure spans
        unsigned int i, ii;
        for (i = sbl; i <= sbl_d; i++) {
            // set the iterator to the first item in the sublevel needing redrawing
            unsigned int index = id + i * menu.persub;
            if (index < menuitems.size()) {
                Menuitems::iterator it = menuitems.begin() + index;
                Menuitems::iterator it_end = menuitems.end();
                for (ii = id; ii <= id_d && it != it_end; ++it, ii++) {
                    unsigned int index = ii + (i * menu.persub);
                    drawItem(index, (which_sub == static_cast<signed>(index)), true, true,
                             ee.x, ee.y, ee.width, ee.height);
                }
            }
        }
    }
}


void Menu::enterNotifyEvent(XCrossingEvent &ce) {

    if (menu.frame != ce.window)
        return;

    menu.x_shift = x(), menu.y_shift = y();
    if (x() + width() > m_screen_width) {
        menu.x_shift = m_screen_width - width() - 2*m_border_width;
        shifted = true;
    } else if (x() < 0) {
        menu.x_shift = 0; //-m_border_width;
        shifted = true;
    }

    if (y() + height() + 2*m_border_width > m_screen_height) {
        menu.y_shift = m_screen_height - height() - 2*m_border_width;
        shifted = true;
    } else if (y() + (signed) menu.title_h < 0) {
        menu.y_shift = 0; // -m_border_width;;
        shifted = true;
    }


    if (shifted)
        menu.window.move(menu.x_shift, menu.y_shift);
    
    if (which_sub >= 0 && static_cast<size_t>(which_sub) < menuitems.size()) {
        MenuItem *tmp = menuitems[which_sub];
        if (tmp->submenu()->isVisible()) {
            int sbl = (ce.x / menu.item_w), i = (ce.y / menu.item_h),
                w = (sbl * menu.persub) + i;

            if (w != which_sub && (! tmp->submenu()->isTorn())) {
                tmp->submenu()->internal_hide();

                drawItem(which_sub, false, true, true);
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

        drawItem(p, (p == which_sub), true, true);

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
    if (event.state) // dont handle modifier with normal key
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
        if (which_press >= 0 && which_press < static_cast<signed>(menuitems.size())) {
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
    m_need_update = true; // redraw items

    menu.bevel_w = m_theme.bevelWidth();
    m_border_width = m_theme.borderWidth();

    if (menu.bevel_w > 10) // clamp to "normal" size
        menu.bevel_w = 10;

    if (m_border_width > 20) // clamp to normal size
        m_border_width = 20;
    if (m_border_width < 0)
        m_border_width = 0;

    menu.window.setBackgroundColor(m_theme.borderColor());
    menu.title.setBackgroundColor(m_theme.borderColor());    

    menu.window.setBorderColor(m_theme.borderColor());
    menu.title.setBorderColor(m_theme.borderColor());
    menu.frame.setBorderColor(m_theme.borderColor());

    menu.window.setBorderWidth(m_border_width);
    menu.title.setBorderWidth(m_border_width);
    
    menu.frame.setAlpha(alpha());
    menu.title.setAlpha(alpha());
    menu.window.setAlpha(alpha());

    update();
}
    
void Menu::renderTransFrame() {
    menu.frame.clear();
    menu.frame.updateTransparent();
}

void Menu::openSubmenu() {
    if (!isVisible() || which_press < 0 || which_press >= static_cast<signed>(menuitems.size()) ||
        which_sbl < 0 || which_sbl >= static_cast<signed>(menuitems.size()))
        return;

    int item = which_sbl * menu.persub + which_press;
    if (item < 0 || item >= static_cast<signed>(menuitems.size()))
        return;

    drawItem(item, true);
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

}; // end namespace FbTk
