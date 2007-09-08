// Menu.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include "Menu.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#include "MenuItem.hh"
#include "MenuSeparator.hh"
#include "ImageControl.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "EventManager.hh"
#include "Transparent.hh"
#include "SimpleCommand.hh"
#include "FbPixmap.hh"

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <algorithm>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#include <typeinfo>

#include <algorithm>


#ifdef DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif // DEBUG

namespace FbTk {

Menu *Menu::shown = 0;

Menu *Menu::s_focused = 0;

Menu::Menu(MenuTheme &tm, ImageControl &imgctrl):
    m_theme(tm),
    m_parent(0),
    m_image_ctrl(imgctrl),
    m_screen_x(0),
    m_screen_y(0),
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

    m_title_vis = true;

    m_internal_menu =
        m_moving =
        m_closing =
        m_torn =
        m_visible = false;


    m_type_ahead.init(menuitems);

    menu.x_move =
        menu.y_move = 0;

    m_which_sub = -1;

    menu.frame_pixmap =
        menu.title_pixmap =
        menu.hilite_pixmap = None;

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
    menu.title.setRenderer(*this);

    event_mask |= PointerMotionMask;
    menu.frame = FbTk::FbWindow(menu.window, // parent
                                0, theme().titleHeight(), // pos
                                width(), menu.frame_h ? menu.frame_h : 1, // size
                                event_mask,  // mask
                                false,  // override redirect
                                true); // save under
    evm.add(*this, menu.frame);
    menu.frame.setRenderer(*this);

    menu.title.raise();

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

    if (s_focused == this)
        s_focused = 0;
}

int Menu::insert(const FbString &label, RefCount<Command> &cmd, int pos) {
    return insert(new MenuItem(label, cmd), pos);
}

int Menu::insert(const FbString &label, int pos) {
    return insert(new MenuItem(label), pos);
}

int Menu::insert(const FbString &label, Menu *submenu, int pos) {
    submenu->m_parent = this;
    return insert(new MenuItem(label, submenu), pos);
}

int Menu::insert(MenuItem *item, int pos) {
    if (item == 0)
        return menuitems.size();
    if (pos == -1) {
        item->setIndex(menuitems.size());
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
        fixMenuItemIndices();
    }
    m_need_update = true; // we need to redraw the menu
    return menuitems.size();
}

void Menu::fixMenuItemIndices() {
    for (size_t i = 0; i < menuitems.size(); i++)
        menuitems[i]->setIndex(i);
}

int Menu::remove(unsigned int index) {
    if (index >= menuitems.size()) {
#ifdef DEBUG
        cout << "Bad index (" << index << ") given to Menu::remove()"
                  << " -- should be between 0 and " << menuitems.size()
                  << " inclusive." << endl;
#endif // DEBUG
        return -1;
    }

    Menuitems::iterator it = menuitems.begin() + index;
    MenuItem *item = (*it);

    if (item) {
        menuitems.erase(it);
        // avoid O(n^2) algorithm with removeAll()
        if (index != menuitems.size())
            fixMenuItemIndices();

        if (item->submenu() != 0) {
            Menu *tmp = item->submenu();
            // if menu is internal we should just hide it instead
            // if destroying it
            if (! tmp->m_internal_menu) {
                delete tmp;
            }
            // We can't internal_hide here, as the child may be deleted!
//            } else
//                tmp->internal_hide();
        }


        delete item;
    }

    if (static_cast<unsigned int>(m_which_sub) == index)
        m_which_sub = -1;
    else if (static_cast<unsigned int>(m_which_sub) > index)
        m_which_sub--;

    m_need_update = true; // we need to redraw the menu

    return menuitems.size();
}

void Menu::removeAll() {
    while (!menuitems.empty())
        remove(menuitems.size()-1);
}

void Menu::raise() {
    menu.window.raise();
}

void Menu::lower() {
    menu.window.lower();
}

