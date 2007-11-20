// FbWinFrame.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "FbWinFrame.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/App.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Compose.hh"
#include "FbTk/Transparent.hh"
#include "CompareWindow.hh"
#include "FbWinFrameTheme.hh"
#include "Screen.hh"

#include "IconButton.hh"
#include "Container.hh"

#ifdef SHAPE
#include "Shape.hh"
#endif // SHAPE

#include <algorithm>
#include <X11/X.h>

using std::mem_fun;
using std::string;

FbWinFrame::FbWinFrame(BScreen &screen, FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl,
                       FbTk::XLayer &layer,
                       int x, int y,
                       unsigned int width, unsigned int height):
    m_screen(screen),
    m_theme(theme),
    m_imagectrl(imgctrl),
    m_window(theme.screenNum(), x, y, width, height,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | EnterWindowMask, true),
    m_layeritem(window(), layer),
    m_titlebar(m_window, 0, 0, 100, 16,
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask),
    m_tab_container(m_titlebar),
    m_label(m_titlebar, m_theme.font(), ""),
    m_handle(m_window, 0, 0, 100, 5,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | ExposureMask |
             EnterWindowMask | LeaveWindowMask),
    m_grip_right(m_handle, 0, 0, 10, 4,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_grip_left(m_handle, 0, 0, 10, 4,
		ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | ExposureMask |
		EnterWindowMask | LeaveWindowMask),
    m_clientarea(m_window, 0, 0, 100, 100,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_bevel(1),
    m_use_titlebar(true),
    m_use_tabs(true),
    m_use_handle(true),
    m_focused(false),
    m_visible(false),
    m_use_default_alpha(2),
    m_button_pm(0),
    m_tabmode(screen.getDefaultInternalTabs()?INTERNAL:EXTERNAL),
    m_active_gravity(0),
    m_active_orig_client_bw(0),
    m_need_render(true),
    m_button_size(1),
    m_height_before_shade(1),
    m_shaded(false),
    m_focused_alpha(0),
    m_unfocused_alpha(0),
    m_themelistener(*this),
    m_shape(m_window, theme.shapePlace()),
    m_disable_themeshape(false) {
    m_theme.reconfigSig().attach(&m_themelistener);
    init();
}

FbWinFrame::~FbWinFrame() {
    removeEventHandler();
    removeAllButtons();
}

bool FbWinFrame::setTabMode(TabMode tabmode) {
    if (m_tabmode == tabmode)
        return false;

    bool ret = true;

    // setting tabmode to notset forces it through when
    // something is likely to change
    if (tabmode == NOTSET)
        tabmode = m_tabmode;

    m_tabmode = tabmode;

    // reparent tab container
    if (tabmode == EXTERNAL) {
        m_label.show();
        m_tab_container.setBorderWidth(m_window.borderWidth());
        m_tab_container.setBorderColor(theme().border().color());
        m_tab_container.setEventMask(
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask);

        XGrabButton(m_tab_container.display(), Button1, AnyModifier,
                    m_tab_container.window(), True, ButtonPressMask,
                    GrabModeSync, GrabModeSync, None, None);
        XUngrabButton(m_tab_container.display(), Button1, Mod1Mask|Mod2Mask|Mod3Mask, m_tab_container.window());

        alignTabs();

        // TODO: tab position
        if (m_use_tabs && m_visible)
            m_tab_container.show();
        else {
            ret = false;
            m_tab_container.hide();
        }

    } else {
        m_tab_container.setUpdateLock(true);

        m_tab_container.setAlignment(Container::RELATIVE);
        m_tab_container.setOrientation(FbTk::ROT0);
        if (m_tab_container.parent()->window() == m_screen.rootWindow().window()) {
            m_layeritem.removeWindow(m_tab_container);
            m_tab_container.hide();
            m_tab_container.reparent(m_titlebar, m_label.x(), m_label.y());
            m_tab_container.invalidateBackground();
            m_tab_container.resize(m_label.width(), m_label.height());
            m_tab_container.raise();
        }
        m_tab_container.setBorderWidth(0);
        m_tab_container.setMaxTotalSize(0);
        m_tab_container.setUpdateLock(false);
        m_tab_container.setMaxSizePerClient(0);

        renderTabContainer();
        applyTabContainer();
        m_tab_container.clear();

        m_tab_container.raise();
        m_tab_container.show();

        if (!m_use_tabs)
            ret = false;

        m_label.hide();
//        reconfigure();
    }

    return true;
}

void FbWinFrame::hide() {
    m_window.hide();
    if (m_tabmode == EXTERNAL && m_use_tabs)
        m_tab_container.hide();

    m_visible = false;
}

void FbWinFrame::show() {
    m_visible = true;

    if (m_need_render) {
        renderAll();
         applyAll();
        clearAll();
    }

    if (m_tabmode == EXTERNAL && m_use_tabs)
        m_tab_container.show();

    m_window.showSubwindows();
    m_window.show();
}

/**
 Toggle shade state, and resize window
 */
void FbWinFrame::shade() {
    if (!m_use_titlebar)
        return;

    // toggle shade
    m_shaded = !m_shaded;
    if (m_shaded) { // i.e. should be shaded now
        m_height_before_shade = m_window.height();
        m_window.resize(m_window.width(), m_titlebar.height());
        alignTabs();
        // need to update our shape
        m_shape.update();
    } else { // should be unshaded
        m_window.resize(m_window.width(), m_height_before_shade);
        reconfigure();
    }
}


void FbWinFrame::move(int x, int y) {
    moveResize(x, y, 0, 0, true, false);
}

void FbWinFrame::resize(unsigned int width, unsigned int height) {
    moveResize(0, 0, width, height, false, true);
}

// need an atomic moveresize where possible
void FbWinFrame::moveResizeForClient(int x, int y,
                                     unsigned int width, unsigned int height,
                                     int win_gravity,
                                     unsigned int client_bw,
                                     bool move, bool resize) {
    // total height for frame

    if (resize) // these fns check if the elements are "on"
        height += titlebarHeight() + handleHeight();

    gravityTranslate(x, y, win_gravity, client_bw, false);
    setActiveGravity(win_gravity, client_bw);
    moveResize(x, y, width, height, move, resize);
}

void FbWinFrame::resizeForClient(unsigned int width, unsigned int height,
                                 int win_gravity, unsigned int client_bw) {
    moveResizeForClient(0, 0, width, height, win_gravity, client_bw, false, true);
}

void FbWinFrame::moveResize(int x, int y, unsigned int width, unsigned int height, bool move, bool resize) {
    if (move && x == window().x() && y == window().y())
        move = false;

    if (resize && (m_shaded || width == FbWinFrame::width() &&
                               height == FbWinFrame::height()))
        resize = false;

    if (!move && !resize)
        return;

    if (move && resize) {
        m_window.moveResize(x, y, width, height);
        notifyMoved(false); // will reconfigure
    } else if (move) {
        m_window.move(x, y);
        // this stuff will be caught by reconfigure if resized
        notifyMoved(true);
    } else {
        m_window.resize(width, height);
    }

    if (move || resize && m_screen.getTabPlacement() != TOPLEFT)
        alignTabs();

    if (resize) {
        if (m_tabmode == EXTERNAL) {
            switch(m_screen.getTabPlacement()) {
            case LEFTTOP:
            case RIGHTTOP:
            case LEFTBOTTOM:
            case RIGHTBOTTOM:
                m_tab_container.setMaxTotalSize(height);
                break;
            default:
                m_tab_container.setMaxTotalSize(width);
                break;
            }
        }
        reconfigure();
    }
}

void FbWinFrame::quietMoveResize(int x, int y,
                                 unsigned int width, unsigned int height) {
    m_window.moveResize(x, y, width, height);
    if (m_tabmode == EXTERNAL) {

        switch(m_screen.getTabPlacement()) {
        case LEFTTOP:
        case RIGHTTOP:
        case LEFTBOTTOM:
        case RIGHTBOTTOM:
            m_tab_container.setMaxTotalSize(height);
            break;
        default:
            m_tab_container.setMaxTotalSize(width);
            break;
        }
        alignTabs();
    }
}

void FbWinFrame::alignTabs() {
    if (m_tabmode != EXTERNAL)
        return;


    FbTk::Orientation orig_orient = m_tab_container.orientation();
    unsigned int orig_tabwidth = m_tab_container.maxWidthPerClient();

    if (orig_tabwidth != m_screen.getTabWidth())
        m_tab_container.setMaxSizePerClient(m_screen.getTabWidth());

    int tabx = 0, taby = 0;
    switch (m_screen.getTabPlacement()) {
    case TOPLEFT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x();
        taby = y() - yOffset();
        break;
    case TOPRIGHT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x() + width() - m_tab_container.width();
        taby = y() - yOffset();
        break;
    case LEFTTOP:
        if (orig_orient != FbTk::ROT270) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT270);
        m_tab_container.setAlignment(Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() - xOffset();
        taby = y();
        break;
    case LEFTBOTTOM:
        if (orig_orient != FbTk::ROT270) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT270);
        m_tab_container.setAlignment(Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() - xOffset();
        taby = y() + height() - m_tab_container.height();
        break;
    case RIGHTTOP:
        if (orig_orient != FbTk::ROT90) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT90);
        m_tab_container.setAlignment(Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() + width() + m_window.borderWidth();
        taby = y();
        break;
    case RIGHTBOTTOM:
        if (orig_orient != FbTk::ROT90) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT90);
        m_tab_container.setAlignment(Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() + width() + m_window.borderWidth();
        taby = y() + height() - m_tab_container.height();
        break;
    case BOTTOMLEFT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x();
        taby = y() + height() + m_window.borderWidth();
        break;
    case BOTTOMRIGHT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x() + width() - m_tab_container.width();
        taby = y() + height() + m_window.borderWidth();
        break;
    }

    unsigned int w = m_window.width(), h = m_window.height();
    translateSize(m_tab_container.orientation(), w, h);

    if (m_tab_container.orientation() != orig_orient ||
        m_tab_container.maxWidthPerClient() != orig_tabwidth) {
        renderTabContainer();
        if (m_visible && m_use_tabs) {
            applyTabContainer();
            m_tab_container.clear();
            m_tab_container.show();
        }
    }

    if (m_tab_container.parent()->window() != m_screen.rootWindow().window()) {
        m_tab_container.reparent(m_screen.rootWindow(), tabx, taby);
        m_label.clear();
        m_layeritem.addWindow(m_tab_container);
    } else {
        m_tab_container.move(tabx, taby);
    }
}

void FbWinFrame::notifyMoved(bool clear) {
    // not important if no alpha...
    unsigned char alpha = getAlpha(m_focused);
    if (alpha == 255)
        return;

    if (m_tabmode == EXTERNAL && m_use_tabs || m_use_titlebar) {
        m_tab_container.parentMoved();
        m_tab_container.for_each(mem_fun(&FbTk::Button::parentMoved));
    }

    if (m_use_titlebar) {
        if (m_tabmode != INTERNAL)
            m_label.parentMoved();

        m_titlebar.parentMoved();

        for_each(m_buttons_left.begin(),
                 m_buttons_left.end(),
                 mem_fun(&FbTk::Button::parentMoved));
        for_each(m_buttons_right.begin(),
                 m_buttons_right.end(),
                 mem_fun(&FbTk::Button::parentMoved));
    }

    if (m_use_handle) {
        m_handle.parentMoved();
        m_grip_left.parentMoved();
        m_grip_right.parentMoved();
    }

    if (clear && (m_use_handle || m_use_titlebar)) {
        clearAll();
    } else if (clear && m_tabmode == EXTERNAL && m_use_tabs)
        m_tab_container.clear();
}

void FbWinFrame::clearAll() {

    if  (m_use_titlebar) {
        redrawTitlebar();

        for_each(m_buttons_left.begin(),
                 m_buttons_left.end(),
                 mem_fun(&FbTk::Button::clear));
        for_each(m_buttons_right.begin(),
                 m_buttons_right.end(),
                 mem_fun(&FbTk::Button::clear));
    } else if (m_tabmode == EXTERNAL && m_use_tabs)
        m_tab_container.clear();

    if (m_use_handle) {
        m_handle.clear();
        m_grip_left.clear();
        m_grip_right.clear();
    }
}

void FbWinFrame::setFocus(bool newvalue) {
    if (m_focused == newvalue)
        return;

    m_focused = newvalue;

    if (FbTk::Transparent::haveRender() && 
        getAlpha(true) != getAlpha(false)) { // different alpha for focused and unfocused
        unsigned char alpha = getAlpha(m_focused);
        if (FbTk::Transparent::haveComposite()) {
            m_tab_container.setAlpha(255);
            m_window.setOpaque(alpha);
        } else {
            m_tab_container.setAlpha(alpha);
            m_window.setOpaque(255);
        }
    }

    applyAll();
    clearAll();
}

void FbWinFrame::setAlpha(bool focused, unsigned char alpha) {
    if (m_use_default_alpha == 2) {
        /// Set basic defaults
        m_focused_alpha = getAlpha(true);
        m_unfocused_alpha = getAlpha(false);
    }
    m_use_default_alpha = 0;

    if (focused)
        m_focused_alpha = alpha;
    else
        m_unfocused_alpha = alpha;

    if (m_focused == focused) {
        if (FbTk::Transparent::haveComposite())
            m_window.setOpaque(alpha);
        else {
            // don't need to setAlpha, since apply updates them anyway
            applyAll();
            clearAll();
        }
    }
}

unsigned char FbWinFrame::getAlpha(bool focused) const
{
  return getUseDefaultAlpha() ?
        (focused ? theme().focusedAlpha() : theme().unfocusedAlpha())
      : (focused ? m_focused_alpha        : m_unfocused_alpha);
}

void FbWinFrame::setUseDefaultAlpha(bool default_alpha)
{
    if (getUseDefaultAlpha() == default_alpha)
        return;

    if (!default_alpha && m_use_default_alpha == 2) {
        m_focused_alpha = theme().focusedAlpha();
        m_unfocused_alpha = theme().unfocusedAlpha();
    }

    m_use_default_alpha = default_alpha;

    if (FbTk::Transparent::haveComposite())
        m_window.setOpaque(getAlpha(m_focused));
    else {
        // don't need to setAlpha, since apply updates them anyway
        applyAll();
        clearAll();
    }
}

void FbWinFrame::addLeftButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    applyButton(*btn); // setup theme and other stuff

    m_buttons_left.push_back(btn);
}

void FbWinFrame::addRightButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    applyButton(*btn); // setup theme and other stuff

    m_buttons_right.push_back(btn);
}

