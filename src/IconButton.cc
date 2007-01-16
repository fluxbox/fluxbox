// IconButton.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "IconButton.hh"
#include "IconbarTool.hh"

#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "CommandParser.hh"
#include "WindowCmd.hh"

#include "FbTk/App.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Menu.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

typedef FbTk::RefCount<FbTk::Command> RefCmd;

namespace {

class ShowMenu: public FbTk::Command {
public:
    explicit ShowMenu(FluxboxWindow &win):m_win(win) { }
    void execute() {
        // hide the menu if it's already showing for this FluxboxWindow
        if (m_win.menu().isVisible() && WindowCmd<void>::window() == &m_win) {
            m_win.screen().hideMenus();
            return;
        }
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
    explicit FocusCommand(const IconbarTool& tool, FluxboxWindow &win) : 
        m_win(win), m_tool(tool) { }
    void execute() {
        // this needs to be a local variable, as this object could be destroyed
        // if the workspace is changed.
        FluxboxWindow &win = m_win;
        if(win.isIconic() || !win.isFocused()) {
            switch(win.screen().getUserFollowModel()) {
            case BScreen::SEMIFOLLOW_ACTIVE_WINDOW:
                if (win.isIconic()) {
                    win.screen().sendToWorkspace(win.screen().currentWorkspaceID(), &win);
                } else {
                    win.screen().changeWorkspaceID(win.workspaceNumber());
                }
                break;
            case BScreen::FETCH_ACTIVE_WINDOW:
                win.screen().sendToWorkspace(win.screen().currentWorkspaceID(), &win);
                break;
            case BScreen::FOLLOW_ACTIVE_WINDOW:
                if (!win.isStuck())
                    win.screen().changeWorkspaceID(win.workspaceNumber());
            default:
                break;
            };
            win.raiseAndFocus();
       } else
           win.iconify();
    }

private:
    FluxboxWindow &m_win;
    const IconbarTool& m_tool;
};

// simple forwarding of wheeling, but only 
// if desktopwheeling is enabled
class WheelWorkspaceCmd : public FbTk::Command {
public:
    explicit WheelWorkspaceCmd(const IconbarTool& tool, FluxboxWindow &win, const char* cmd) : 
        m_win(win), m_cmd(CommandParser::instance().parseLine(cmd)), m_tool(tool) { }
    void execute() {

        switch(m_tool.wheelMode()) {
        case IconbarTool::ON:
            m_cmd->execute();
            break;
        case IconbarTool::SCREEN:
            if(m_win.screen().isDesktopWheeling())
                m_cmd->execute();
            break;
        case IconbarTool::OFF:
        default:
            break;
        };
    }

private:
    FluxboxWindow &m_win;
    RefCmd m_cmd;
    const IconbarTool& m_tool;
};

} // end anonymous namespace



IconButton::IconButton(const IconbarTool& tool, const FbTk::FbWindow &parent, 
                       FbTk::Font &font, FluxboxWindow &win):
    FbTk::TextButton(parent, font, win.winClient().title()),
    m_win(win), 
    m_icon_window(*this, 1, 1, 1, 1, 
                  ExposureMask | ButtonPressMask | ButtonReleaseMask),
    m_use_pixmap(true) {


    RefCmd next_workspace(new ::WheelWorkspaceCmd(tool, m_win, "nextworkspace"));
    RefCmd prev_workspace(new ::WheelWorkspaceCmd(tool, m_win, "prevworkspace"));
    
    RefCmd focus_cmd(new ::FocusCommand(tool, m_win));
    RefCmd menu_cmd(new ::ShowMenu(m_win));
    setOnClick(focus_cmd, 1);
    setOnClick(menu_cmd, 3);
    if(win.screen().isReverseWheeling()) {
        setOnClick(next_workspace, 5);
        setOnClick(prev_workspace, 4);
    } else {
        setOnClick(next_workspace, 4);
        setOnClick(prev_workspace, 5);
    }

    m_win.hintSig().attach(this);
    m_win.titleSig().attach(this);
    
    FbTk::EventManager::instance()->add(*this, m_icon_window);

    update(0);
}

IconButton::~IconButton() {
    // ~FbWindow cleans event manager
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
        m_icon_window.height() != FbTk::Button::height()) {
        update(0); // update icon window
    }
}

void IconButton::resize(unsigned int width, unsigned int height) {
    FbTk::TextButton::resize(width, height);
    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height()) {
        update(0); // update icon window
    }
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

    Display *display = FbTk::App::instance()->display();

    int screen = m_win.screen().screenNumber();

    if (m_use_pixmap && m_win.usePixmap()) {
        // setup icon window
        m_icon_window.show();
        unsigned int w = width();
        unsigned int h = height();
        FbTk::translateSize(orientation(), w, h);
        int iconx = 1, icony = 1;
        unsigned int neww = w, newh = h;
        if (newh > 2*static_cast<unsigned>(icony))
            newh -= 2*icony;
        else
            newh = 1;
        neww = newh;

        FbTk::translateCoords(orientation(), iconx, icony, w, h);
        FbTk::translatePosition(orientation(), iconx, icony, neww, newh, 0);
        
        m_icon_window.moveResize(iconx, icony, neww, newh);

        m_icon_pixmap.copy(m_win.iconPixmap().drawable(), DefaultDepth(display, screen), screen);
        m_icon_pixmap.scale(m_icon_window.width(), m_icon_window.height());

        // rotate the icon or not?? lets go not for now, and see what they say...
        // need to rotate mask too if we do do this
        m_icon_pixmap.rotate(orientation());

        m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable());
    } else {
        // no icon pixmap
        m_icon_window.move(0, 0);
        m_icon_window.hide();
        m_icon_pixmap = 0;
    }

    if(m_use_pixmap && m_win.useMask()) {
        m_icon_mask.copy(m_win.iconMask().drawable(), 0, 0);
        m_icon_mask.scale(m_icon_pixmap.width(), m_icon_pixmap.height());
        m_icon_mask.rotate(orientation());
    } else
        m_icon_mask = 0;

#ifdef SHAPE

    XShapeCombineMask(display,
                      m_icon_window.drawable(),
                      ShapeBounding,
                      0, 0,
                      m_icon_mask.drawable(),
                      ShapeSet);

#endif // SHAPE

    if (subj == &(m_win.titleSig()))
        setText(m_win.title());

    if (subj != 0) {
        setupWindow();
    } else {
        m_icon_window.clear();
    }
}

void IconButton::setupWindow() {

    m_icon_window.clear();

    if (!m_win.clientList().empty()) {
        setText(m_win.winClient().title());
        // draw with x offset and y offset
    }
    FbTk::TextButton::clear();
}

void IconButton::drawText(int x, int y, FbTk::FbDrawable *drawable) {
    // offset text
    if (m_icon_pixmap.drawable() != 0)
        FbTk::TextButton::drawText(m_icon_window.x() + m_icon_window.width() + 1, y, drawable);
    else
        FbTk::TextButton::drawText(1, y, drawable);
}
                          
bool IconButton::setOrientation(FbTk::Orientation orient) {
    if (orientation() == orient)
        return true;

    if (FbTk::TextButton::setOrientation(orient)) {
        int iconx = 1, icony = 1;
        unsigned int tmpw = width(), tmph = height();
        FbTk::translateSize(orient, tmpw, tmph);
        FbTk::translateCoords(orient, iconx, icony, tmpw, tmph);
        FbTk::translatePosition(orient, iconx, icony, m_icon_window.width(), m_icon_window.height(), 0);
        m_icon_window.move(iconx, icony);
        return true;
    } else {
        return false;
    }
}