void Menu::cycleItems(bool reverse) {
    Menuitems vec;
    if (m_type_ahead.stringSize())
        vec = m_matches;
    else
        vec = menuitems;

    if (vec.size() < 1)
        return;

    // find the next item to select
    // this algorithm assumes menuitems are sorted properly
    int new_index = -1;
    bool passed = !validIndex(m_active_index);
    for (size_t i = 0; i < vec.size(); i++) {
        if (!isItemSelectable(vec[i]->getIndex()) ||
            vec[i]->getIndex() == m_active_index)
            continue;

        // determine whether or not we've passed the active index
        if (!passed && vec[i]->getIndex() > m_active_index) {
            if (reverse && new_index != -1)
                break;
            passed = true;
        }

        // decide if we want to keep this item
        if (passed && !reverse) {
            new_index = vec[i]->getIndex();
            break;
        } else if (reverse || new_index == -1)
            new_index = vec[i]->getIndex();
    }

    if (new_index == -1)
        return;

    // clear the items and close any open submenus
    int old_active_index = m_active_index;
    m_active_index = new_index;
    if (validIndex(old_active_index) &&
        menuitems[old_active_index] != 0) {
        if (menuitems[old_active_index]->submenu()) {
            // we need to do this explicitly on the menu.window
            // since it might hide the parent if we use Menu::hide
            menuitems[old_active_index]->submenu()->internal_hide();
        }
        clearItem(old_active_index);
    }
    clearItem(new_index);
}

void Menu::enterSubmenu() {
    if (!validIndex(m_active_index))
        return;

    Menu *submenu = menuitems[m_active_index]->submenu();
    if (submenu == 0)
        return;

    if (submenu->menuitems.size() == 0)
        return;

    drawSubmenu(m_active_index);
    submenu->grabInputFocus();
    submenu->m_active_index = -1; // so we land on 0 after nextItem()
    submenu->cycleItems(false);
}

void Menu::disableTitle() {
    setTitleVisibility(false);
}

void Menu::enableTitle() {
    setTitleVisibility(true);
}

void Menu::updateMenu(int active_index) {
    if (m_title_vis) {
        menu.item_w = theme().titleFont().textWidth(menu.label,
                                                    menu.label.size());
        menu.item_w += (theme().bevelWidth() * 2);
    } else
        menu.item_w = 1;

    unsigned int ii = 0;
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it) {
        ii = (*it)->width(theme());
        menu.item_w = (ii > menu.item_w ? ii : menu.item_w);
    }

    if (menu.item_w < 1)
        menu.item_w = 1;

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
    menu.frame_h = itmp < 1 ? 1 : itmp;

    unsigned int new_width = (menu.sublevels * menu.item_w);
    unsigned int new_height = menu.frame_h;

    if (m_title_vis)
        new_height += theme().titleHeight() + ((menu.frame_h > 0)?menu.title.borderWidth():0);


    if (new_width == 0)
        new_width = menu.item_w;

    if (new_height < 1)
        new_height = 1;

    // must update main window size whether visible or not
    // the rest can wait until the end
    if (menu.window.width() != new_width)
        m_need_update = true;

    menu.window.resize(new_width, new_height);

    if (!isVisible())
        return;

    if (menu.frame.alpha() != alpha())
        menu.frame.setAlpha(alpha());

    Pixmap tmp = menu.hilite_pixmap;
    const FbTk::Texture &hilite_tex = theme().hiliteTexture();
    if (!hilite_tex.usePixmap()) {
        menu.hilite_pixmap = None;
    } else
        menu.hilite_pixmap =
            m_image_ctrl.renderImage(menu.item_w, theme().itemHeight(), hilite_tex);
    if (tmp)
        m_image_ctrl.removeImage(tmp);


    if (!theme().selectedPixmap().pixmap().drawable()) {
        int hw = theme().itemHeight() / 2;
        m_theme.setSelectedPixmap(m_image_ctrl.renderImage(hw, hw, theme().hiliteTexture()), true);

        if (!theme().highlightSelectedPixmap().pixmap().drawable()) {
            int hw = theme().itemHeight() / 2;
            m_theme.setHighlightSelectedPixmap(m_image_ctrl.renderImage(hw, hw, theme().frameTexture()), true);
        }
    }

    if (m_title_vis) {
        menu.title.moveResize(-menu.title.borderWidth(), -menu.title.borderWidth(),
                              width() + menu.title.borderWidth(), theme().titleHeight());
    }

    menu.frame.moveResize(0, ((m_title_vis) ? menu.title.y() + menu.title.height() +
                              menu.title.borderWidth()*2 : 0),
                          width(), menu.frame_h);

    if (m_title_vis && m_need_update) {
        tmp = menu.title_pixmap;
        const FbTk::Texture &tex = theme().titleTexture();
        if (!tex.usePixmap()) {
            menu.title_pixmap = None;
            menu.title.setBackgroundColor(tex.color());
        } else {
            menu.title_pixmap =
                m_image_ctrl.renderImage(width(), theme().titleHeight(), tex);
            // set pixmap that we have as real face to the user
            menu.title.setBackgroundPixmap(menu.title_pixmap);
        }

        if (tmp)
            m_image_ctrl.removeImage(tmp);
    }

    tmp = menu.frame_pixmap;
    const FbTk::Texture &frame_tex = theme().frameTexture();
    if (m_need_update) {
        if (!frame_tex.usePixmap()) {
            menu.frame_pixmap = None;
            menu.frame.setBackgroundColor(frame_tex.color());
        } else {
            menu.frame_pixmap =
                m_image_ctrl.renderImage(width(), menu.frame_h, frame_tex);
            menu.frame.setBackgroundPixmap(menu.frame_pixmap);
        }

        if (tmp)
            m_image_ctrl.removeImage(tmp);

    }

    clearWindow();
    m_need_update = false;
}