void FbWinFrame::removeAllButtons() {
    // destroy left side
    while (!m_buttons_left.empty()) {
        delete m_buttons_left.back();
        m_buttons_left.pop_back();
    }
    // destroy right side
    while (!m_buttons_right.empty()) {
        delete m_buttons_right.back();
        m_buttons_right.pop_back();
    }
}

IconButton *FbWinFrame::createTab(Focusable &client) {
    IconButton *button = new IconButton(m_tab_container, theme().iconbarTheme(),
                                        client);

    button->show();
    button->setEventMask(ExposureMask | ButtonPressMask |
                         ButtonReleaseMask | ButtonMotionMask |
                         EnterWindowMask);
    FbTk::EventManager::instance()->add(*button, button->window());

    m_tab_container.insertItem(button);

    return button;
}

void FbWinFrame::removeTab(IconButton *btn) {
    if (m_tab_container.removeItem(btn))
        delete btn;
}


void FbWinFrame::moveLabelButtonLeft(FbTk::TextButton &btn) {
    m_tab_container.moveItem(&btn, -1);
}

void FbWinFrame::moveLabelButtonRight(FbTk::TextButton &btn) {
    m_tab_container.moveItem(&btn, +1);
}

void FbWinFrame::moveLabelButtonTo(FbTk::TextButton &btn, int x, int y) {
    m_tab_container.moveItemTo(&btn, x, y);
}



