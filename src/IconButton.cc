// IconButton.cc
// Copyright (c) 2003 - 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: IconButton.cc,v 1.17 2004/01/21 14:13:32 fluxgen Exp $

#include "IconButton.hh"


#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"

#include "FbTk/SimpleCommand.hh"
#include "FbTk/App.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

namespace {

class ShowMenu: public FbTk::Command {
public:
    explicit ShowMenu(FluxboxWindow &win):m_win(win) { }
    void execute() {
        m_win.screen().hideMenus();
        // get last button pos
        const XEvent &event = Fluxbox::instance()->lastEvent();
        int x = event.xbutton.x_root - (m_win.menu().width() / 2);
        int y = event.xbutton.y_root - (m_win.menu().height() / 2);

        m_win.showMenu(x, y);
    }
private:
    FluxboxWindow &m_win;
};

class FocusCommand: public FbTk::Command {
public:
    explicit FocusCommand(FluxboxWindow &win):m_win(win) { }
    void execute() {
       if(m_win.isIconic() || !m_win.isFocused())
         m_win.raiseAndFocus();
       else
         m_win.iconify();
    }
private:
    FluxboxWindow &m_win;
};


} // end anonymous namespace



IconButton::IconButton(const FbTk::FbWindow &parent, const FbTk::Font &font,
                       FluxboxWindow &win):
    FbTk::TextButton(parent, font, win.winClient().title()),
    m_win(win), 
    m_icon_window(*this, 1, 1, 1, 1, 
                  ExposureMask | ButtonPressMask | ButtonReleaseMask),
    m_use_pixmap(true) {

    FbTk::RefCount<FbTk::Command> hidemenus(new FbTk::SimpleCommand<BScreen>(win.screen(), &BScreen::hideMenus));
    //!! TODO: There're some issues with MacroCommand when
    //         this object dies when the last macrocommand is executed (focused cmd)
    //         In iconbar mode Icons
    //    FbTk::MacroCommand *focus_macro = new FbTk::MacroCommand();
    //    focus_macro->add(hidemenus);
    //    focus_macro->add(focus);
    FbTk::RefCount<FbTk::Command> focus_cmd(new ::FocusCommand(m_win));
    FbTk::RefCount<FbTk::Command> menu_cmd(new ::ShowMenu(m_win));
    setOnClick(focus_cmd, 1);
    setOnClick(menu_cmd, 3);
    m_win.hintSig().attach(this);
    
    FbTk::EventManager::instance()->add(*this, m_icon_window);
    
    update(0);
}

IconButton::~IconButton() {

}


void IconButton::exposeEvent(XExposeEvent &event) {
    if (m_icon_window == event.window)
        m_icon_window.clear();
    else
        FbTk::TextButton::exposeEvent(event);
}
void IconButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {

    FbTk::TextButton::moveResize(x, y, width, height);

    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::resize(unsigned int width, unsigned int height) {
    FbTk::TextButton::resize(width, height);
    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::clear() {
    setupWindow();
}

void IconButton::clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposure) {
    FbTk::TextButton::clearArea(x, y,
                                width, height, exposure);
}

void IconButton::setPixmap(bool use) {
    if (m_use_pixmap != use) {
        m_use_pixmap = use;
        update(0);
    }
}

void IconButton::update(FbTk::Subject *subj) {
    // we got signal that either title or 
    // icon pixmap was updated, 
    // so we refresh everything

    // we need to check our client first
    if (m_win.clientList().empty())
        return;

    XWMHints *hints = XGetWMHints(FbTk::App::instance()->display(), m_win.winClient().window());
    if (hints == 0)
        return;

    if (m_use_pixmap && (hints->flags & IconPixmapHint) && hints->icon_pixmap != 0) {
        // setup icon window
        m_icon_window.show();
        int new_height = height() - 2*m_icon_window.y(); // equally padded
        int new_width = new_height;
        m_icon_window.resize((new_width>0) ? new_width : 1, (new_height>0) ? new_height : 1);

        m_icon_pixmap.copy(hints->icon_pixmap);
        m_icon_pixmap.scale(m_icon_window.width(), m_icon_window.height());

        m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable());
    } else {
        // no icon pixmap
        m_icon_window.move(0, 0);
        m_icon_window.hide();
        m_icon_pixmap = 0;
    }

    if(m_use_pixmap && (hints->flags & IconMaskHint)) {
        m_icon_mask.copy(hints->icon_mask);
        m_icon_mask.scale(m_icon_pixmap.width(), m_icon_pixmap.height());
    } else
        m_icon_mask = 0;

    XFree(hints);
    hints = 0;

#ifdef SHAPE

    if (m_icon_mask.drawable() != 0) {
        XShapeCombineMask(FbTk::App::instance()->display(),
                          m_icon_window.drawable(),
                          ShapeBounding,
                          0, 0,
                          m_icon_mask.drawable(),
                          ShapeSet);
    }

#endif // SHAPE

    setupWindow();
}

void IconButton::setupWindow() {

    m_icon_window.clear();

    if (!m_win.clientList().empty()) {
        setText(m_win.winClient().title());
        // draw with x offset and y offset
    }
    FbTk::TextButton::clear();
}

void IconButton::drawText(int x, int y) {
    // offset text
    if (m_icon_pixmap.drawable() != 0)
        FbTk::TextButton::drawText(m_icon_window.x() + m_icon_window.width() + 1, y);
    else
        FbTk::TextButton::drawText(1, y);
}
                          