void Menu::show() {

    if (isVisible() || menuitems.empty())
        return;

    m_visible = true;

    if (m_need_update)
        updateMenu();

    m_type_ahead.reset();

    menu.window.showSubwindows();
    menu.window.show();
    raise();

    if (! m_parent && shown != this) {
        if (shown && (! shown->m_torn))
            shown->hide();

        shown = this;
    }

}


void Menu::hide() {

    if (!isVisible())
        return;

    // if not m_torn and parent is m_visible, go to first parent
    // and hide it
    if (!m_torn && m_parent && m_parent->isVisible()) {
        Menu *p = m_parent;

        while ((! p->m_torn) && p->m_parent && p->m_parent->isVisible())
            p = p->m_parent;

        p->internal_hide();
    } else if (!m_torn) // if we dont have a parent then do hide here
        internal_hide();

}

void Menu::grabInputFocus() {
    // if there's a submenu open, focus it instead
    if (validIndex(m_which_sub) &&
            menuitems[m_which_sub]->submenu()->isVisible()) {
        menuitems[m_which_sub]->submenu()->grabInputFocus();
        return;
    }

    s_focused = this;

    // grab input focus
    menu.window.setInputFocus(RevertToPointerRoot, CurrentTime);
}


void Menu::clearWindow() {
    menu.title.clear();
    menu.frame.clear();

    // clear foreground bits of frame items
    for (size_t i = 0; i < menuitems.size(); i++) {
        clearItem(i, false);   // no clear
    }
}

void Menu::redrawFrame(FbDrawable &drawable) {
    for (size_t i = 0; i < menuitems.size(); i++) {
        drawItem(drawable, i);
    }

}

void Menu::internal_hide(bool first) {

    if (validIndex(m_which_sub)) {
        MenuItem *tmp = menuitems[m_which_sub];
        tmp->submenu()->internal_hide(false);
    }

    // if we have an active index we need to redraw it
    // as non active
    int old = m_active_index;
    m_active_index = -1;
    clearItem(old); // clear old area from highlight

    if (shown && shown->menu.window == menu.window) {
        if (m_parent && m_parent->isVisible())
            shown = m_parent;
        else
            shown = (Menu *) 0;
    }

    m_torn = m_visible = m_closing = false;
    m_which_sub = -1;

    if (first && m_parent && m_parent->isVisible() &&
        s_focused && !s_focused->isVisible())
        m_parent->grabInputFocus();

    menu.window.hide();
}


void Menu::move(int x, int y) {
    if (x == this->x() && y == this->y())
        return;

    menu.window.move(x, y);
    // potentially transparent children
    menu.title.parentMoved();
    menu.frame.parentMoved();

    if (!isVisible())
        return;

    if (alpha() < 255)
        clearWindow();

    if (validIndex(m_which_sub) &&
            menuitems[m_which_sub]->submenu()->isVisible())
        drawSubmenu(m_which_sub);
}