void FbWinFrame::moveLabelButtonLeftOf(FbTk::TextButton &btn, const FbTk::TextButton &dest) {
    int dest_pos = m_tab_container.find(&dest);
    int cur_pos = m_tab_container.find(&btn);
    if (dest_pos < 0 || cur_pos < 0)
        return;
    int movement=dest_pos - cur_pos;
    if(movement>0)
        movement-=1;
//    else
  //      movement-=1;

    m_tab_container.moveItem(&btn, movement);
}

void FbWinFrame::moveLabelButtonRightOf(FbTk::TextButton &btn, const FbTk::TextButton &dest) {
    int dest_pos = m_tab_container.find(&dest);
    int cur_pos = m_tab_container.find(&btn);
    if (dest_pos < 0 || cur_pos < 0 )
        return;
    int movement=dest_pos - cur_pos;
    if(movement<0)
        movement+=1;

    m_tab_container.moveItem(&btn, movement);
}

void FbWinFrame::setLabelButtonFocus(IconButton &btn) {
    if (btn.parent() != &m_tab_container)
        return;
    m_label.setText(btn.text());
}

void FbWinFrame::setClientWindow(FbTk::FbWindow &win) {

    win.setBorderWidth(0);

    XChangeSaveSet(win.display(), win.window(), SetModeInsert);

    m_window.setEventMask(NoEventMask);

    // we need to mask this so we don't get unmap event
    win.setEventMask(NoEventMask);
    win.reparent(m_window, 0, clientArea().y());
    // remask window so we get events
    win.setEventMask(PropertyChangeMask | StructureNotifyMask |
                     FocusChangeMask | KeyPressMask);

    m_window.setEventMask(ButtonPressMask | ButtonReleaseMask |
                          ButtonMotionMask | EnterWindowMask | SubstructureRedirectMask);

    XFlush(win.display());

    XSetWindowAttributes attrib_set;
    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;

    XChangeWindowAttributes(win.display(), win.window(), CWEventMask|CWDontPropagate, &attrib_set);

    m_clientarea.raise();
    if (isVisible())
        win.show();
    win.raise();
    m_window.showSubwindows();

}

