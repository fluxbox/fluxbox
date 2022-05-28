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

#include "Menu.hh"

#include "MenuItem.hh"
#include "MenuSearch.hh"
#include "MenuSeparator.hh"
#include "ImageControl.hh"
#include "MemFun.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "EventManager.hh"
#include "Transparent.hh"
#include "SimpleCommand.hh"
#include "FbPixmap.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

namespace {

// if 'win' is given, 'pm' is used as the backGroundPixmap
void renderMenuPixmap(Pixmap& pm, FbTk::FbWindow* win, int width, int height, const FbTk::Texture& tex, FbTk::ImageControl& img_ctrl) {

    img_ctrl.removeImage(pm);
    if (!tex.usePixmap()) {
        pm = None;
        if (win)
            win->setBackgroundColor(tex.color());
    } else {
        pm = img_ctrl.renderImage(width, height, tex);
        if (win)
            win->setBackgroundPixmap(pm);
    }
}

} // end of anonymous namespace


namespace FbTk {


Menu* s_shown = 0; // if there's a menu open at all
Menu* s_focused = 0; // holds currently focused menu


Menu* Menu::shownMenu() { return s_shown; }
Menu* Menu::focused() { return s_focused; }

void Menu::hideShownMenu() {
    if (s_shown)
        s_shown->hide();
}


Menu::Menu(FbTk::ThemeProxy<MenuTheme> &tm, ImageControl &imgctrl):
    m_parent(0),
    m_need_update(true),
    m_active_index(-1),
    m_theme(tm),
    m_image_ctrl(imgctrl),
    m_alignment(ALIGNDONTCARE) {

    Display* disp = FbTk::App::instance()->display();
    m_screen.x = 0;
    m_screen.y = 0;
    m_screen.width = DisplayWidth(disp, tm->screenNum());
    m_screen.height = DisplayHeight(disp, tm->screenNum());

    // setup timers
    RefCount<Command<void> > show_cmd(new SimpleCommand<Menu>(*this, &Menu::openSubmenu));
    m_submenu_timer.setCommand(show_cmd);
    m_submenu_timer.fireOnce(true);

    RefCount<Command<void> > hide_cmd(new SimpleCommand<Menu>(*this, &Menu::closeMenu));
    m_hide_timer.setCommand(hide_cmd);
    m_hide_timer.fireOnce(true);

    // make sure we get updated when the theme is reloaded
    m_tracker.join(tm.reconfigSig(), MemFun(*this, &Menu::themeReconfigured));

    m_internal_menu = false;
    m_state.moving = m_state.closing = m_state.torn = m_state.visible = false;

    m_search.reset(new MenuSearch(m_items));

    m_x_move = m_y_move = 0;
    m_which_sub = -1;

    m_hilite_pixmap = None;

    m_title.visible = true;
    m_title.pixmap = None;

    m_frame.pixmap = None;
    m_frame.height = theme()->titleFont().height() + theme()->bevelWidth() * 2;

    m_item_w = m_frame.height;

    m_columns = m_rows_per_column = m_min_columns = 0;

    long event_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask | KeyPressMask | ExposureMask | FocusChangeMask;

    // create menu window
    m_window = FbTk::FbWindow(tm->screenNum(),
                                 0, 0, 10, 10,
                                 event_mask,
                                 true,  // override redirect
                                 true); // save_under

    // initialize 'shape' here AFTER we created m_window aka fbwindow()
    m_shape.reset(new Shape(fbwindow(), tm->shapePlaces()));

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(*this, m_window);

    // strip focus change mask from attrib, since we should only use it with
    // main window
    event_mask ^= FocusChangeMask;
    event_mask |= EnterWindowMask | LeaveWindowMask;

    int w = width();
    int th = theme()->titleHeight(!m_title.label.logical().empty());
    int fh = std::max(m_frame.height, m_frame.height);

    //create menu title
    m_title.win = FbTk::FbWindow(m_window, 0, 0, w, th, event_mask, false, true);

    evm.add(*this, m_title.win);
    m_title.win.setRenderer(*this);

    event_mask |= PointerMotionMask;
    m_frame.win = FbTk::FbWindow(m_window, 0, th, w, fh, event_mask, false, true);
    evm.add(*this, m_frame.win);
    m_frame.win.setRenderer(*this);
    m_title.win.raise();
    reconfigure();
}

Menu::~Menu() {

    m_window.hide();
    removeAll();

    if (s_shown && s_shown->window() == window())
        s_shown = 0;

    if (m_title.pixmap)
        m_image_ctrl.removeImage(m_title.pixmap);

    if (m_frame.pixmap)
        m_image_ctrl.removeImage(m_frame.pixmap);

    if (m_hilite_pixmap)
        m_image_ctrl.removeImage(m_hilite_pixmap);

    if (s_focused == this)
        s_focused = 0;
}

int Menu::insertCommand(const FbString &label, RefCount<Command<void> > &cmd, int pos) {
    return insertItem(new MenuItem(label, cmd, this), pos);
}

int Menu::insert(const FbString &label, int pos) {
    return insertItem(new MenuItem(label, *this), pos);
}

int Menu::insertSubmenu(const FbString &label, Menu *submenu, int pos) {
    return insertItem(new MenuItem(label, submenu, this), pos);
}

int Menu::insertItem(MenuItem *item, int pos) {

    if (item == 0)
        return m_items.size();
    if (pos == -1) {
        m_items.push_back(item);
    } else {
        m_items.insert(m_items.begin() + pos, item);
        if (m_active_index >= pos)
            m_active_index++;
    }
    m_need_update = true; // we need to redraw the menu
    return m_items.size();
}


int Menu::findSubmenuIndex(const FbTk::Menu* submenu) const {
    size_t i;
    for (i = 0; i < m_items.size(); i++) {
        if (m_items[i]->submenu() == submenu) {
            return i;
        }
    }
    return -1;
}


int Menu::remove(unsigned int index) {
    if (!validIndex(index)) {
#ifdef DEBUG
        cerr << __FILE__ << "(" << __LINE__ << ") Bad index (" << index
             << ") given to Menu::remove()"
             << " -- should be between 0 and " << m_items.size()-1
             << " inclusive." << endl;
#endif // DEBUG
        return -1;
    }

    return removeItem(m_items[index]);
}

int Menu::removeItem(FbTk::MenuItem* item) {

    size_t l = m_items.size();
    size_t i;
    size_t found = 0;

    // find index of first occurance of item
    for (i = 0; i < l; i++) {
        if (m_items[i] == item) {
            found++;
        }
    }

    if (found == 0) {
        return l;
    }

    // sigh. http://en.wikipedia.org/wiki/Erase-remove_idiom
    std::vector<MenuItem*>::iterator start_erase = std::remove(m_items.begin(), m_items.end(), item);
    m_items.erase(start_erase, m_items.end());

    if (item) {
        Menu* sm = item->submenu();
        if (sm) {
            if (! sm->m_internal_menu) {
                delete sm;
            }
        }
        delete item;
    }

    m_need_update = true;

    if (m_items.empty()) {
        m_which_sub = -1;
        m_active_index = 0;
        return 0;
    }

    if (static_cast<unsigned int>(m_which_sub) == i)
        m_which_sub = -1;
    else if (static_cast<unsigned int>(m_which_sub) > i)
        m_which_sub -= found;

    if (static_cast<unsigned int>(m_active_index) > i) {
        int ai = static_cast<int>(m_active_index) - found;
        if (ai >= 0) {
            m_active_index = ai;
        } else {
            m_active_index = 0;
        }
    }

    return m_items.size();
}


void Menu::removeAll() {
    while (!m_items.empty())
        remove(m_items.size()-1);
}

void Menu::raise() {
    m_window.raise();
}

void Menu::lower() {
    m_window.lower();
}

void Menu::cycleItems(bool reverse) {

    int l = m_items.size();
    int i;
    int offset = reverse ? -1 : 1;
    size_t ignore;

    // check if there is _any_ selectable item
    for (i = 0; i < l; i++) {
        if (isItemSelectable(i)) {
            break;
        }
    }

    // no selectable item
    if (i >= l) {
        return;
    }

    for (i = m_active_index + offset; i != m_active_index; i += offset ) {
        if (i < 0) {
            i = l - 1;
        } else if (i >= l) {
            i = 0;
        }

        if (!isItemSelectable(i)) {
            continue;
        }

        // empty-string ("nothing was typed") matches always
        if (m_search->get_match(i, ignore)) {
            setActiveIndex(i);
            break;
        }
    }
}

void Menu::setActiveIndex(int new_index) {
    // clear the items and close any open submenus
    int old_active_index = m_active_index;
    m_active_index = new_index;
    if (validIndex(old_active_index) &&
        m_items[old_active_index] != 0) {
        if (m_items[old_active_index]->submenu()) {
            // we need to do this explicitly on the m_window
            // since it might hide the parent if we use Menu::hide
            m_items[old_active_index]->submenu()->internal_hide();
        }
        clearItem(old_active_index);
    }
    clearItem(new_index);
}

void Menu::enterSubmenu() {
    if (!validIndex(m_active_index))
        return;

    Menu *submenu = m_items[m_active_index]->submenu();
    if (submenu == 0)
        return;

    if (submenu->m_items.empty())
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

void Menu::updateMenu() {

    int bevel = theme()->bevelWidth();
    int bw = theme()->borderWidth();
    int ih = theme()->itemHeight();
    unsigned int iw = 1;
    int th = theme()->titleHeight(!m_title.label.logical().empty());
    int tbw = m_title.win.borderWidth();
    int w = static_cast<int>(width());
    size_t l = m_items.size();
    size_t i;


    // find the nearest enabled menuitem and highlight it
    if (validIndex(m_active_index) && !m_items[m_active_index]->isEnabled()) {
        for (i = 1; i < l; i++) {
            if (validIndex(m_active_index + i) &&
                m_items[m_active_index + i]->isEnabled()) {
                m_active_index += i;
                break;
            } else if (validIndex(m_active_index - i) &&
                       m_items[m_active_index - i]->isEnabled()) {
                m_active_index -= i;
                break;
            }
        }
    }


    // calculate needed item width
    m_item_w = 1;
    if (m_title.visible) {
        m_item_w = theme()->titleFont().textWidth(m_title.label);
        m_item_w += bevel * 2;
    }
    m_item_w = std::max(iw, m_item_w);
    for (i = 0; i < l; i++) {
        iw = m_items[i]->width(theme());
        m_item_w = std::max(iw, m_item_w);
    }

    // calculate needed columns
    m_columns = 0;
    m_rows_per_column = 0;
    if (!m_items.empty()) {
        m_columns = 1;

        while (ih * (l + 1) / m_columns + th + bw > m_screen.height) {
            m_columns++;
        }

        m_columns = std::max(m_min_columns, m_columns);

        // the menu width should be as wide as the widest menu item
        w = m_item_w * m_columns;

        m_rows_per_column = m_items.size() / m_columns;
        if (m_items.size() % m_columns)
            m_rows_per_column++;
    }

    int itmp = ih * m_rows_per_column;
    m_frame.height = std::max(1, itmp);

    unsigned int new_width = (m_columns * m_item_w);
    unsigned int new_height = m_frame.height;

    if (m_title.visible)
        new_height += th + ((m_frame.height > 0) ? tbw : 0);


    if (new_width == 0)
        new_width = m_item_w;

    if (new_height < 1)
        new_height = 1;

    // must update main window size whether visible or not
    // the rest can wait until the end
    if (m_window.width() != new_width)
        m_need_update = true;

    m_window.resize(new_width, new_height);

    if (!isVisible())
        return;

    if (m_frame.win.alpha() != alpha())
        m_frame.win.setAlpha(alpha());

    renderMenuPixmap(m_hilite_pixmap, NULL, m_item_w, ih, theme()->hiliteTexture(), m_image_ctrl);


    if (!theme()->selectedPixmap().pixmap().drawable()) {
        int hw = ih / 2;
        // render image, disable cache and let the theme remove the pixmap
        Pixmap pm = m_image_ctrl.renderImage(hw, hw, theme()->hiliteTexture(), ROT0, false);
        theme()->setSelectedPixmap(pm, false); // the theme takes care of this pixmap

        if (!theme()->highlightSelectedPixmap().pixmap().drawable()) {
            hw = ih / 2;
            // render image, disable cache and let the theme remove the pixmap
            pm = m_image_ctrl.renderImage(hw, hw, theme()->frameTexture(), ROT0, false);
            theme()->setHighlightSelectedPixmap(pm, false); // theme takes care of this pixmap
       }
    }

    int y = 0;
    if (m_title.visible) {
        m_title.win.moveResize(-tbw, -tbw, w + tbw, th);
        y = m_title.win.y() + m_title.win.height() + 2 * tbw;
    }

    m_frame.win.moveResize(0, y, w, m_frame.height);

    if (m_title.visible && m_need_update) {
        renderMenuPixmap(m_title.pixmap, &m_title.win,
                w, th, theme()->titleTexture(), m_image_ctrl);
    }

    if (m_need_update) {
        renderMenuPixmap(m_frame.pixmap, &m_frame.win,
                w, m_frame.height, theme()->frameTexture(), m_image_ctrl);
    }

    clearWindow();
    m_need_update = false;
    m_shape->update();
}


void Menu::show() {

    if (isVisible() || m_items.empty())
        return;

    m_state.visible = true;
    if (m_need_update)
        updateMenu();

    m_search->clear();

    m_window.showSubwindows();
    m_window.show();
    raise();

    if (s_shown && s_shown != this)
        s_shown->hide();
    s_shown = this;

}


void Menu::hide(bool force) {

    if (!isVisible())
        return;

    // if parent is visible, go to first parent and hide it
    Menu *p = this;
    while (p && p->isVisible()) {
        Menu *tmp = p->m_parent;
        if (force || !p->m_state.torn)
            p->internal_hide();
        else
            p->m_parent = 0;
        p = tmp;
    }

}

void Menu::grabInputFocus() {
    // if there's a submenu open, focus it instead
    if (validIndex(m_which_sub) &&
            m_items[m_which_sub]->submenu()->isVisible()) {
        m_items[m_which_sub]->submenu()->grabInputFocus();
        return;
    }

    s_focused = this;

    // grab input focus
    m_window.setInputFocus(RevertToPointerRoot, CurrentTime);
}


void Menu::clearWindow() {
    m_title.win.clear();
    m_frame.win.clear();

    // clear foreground bits of frame items
    size_t i;
    size_t l = m_items.size();
    for (i = 0; i < l; i++) {
        clearItem(i, false);   // no clear
    }
    m_shape->update();
}

void Menu::redrawFrame(FbDrawable &drawable) {
    for (size_t i = 0; i < m_items.size(); i++) {
        drawItem(drawable, i);
    }

}

void Menu::internal_hide(bool first) {

    if (validIndex(m_which_sub)) {
        MenuItem *tmp = m_items[m_which_sub];
        if (tmp && tmp->submenu() && tmp->submenu()->isVisible())
            tmp->submenu()->internal_hide(false);
    }

    // if we have an active index we need to redraw it
    // as non active
    int old = m_active_index;
    m_active_index = -1;
    clearItem(old); // clear old area from highlight

    if (s_shown == this) {
        if (m_parent && m_parent->isVisible())
            s_shown = m_parent;
        else
            s_shown = 0;
    }

    m_state.torn = m_state.visible = m_state.closing = false;
    m_which_sub = -1;

    if (first && m_parent && m_parent->isVisible() &&
        s_focused && !s_focused->isVisible())
        m_parent->grabInputFocus();

    m_parent = 0;
    m_window.hide();
}


void Menu::move(int x, int y) {
    if (x == this->x() && y == this->y())
        return;

    m_window.move(x, y);
    // potentially transparent children
    m_title.win.parentMoved();
    m_frame.win.parentMoved();

    if (!isVisible())
        return;

    if (alpha() < 255)
        clearWindow();

    if (validIndex(m_which_sub) &&
            m_items[m_which_sub]->submenu()->isVisible())
        drawSubmenu(m_which_sub);
}


void Menu::redrawTitle(FbDrawable &drawable) {

    const FbTk::Font &font = theme()->titleFont();
    int dx = theme()->bevelWidth();
    int l = static_cast<int>(font.textWidth(m_title.label) + dx*2);

    switch (theme()->titleFontJustify()) {
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
    int height_offset = theme()->titleHeight(!m_title.label.logical().empty()) - (font.height() + 2*dx);
    font.drawText(drawable, screenNumber(), theme()->titleTextGC().gc(), m_title.label,
                  dx, font.ascent() + dx + height_offset/2);  // position
}


void Menu::drawSubmenu(unsigned int index) {
    if (validIndex(m_which_sub) && static_cast<unsigned int>(m_which_sub) != index) {
        MenuItem *itmp = m_items[m_which_sub];

        if (! itmp->submenu()->isTorn())
            itmp->submenu()->internal_hide();
    }

    if (index >= m_items.size())
        return;


    MenuItem *item = m_items[index];
    if (item->submenu() && isVisible() && (! item->submenu()->isTorn()) &&
        item->isEnabled()) {

        if (item->submenu()->m_parent != this)
            item->submenu()->m_parent = this;

        item->submenu()->setScreen(m_screen.x, m_screen.y, m_screen.width, m_screen.height);

        // ensure we do not divide by 0 and thus cause a SIGFPE
        if (m_rows_per_column == 0) {
#if DEBUG
            cerr << __FILE__ << "(" << __LINE__
                 << ") Error: m_rows_per_column == 0 in FbTk::Menu::drawSubmenu()\n";
#endif
            return;
        }

        int bw = m_window.borderWidth();
        int h = static_cast<int>(height());
        int title_bw = m_title.win.borderWidth();
        int title_height = (m_title.visible ? theme()->titleHeight(!m_title.label.logical().empty()) + title_bw : 0);

        int subm_title_height = (item->submenu()->m_title.visible) ?
                item->submenu()->theme()->titleHeight(!m_title.label.logical().empty()) + bw : 0;
        int subm_height = static_cast<int>(item->submenu()->height());
        int subm_width = static_cast<int>(item->submenu()->width());
        int subm_bw = item->submenu()->fbwindow().borderWidth();

        int column = index / m_rows_per_column;
        int row = index - (column * m_rows_per_column);
        int new_x = x() + ((m_item_w * (column + 1)) + bw);
        int new_y = y() + title_height - subm_title_height;

        if (m_alignment != ALIGNTOP) {
            new_y = new_y + (theme()->itemHeight() * row);
        }

        if (m_alignment == ALIGNBOTTOM && (new_y + subm_height) > (y() + h)) {
            new_y = (y() + h - subm_height);
        }

        if ((new_x + subm_width + 2*subm_bw) > (m_screen.x + static_cast<int>(m_screen.width))) {
            new_x = x() - subm_width - bw;
        }

        if (new_x < m_screen.x)
            new_x = m_screen.x;

        if ((new_y + subm_height) > (m_screen.y + static_cast<int>(m_screen.height))) {
            new_y = m_screen.y + static_cast<int>(m_screen.height) - subm_height - 2*bw;
        }

        item->submenu()->m_state.moving = m_state.moving;
        m_which_sub = index;

        if (new_y < m_screen.y)
            new_y = m_screen.y;

        item->submenu()->move(new_x, new_y);
        if (! m_state.moving)
            clearItem(index);

        if (! item->submenu()->isVisible() && item->submenu()->numberOfItems() > 0) {
            s_shown = item->submenu();
            item->showSubmenu();
            item->submenu()->raise();
        }


    } else
        m_which_sub = -1;

}

int Menu::drawItem(FbDrawable &drawable, unsigned int index,
                   bool highlight, bool exclusive_drawable) {

    if (!validIndex(index) || m_items.empty()) {
        return 0;
    }

    // ensure we do not divide by 0 and thus cause a SIGFPE
    if (m_rows_per_column == 0) {
#if DEBUG
        cerr << __FILE__ << "(" << __LINE__
             << ") Error: m_rows_per_column == 0 in FbTk::Menu::drawItem()\n";
#endif
        return 0;
    }

    MenuItem *item = m_items[index];
    if (!item)
        return 0;

    int column = index / m_rows_per_column;
    int row = index - (column * m_rows_per_column);
    int item_x = (column * m_item_w);
    int item_y = (row * theme()->itemHeight());

    if (exclusive_drawable)
        item_x = item_y = 0;

    item->draw(drawable, theme(), highlight,
               exclusive_drawable, true, // draw fg, draw bg
               item_x, item_y,
               m_item_w, theme()->itemHeight());

    return item_y;
}

void Menu::setLabel(const FbTk::BiDiString &labelstr) {
    m_title.label = labelstr;
    reconfigure();
}


void Menu::setItemSelected(unsigned int index, bool sel) {
    if (!validIndex(index)) {
        return;
    }
    MenuItem *item = find(index);
    if (! item) return;

    item->setSelected(sel);
}


bool Menu::isItemSelected(unsigned int index) const{
    if (!validIndex(index)) {
        return false;
    }
    const MenuItem *item = find(index);
    if (!item)
        return false;

    return item->isSelected();
}


void Menu::setItemEnabled(unsigned int index, bool enable) {
    if (!validIndex(index)) {
        return;
    }
    MenuItem *item = find(index);
    if (item) {
        item->setEnabled(enable);
    }
}


bool Menu::isItemEnabled(unsigned int index) const {
    if (!validIndex(index)) {
        return false;
    }
    const MenuItem *item = find(index);
    if (!item)
        return false;
    return item->isEnabled();
}

bool Menu::isItemSelectable(unsigned int index) const {
    if (!validIndex(index)) {
        return false;
    }
    const MenuItem *item = find(index);
    return (!item || !item->isEnabled()) ? false : true;
}


void Menu::handleEvent(XEvent &event) {
    if (event.type == FocusOut) {
        if (s_focused == this)
            s_focused = 0;
    // I don't know why, but I get a FocusIn event when closing the menu with
    // the mouse over it -- probably an xorg bug, but it's easy to address here
    } else if (event.type == FocusIn && isVisible()) {
        if (s_focused != this)
            s_focused = this;
        // if there's a submenu open, focus it instead
        if (validIndex(m_which_sub) &&
                m_items[m_which_sub]->submenu()->isVisible())
            m_items[m_which_sub]->submenu()->grabInputFocus();
    }
}

void Menu::buttonPressEvent(XButtonEvent &be) {

    m_state.closing = false;
    if (be.window == m_title.win) {
        grabInputFocus();
        m_state.closing = (be.button == 3);
    }

    if (be.window == m_frame.win && m_item_w != 0) {

        int column = (be.x / m_item_w);
        int i = (be.y / theme()->itemHeight());
        int w = (column * m_rows_per_column) + i;

        if (isItemSelectable(static_cast<unsigned int>(w))) {
            MenuItem *item = m_items[w];

            if (item->submenu()) {
                if (!item->submenu()->isVisible())
                    drawSubmenu(w);
            }
        }
    } else {
        m_x_move = be.x_root - x();
        m_y_move = be.y_root - y();
    }
}


void Menu::buttonReleaseEvent(XButtonEvent &re) {
    if (re.window == m_title.win) {
        if (m_state.moving) {
            m_state.moving = false;

            if (validIndex(m_which_sub) &&
                    m_items[m_which_sub]->submenu()->isVisible())
                drawSubmenu(m_which_sub);

            if (alpha() < 255) {
                // update these since we've (probably) moved
                m_title.win.parentMoved();
                m_frame.win.parentMoved();
                clearWindow();
            }
        }

        if (re.button == 3 && m_state.closing)
            internal_hide();

    } else if (re.window == m_frame.win) {

        int column = (re.x / m_item_w);
        int i = (re.y / theme()->itemHeight());
        int ix = column * m_item_w;
        int iy = i * theme()->itemHeight();
        int w = (column * m_rows_per_column) + i;

        if (validIndex(w) && isItemSelectable(static_cast<unsigned int>(w))) {
            if (m_active_index == w && isItemEnabled(w) &&
                re.x > ix && re.x < (signed) (ix + m_item_w) &&
                re.y > iy && re.y < (signed) (iy + theme()->itemHeight())) {
                m_items[w]->click(re.button, re.time, re.state);
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
    if (me.window == m_title.win && (me.state & Button1Mask)) {
        stopHide();

        if (! m_state.moving) { // if not m_moving: start m_moving operation

            m_state.moving = m_state.torn = true;
            if (m_parent)
                m_parent->m_which_sub = -1;

            clearItem(m_active_index);

            if (validIndex(m_which_sub) &&
                    m_items[m_which_sub]->submenu()->isVisible())
                drawSubmenu(m_which_sub);
        } else {
            // we dont call ::move here 'cause we dont want to update transparency
            // while draging the menu (which is slow)
            m_window.move(me.x_root - m_x_move, me.y_root - m_y_move);
        }

    } else if (!(me.state & Button1Mask) && me.window == m_frame.win) {
        stopHide();
        int column = (me.x / m_item_w);
        int i = (me.y / theme()->itemHeight());
        int w = (column * m_rows_per_column) + i;

        if (w == m_active_index || !validIndex(w))
            return;

        // if another menu is focused, change focus to this one, so arrow keys
        // work as expected
        if (s_focused != this && s_focused != 0)
            grabInputFocus();

        MenuItem *itmp = m_items[w];
        if (itmp == 0)
            return;

        if (itmp->isEnabled()) {
            int old = m_active_index;
            m_active_index = w;
            clearItem(w);
            clearItem(old);

            MenuItem *item = validIndex(m_which_sub) ? m_items[m_which_sub] : 0;
            if (item != 0 && item->submenu() && item->submenu()->isVisible() &&
                !item->submenu()->isTorn()) {
                // setup hide timer for submenu
                item->submenu()->startHide();
            }

        }

        if (itmp->submenu()) { // start submenu open delay
            m_submenu_timer.setTimeout(theme()->getDelay() * FbTk::FbTime::IN_MILLISECONDS);
            m_submenu_timer.start();
        } else if (isItemSelectable(w)){
            // else normal menu item
            // draw highlighted
            m_submenu_timer.stop();
        }

    }
}


void Menu::exposeEvent(XExposeEvent &ee) {

    // some xservers (eg: nxserver) send XExposeEvent for the unmapped menu.
    // this caused a SIGFPE in ::clearItem(), since m_rows_per_column is
    // still 0 -> division by 0.
    //
    // it is still unclear, why nxserver behaves this way
    if (!isVisible())
        return;

    if (ee.window == m_title.win) {
        m_title.win.clearArea(ee.x, ee.y, ee.width, ee.height);
    } else if (ee.window == m_frame.win) {

        // the menu has a list of items. the expose-event spans
        // a number of columns across a number of rows
        //
        //  |item1|item4|item7|     * - exposed items
        //  |item2|*tem5|*tem8| i
        //  |item3|*tem6|*tem9| |
        //           j   ->    ts
        //

        size_t item_h = theme()->itemHeight();
        size_t t = ((ee.x + ee.width) / m_item_w) + 1;
        size_t row = ee.y / item_h;
        size_t end_row = ((ee.y + ee.height) / item_h);

        if (end_row > static_cast<size_t>(m_rows_per_column))
            end_row = static_cast<size_t>(m_rows_per_column);

        for (size_t j = (ee.x / m_item_w); j < t; j++) {

            size_t offset = j * m_rows_per_column;
            size_t s = end_row + offset;
            s = std::min(m_items.size(), s);
            for (size_t i = row + offset; i < s; i++ ) {
                clearItem(i);
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
        cycleItems(UP);
        break;
    case XK_Down:
        resetTypeAhead();
        cycleItems(DOWN);
        break;
    case XK_Left: // enter parent if we have one
        resetTypeAhead();
        if (m_columns > 1 && m_active_index >= m_rows_per_column) {
            int new_index = m_active_index - m_rows_per_column;
            while (new_index >= 0 && !isItemEnabled(new_index))
                new_index -= m_rows_per_column;
            if (new_index >= 0)
                setActiveIndex(new_index);
        } else
            internal_hide();
        break;
    case XK_Right: // enter submenu if we have one
        resetTypeAhead();
        if (m_columns > 1 && validIndex(m_active_index) &&
            validIndex(m_active_index + m_rows_per_column)) {
            int new_index = m_active_index + m_rows_per_column;
            while (validIndex(new_index) && !isItemEnabled(new_index))
                new_index += m_rows_per_column;
            if (validIndex(new_index))
                setActiveIndex(new_index);
        } else
            enterSubmenu();
        break;
    case XK_Escape: // close menu
        m_search->clear();
        m_state.torn = false;
        hide(true);
        break;
    case XK_BackSpace:
        if (m_search->size() == 0) {
            internal_hide();
            break;
        }
        m_search->backspace();
        drawTypeAheadItems();
        break;
    case XK_KP_Enter:
    case XK_Return:
        resetTypeAhead();
        if (validIndex(m_active_index) &&
            isItemEnabled(m_active_index)) {
            // send fake button click
            int button = (event.state & ShiftMask) ? 3 : 1;
            if (m_items[m_active_index]->submenu() != 0 && button == 1)
                enterSubmenu();
            else {
                find(m_active_index)->click(button, event.time, event.state);
                m_need_update = true;
                updateMenu();
            }
        }
        break;
    case XK_Tab:
    case XK_ISO_Left_Tab: // XXX: number matches == 1 -> enter submenu
        if (validIndex(m_active_index) && isItemEnabled(m_active_index) &&
            m_items[m_active_index]->submenu() && m_search->num_matches() == 1) {
            enterSubmenu();
            m_search->clear();
        } else {
            cycleItems((bool)(event.state & ShiftMask));
        }
        drawTypeAheadItems();
        break;
    default:

        if (m_search->would_match(m_search->pattern + keychar[0])) {
            m_search->add(keychar[0]);
            drawTypeAheadItems();
            // if current item doesn't match new search string, find
            // the next one
            size_t ignore;
            if (!m_search->get_match(m_active_index, ignore)) {
                cycleItems(DOWN);
            }
        }

        break;
    }
}

void Menu::leaveNotifyEvent(XCrossingEvent &ce) {
    m_state.closing = false;
    // if there's a submenu open, highlight its index and stop hide
    if (validIndex(m_which_sub) && m_active_index != m_which_sub &&
        m_items[m_which_sub]->submenu()->isVisible()) {
        int old = m_active_index;
        m_active_index = m_which_sub;
        clearItem(m_active_index);
        clearItem(old);
        m_items[m_which_sub]->submenu()->stopHide();
    }
}

void Menu::reconfigure() {

    m_shape->setPlaces(theme()->shapePlaces());

    FbTk::Color bc = theme()->borderColor();
    int bw = theme()->borderWidth();
    int alpha = this->alpha();
    int opaque = 255;

    if (FbTk::Transparent::haveComposite()) {
        std::swap(opaque, alpha);
    }

    m_window.setOpaque(opaque);
    m_title.win.setAlpha(alpha);
    m_frame.win.setAlpha(alpha);

    m_need_update = true; // redraw items

    m_window.setBorderColor(bc);
    m_title.win.setBorderColor(bc);
    m_frame.win.setBorderColor(bc);

    m_window.setBorderWidth(bw);
    m_title.win.setBorderWidth(bw);

    updateMenu();
}


void Menu::openSubmenu() {

    size_t i = m_active_index;
    if (!isVisible() || !validIndex(i) || !m_items[i]->isEnabled() ||
        (s_focused != this && s_focused && s_focused->isVisible()))
        return;

    clearItem(i);

    if (m_items[i]->submenu() != 0) {
        // stop hide timer, so it doesnt hides the menu if we
        // have the same submenu as the last shown submenu
        // (window menu for clients inside workspacemenu for example)
        m_items[i]->submenu()->m_hide_timer.stop();
        drawSubmenu(i);
    }

}

void Menu::closeMenu() {
    if (isVisible() && !isTorn())
        internal_hide();
}

void Menu::startHide() {
    m_hide_timer.setTimeout(theme()->getDelay() * FbTk::FbTime::IN_MILLISECONDS);
    m_hide_timer.start();
}

void Menu::stopHide() {
    m_hide_timer.stop();
}

void Menu::themeReconfigured() {

    m_need_update = true;

    size_t l = m_items.size();
    size_t i;
    for (i = 0; i < l; i++) {
        m_items[i]->updateTheme(theme());
    }
    reconfigure();
}

void Menu::setScreen(int x, int y, unsigned int w, unsigned int h) {
    m_screen.x = x;
    m_screen.y = y;
    m_screen.width = w;
    m_screen.height = h;
}

// Render the foreground objects of given window onto given pixmap
void Menu::renderForeground(FbWindow &win, FbDrawable &drawable) {
    if (&win == &m_frame.win) {
        redrawFrame(drawable);
    } else if (&win == &m_title.win) {
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

    // ensure we do not divide by 0 and thus cause a SIGFPE
    if (m_rows_per_column == 0) {
#if DEBUG
        cerr << __FILE__ << "(" << __LINE__
             << ") Error: m_rows_per_column == 0 in FbTk::Menu::clearItem()\n";
#endif
        return;
    }

    int column = index / m_rows_per_column;
    int row = index - (column * m_rows_per_column);
    int item_w = m_item_w;
    int item_h = theme()->itemHeight();
    int item_x = (column * item_w);
    int item_y = (row * item_h);
    bool highlight = (index == m_active_index && isItemSelectable(index));

    size_t start_idx = std::string::npos;
    size_t end_idx = std::string::npos;

    if (m_search->get_match(index, start_idx)) {
        end_idx = start_idx + m_search->size();

#if 0
        std::cerr << "m_search " << index << "|"
            << m_items[index]->iTypeString() << "|" << m_search->text
            << "|" << start_idx << " " << end_idx  << "\n";
#endif
    }

    // highlight only if not if moving, doesn't work with alpha on
    if (highlight && !m_state.moving) {
        highlightItem(index);
    } else {
        if (clear) {
            m_frame.win.clearArea(item_x, item_y, item_w, item_h);
        }

        m_items[index]->draw(m_frame.win, theme(), highlight,
                true, false, item_x, item_y,
                item_w, item_h);
    }

    // need a underline (aka "matched item")
    if (start_idx != end_idx) {
        m_items[index]->drawLine(m_frame.win, theme(),
                end_idx - start_idx, item_x, item_y, m_item_w, start_idx);
    }
}

// Area must have been cleared before calling highlight
void Menu::highlightItem(int index) {

    // ensure we do not divide by 0 and thus cause a SIGFPE
    if (m_rows_per_column == 0) {
#if DEBUG
        cerr << __FILE__ << "(" << __LINE__
             << ") Error: m_rows_per_column == 0 in FbTk::Menu::highlightItem()\n";
#endif
        return;
    }

    int column = index / m_rows_per_column;
    int row = index - (column * m_rows_per_column);
    int item_w = m_item_w;
    int item_h = theme()->itemHeight();
    int item_x = (column * m_item_w);
    int item_y = (row * item_h);
    FbPixmap buffer = FbPixmap(m_frame.win, item_w, item_h, m_frame.win.depth());
    bool parent_rel = (m_hilite_pixmap == ParentRelative);
    Pixmap pixmap = parent_rel ? m_frame.pixmap : m_hilite_pixmap;
    int pixmap_x = parent_rel ? item_x : 0;
    int pixmap_y = parent_rel ? item_y : 0;

    if (pixmap) {
        buffer.copyArea(pixmap,
                        theme()->hiliteGC().gc(), pixmap_x, pixmap_y,
                        0, 0,
                        item_w, item_h);
    } else {
        buffer.fillRectangle(theme()->hiliteGC().gc(),
                             0, 0, item_w, item_h);
    }

    m_frame.win.updateTransparent(item_x, item_y, item_w, item_h, buffer.drawable(), true);
    drawItem(buffer, index, true, true);
    m_frame.win.copyArea(buffer.drawable(), theme()->hiliteGC().gc(),
                        0, 0,
                        item_x, item_y,
                        item_w, item_h);

}

void Menu::resetTypeAhead() {
    m_search->clear();
    drawTypeAheadItems();
}

void Menu::drawTypeAheadItems() {
    size_t i;
    for (i = 0; i < m_items.size(); i++) {
        clearItem(i, true);
    }
}

void Menu::setTitleVisibility(bool b) {
    m_title.visible = b;
    m_need_update = true;
    if (!b)
        titleWindow().lower();
    else
        titleWindow().raise();
}

} // end namespace FbTk