void Menu::redrawTitle(FbDrawable &drawable) {

    const FbTk::Font &font = theme().titleFont();
    int dx = theme().bevelWidth();
    size_t len = menu.label.size();
    unsigned int l = font.textWidth(menu.label, len) + theme().bevelWidth()*2;

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

    // difference between height based on font, and style-set height
    int height_offset = theme().titleHeight() - (font.height() + 2*theme().bevelWidth());
    font.drawText(drawable, // drawable
                  screenNumber(),
                  theme().titleTextGC().gc(), // graphic context
                  menu.label, len,  // text string with length
                  dx, font.ascent() + theme().bevelWidth() + height_offset/2);  // position
}


void Menu::drawSubmenu(unsigned int index) {
    if (validIndex(m_which_sub) && static_cast<unsigned int>(m_which_sub) != index) {
        MenuItem *itmp = menuitems[m_which_sub];

        if (! itmp->submenu()->isTorn())
            itmp->submenu()->internal_hide();
    }

    if (index >= menuitems.size())
        return;


    MenuItem *item = menuitems[index];
    if (item->submenu() && isVisible() && (! item->submenu()->isTorn()) &&
        item->isEnabled()) {

        if (item->submenu()->m_parent != this)
            item->submenu()->m_parent = this;

        item->submenu()->setScreen(m_screen_x, m_screen_y, m_screen_width, m_screen_height);

        int sbl = index / menu.persub, i = index - (sbl * menu.persub);
        int new_x = x() + ((menu.item_w * (sbl + 1)) + menu.window.borderWidth());
        int new_y;

        if (m_alignment == ALIGNTOP) {
            new_y = (y() + ((m_title_vis) ? theme().titleHeight() + menu.title.borderWidth() : 0) -
                     ((item->submenu()->m_title_vis) ?
                      item->submenu()->theme().titleHeight() + menu.window.borderWidth() : 0));
        } else {
            new_y = (y() + (theme().itemHeight() * i) +
                     ((m_title_vis) ? theme().titleHeight() + menu.window.borderWidth() : 0) -
                     ((item->submenu()->m_title_vis) ?
                      item->submenu()->theme().titleHeight() + menu.window.borderWidth() : 0));
        }

        if (m_alignment == ALIGNBOTTOM &&
            (new_y + item->submenu()->height()) > (y() + height())) {
            new_y = (y() + height() - item->submenu()->height());
        }

        int borderw = item->submenu()->fbwindow().borderWidth();

        if ((new_x + item->submenu()->width()) + 2*borderw > m_screen_x + m_screen_width) {
            new_x = x() - item->submenu()->width() - menu.window.borderWidth();
        }

        if (new_x < m_screen_x)
            new_x = m_screen_x;

        if ((new_y + item->submenu()->height()) > m_screen_y + m_screen_height) {
            new_y = m_screen_y + m_screen_height - item->submenu()->height() -
                menu.window.borderWidth() * 2;
        }

        item->submenu()->m_moving = m_moving;
        m_which_sub = index;

        if (new_y < m_screen_y)
            new_y = m_screen_y;

        item->submenu()->move(new_x, new_y);
        if (! m_moving)
            clearItem(index);

        if (! item->submenu()->isVisible()) {
            item->showSubmenu();
            item->submenu()->raise();
        }


    } else
        m_which_sub = -1;

}

int Menu::drawItem(FbDrawable &drawable, unsigned int index,
                   bool highlight, bool exclusive_drawable) {

    if (index >= menuitems.size() || menuitems.size() == 0 ||
        menu.persub == 0)
        return 0;

    MenuItem *item = menuitems[index];
    if (! item) return 0;

    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    int item_x = (sbl * menu.item_w), item_y = (i * theme().itemHeight());

    if (exclusive_drawable)
        item_x = item_y = 0;

    item->draw(drawable, theme(), highlight,
               exclusive_drawable, true, // draw fg, draw bg
               item_x, item_y,
               menu.item_w, theme().itemHeight());

    return item_y;
}