bool FbWinFrame::hideTabs() {
    if (m_tabmode == INTERNAL || !m_use_tabs) {
        m_use_tabs = false;
        return false;
    }

    m_use_tabs = false;
    m_tab_container.hide();
    return true;
}

bool FbWinFrame::showTabs() {
    if (m_tabmode == INTERNAL || m_use_tabs) {
        m_use_tabs = true;
        return false; // nothing changed
    }

    m_use_tabs = true;
    if (m_visible)
        m_tab_container.show();
    return true;
}

bool FbWinFrame::hideTitlebar() {
    if (!m_use_titlebar)
        return false;

    m_titlebar.hide();
    m_use_titlebar = false;
    if (static_cast<signed int>(m_window.height() - m_titlebar.height() -
                                m_titlebar.borderWidth()) <= 0) {
        m_window.resize(m_window.width(), 1);
    } else {
        // only take away one borderwidth (as the other border is still the "top" border)
        m_window.resize(m_window.width(), m_window.height() - m_titlebar.height() -
                        m_titlebar.borderWidth());
    }

    return true;
}

bool FbWinFrame::showTitlebar() {
    if (m_use_titlebar)
        return false;

    m_titlebar.show();
    m_use_titlebar = true;

    // only add one borderwidth (as the other border is still the "top" border)
    m_window.resize(m_window.width(), m_window.height() + m_titlebar.height() +
                    m_titlebar.borderWidth());

    return true;

}

bool FbWinFrame::hideHandle() {
    if (!m_use_handle)
        return false;
    m_handle.hide();
    m_grip_left.hide();
    m_grip_right.hide();
    m_use_handle = false;

    if (static_cast<signed int>(m_window.height() - m_handle.height() -
                                m_handle.borderWidth()) <= 0) {
        m_window.resize(m_window.width(), 1);
    } else {
        // only take away one borderwidth (as the other border is still the "top" border)
        m_window.resize(m_window.width(), m_window.height() - m_handle.height() -
                        m_handle.borderWidth());
    }

    return true;

}

bool FbWinFrame::showHandle() {
    if (m_use_handle || theme().handleWidth() == 0)
        return false;

    m_use_handle = true;

    // weren't previously rendered...
    renderHandles();
    applyHandles();

    m_handle.show();
    m_handle.showSubwindows(); // shows grips

    m_window.resize(m_window.width(), m_window.height() + m_handle.height() +
                    m_handle.borderWidth());
    return true;
}

bool FbWinFrame::hideAllDecorations() {
    bool changed = false;
    changed |= hideHandle();
    changed |= hideTitlebar();
    // resize done by hide*
    reconfigure();

    return changed;
}

bool FbWinFrame::showAllDecorations() {
    bool changed = false;
    if (!m_use_handle)
        changed |= showHandle();
    if (!m_use_titlebar)
        changed |= showTitlebar();
    // resize shouldn't be necessary
    reconfigure();

    return changed;
}

/**
   Set new event handler for the frame's windows
*/
void FbWinFrame::setEventHandler(FbTk::EventHandler &evh) {

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(evh, m_tab_container);
    evm.add(evh, m_label);
    evm.add(evh, m_titlebar);
    evm.add(evh, m_handle);
    evm.add(evh, m_grip_right);
    evm.add(evh, m_grip_left);
    evm.add(evh, m_window);
    evm.add(evh, m_clientarea);
}

/**
   remove event handler from windows
*/
void FbWinFrame::removeEventHandler() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(m_tab_container);
    evm.remove(m_label);
    evm.remove(m_titlebar);
    evm.remove(m_handle);
    evm.remove(m_grip_right);
    evm.remove(m_grip_left);
    evm.remove(m_window);
    evm.remove(m_clientarea);
}

void FbWinFrame::exposeEvent(XExposeEvent &event) {
    if (m_titlebar == event.window) {
        m_titlebar.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_tab_container == event.window) {
        m_tab_container.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_label == event.window) {
        m_label.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_handle == event.window) {
        m_handle.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_grip_left == event.window) {
        m_grip_left.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_grip_right == event.window) {
        m_grip_right.clearArea(event.x, event.y, event.width, event.height);
    } else {

        if (m_tab_container.tryExposeEvent(event))
            return;

        // create compare function
        // that we should use with find_if
        FbTk::CompareEqual_base<FbTk::FbWindow, Window> compare(&FbTk::FbWindow::window,
                                                                event.window);

        ButtonList::iterator it = find_if(m_buttons_left.begin(),
                                          m_buttons_left.end(),
                                          compare);
        if (it != m_buttons_left.end()) {
            (*it)->exposeEvent(event);
            return;
        }

        it = find_if(m_buttons_right.begin(),
                     m_buttons_right.end(),
                     compare);

        if (it != m_buttons_right.end())
            (*it)->exposeEvent(event);
    }

}

