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

#include "Keys.hh"
#include "FbWinFrameTheme.hh"
#include "Screen.hh"
#include "FocusableTheme.hh"
#include "IconButton.hh"
#include "RectangleUtil.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/App.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Compose.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/CompareEqual.hh"
#include "FbTk/TextUtils.hh"
#include "FbTk/STLUtil.hh"

#include <X11/X.h>

#include <algorithm>

using std::max;
using std::mem_fun;
using std::string;

using FbTk::STLUtil::forAll;

namespace {

enum { UNFOCUS = 0, FOCUS, PRESSED };

const int s_button_size = 26;
const long s_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | EnterWindowMask | LeaveWindowMask;

const struct {
    FbWinFrame::TabPlacement where;
    FbTk::Orientation orient;
    FbTk::Container::Alignment align;
    bool is_horizontal;
} s_place[] = {
    { /* unused */ },
    { FbWinFrame::TOPLEFT,    FbTk::ROT0,   FbTk::Container::LEFT,   true },
    { FbWinFrame::TOP,        FbTk::ROT0,   FbTk::Container::CENTER, true },
    { FbWinFrame::TOPRIGHT,   FbTk::ROT0,   FbTk::Container::RIGHT,  true },
    { FbWinFrame::BOTTOMLEFT, FbTk::ROT0,   FbTk::Container::LEFT,   true },
    { FbWinFrame::BOTTOM,     FbTk::ROT0,   FbTk::Container::CENTER, true },
    { FbWinFrame::BOTTOMRIGHT,FbTk::ROT0,   FbTk::Container::RIGHT,  true },
    { FbWinFrame::LEFTTOP,    FbTk::ROT270, FbTk::Container::RIGHT,  false },
    { FbWinFrame::LEFT,       FbTk::ROT270, FbTk::Container::CENTER, false },
    { FbWinFrame::LEFTBOTTOM, FbTk::ROT270, FbTk::Container::LEFT,   false },
    { FbWinFrame::RIGHTTOP,   FbTk::ROT90,  FbTk::Container::LEFT,   false },
    { FbWinFrame::RIGHT,      FbTk::ROT90,  FbTk::Container::LEFT,   false },
    { FbWinFrame::RIGHTBOTTOM,FbTk::ROT90,  FbTk::Container::LEFT,   false },
};

/// renders to pixmap or sets color
void render(FbTk::Color &col, Pixmap &pm, unsigned int width, unsigned int height,
            const FbTk::Texture &tex,
            FbTk::ImageControl& ictl,
            FbTk::Orientation orient = FbTk::ROT0) {

    Pixmap tmp = pm;
    if (!tex.usePixmap()) {
        pm = None;
        col = tex.color();
    } else {
        pm = ictl.renderImage(width, height, tex, orient);
    }

    if (tmp)
        ictl.removeImage(tmp);

}

void bg_pm_or_color(FbTk::FbWindow& win, const Pixmap& pm, const FbTk::Color& color) {
    if (pm) {
        win.setBackgroundPixmap(pm);
    } else {
        win.setBackgroundColor(color);
    }
}


} // end anonymous