void Menu::setLabel(const FbString &labelstr) {
    //make sure we don't send 0 to std::string
    menu.label = labelstr;
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

bool Menu::isItemSelectable(unsigned int index) const {

    if (index >= menuitems.size()) return false;

    const MenuItem *item = find(index);
    return (!item ||
        (typeid(*item) == typeid(FbTk::MenuSeparator)) ||
        !item->isEnabled()) ? false : true;
}


void Menu::handleEvent(XEvent &event) {
    if (event.type == FocusOut) {
        if (s_focused == this)
            s_focused = 0;
    // I don't know why, but I get a FocusIn event when closing the menu with
    // the mouse over it -- probably an xorg bug, but it's easy to address here
    } else if (event.type == FocusIn && m_visible) {
        if (s_focused != this)
            s_focused = this;
        // if there's a submenu open, focus it instead
        if (validIndex(m_which_sub) &&
                menuitems[m_which_sub]->submenu()->isVisible())
            menuitems[m_which_sub]->submenu()->grabInputFocus();
    } else if (event.type == LeaveNotify)
        m_closing = false;
}

void Menu::buttonPressEvent(XButtonEvent &be) {
    if (be.window == menu.title) {
        grabInputFocus();
        m_closing = (be.button == 3);
    } else
        m_closing = false;

    if (be.window == menu.frame && menu.item_w != 0) {

        int sbl = (be.x / menu.item_w), i = (be.y / theme().itemHeight());
        int w = (sbl * menu.persub) + i;

        if (validIndex(w) && isItemSelectable(static_cast<unsigned int>(w))) {
            MenuItem *item = menuitems[w];

            if (item->submenu()) {
                if (!item->submenu()->isVisible())
                    drawSubmenu(w);
            }
        }
    } else {
        menu.x_move = be.x_root - x();
        menu.y_move = be.y_root - y();
    }
}


void Menu::buttonReleaseEvent(XButtonEvent &re) {
    if (re.window == menu.title) {
        if (m_moving) {
            m_moving = false;

            if (validIndex(m_which_sub) &&
                    menuitems[m_which_sub]->submenu()->isVisible())
                drawSubmenu(m_which_sub);

            if (alpha() < 255) {
                // update these since we've (probably) moved
                menu.title.parentMoved();
                menu.frame.parentMoved();
                clearWindow();
            }
        }

        if (re.button == 3 && m_closing)
            internal_hide();

    } else if (re.window == menu.frame) {

        int sbl = (re.x / menu.item_w), i = (re.y / theme().itemHeight()),
            ix = sbl * menu.item_w, iy = i * theme().itemHeight(),
            w = (sbl * menu.persub) + i;

        if (validIndex(w) && isItemSelectable(static_cast<unsigned int>(w))) {
            if (m_active_index == w && isItemEnabled(w) &&
                re.x > ix && re.x < (signed) (ix + menu.item_w) &&
                re.y > iy && re.y < (signed) (iy + theme().itemHeight())) {
                menuitems[w]->click(re.button, re.time);
            } else {
                int old = m_active_index;
                m_active_index = w;
                clearItem(old);
            }
            clearItem(w);
        }
    }
}


void Menu::motionNotifyEvent(XMotionEvent &me) {
    // if draging the with the titlebar:
    if (me.window == menu.title && (me.state & Button1Mask)) {
        stopHide();

        if (! m_moving) {
            // if not m_moving: start m_moving operation
            m_moving = m_torn = true;
            // clear current highlighted item
            clearItem(m_active_index);

            if (validIndex(m_which_sub) &&
                    menuitems[m_which_sub]->submenu()->isVisible())
                drawSubmenu(m_which_sub);
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

        if (w == m_active_index || !validIndex(w))
            return;

        // if another menu is focused, change focus to this one, so arrow keys
        // work as expected
        if (s_focused != this && s_focused != 0)
            grabInputFocus();

        MenuItem *itmp = menuitems[w];
        if (itmp == 0)
            return;

        if (itmp->isEnabled()) {
            int old = m_active_index;
            m_active_index = w;
            clearItem(w);
            clearItem(old);

            MenuItem *item = validIndex(m_which_sub) ? menuitems[m_which_sub] : 0;
            if (item != 0 && item->submenu() && item->submenu()->isVisible() &&
                !item->submenu()->isTorn()) {
                // setup hide timer for submenu
                item->submenu()->startHide();
            }

        }

        if (itmp->submenu() && theme().menuMode() == MenuTheme::DELAY_OPEN) {
            // start submenu open delay
            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = theme().delayOpen() * 1000; // transformed to usec
            m_submenu_timer.setTimeout(timeout);
            m_submenu_timer.start();
        } else if (isItemSelectable(w)){
            // else normal menu item
            // draw highlighted
            m_submenu_timer.stop();
        }

    }
}


void Menu::exposeEvent(XExposeEvent &ee) {
    if (ee.window == menu.title) {
        menu.title.clearArea(ee.x, ee.y, ee.width, ee.height);
    } else if (ee.window == menu.frame) {
        // find where to clear
        // this is a compilicated algorithm... lets do it step by step...
        // first... we see in which sub level the expose starts... and how many
        // items down in that sublevel

        int sbl = (ee.x / menu.item_w), id = (ee.y / theme().itemHeight()),
            // next... figure out how many sublevels over the redrawspans
            sbl_d = ((ee.x + ee.width) / menu.item_w),
            // then we see how many items down to redraw
            id_d = ((ee.y + ee.height) / theme().itemHeight());

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
                    clearItem(index);
                }
            }
        }
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
        resetTypeAhead();
        cycleItems(true);
        break;
    case XK_Down:
        resetTypeAhead();
        cycleItems(false);
        break;
    case XK_Left: // enter parent if we have one
        resetTypeAhead();
        internal_hide();
        break;
    case XK_Right: // enter submenu if we have one
        resetTypeAhead();
        enterSubmenu();
        break;
    case XK_Escape: // close menu
        m_type_ahead.reset();
        m_torn = false;
        hide();
        break;
    case XK_BackSpace:
        if (m_type_ahead.stringSize() == 0) {
            internal_hide();
            break;           
        }    

        m_type_ahead.putBackSpace();
        drawTypeAheadItems();
        break;
    case XK_KP_Enter:
    case XK_Return:
        resetTypeAhead();
        if (validIndex(m_active_index) &&
            isItemEnabled(m_active_index)) {
            if (menuitems[m_active_index]->submenu() != 0)
                enterSubmenu();
            else {
                // send fake button click
                int button = (event.state & ShiftMask) ? 3 : 1;
                find(m_active_index)->click(button, event.time);
                m_need_update = true;
                updateMenu();
            }
        }
        break;
    case XK_Tab:
    case XK_ISO_Left_Tab:
        if (validIndex(m_active_index) && isItemEnabled(m_active_index) &&
            menuitems[m_active_index]->submenu() && m_matches.size() == 1) {
            enterSubmenu();
            m_type_ahead.reset();
        } else {
            m_type_ahead.seek();
            cycleItems((bool)(event.state & ShiftMask));
        }
        drawTypeAheadItems();
        break;
    default:
        m_type_ahead.putCharacter(keychar[0]);
        // if current item doesn't match new search string, find the next one
        drawTypeAheadItems();
        if (!m_matches.empty() && (!validIndex(m_active_index) ||
            std::find(m_matches.begin(), m_matches.end(),
                      find(m_active_index)) == m_matches.end()))
            cycleItems(false);
        break;
    }
}