void FbWinFrame::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify && event.xconfigure.window == window().window())
        configureNotifyEvent(event.xconfigure);
}

void FbWinFrame::configureNotifyEvent(XConfigureEvent &event) {
    resize(event.width, event.height);
}

void FbWinFrame::reconfigure() {
    if (m_tab_container.empty())
        return;

    int grav_x=0, grav_y=0;
    // negate gravity
    gravityTranslate(grav_x, grav_y, -m_active_gravity, m_active_orig_client_bw, false);

    m_bevel = theme().bevelWidth();
    // reconfigure can't set borderwidth, as it doesn't know
    // if it's meant to be borderless or not

    unsigned int orig_handle_h = handle().height();
    if (m_use_handle && orig_handle_h != theme().handleWidth())
        m_window.resize(m_window.width(), m_window.height() -
                        orig_handle_h + theme().handleWidth());

    handle().resize(handle().width(),
                    theme().handleWidth());
    gripLeft().resize(buttonHeight(),
                      theme().handleWidth());
    gripRight().resize(gripLeft().width(),
                       gripLeft().height());

    // align titlebar and render it
    if (m_use_titlebar) {
        reconfigureTitlebar();
        m_titlebar.raise();
    } else
        m_titlebar.lower();

    if (m_tabmode == EXTERNAL) {
        unsigned int neww, newh;
        switch (m_screen.getTabPlacement()) {
        case TOPLEFT:
        case TOPRIGHT:
        case BOTTOMLEFT:
        case BOTTOMRIGHT:
            neww = m_tab_container.width();
            newh = buttonHeight();
            break;
        default:
            neww = buttonHeight();
            newh = m_tab_container.height();
            break;
        }
        m_tab_container.resize(neww, newh);
        alignTabs();
    }

    // leave client+grips alone if we're shaded (it'll get fixed when we unshade)
    if (!m_shaded) {
        int client_top = 0;
        int client_height = m_window.height();
        if (m_use_titlebar) {
            // only one borderwidth as titlebar is really at -borderwidth
            int titlebar_height = m_titlebar.height() + m_titlebar.borderWidth();
            client_top += titlebar_height;
            client_height -= titlebar_height;
        }

        // align handle and grips
        const int grip_height = m_handle.height();
        const int grip_width = 20; //TODO
        const int handle_bw = static_cast<signed>(m_handle.borderWidth());

        int ypos = m_window.height();

        // if the handle isn't on, it's actually below the window
        if (m_use_handle)
            ypos -= grip_height + handle_bw;

        // we do handle settings whether on or not so that if they get toggled
        // then things are ok...
        m_handle.invalidateBackground();
        m_handle.moveResize(-handle_bw, ypos,
                            m_window.width(), grip_height);

        m_grip_left.invalidateBackground();
        m_grip_left.moveResize(-handle_bw, -handle_bw,
                               grip_width, grip_height);

        m_grip_right.invalidateBackground();
        m_grip_right.moveResize(m_handle.width() - grip_width - handle_bw, -handle_bw,
                                grip_width, grip_height);

        if (m_use_handle) {
            m_handle.raise();
            client_height -= m_handle.height() + m_handle.borderWidth();
        } else {
            m_handle.lower();
        }

        m_clientarea.moveResize(0, client_top,
                                m_window.width(), client_height);
    }

    gravityTranslate(grav_x, grav_y, m_active_gravity, m_active_orig_client_bw, false);
    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0)
        move(grav_x + x(), grav_y + y());

    // render the theme
    if (isVisible()) {
        // update transparency settings
        if (FbTk::Transparent::haveRender()) {
            unsigned char alpha =
                getAlpha(m_focused);
            if (FbTk::Transparent::haveComposite()) {
                m_tab_container.setAlpha(255);
                m_window.setOpaque(alpha);
            } else {
                m_tab_container.setAlpha(alpha);
                m_window.setOpaque(255);
            }
        }
        renderAll();
        applyAll();
        clearAll();
    } else {
        m_need_render = true;
    }

    if (m_disable_themeshape)
        m_shape.setPlaces(Shape::NONE);
    else
        m_shape.setPlaces(theme().shapePlace());

    m_shape.setShapeOffsets(0, titlebarHeight());

    // titlebar stuff rendered already by reconftitlebar
}

void FbWinFrame::setUseShape(bool value) {
    m_disable_themeshape = !value;

    if (m_disable_themeshape)
        m_shape.setPlaces(Shape::NONE);
    else
        m_shape.setPlaces(theme().shapePlace());

}

void FbWinFrame::setShapingClient(FbTk::FbWindow *win, bool always_update) {
    m_shape.setShapeSource(win, 0, titlebarHeight(), always_update);
}

unsigned int FbWinFrame::buttonHeight() const {
    return m_titlebar.height() - m_bevel*2;
}

//--------------------- private area

/**
   aligns and redraws title
*/
void FbWinFrame::redrawTitlebar() {
    if (!m_use_titlebar || m_tab_container.empty())
        return;

    if (isVisible()) {
        m_tab_container.clear();
        m_label.clear();
        m_titlebar.clear();
    }
}