FbWinFrame::FbWinFrame(BScreen &screen, unsigned int client_depth,
                       WindowState &state,
                       FocusableTheme<FbWinFrameTheme> &theme):
    m_screen(screen),
    m_theme(theme),
    m_imagectrl(screen.imageControl()),
    m_state(state),
    m_window(theme->screenNum(), state.x, state.y, state.width, state.height, s_mask, true, false,
        client_depth, InputOutput,
        (client_depth == screen.rootWindow().maxDepth() ? screen.rootWindow().visual() : CopyFromParent),
        (client_depth == screen.rootWindow().maxDepth() ? screen.rootWindow().colormap() : CopyFromParent)),
    m_layeritem(window(), *screen.layerManager().getLayer(ResourceLayer::NORMAL)),
    m_titlebar(m_window, 0, 0, 100, 16, s_mask, false, false, 
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap()),
    m_tab_container(m_titlebar),
    m_label(m_titlebar, m_theme->font(), FbTk::BiDiString("")),
    m_handle(m_window, 0, 0, 100, 5, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap()),
    m_grip_right(m_handle, 0, 0, 10, 4, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap()),
    m_grip_left(m_handle, 0, 0, 10, 4, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap()),
    m_clientarea(m_window, 0, 0, 100, 100, s_mask),
    m_bevel(1),
    m_use_titlebar(true),
    m_use_tabs(true),
    m_use_handle(true),
    m_visible(false),
    m_tabmode(screen.getDefaultInternalTabs()?INTERNAL:EXTERNAL),
    m_active_orig_client_bw(0),
    m_need_render(true),
    m_button_size(1),
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

    FbTk::Container& tabs = tabcontainer();
    bool ret = true;

    // setting tabmode to notset forces it through when
    // something is likely to change
    if (tabmode == NOTSET)
        tabmode = m_tabmode;

    m_tabmode = tabmode;

    // reparent tab container
    if (tabmode == EXTERNAL) {
        m_label.show();
        tabs.setBorderWidth(m_window.borderWidth());
        tabs.setEventMask(s_mask);
        alignTabs();

        // TODO: tab position
        if (m_use_tabs && m_visible)
            tabs.show();
        else {
            ret = false;
            tabs.hide();
        }

    } else {
        tabs.setUpdateLock(true);

        tabs.setAlignment(FbTk::Container::RELATIVE);
        tabs.setOrientation(FbTk::ROT0);
        if (tabs.parent()->window() == m_screen.rootWindow().window()) {
            m_layeritem.removeWindow(m_tab_container);
            tabs.hide();
            tabs.reparent(m_titlebar, m_label.x(), m_label.y());
            tabs.invalidateBackground();
            tabs.resize(m_label.width(), m_label.height());
            tabs.raise();
        }
        tabs.setBorderWidth(0);
        tabs.setMaxTotalSize(0);
        tabs.setUpdateLock(false);
        tabs.setMaxSizePerClient(0);

        renderTabContainer();
        applyTabContainer();

        tabs.clear();
        tabs.raise();
        tabs.show();

        if (!m_use_tabs)
            ret = false;

        m_label.hide();
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

void FbWinFrame::moveResize(int x, int y, unsigned int width, unsigned int height, bool move, bool resize, bool force) {
    if (!force && move && x == window().x() && y == window().y())
        move = false;

    if (!force && resize && width == FbWinFrame::width() &&
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
            unsigned int s = width;
            if (!s_place[m_screen.getTabPlacement()].is_horizontal) {
                s = height;
            }
            m_tab_container.setMaxTotalSize(s);
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
        unsigned int s = width;
        if (!s_place[m_screen.getTabPlacement()].is_horizontal) {
            s = height;
        }
        m_tab_container.setMaxTotalSize(s);
        alignTabs();
    }
}

void FbWinFrame::alignTabs() {
    if (m_tabmode != EXTERNAL)
        return;


    FbTk::Container& tabs = tabcontainer();
    FbTk::Orientation orig_orient = tabs.orientation();
    unsigned int orig_tabwidth = tabs.maxWidthPerClient();

    if (orig_tabwidth != m_screen.getTabWidth())
        tabs.setMaxSizePerClient(m_screen.getTabWidth());

    int bw = window().borderWidth();
    int size = width();
    int tab_x = x();
    int tab_y = y();

    TabPlacement p = m_screen.getTabPlacement();
    if (orig_orient != s_place[p].orient) {
        tabs.hide();
    }
    if (!s_place[p].is_horizontal) {
        size = height();
    }
    tabs.setOrientation(s_place[p].orient);
    tabs.setAlignment(s_place[p].align);
    tabs.setMaxTotalSize(size);

    int w = static_cast<int>(width());
    int h = static_cast<int>(height());
    int xo = xOffset();
    int yo = yOffset();
    int tw = static_cast<int>(tabs.width());
    int th = static_cast<int>(tabs.height());

    switch (p) {
    case TOPLEFT:                          tab_y -= yo;         break;
    case TOP:         tab_x += (w - tw)/2; tab_y -= yo;         break;
    case TOPRIGHT:    tab_x +=  w - tw;    tab_y -= yo;         break;
    case BOTTOMLEFT:                       tab_y +=  h + bw;    break;
    case BOTTOM:      tab_x += (w - tw)/2; tab_y +=  h + bw;    break;
    case BOTTOMRIGHT: tab_x +=  w - tw;    tab_y +=  h + bw;    break;
    case LEFTTOP:     tab_x -=  xo;                             break;
    case LEFT:        tab_x -=  xo;        tab_y += (h - th)/2; break;
    case LEFTBOTTOM:  tab_x -=  xo;        tab_y +=  h - th;    break;
    case RIGHTTOP:    tab_x +=  w + bw;                         break;
    case RIGHT:       tab_x +=  w + bw;    tab_y += (h - th)/2; break;
    case RIGHTBOTTOM: tab_x +=  w + bw;    tab_y +=  h - th;    break;
    }

    if (tabs.orientation() != orig_orient ||
        tabs.maxWidthPerClient() != orig_tabwidth) {
        renderTabContainer();
        if (m_visible && m_use_tabs) {
            applyTabContainer();
            tabs.clear();
            tabs.show();
        }
    }

    if (tabs.parent()->window() != m_screen.rootWindow().window()) {
        tabs.reparent(m_screen.rootWindow(), tab_x, tab_y);
        tabs.clear();
        m_layeritem.addWindow(tabs);
    } else {
        tabs.move(tab_x, tab_y);
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
        int opaque = 255;
        if (FbTk::Transparent::haveComposite()) {
            std::swap(alpha, opaque);
        }
        m_tab_container.setAlpha(alpha);
        m_window.setOpaque(opaque);
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

    if (m_state.shaded) {
        new_h = m_titlebar.height();
    } else if (m_state.fullscreen) {
        new_x = m_screen.getHeadX(head);
        new_y = m_screen.getHeadY(head);
        new_w = m_screen.getHeadWidth(head);
        new_h = m_screen.getHeadHeight(head);
    } else if (m_state.maximized == WindowState::MAX_NONE || !m_screen.getMaxIgnoreIncrement()) {
        applySizeHints(new_w, new_h, true);
    }

    moveResize(new_x, new_y, new_w, new_h, true, true, true);
    frameExtentSig().emit();
}

void FbWinFrame::setAlpha(bool focused, int alpha) {
    m_alpha[focused] = alpha;
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
    return m_alpha[focused];
}

void FbWinFrame::setDefaultAlpha() {
    if (getUseDefaultAlpha())
        return;
    m_alpha[UNFOCUS] = theme().unfocusedTheme()->alpha();
    m_alpha[FOCUS] = theme().unfocusedTheme()->alpha();
    applyAlpha();
}

bool FbWinFrame::getUseDefaultAlpha() const {
    if (m_alpha[UNFOCUS] != theme().unfocusedTheme()->alpha()) {
        return false;
    } else if (m_alpha[FOCUS] != theme().focusedTheme()->alpha()) {
        return false;
    }

    return true;
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

    int h = height();
    int th = m_titlebar.height();
    int tbw = m_titlebar.borderWidth();

    // only take away one borderwidth (as the other border is still the "top"
    // border)
    h = std::max(1, h - th - tbw);
    m_window.resize(m_window.width(), h);

    return true;
}

bool FbWinFrame::showTitlebar() {
    if (m_use_titlebar)
        return false;

    m_titlebar.show();
    m_use_titlebar = true;

    // only add one borderwidth (as the other border is still the "top"
    // border)
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

    int h = m_window.height();
    int hh = m_handle.height();
    int hbw = m_handle.borderWidth();

    // only take away one borderwidth (as the other border is still the "top"
    // border)
    h = std::max(1, h - hh - hbw);
    m_window.resize(m_window.width(), h);

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
    FbTk::FbWindow* win = 0;
    if (m_titlebar == event.window) {
        win = &m_titlebar;
    } else if (m_tab_container == event.window) {
        win = &m_tab_container;
    } else if (m_label == event.window) {
        win = &m_label;
    } else if (m_handle == event.window) {
        win = &m_handle;
    } else if (m_grip_left == event.window) {
        win = &m_grip_left;
    } else if (m_grip_right == event.window) {
        win = &m_grip_right;
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

        return;
    }

    win->clearArea(event.x, event.y, event.width, event.height);
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

    handle().resize(handle().width(), theme()->handleWidth());
    gripLeft().resize(buttonHeight(), theme()->handleWidth());
    gripRight().resize(gripLeft().width(), gripLeft().height());

    // align titlebar and render it
    if (m_use_titlebar) {
        reconfigureTitlebar();
        m_titlebar.raise();
    } else
        m_titlebar.lower();

    if (m_tabmode == EXTERNAL) {
        unsigned int h = buttonHeight();
        unsigned int w = m_tab_container.width();
        if (!s_place[m_screen.getTabPlacement()].is_horizontal) {
            w = m_tab_container.height();
            std::swap(w, h);
        }
        m_tab_container.resize(w, h);
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
            int opaque = 255;
            if (FbTk::Transparent::haveComposite()) {
                std::swap(alpha, opaque);
            }
            m_tab_container.setAlpha(alpha);
            m_window.setOpaque(opaque);
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
        m_tab_container.moveResize(next_x, m_bevel, space_left, button_size);
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

    typedef FbTk::ThemeProxy<FbWinFrameTheme> TP;
    TP& ft = theme().focusedTheme();
    TP& uft = theme().unfocusedTheme();

    // render pixmaps
    render(m_title_face.color[FOCUS], m_title_face.pm[FOCUS], m_titlebar.width(), m_titlebar.height(),
           ft->titleTexture(), m_imagectrl);

    render(m_title_face.color[UNFOCUS], m_title_face.pm[UNFOCUS], m_titlebar.width(), m_titlebar.height(),
           uft->titleTexture(), m_imagectrl);

    //!! TODO: don't render label if internal tabs

    render(m_label_face.color[FOCUS], m_label_face.pm[FOCUS], m_label.width(), m_label.height(),
           ft->iconbarTheme()->texture(), m_imagectrl);

    render(m_label_face.color[UNFOCUS], m_label_face.pm[UNFOCUS], m_label.width(), m_label.height(),
           uft->iconbarTheme()->texture(), m_imagectrl);
}

void FbWinFrame::renderTabContainer() {
    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    typedef FbTk::ThemeProxy<FbWinFrameTheme> TP;
    TP& ft = theme().focusedTheme();
    TP& uft = theme().unfocusedTheme();
    FbTk::Container& tabs = tabcontainer();
    const FbTk::Texture *tc_focused = &ft->iconbarTheme()->texture();
    const FbTk::Texture *tc_unfocused = &uft->iconbarTheme()->texture();

    if (m_tabmode == EXTERNAL && tc_focused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_focused = &ft->titleTexture();
    if (m_tabmode == EXTERNAL && tc_unfocused->type() & FbTk::Texture::PARENTRELATIVE)
        tc_unfocused = &uft->titleTexture();

    render(m_tabcontainer_face.color[FOCUS], m_tabcontainer_face.pm[FOCUS],
           tabs.width(), tabs.height(), *tc_focused, m_imagectrl, tabs.orientation());

    render(m_tabcontainer_face.color[UNFOCUS], m_tabcontainer_face.pm[UNFOCUS],
           tabs.width(), tabs.height(), *tc_unfocused, m_imagectrl, tabs.orientation());

    renderButtons();

}

void FbWinFrame::applyTitlebar() {

    int f = m_state.focused;
    int alpha = getAlpha(f);
    m_titlebar.setAlpha(alpha);
    m_label.setAlpha(alpha);

    if (m_tabmode != INTERNAL) {
        m_label.setGC(theme()->iconbarTheme()->text().textGC());
        m_label.setJustify(theme()->iconbarTheme()->text().justify());

        bg_pm_or_color(m_label, m_label_face.pm[f], m_label_face.color[f]);
    }

    bg_pm_or_color(m_titlebar, m_title_face.pm[f], m_title_face.color[f]);
    applyButtons();
}


void FbWinFrame::renderHandles() {
    if (!m_use_handle)
        return;

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    typedef FbTk::ThemeProxy<FbWinFrameTheme> TP;
    TP& ft = theme().focusedTheme();
    TP& uft = theme().unfocusedTheme();

    render(m_handle_face.color[FOCUS], m_handle_face.pm[FOCUS],
           m_handle.width(), m_handle.height(),
           ft->handleTexture(), m_imagectrl);

    render(m_handle_face.color[UNFOCUS], m_handle_face.pm[UNFOCUS],
           m_handle.width(), m_handle.height(),
           uft->handleTexture(), m_imagectrl);

    render(m_grip_face.color[FOCUS], m_grip_face.pm[FOCUS],
           m_grip_left.width(), m_grip_left.height(),
           ft->gripTexture(), m_imagectrl);

    render(m_grip_face.color[UNFOCUS], m_grip_face.pm[UNFOCUS],
           m_grip_left.width(), m_grip_left.height(),
           uft->gripTexture(), m_imagectrl);
}

void FbWinFrame::applyHandles() {

    bool f = m_state.focused;
    int alpha = getAlpha(f);

    m_handle.setAlpha(alpha);
    bg_pm_or_color(m_handle, m_handle_face.pm[f], m_handle_face.color[f]);

    m_grip_left.setAlpha(alpha);
    m_grip_right.setAlpha(alpha);

    bg_pm_or_color(m_grip_left, m_grip_face.pm[f], m_grip_face.color[f]);
    bg_pm_or_color(m_grip_right, m_grip_face.pm[f], m_grip_face.color[f]);
}

void FbWinFrame::renderButtons() {

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    typedef FbTk::ThemeProxy<FbWinFrameTheme> TP;
    TP& ft = theme().focusedTheme();
    TP& uft = theme().unfocusedTheme();

    render(m_button_face.color[UNFOCUS], m_button_face.pm[UNFOCUS],
           m_button_size, m_button_size,
           uft->buttonTexture(), m_imagectrl);

    render(m_button_face.color[FOCUS], m_button_face.pm[FOCUS],
           m_button_size, m_button_size,
           ft->buttonTexture(), m_imagectrl);

    render(m_button_face.color[PRESSED], m_button_face.pm[PRESSED],
           m_button_size, m_button_size,
           theme()->buttonPressedTexture(), m_imagectrl);

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

    m_alpha[UNFOCUS] = theme().unfocusedTheme()->alpha();
    m_alpha[FOCUS] = theme().focusedTheme()->alpha();

    m_handle.showSubwindows();

    // clear pixmaps
    m_title_face.pm[UNFOCUS] = m_title_face.pm[FOCUS] = 0;
    m_label_face.pm[UNFOCUS] = m_label_face.pm[FOCUS] = 0;
    m_tabcontainer_face.pm[UNFOCUS] = m_tabcontainer_face.pm[FOCUS] = 0;
    m_handle_face.pm[UNFOCUS] = m_handle_face.pm[FOCUS] = 0;
    m_button_face.pm[UNFOCUS] = m_button_face.pm[FOCUS] = m_button_face.pm[PRESSED] = 0;
    m_grip_face.pm[UNFOCUS] = m_grip_face.pm[FOCUS] = 0;

    m_button_size = s_button_size;

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

    FbWinFrame::BtnFace& face = m_button_face;

    if (m_button_face.pm[PRESSED]) {
        btn.setPressedPixmap(face.pm[PRESSED]);
    } else {
        btn.setPressedColor(face.color[PRESSED]);
    }

    bool f = m_state.focused;

    btn.setAlpha(getAlpha(f));
    btn.setGC(theme()->buttonPicGC());

    bg_pm_or_color(btn, face.pm[f], face.color[f]);
}


void FbWinFrame::applyTabContainer() {

    FbTk::Container& tabs = tabcontainer();
    FbWinFrame::Face& face = m_tabcontainer_face;

    tabs.setAlpha(getAlpha(m_state.focused));
    bg_pm_or_color(tabs, face.pm[m_state.focused], face.color[m_state.focused]);

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
        FbTk::Color c = theme()->border().color();
        window().setBorderColor(c);
        titlebar().setBorderColor(c);
        handle().setBorderColor(c);
        gripLeft().setBorderColor(c);
        gripRight().setBorderColor(c);
        tabcontainer().setBorderColor(c);
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
    int bw = static_cast<int>(m_window.borderWidth());
    int bw_diff = static_cast<int>(client_bw) - bw;
    int height_diff = 2*bw_diff - static_cast<int>(titlebarHeight()) - static_cast<int>(handleHeight());
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
    if (s_place[m_screen.getTabPlacement()].is_horizontal) {
        return 0;
    }
    return m_tab_container.width() + m_window.borderWidth();
}

int FbWinFrame::heightOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;

    if (!s_place[m_screen.getTabPlacement()].is_horizontal) {
        return 0;
    }
    return m_tab_container.height() + m_window.borderWidth();
}

int FbWinFrame::xOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;
    TabPlacement p = m_screen.getTabPlacement();
    if (p == LEFTTOP || p == LEFT || p == LEFTBOTTOM) {
        return m_tab_container.width() + m_window.borderWidth();
    }
    return 0;
}

int FbWinFrame::yOffset() const {
    if (m_tabmode != EXTERNAL || !m_use_tabs)
        return 0;
    TabPlacement p = m_screen.getTabPlacement();
    if (p == TOPLEFT || p == TOP || p == TOPRIGHT) {
        return m_tab_container.height() + m_window.borderWidth();
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

int FbWinFrame::getContext(Window win, int x, int y, int last_x, int last_y, bool doBorders) {
    int context = 0;
    if (gripLeft().window()  == win) return Keys::ON_LEFTGRIP;
    if (gripRight().window() == win) return Keys::ON_RIGHTGRIP;
    if (doBorders) {
        using RectangleUtil::insideBorder;
        int borderw = window().borderWidth();
        if ( // if mouse is currently on the window border, ignore it
                (
                    ! insideBorder(window(), x, y, borderw)
                    && ( externalTabMode()
                        || ! insideBorder(tabcontainer(), x, y, borderw) )
                )
                || // or if mouse was on border when it was last clicked
                (
                    ! insideBorder(window(), last_x, last_y, borderw)
                    && ( externalTabMode()
                        || ! insideBorder(tabcontainer(), last_x, last_y, borderw ) )
                )
           ) context = Keys::ON_WINDOWBORDER;
    }

    if (window().window()    == win) return context | Keys::ON_WINDOW;
    // /!\ old code: handle = titlebar in motionNotifyEvent but only there !
    // handle() as border ??
    if (handle().window()    == win) {
        const int px = x - this->x() - window().borderWidth();
        if (px < gripLeft().x() + gripLeft().width() || px > gripRight().x())
            return context; // one of the corners
        return Keys::ON_WINDOWBORDER | Keys::ON_WINDOW;
    }
    if (titlebar().window()  == win) {
        const int px = x - this->x() - window().borderWidth();
        if (px < label().x() || px > label().x() + label().width())
            return context; // one of the buttons, asked from a grabbed event
        return context | Keys::ON_TITLEBAR;
    }
    if (label().window()     == win) return context | Keys::ON_TITLEBAR;
    // internal tabs are on title bar
    if (tabcontainer().window() == win)
        return context | Keys::ON_TAB | (externalTabMode()?0:Keys::ON_TITLEBAR);


    FbTk::Container::ItemList::iterator it = tabcontainer().begin();
    FbTk::Container::ItemList::iterator it_end = tabcontainer().end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == win)
            break;
    }
    // internal tabs are on title bar
    if (it != it_end)
        return context | Keys::ON_TAB | (externalTabMode()?0:Keys::ON_TITLEBAR);

    return context;
}