void Menu::reconfigure() {

    if (FbTk::Transparent::haveComposite()) {
        menu.window.setOpaque(alpha());
        menu.title.setAlpha(255);
        menu.frame.setAlpha(255);
    } else {
        menu.window.setOpaque(255);
        menu.title.setAlpha(alpha());
        menu.frame.setAlpha(alpha());
    }

    m_need_update = true; // redraw items

    menu.window.setBorderColor(theme().borderColor());
    menu.title.setBorderColor(theme().borderColor());
    menu.frame.setBorderColor(theme().borderColor());

    menu.window.setBorderWidth(theme().borderWidth());
    menu.title.setBorderWidth(theme().borderWidth());

    updateMenu();
}


void Menu::openSubmenu() {

    int item = m_active_index;
    if (!isVisible() || !validIndex(item) || !menuitems[item]->isEnabled() ||
        s_focused != this && s_focused && s_focused->isVisible())
        return;

    clearItem(item);

    if (menuitems[item]->submenu() != 0) {
        // stop hide timer, so it doesnt hides the menu if we
        // have the same submenu as the last shown submenu
        // (window menu for clients inside workspacemenu for example)
        menuitems[item]->submenu()->m_hide_timer.stop();
        drawSubmenu(item);
    }

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
    for (; it != it_end; ++it) {
        (*it)->updateTheme(theme());
    }
    reconfigure();
}