/**
   Align buttons with title text window
*/
void FbWinFrame::reconfigureTitlebar() {
    if (!m_use_titlebar)
        return;

    int orig_height = m_titlebar.height();
    // resize titlebar to window size with font height
    int title_height = m_theme.font().height() == 0 ? 16 :
        m_theme.font().height() + m_bevel*2 + 2;
    if (m_theme.titleHeight() != 0)
        title_height = m_theme.titleHeight();

    // if the titlebar grows in size, make sure the whole window does too
    if (orig_height != title_height)
        m_window.resize(m_window.width(), m_window.height()-orig_height+title_height);
    m_titlebar.invalidateBackground();
    m_titlebar.moveResize(-m_titlebar.borderWidth(), -m_titlebar.borderWidth(),
                          m_window.width(), title_height);

    // draw left buttons first
    unsigned int next_x = m_bevel;
    unsigned int button_size = buttonHeight();
    m_button_size = button_size;
    for (size_t i=0; i < m_buttons_left.size(); i++, next_x += button_size + m_bevel) {
        // probably on theme reconfigure, leave bg alone for now
        m_buttons_left[i]->invalidateBackground();
        m_buttons_left[i]->moveResize(next_x, m_bevel,
                                      button_size, button_size);
    }

    next_x += m_bevel;

    // space left on titlebar between left and right buttons
    int space_left = m_titlebar.width() - next_x;

    if (!m_buttons_right.empty())
        space_left -= m_buttons_right.size() * (button_size + m_bevel);

    space_left -= m_bevel;

    if (space_left <= 0)
        space_left = 1;

    m_label.invalidateBackground();
    m_label.moveResize(next_x, m_bevel, space_left, button_size);

    m_tab_container.invalidateBackground();
    if (m_tabmode == INTERNAL)
        m_tab_container.moveResize(next_x, m_bevel,
                                   space_left, button_size);
    else {
        if (m_use_tabs) {
            if (m_tab_container.orientation() == FbTk::ROT0) {
                m_tab_container.resize(m_tab_container.width(), button_size);
            } else {
                m_tab_container.resize(button_size, m_tab_container.height());
            }
        }
    }

    next_x += m_label.width() + m_bevel;

    // finaly set new buttons to the right
    for (size_t i=0; i < m_buttons_right.size();
         ++i, next_x += button_size + m_bevel) {
        m_buttons_right[i]->invalidateBackground();
        m_buttons_right[i]->moveResize(next_x, m_bevel,
                                       button_size, button_size);
    }

    m_titlebar.raise(); // always on top
}

void FbWinFrame::renderAll() {
    m_need_render = false;

    renderTitlebar();
    renderHandles();
    renderTabContainer();
}

void FbWinFrame::applyAll() {
    applyTitlebar();
    applyHandles();
    applyTabContainer();
}

void FbWinFrame::renderTitlebar() {
    if (!m_use_titlebar)
        return;

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    // render pixmaps
    render(m_theme.titleFocusTexture(), m_title_focused_color,
           m_title_focused_pm,
           m_titlebar.width(), m_titlebar.height());

    render(m_theme.titleUnfocusTexture(), m_title_unfocused_color,
           m_title_unfocused_pm,
           m_titlebar.width(), m_titlebar.height());

    //!! TODO: don't render label if internal tabs

    render(m_theme.iconbarTheme().focusedTexture(), m_label_focused_color,
           m_label_focused_pm,
           m_label.width(), m_label.height());

    render(m_theme.iconbarTheme().unfocusedTexture(), m_label_unfocused_color,
           m_label_unfocused_pm,
           m_label.width(), m_label.height());

}

void FbWinFrame::renderTabContainer() {
    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    const FbTk::Texture *tc_focused = &m_theme.iconbarTheme().focusedTexture();
    const FbTk::Texture *tc_unfocused = &m_theme.iconbarTheme().unfocusedTexture();

    if (m_tabmode == EXTERNAL && tc_focused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_focused = &m_theme.titleFocusTexture();
    if (m_tabmode == EXTERNAL && tc_unfocused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_unfocused = &m_theme.titleUnfocusTexture();

    render(*tc_focused, m_tabcontainer_focused_color,
           m_tabcontainer_focused_pm,
           m_tab_container.width(), m_tab_container.height(), m_tab_container.orientation());

    render(*tc_unfocused, m_tabcontainer_unfocused_color,
           m_tabcontainer_unfocused_pm,
           m_tab_container.width(), m_tab_container.height(), m_tab_container.orientation());

    renderButtons();

}

void FbWinFrame::applyTitlebar() {

    // set up pixmaps for titlebar windows
    Pixmap label_pm = None;
    Pixmap title_pm = None;
    FbTk::Color label_color;
    FbTk::Color title_color;
    getCurrentFocusPixmap(label_pm, title_pm,
                          label_color, title_color);

    unsigned char alpha = getAlpha (m_focused);
    m_titlebar.setAlpha(alpha);
    m_label.setAlpha(alpha);

    if (m_tabmode != INTERNAL) {
        m_label.setGC(m_focused ?
                      theme().iconbarTheme().focusedText().textGC() :
                      theme().iconbarTheme().unfocusedText().textGC());
        m_label.setJustify(m_focused ?
                           theme().iconbarTheme().focusedText().justify() :
                           theme().iconbarTheme().unfocusedText().justify());

        if (label_pm != 0)
            m_label.setBackgroundPixmap(label_pm);
        else
            m_label.setBackgroundColor(label_color);
    }

    if (title_pm != 0)
        m_titlebar.setBackgroundPixmap(title_pm);
    else
        m_titlebar.setBackgroundColor(title_color);

    applyButtons();
}


void FbWinFrame::renderHandles() {
    if (!m_use_handle)
        return;

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    render(m_theme.handleFocusTexture(), m_handle_focused_color,
           m_handle_focused_pm,
           m_handle.width(), m_handle.height());

    render(m_theme.handleUnfocusTexture(), m_handle_unfocused_color,
           m_handle_unfocused_pm,
           m_handle.width(), m_handle.height());

    render(m_theme.gripFocusTexture(), m_grip_focused_color, m_grip_focused_pm,
           m_grip_left.width(), m_grip_left.height());

    render(m_theme.gripUnfocusTexture(), m_grip_unfocused_color,
           m_grip_unfocused_pm,
           m_grip_left.width(), m_grip_left.height());

}

void FbWinFrame::applyHandles() {

    unsigned char alpha = getAlpha (m_focused);
    m_handle.setAlpha(alpha);
    m_grip_left.setAlpha(alpha);
    m_grip_right.setAlpha(alpha);

    if (m_focused) {

        if (m_handle_focused_pm) {
            m_handle.setBackgroundPixmap(m_handle_focused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_focused_color);
        }

        if (m_grip_focused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_focused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_focused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_focused_color);
            m_grip_right.setBackgroundColor(m_grip_focused_color);
        }

    } else {

        if (m_handle_unfocused_pm) {
            m_handle.setBackgroundPixmap(m_handle_unfocused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_unfocused_color);
        }

        if (m_grip_unfocused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_unfocused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_unfocused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_unfocused_color);
            m_grip_right.setBackgroundColor(m_grip_unfocused_color);
        }
    }

}

