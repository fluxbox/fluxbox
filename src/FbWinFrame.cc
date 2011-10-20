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

#include "FbWinFrame.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/App.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Compose.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/CompareEqual.hh"
#include "FbTk/TextUtils.hh"
#include "FbTk/STLUtil.hh"

#include "FbWinFrameTheme.hh"
#include "Screen.hh"
#include "FocusableTheme.hh"
#include "IconButton.hh"

#include <algorithm>
#include <X11/X.h>

using std::max;
using std::mem_fun;
using std::string;

using FbTk::STLUtil::forAll;

FbWinFrame::FbWinFrame(BScreen &screen, unsigned int client_depth,
                       WindowState &state,
                       FocusableTheme<FbWinFrameTheme> &theme):
    m_screen(screen),
    m_theme(theme),
    m_imagectrl(screen.imageControl()),
    m_state(state),
    m_window(theme->screenNum(), state.x, state.y, state.width, state.height,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | EnterWindowMask |
             LeaveWindowMask, true, false,
             client_depth, InputOutput,
             ((client_depth == 32) && (screen.rootWindow().depth() == 32) ? screen.rootWindow().visual() : CopyFromParent),
             ((client_depth == 32) && (screen.rootWindow().depth() == 32) ? screen.rootWindow().colormap() : CopyFromParent)),
    m_layeritem(window(), *screen.layerManager().getLayer(ResourceLayer::NORMAL)),
    m_titlebar(m_window, 0, 0, 100, 16,
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask,
               false, false, screen.rootWindow().decorationDepth(), InputOutput,
               screen.rootWindow().decorationVisual(),
               screen.rootWindow().decorationColormap()),
    m_tab_container(m_titlebar),
    m_label(m_titlebar, m_theme->font(), FbTk::BiDiString("")),
    m_handle(m_window, 0, 0, 100, 5,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | ExposureMask |
             EnterWindowMask | LeaveWindowMask,
             false, false, screen.rootWindow().decorationDepth(), InputOutput,
             screen.rootWindow().decorationVisual(),
             screen.rootWindow().decorationColormap()),
    m_grip_right(m_handle, 0, 0, 10, 4,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask,
                 false, false, screen.rootWindow().decorationDepth(), InputOutput,
                 screen.rootWindow().decorationVisual(),
                 screen.rootWindow().decorationColormap()),
    m_grip_left(m_handle, 0, 0, 10, 4,
                ButtonPressMask | ButtonReleaseMask |
                ButtonMotionMask | ExposureMask |
                EnterWindowMask | LeaveWindowMask,
                false, false, screen.rootWindow().decorationDepth(), InputOutput,
                screen.rootWindow().decorationVisual(),
                screen.rootWindow().decorationColormap()),
    m_clientarea(m_window, 0, 0, 100, 100,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_bevel(1),
    m_use_titlebar(true),
    m_use_tabs(true),
    m_use_handle(true),
    m_visible(false),
    m_button_pm(0),
    m_tabmode(screen.getDefaultInternalTabs()?INTERNAL:EXTERNAL),
    m_active_orig_client_bw(0),
    m_need_render(true),
    m_button_size(1),
    m_focused_alpha(AlphaAcc(*theme.focusedTheme(), &FbWinFrameTheme::alpha)),
    m_unfocused_alpha(AlphaAcc(*theme.unfocusedTheme(), &FbWinFrameTheme::alpha)),
    m_shape(m_window, theme->shapePlace()) {
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
        m_tab_container.setEventMask(
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask);

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

        m_tab_container.setAlignment(FbTk::Container::RELATIVE);
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

    return ret;
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

    if (resize && width == FbWinFrame::width() &&
                  height == FbWinFrame::height())
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

    m_state.saveGeometry(window().x(), window().y(),
                         window().width(), window().height());

    if (move || (resize && m_screen.getTabPlacement() != TOPLEFT &&
                           m_screen.getTabPlacement() != LEFTTOP))
        alignTabs();

    if (resize) {
        if (m_tabmode == EXTERNAL) {
            switch(m_screen.getTabPlacement()) {
            case LEFTTOP:
            case RIGHTTOP:
            case LEFT:
            case RIGHT:
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
    m_state.saveGeometry(window().x(), window().y(),
                         window().width(), window().height());
    if (m_tabmode == EXTERNAL) {

        switch(m_screen.getTabPlacement()) {
        case LEFTTOP:
        case RIGHTTOP:
        case LEFT:
        case RIGHT:
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
        m_tab_container.setAlignment(FbTk::Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x();
        taby = y() - yOffset();
        break;
    case TOP:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(FbTk::Container::CENTER);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x() + (width() - m_tab_container.width())/2;
        taby = y() - yOffset();
        break;
    case TOPRIGHT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(FbTk::Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x() + width() - m_tab_container.width();
        taby = y() - yOffset();
        break;
    case LEFTTOP:
        if (orig_orient != FbTk::ROT270) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT270);
        m_tab_container.setAlignment(FbTk::Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() - xOffset();
        taby = y();
        break;
    case LEFT:
        if (orig_orient != FbTk::ROT270) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT270);
        m_tab_container.setAlignment(FbTk::Container::CENTER);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() - xOffset();
        taby = y() + (height() - m_tab_container.height())/2;
        break;
    case LEFTBOTTOM:
        if (orig_orient != FbTk::ROT270) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT270);
        m_tab_container.setAlignment(FbTk::Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() - xOffset();
        taby = y() + height() - m_tab_container.height();
        break;
    case RIGHTTOP:
        if (orig_orient != FbTk::ROT90) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT90);
        m_tab_container.setAlignment(FbTk::Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() + width() + m_window.borderWidth();
        taby = y();
        break;
    case RIGHT:
        if (orig_orient != FbTk::ROT90) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT90);
        m_tab_container.setAlignment(FbTk::Container::CENTER);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() + width() + m_window.borderWidth();
        taby = y() + (height() - m_tab_container.height())/2;
        break;
    case RIGHTBOTTOM:
        if (orig_orient != FbTk::ROT90) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT90);
        m_tab_container.setAlignment(FbTk::Container::RIGHT);
        m_tab_container.setMaxTotalSize(m_window.height());
        tabx = x() + width() + m_window.borderWidth();
        taby = y() + height() - m_tab_container.height();
        break;
    case BOTTOMLEFT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(FbTk::Container::LEFT);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x();
        taby = y() + height() + m_window.borderWidth();
        break;
    case BOTTOM:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(FbTk::Container::CENTER);
        m_tab_container.setMaxTotalSize(m_window.width());
        tabx = x() + (width() - m_tab_container.width())/2;
        taby = y() + height() + m_window.borderWidth();
        break;
    case BOTTOMRIGHT:
        if (orig_orient != FbTk::ROT0) m_tab_container.hide();
        m_tab_container.setOrientation(FbTk::ROT0);
        m_tab_container.setAlignment(FbTk::Container::RIGHT);
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
    int alpha = getAlpha(m_state.focused);
    if (alpha == 255)
        return;

    if ((m_tabmode == EXTERNAL && m_use_tabs) || m_use_titlebar) {
        m_tab_container.parentMoved();
        m_tab_container.for_each(mem_fun(&FbTk::Button::parentMoved));
    }

    if (m_use_titlebar) {
        if (m_tabmode != INTERNAL)
            m_label.parentMoved();

        m_titlebar.parentMoved();

        forAll(m_buttons_left, mem_fun(&FbTk::Button::parentMoved));
        forAll(m_buttons_right, mem_fun(&FbTk::Button::parentMoved));
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
        forAll(m_buttons_left, mem_fun(&FbTk::Button::clear));
        forAll(m_buttons_right, mem_fun(&FbTk::Button::clear));
    } else if (m_tabmode == EXTERNAL && m_use_tabs)
        m_tab_container.clear();

    if (m_use_handle) {
        m_handle.clear();
        m_grip_left.clear();
        m_grip_right.clear();
    }
}

void FbWinFrame::setFocus(bool newvalue) {
    if (m_state.focused == newvalue)
        return;

    m_state.focused = newvalue;

    if (FbTk::Transparent::haveRender() && 
        getAlpha(true) != getAlpha(false)) { // different alpha for focused and unfocused
        int alpha = getAlpha(m_state.focused);
        if (FbTk::Transparent::haveComposite()) {
            m_tab_container.setAlpha(255);
            m_window.setOpaque(alpha);
        } else {
            m_tab_container.setAlpha(alpha);
            m_window.setOpaque(255);
        }
    }

    setBorderWidth();

    applyAll();
    clearAll();
}

void FbWinFrame::applyState() {
    applyDecorations(false);

    const int head = m_screen.getHead(window());
    int new_x = m_state.x, new_y = m_state.y;
    unsigned int new_w = m_state.width, new_h = m_state.height;

    if (m_state.isMaximizedVert()) {
        new_y = m_screen.maxTop(head);
        new_h = m_screen.maxBottom(head) - new_y - 2*window().borderWidth();
        if (!m_screen.getMaxOverTabs()) {
            new_y += yOffset();
            new_h -= heightOffset();
        }
    }
    if (m_state.isMaximizedHorz()) {
        new_x = m_screen.maxLeft(head);
        new_w = m_screen.maxRight(head) - new_x - 2*window().borderWidth();
        if (!m_screen.getMaxOverTabs()) {
            new_x += xOffset();
            new_w -= widthOffset();
        }
    }

    if (m_state.shaded)
        new_h = m_titlebar.height();

    if (m_state.fullscreen) {
        new_x = m_screen.getHeadX(head);
        new_y = m_screen.getHeadY(head);
        new_w = m_screen.getHeadWidth(head);
        new_h = m_screen.getHeadHeight(head);
    }

    moveResize(new_x, new_y, new_w, new_h);
    frameExtentSig().emit();
}

void FbWinFrame::setAlpha(bool focused, int alpha) {
    if (focused)
        m_focused_alpha = alpha;
    else
        m_unfocused_alpha = alpha;

    if (m_state.focused == focused)
        applyAlpha();
}

void FbWinFrame::applyAlpha() {
    int alpha = getAlpha(m_state.focused);
    if (FbTk::Transparent::haveComposite())
        m_window.setOpaque(alpha);
    else {
        // don't need to setAlpha, since apply updates them anyway
        applyAll();
        clearAll();
    }
}

int FbWinFrame::getAlpha(bool focused) const {
    if (focused)
        return m_focused_alpha;
    else
        return m_unfocused_alpha;
}

void FbWinFrame::setDefaultAlpha() {
    if (getUseDefaultAlpha())
        return;
    m_focused_alpha.restoreDefault();
    m_unfocused_alpha.restoreDefault();
    applyAlpha();
}

bool FbWinFrame::getUseDefaultAlpha() const {
    return m_focused_alpha.isDefault() && m_unfocused_alpha.isDefault();
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

    FbTk::STLUtil::destroyAndClear(m_buttons_left);
    FbTk::STLUtil::destroyAndClear(m_buttons_right);
}

void FbWinFrame::createTab(FbTk::Button &button) {
    button.show();
    button.setEventMask(ExposureMask | ButtonPressMask |
                        ButtonReleaseMask | ButtonMotionMask |
                        EnterWindowMask);
    FbTk::EventManager::instance()->add(button, button.window());

    m_tab_container.insertItem(&button);
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

void FbWinFrame::setClientWindow(FbTk::FbWindow &win) {

    win.setBorderWidth(0);

    XChangeSaveSet(win.display(), win.window(), SetModeInsert);

    m_window.setEventMask(NoEventMask);

    // we need to mask this so we don't get unmap event
    win.setEventMask(NoEventMask);
    win.reparent(m_window, clientArea().x(), clientArea().y());

    m_window.setEventMask(ButtonPressMask | ButtonReleaseMask |
                          ButtonMotionMask | EnterWindowMask |
                          LeaveWindowMask | SubstructureRedirectMask);

    XFlush(win.display());

    // remask window so we get events
    XSetWindowAttributes attrib_set;
    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask | KeyPressMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;

    XChangeWindowAttributes(win.display(), win.window(), CWEventMask|CWDontPropagate, &attrib_set);

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
    if (m_use_handle || theme()->handleWidth() == 0)
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
    gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity, m_active_orig_client_bw, false);

    m_bevel = theme()->bevelWidth();

    unsigned int orig_handle_h = handle().height();
    if (m_use_handle && orig_handle_h != theme()->handleWidth())
        m_window.resize(m_window.width(), m_window.height() -
                        orig_handle_h + theme()->handleWidth());

    handle().resize(handle().width(),
                    theme()->handleWidth());
    gripLeft().resize(buttonHeight(),
                      theme()->handleWidth());
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
        case TOP:
        case TOPRIGHT:
        case BOTTOMLEFT:
        case BOTTOM:
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
    if (!m_state.shaded || m_state.fullscreen) {
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

    gravityTranslate(grav_x, grav_y, sizeHints().win_gravity, m_active_orig_client_bw, false);
    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0)
        move(grav_x + x(), grav_y + y());

    // render the theme
    if (isVisible()) {
        // update transparency settings
        if (FbTk::Transparent::haveRender()) {
            int alpha = getAlpha(m_state.focused);
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

    m_shape.setPlaces(getShape());
    m_shape.setShapeOffsets(0, titlebarHeight());

    // titlebar stuff rendered already by reconftitlebar
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
    int title_height = theme()->font().height() == 0 ? 16 :
        theme()->font().height() + m_bevel*2 + 2;
    if (theme()->titleHeight() != 0)
        title_height = theme()->titleHeight();

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
    render(theme().focusedTheme()->titleTexture(), m_title_focused_color,
           m_title_focused_pm,
           m_titlebar.width(), m_titlebar.height());

    render(theme().unfocusedTheme()->titleTexture(), m_title_unfocused_color,
           m_title_unfocused_pm,
           m_titlebar.width(), m_titlebar.height());

    //!! TODO: don't render label if internal tabs

    render(theme().focusedTheme()->iconbarTheme()->texture(),
           m_label_focused_color, m_label_focused_pm,
           m_label.width(), m_label.height());

    render(theme().unfocusedTheme()->iconbarTheme()->texture(),
           m_label_unfocused_color, m_label_unfocused_pm,
           m_label.width(), m_label.height());

}

void FbWinFrame::renderTabContainer() {
    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    const FbTk::Texture *tc_focused = &theme().focusedTheme()->iconbarTheme()->texture();
    const FbTk::Texture *tc_unfocused = &theme().unfocusedTheme()->iconbarTheme()->texture();

    if (m_tabmode == EXTERNAL && tc_focused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_focused = &theme().focusedTheme()->titleTexture();
    if (m_tabmode == EXTERNAL && tc_unfocused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_unfocused = &theme().unfocusedTheme()->titleTexture();

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

    int alpha = getAlpha (m_state.focused);
    m_titlebar.setAlpha(alpha);
    m_label.setAlpha(alpha);

    if (m_tabmode != INTERNAL) {
        m_label.setGC(theme()->iconbarTheme()->text().textGC());
        m_label.setJustify(theme()->iconbarTheme()->text().justify());

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

    render(theme().focusedTheme()->handleTexture(), m_handle_focused_color,
           m_handle_focused_pm,
           m_handle.width(), m_handle.height());

    render(theme().unfocusedTheme()->handleTexture(), m_handle_unfocused_color,
           m_handle_unfocused_pm,
           m_handle.width(), m_handle.height());

    render(theme().focusedTheme()->gripTexture(), m_grip_focused_color,
           m_grip_focused_pm,
           m_grip_left.width(), m_grip_left.height());

    render(theme().unfocusedTheme()->gripTexture(), m_grip_unfocused_color,
           m_grip_unfocused_pm,
           m_grip_left.width(), m_grip_left.height());

}

void FbWinFrame::applyHandles() {

    int alpha = getAlpha(m_state.focused);
    m_handle.setAlpha(alpha);
    m_grip_left.setAlpha(alpha);
    m_grip_right.setAlpha(alpha);

    if (m_state.focused) {

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

    render(theme().focusedTheme()->buttonTexture(), m_button_color,
           m_button_pm,
           m_button_size, m_button_size);

    render(theme().unfocusedTheme()->buttonTexture(), m_button_unfocused_color,
           m_button_unfocused_pm,
           m_button_size, m_button_size);

    render(theme()->buttonPressedTexture(), m_button_pressed_color,
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

    if (theme()->handleWidth() == 0)
        m_use_handle = false;

    m_handle.showSubwindows();

    // clear pixmaps
    m_title_focused_pm = m_title_unfocused_pm = 0;
    m_label_focused_pm = m_label_unfocused_pm = 0;
    m_tabcontainer_focused_pm = m_tabcontainer_unfocused_pm = 0;
    m_handle_focused_pm = m_handle_unfocused_pm = 0;
    m_button_pm = m_button_unfocused_pm = m_button_pressed_pm = 0;
    m_grip_unfocused_pm = m_grip_focused_pm = 0;

    m_button_size = 26;

    m_label.setBorderWidth(0);

    setTabMode(NOTSET);

    m_label.setEventMask(ExposureMask | ButtonPressMask |
                         ButtonReleaseMask | ButtonMotionMask |
                         EnterWindowMask);

    showHandle();
    showTitlebar();

    // Note: we don't show clientarea yet

    setEventHandler(*this);

    // setup cursors for resize grips
    gripLeft().setCursor(theme()->lowerLeftAngleCursor());
    gripRight().setCursor(theme()->lowerRightAngleCursor());
}

/**
   Setups upp background, pressed pixmap/color of the button to current theme
*/
void FbWinFrame::applyButton(FbTk::Button &btn) {
    if (m_button_pressed_pm)
        btn.setPressedPixmap(m_button_pressed_pm);
    else
        btn.setPressedColor(m_button_pressed_color);

    Pixmap pm = m_state.focused ? m_button_pm : m_button_unfocused_pm;
    btn.setAlpha(getAlpha(m_state.focused));
    btn.setGC(theme()->buttonPicGC());
    if (pm)
        btn.setBackgroundPixmap(pm);
    else
        btn.setBackgroundColor(m_state.focused ? m_button_color
                                         : m_button_unfocused_color);
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
    if (m_state.focused) {
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
    m_tab_container.setAlpha(getAlpha(m_state.focused));

    // do the parent container
    Pixmap tabcontainer_pm = None;
    FbTk::Color *tabcontainer_color = NULL;
    if (m_state.focused) {
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
    FbTk::Container::ItemList::iterator btn_it = m_tab_container.begin();
    FbTk::Container::ItemList::iterator btn_it_end = m_tab_container.end();
    for (; btn_it != btn_it_end; ++btn_it) {
        IconButton *btn = static_cast<IconButton *>(*btn_it);
        btn->reconfigTheme();
    }
}

int FbWinFrame::getShape() const {
    int shape = theme()->shapePlace();
    if (!m_state.useTitlebar())
        shape &= ~(FbTk::Shape::TOPRIGHT|FbTk::Shape::TOPLEFT);
    if (!m_state.useHandle())
        shape &= ~(FbTk::Shape::BOTTOMRIGHT|FbTk::Shape::BOTTOMLEFT);
    return shape;
}

void FbWinFrame::applyDecorations(bool do_move) {
    int grav_x=0, grav_y=0;
    // negate gravity
    gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity, m_active_orig_client_bw,
                     false);

    bool client_move = setBorderWidth(false);

    // tab deocration only affects if we're external
    // must do before the setTabMode in case it goes
    // to external and is meant to be hidden
    if (m_state.useTabs())
        client_move |= showTabs();
    else
        client_move |= hideTabs();

    // we rely on frame not doing anything if it is already shown/hidden
    if (m_state.useTitlebar()) {
        client_move |= showTitlebar();
        if (m_screen.getDefaultInternalTabs())
            client_move |= setTabMode(INTERNAL);
        else
            client_move |= setTabMode(EXTERNAL);
    } else {
        client_move |= hideTitlebar();
        if (m_state.useTabs())
            client_move |= setTabMode(EXTERNAL);
    }

    if (m_state.useHandle())
        client_move |= showHandle();
    else
        client_move |= hideHandle();

    // apply gravity once more
    gravityTranslate(grav_x, grav_y, sizeHints().win_gravity, m_active_orig_client_bw,
                     false);

    // if the location changes, shift it
    if (do_move && (grav_x != 0 || grav_y != 0)) {
        move(grav_x + x(), grav_y + y());
        client_move = true;
    }

    if (do_move) {
        reconfigure();
        m_state.saveGeometry(x(), y(), width(), height());
    }
    if (client_move)
        frameExtentSig().emit();
}

bool FbWinFrame::setBorderWidth(bool do_move) {
    unsigned int border_width = theme()->border().width();
    unsigned int win_bw = m_state.useBorder() ? border_width : 0;

    if (border_width &&
        theme()->border().color().pixel() != window().borderColor()) {
        window().setBorderColor(theme()->border().color());
        titlebar().setBorderColor(theme()->border().color());
        handle().setBorderColor(theme()->border().color());
        gripLeft().setBorderColor(theme()->border().color());
        gripRight().setBorderColor(theme()->border().color());
        tabcontainer().setBorderColor(theme()->border().color());
    }

    if (border_width == handle().borderWidth() &&
        win_bw == window().borderWidth())
        return false;

    int grav_x=0, grav_y=0;
    // negate gravity
    if (do_move)
        gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity,
                         m_active_orig_client_bw, false);

    int bw_changes = 0;
    // we need to change the size of the window
    // if the border width changes...
    if (m_use_titlebar)
        bw_changes += static_cast<signed>(border_width - titlebar().borderWidth());
    if (m_use_handle)
        bw_changes += static_cast<signed>(border_width - handle().borderWidth());

    window().setBorderWidth(win_bw);

    setTabMode(NOTSET);

    titlebar().setBorderWidth(border_width);
    handle().setBorderWidth(border_width);
    gripLeft().setBorderWidth(border_width);
    gripRight().setBorderWidth(border_width);

    if (bw_changes != 0)
        resize(width(), height() + bw_changes);

    if (m_tabmode == EXTERNAL)
        alignTabs();

    if (do_move) {
        frameExtentSig().emit();
        gravityTranslate(grav_x, grav_y, sizeHints().win_gravity,
                         m_active_orig_client_bw, false);
        // if the location changes, shift it
        if (grav_x != 0 || grav_y != 0)
            move(grav_x + x(), grav_y + y());
    }

    return true;
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

    // these functions already check if the title/handle is used
    int bw_diff = client_bw - m_window.borderWidth();
    int height_diff = 2*bw_diff - titlebarHeight() - handleHeight();
    int width_diff = 2*bw_diff;

    if (win_gravity == SouthWestGravity || win_gravity == SouthGravity ||
        win_gravity == SouthEastGravity)
        y_offset = height_diff;

    if (win_gravity == WestGravity || win_gravity == CenterGravity ||
        win_gravity == EastGravity)
        y_offset = height_diff/2;

    if (win_gravity == NorthEastGravity || win_gravity == EastGravity ||
        win_gravity == SouthEastGravity)
        x_offset = width_diff;

    if (win_gravity == NorthGravity || win_gravity == CenterGravity ||
        win_gravity == SouthGravity)
        x_offset = width_diff/2;

    if (win_gravity == StaticGravity) {
        x_offset = bw_diff;
        y_offset = bw_diff - titlebarHeight();
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

int FbWinFrame::widthOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    // same height offset for top and bottom tabs
    switch (m_screen.getTabPlacement()) {
    case LEFTTOP:
    case RIGHTTOP:
    case LEFT:
    case RIGHT:
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
    case TOP:
    case TOPRIGHT:
    case BOTTOMLEFT:
    case BOTTOM:
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
    case LEFT:
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
    case TOP:
    case TOPRIGHT:
        return m_tab_container.height() + m_window.borderWidth();
        break;
    default: // kill warning
        break;
    }
    return 0;
}

void FbWinFrame::applySizeHints(unsigned int &width, unsigned int &height,
                                bool maximizing) const {
    const int h = height - titlebarHeight() - handleHeight();
    height = max(h, static_cast<int>(titlebarHeight() + handleHeight()));
    sizeHints().apply(width, height, maximizing);
    height += titlebarHeight() + handleHeight();
}

void FbWinFrame::displaySize(unsigned int width, unsigned int height) const {
    unsigned int i, j;
    sizeHints().displaySize(i, j,
                            width, height - titlebarHeight() - handleHeight());
    m_screen.showGeometry(i, j);
}

bool FbWinFrame::insideTitlebar(Window win) const {
    return 
        gripLeft().window() != win &&
        gripRight().window() != win &&
        window().window() != win;
}