void Menu::setScreen(int x, int y, int w, int h) {
    m_screen_x = x;
    m_screen_y = y;
    m_screen_width = w;
    m_screen_height = h;
}

// Render the foreground objects of given window onto given pixmap
void Menu::renderForeground(FbWindow &win, FbDrawable &drawable) {
    if (&win == &menu.frame) {
        redrawFrame(drawable);
    } else if (&win == &menu.title) {
        redrawTitle(drawable);
    }
}

// clear item clears the item and draws the dynamic bits
// thus sometimes it won't perform the actual clear operation
// nothing in here should be rendered transparently
// (unless you use a caching pixmap, which I think we should avoid)
void Menu::clearItem(int index, bool clear, int search_index) {
    if (!validIndex(index))
        return;

    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    unsigned int item_w = menu.item_w, item_h = theme().itemHeight();
    int item_x = (sbl * item_w), item_y = (i * item_h);
    bool highlight = (index == m_active_index && isItemSelectable(index));

    if (search_index < 0)
        // find if we need to underline the item
        search_index = std::find(m_matches.begin(), m_matches.end(),
                                 find(index)) - m_matches.begin();

    // don't highlight if moving, doesn't work with alpha on
    if (highlight && !m_moving) {
        highlightItem(index);
        if (search_index < (int)m_matches.size())
            drawLine(index, m_type_ahead.stringSize());
        return;
    } else if (clear)
        menu.frame.clearArea(item_x, item_y, item_w, item_h);

    MenuItem *item = menuitems[index];
    if (! item) return;

    item->draw(menu.frame, theme(), highlight,
               true, false, item_x, item_y,
               item_w, item_h);

    if (search_index < (int)m_matches.size())
        drawLine(index, m_type_ahead.stringSize());
}

// Area must have been cleared before calling highlight
void Menu::highlightItem(int index) {
    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    unsigned int item_w = menu.item_w, item_h = theme().itemHeight();
    int item_x = (sbl * menu.item_w), item_y = (i * item_h);

    FbPixmap buffer = FbPixmap(menu.frame, item_w, item_h, menu.frame.depth());

    bool parent_rel = menu.hilite_pixmap == ParentRelative;
    Pixmap pixmap = parent_rel ? menu.frame_pixmap : menu.hilite_pixmap;
    int pixmap_x = parent_rel ? item_x : 0, pixmap_y = parent_rel ? item_y : 0;
    if (pixmap) {
        buffer.copyArea(pixmap,
                        theme().hiliteGC().gc(), pixmap_x, pixmap_y,
                        0, 0,
                        item_w, item_h);
    } else {
        buffer.fillRectangle(theme().hiliteGC().gc(),
                             0, 0, item_w, item_h);
    }
    menu.frame.updateTransparent(item_x, item_y, item_w, item_h, buffer.drawable(), true);

    drawItem(buffer, index, true, true);

    menu.frame.copyArea(buffer.drawable(), theme().hiliteGC().gc(),
                        0, 0,
                        item_x, item_y,
                        item_w, item_h);

}

void Menu::resetTypeAhead() {
    Menuitems vec = m_matches;
    Menuitems::iterator it = vec.begin();
    m_type_ahead.reset();
    m_matches.clear();

    for (; it != vec.end(); it++)
        clearItem((*it)->getIndex(), true, 1);
}

void Menu::drawTypeAheadItems() {
    // remove underlines from old matches
    for (size_t i = 0; i < m_matches.size(); i++)
        clearItem(m_matches[i]->getIndex(), true, m_matches.size());

    m_matches = m_type_ahead.matched();
    for (size_t j = 0; j < m_matches.size(); j++)
        clearItem(m_matches[j]->getIndex(), false, j);
}

// underline menuitem[index] with respect to matchstringsize size
void Menu::drawLine(int index, int size){
    if (!validIndex(index))
        return;

    int sbl = index / menu.persub, i = index - (sbl * menu.persub);
    int item_x = (sbl * menu.item_w), item_y = (i * theme().itemHeight());

    FbTk::MenuItem *item = find(index);
    item->drawLine(menu.frame, theme(), size, item_x, item_y, menu.item_w);
}

}; // end namespace FbTk