void FbWinFrame::renderButtons() {

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    render(m_theme.buttonFocusTexture(), m_button_color,
           m_button_pm,
           m_button_size, m_button_size);

    render(m_theme.buttonUnfocusTexture(), m_button_unfocused_color,
           m_button_unfocused_pm,
           m_button_size, m_button_size);

    render(m_theme.buttonPressedTexture(), m_button_pressed_color,
           m_button_pressed_pm,
           m_button_size, m_button_size);
}

void FbWinFrame::applyButtons() {
    // setup left and right buttons
    for (size_t i=0; i < m_buttons_left.size(); ++i)
        applyButton(*m_buttons_left[i]);

    for (size_t i=0; i < m_buttons_right.size(); ++i)
        applyButton(*m_buttons_right[i]);
}

void FbWinFrame::init() {

    if (theme().handleWidth() == 0)
        m_use_handle = false;

    m_disable_themeshape = false;

    m_handle.showSubwindows();

    // clear pixmaps
    m_title_focused_pm = m_title_unfocused_pm = 0;
    m_label_focused_pm = m_label_unfocused_pm = 0;
    m_tabcontainer_focused_pm = m_tabcontainer_unfocused_pm = 0;
    m_handle_focused_pm = m_handle_unfocused_pm = 0;
    m_button_pm = m_button_unfocused_pm = m_button_pressed_pm = 0;
    m_grip_unfocused_pm = m_grip_focused_pm = 0;

    m_button_size = 26;

    m_clientarea.setBorderWidth(0);
    m_label.setBorderWidth(0);
    m_shaded = false;

    setTabMode(NOTSET);

    m_label.setEventMask(ExposureMask | ButtonPressMask |
                         ButtonReleaseMask | ButtonMotionMask |
                         EnterWindowMask);

    showHandle();
    showTitlebar();

    // Note: we don't show clientarea yet

    setEventHandler(*this);
}

/**
   Setups upp background, pressed pixmap/color of the button to current theme
*/
void FbWinFrame::applyButton(FbTk::Button &btn) {
    if (m_button_pressed_pm)
        btn.setPressedPixmap(m_button_pressed_pm);
    else
        btn.setPressedColor(m_button_pressed_color);

    if (focused()) { // focused
        btn.setAlpha(getAlpha(true));

        btn.setGC(m_theme.buttonPicFocusGC());
        if (m_button_pm)
            btn.setBackgroundPixmap(m_button_pm);
        else
            btn.setBackgroundColor(m_button_color);
    } else { // unfocused
        btn.setAlpha(getAlpha(false));

        btn.setGC(m_theme.buttonPicUnfocusGC());
        if (m_button_unfocused_pm)
            btn.setBackgroundPixmap(m_button_unfocused_pm);
        else
            btn.setBackgroundColor(m_button_unfocused_color);
    }

}

void FbWinFrame::render(const FbTk::Texture &tex, FbTk::Color &col, Pixmap &pm,
                        unsigned int w, unsigned int h, FbTk::Orientation orient) {

    Pixmap tmp = pm;
    if (!tex.usePixmap()) {
        pm = None;
        col = tex.color();
    } else {
        pm = m_imagectrl.renderImage(w, h, tex, orient);
    }

    if (tmp)
        m_imagectrl.removeImage(tmp);

}

void FbWinFrame::getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                                       FbTk::Color &label_color, FbTk::Color &title_color) {
    if (m_focused) {
        if (m_label_focused_pm != 0)
            label_pm = m_label_focused_pm;
        else
            label_color = m_label_focused_color;

        if (m_title_focused_pm != 0)
            title_pm = m_title_focused_pm;
        else
            title_color = m_title_focused_color;
    } else {
        if (m_label_unfocused_pm != 0)
            label_pm = m_label_unfocused_pm;
        else
            label_color = m_label_unfocused_color;

        if (m_title_unfocused_pm != 0)
            title_pm = m_title_unfocused_pm;
        else
            title_color = m_title_unfocused_color;
    }
}

void FbWinFrame::applyTabContainer() {
    m_tab_container.setAlpha(getAlpha(m_focused));

    // do the parent container
    Pixmap tabcontainer_pm = None;
    FbTk::Color *tabcontainer_color = NULL;
    if (m_focused) {
        if (m_tabcontainer_focused_pm != 0)
            tabcontainer_pm = m_tabcontainer_focused_pm;
        else
            tabcontainer_color = &m_tabcontainer_focused_color;
    } else {
        if (m_tabcontainer_unfocused_pm != 0)
            tabcontainer_pm = m_tabcontainer_unfocused_pm;
        else
            tabcontainer_color = &m_tabcontainer_unfocused_color;
    }

    if (tabcontainer_pm != 0)
        m_tab_container.setBackgroundPixmap(tabcontainer_pm);
    else
        m_tab_container.setBackgroundColor(*tabcontainer_color);

    // and the labelbuttons in it
    Container::ItemList::iterator btn_it = m_tab_container.begin();
    Container::ItemList::iterator btn_it_end = m_tab_container.end();
    for (; btn_it != btn_it_end; ++btn_it) {
        IconButton *btn = static_cast<IconButton *>(*btn_it);
        btn->reconfigTheme();
    }
}

void FbWinFrame::setBorderWidth(unsigned int border_width) {
    int bw_changes = 0;

    int grav_x=0, grav_y=0;
    // negate gravity
    gravityTranslate(grav_x, grav_y, -m_active_gravity, m_active_orig_client_bw, false);


    // we need to change the size of the window
    // if the border width changes...
    if (m_use_titlebar)
        bw_changes += static_cast<signed>(border_width - titlebar().borderWidth());
    if (m_use_handle)
        bw_changes += static_cast<signed>(border_width - handle().borderWidth());

    window().setBorderWidth(border_width);
    window().setBorderColor(theme().border().color());

    setTabMode(NOTSET);

    titlebar().setBorderWidth(border_width);
    titlebar().setBorderColor(theme().border().color());

    handle().setBorderWidth(border_width);
    handle().setBorderColor(theme().border().color());

    gripLeft().setBorderWidth(border_width);
    gripLeft().setBorderColor(theme().border().color());

    gripRight().setBorderWidth(border_width);
    gripRight().setBorderColor(theme().border().color());

    if (bw_changes != 0)
        resize(width(), height() + bw_changes);

    if (m_tabmode == EXTERNAL)
        alignTabs();

    gravityTranslate(grav_x, grav_y, m_active_gravity, m_active_orig_client_bw, false);
    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0)
        move(grav_x + x(), grav_y + y());

}

// this function translates its arguments according to win_gravity
// if win_gravity is negative, it does an inverse translation
// This function should be used when a window is mapped/unmapped/pos configured
void FbWinFrame::gravityTranslate(int &x, int &y,
                                  int win_gravity, unsigned int client_bw, bool move_frame) {
    bool invert = false;
    if (win_gravity < 0) {
        invert = true;
        win_gravity = -win_gravity; // make +ve
    }

    /* Ok, so, gravity says which point of the frame is put where the
     * corresponding bit of window would have been
     * Thus, x,y always refers to where top left of the WINDOW would be placed
     * but given that we're wrapping it in a frame, we actually place
     * it so that the given reference point is in the same spot as the
     * window's reference point would have been.
     * i.e. east gravity says that the centre of the right hand side of the
     * frame is placed where the centre of the rhs of the window would
     * have been if there was no frame.
     * Hope that makes enough sense.
     *
     * NOTE: the gravity calculations are INDEPENDENT of the client
     *       window width/height.
     *
     * If you get confused with the calculations, draw a picture.
     *
     */

    // We calculate offsets based on the gravity and frame aspects
    // and at the end apply those offsets +ve or -ve depending on 'invert'

    // These will be set to the resulting offsets for adjusting the frame position
    int x_offset = 0;
    int y_offset = 0;

    // These are the amount that the frame is larger than the client window
    // Note that the client window's x,y is offset by it's borderWidth, which
    // is removed by fluxbox, so the gravity needs to account for this change

//    unsigned int width_offset = 0; // no side decorations

    // these functions already check if the title/handle is used
    int height_offset = - titlebarHeight() - handleHeight();

    int bw_diff = client_bw - m_window.borderWidth();

    // mostly no X offset, since we don't have extra frame on the sides
    switch (win_gravity) {
    case NorthEastGravity:
        x_offset += bw_diff;
    case NorthGravity:
        x_offset += bw_diff;
    case NorthWestGravity:
        // no offset, since the top point is still the same
        break;
    case SouthEastGravity:
        x_offset += bw_diff;
    case SouthGravity:
        x_offset += bw_diff;
    case SouthWestGravity:
        // window shifted down by height of titlebar, and the handle
        // since that's necessary to get the bottom of the frame
        // all the way up
        y_offset += 2*bw_diff + height_offset;
        break;
    case EastGravity:
        x_offset += bw_diff;
    case CenterGravity:
        x_offset += bw_diff;
    case WestGravity:
        // these centered ones are a little more interesting
        y_offset += bw_diff + height_offset/2;
        break;
    case StaticGravity:
        x_offset += bw_diff;
        y_offset += -titlebarHeight() + bw_diff;
        break;
    }

    if (invert) {
        x_offset = -x_offset;
        y_offset = -y_offset;
    }

    x += x_offset;
    y += y_offset;

    if (move_frame && (x_offset != 0 || y_offset != 0)) {
        move(x, y);
    }
}

unsigned int FbWinFrame::normalHeight() const {
    if (m_shaded)
        return m_height_before_shade;
    return height();
}

int FbWinFrame::widthOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    // same height offset for top and bottom tabs
    switch (m_screen.getTabPlacement()) {
    case LEFTTOP:
    case RIGHTTOP:
    case LEFTBOTTOM:
    case RIGHTBOTTOM:
        return m_tab_container.width() + m_window.borderWidth();
        break;
    default: // kill warning
        break;
    }
    return 0;
}

int FbWinFrame::heightOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    switch (m_screen.getTabPlacement()) {
    case TOPLEFT:
    case TOPRIGHT:
    case BOTTOMLEFT:
    case BOTTOMRIGHT:
        return m_tab_container.height() + m_window.borderWidth();
        break;
    default: // kill warning
        break;
    }
    return 0;
}

int FbWinFrame::xOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    switch (m_screen.getTabPlacement()) {
    case LEFTTOP:
    case LEFTBOTTOM:
        return m_tab_container.width() + m_window.borderWidth();
        break;
    default: // kill warning
        break;
    }
    return 0;
}

int FbWinFrame::yOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    switch (m_screen.getTabPlacement()) {
    case TOPLEFT:
    case TOPRIGHT:
        return m_tab_container.height() + m_window.borderWidth();
        break;
    default: // kill warning
        break;
    }
    return 0;
}

