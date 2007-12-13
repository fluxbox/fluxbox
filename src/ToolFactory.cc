// ToolFactory.cc for Fluxbox
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#include "ToolFactory.hh"

// Tools
#include "ButtonTool.hh"
#include "ClockTool.hh"
#include "SystemTray.hh"
#include "IconbarTool.hh"
#include "WorkspaceNameTool.hh"
#include "ArrowButton.hh"

// Themes
#include "IconbarTheme.hh"
#include "WorkspaceNameTheme.hh"
#include "ButtonTheme.hh"

#include "FbTk/CommandRegistry.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "fluxbox.hh"

#include "FbTk/FbWindow.hh"

#include <utility>

namespace {
class ShowMenuAboveToolbar: public FbTk::Command {
public:
    explicit ShowMenuAboveToolbar(Toolbar &tbar):m_tbar(tbar) { }
    void execute() {
        // get last button pos
        const XEvent &event = Fluxbox::instance()->lastEvent();
        int head = m_tbar.screen().getHead(event.xbutton.x_root, event.xbutton.y_root);
        std::pair<int, int> m = 
            m_tbar.screen().clampToHead( head,
                                         event.xbutton.x_root - (m_tbar.menu().width() / 2),
                                         event.xbutton.y_root - (m_tbar.menu().height() / 2),
                                         m_tbar.menu().width(),
                                         m_tbar.menu().height());
        m_tbar.menu().setScreen(m_tbar.screen().getHeadX(head),
                                m_tbar.screen().getHeadY(head),
                                m_tbar.screen().getHeadWidth(head),
                                m_tbar.screen().getHeadHeight(head));
        m_tbar.menu().move(m.first, m.second);
        m_tbar.menu().show();
        m_tbar.menu().grabInputFocus();
    }
private:
    Toolbar &m_tbar;
};

};

ToolFactory::ToolFactory(BScreen &screen):m_screen(screen),
    m_clock_theme(screen.screenNumber(), "toolbar.clock", "Toolbar.Clock"),
    m_button_theme(new ButtonTheme(screen.screenNumber(), "toolbar.button", "Toolbar.Button", 
                                   "toolbar.clock", "Toolbar.Clock")),
    m_workspace_theme(new WorkspaceNameTheme(screen.screenNumber(), "toolbar.workspace", "Toolbar.Workspace")),
    m_systray_theme(new ButtonTheme(screen.screenNumber(), "toolbar.systray", "Toolbar.Systray",
                                    "toolbar.clock", "Toolbar.Systray")),
    m_iconbar_theme(screen.screenNumber(), "toolbar.iconbar", "Toolbar.Iconbar") {

}

ToolbarItem *ToolFactory::create(const std::string &name, const FbTk::FbWindow &parent, Toolbar &tbar) {
    ToolbarItem * item = 0;

    unsigned int button_size = 24;
    if (tbar.theme().buttonSize() > 0)
        button_size = tbar.theme().buttonSize();

    if (name == "workspacename") {
        WorkspaceNameTool *witem = new WorkspaceNameTool(parent,
                                                        *m_workspace_theme, screen());
        using namespace FbTk;
        RefCount<Command> showmenu(new ShowMenuAboveToolbar(tbar));
        witem->button().setOnClick(showmenu);
        item = witem;
    } else if (name == "iconbar") {
        item = new IconbarTool(parent, m_iconbar_theme, screen(), tbar.menu());
    } else if (name == "systemtray") {
        item = new SystemTray(parent, dynamic_cast<ButtonTheme &>(*m_systray_theme), screen());
    } else if (name == "clock") {
        item = new ClockTool(parent, m_clock_theme, screen(), tbar.menu());
    } else if (name == "nextworkspace" || 
               name == "prevworkspace") {

        FbTk::RefCount<FbTk::Command> cmd(FbTk::CommandRegistry::instance().parseLine(name));
        if (*cmd == 0) // we need a command
            return 0;

		// TODO maybe direction of arrows should depend on toolbar layout ?
        FbTk::FbDrawable::TriangleType arrow_type = FbTk::FbDrawable::LEFT;
        if (name == "nextworkspace")
            arrow_type = FbTk::FbDrawable::RIGHT;

        ArrowButton *win = new ArrowButton(arrow_type, parent,
                                           0, 0,
                                           button_size, button_size);
        win->setOnClick(cmd);
        item = new ButtonTool(win, ToolbarItem::SQUARE, 
                              dynamic_cast<ButtonTheme &>(*m_button_theme),
                              screen().imageControl());

    } else if (name == "nextwindow" || 
               name == "prevwindow") {

        FbTk::RefCount<FbTk::Command> cmd(FbTk::CommandRegistry::instance().parseLine(name));
        if (*cmd == 0) // we need a command
            return 0;

        FbTk::FbDrawable::TriangleType arrow_type = FbTk::FbDrawable::LEFT;
        if (name == "nextwindow")
            arrow_type = FbTk::FbDrawable::RIGHT;
                    
        ArrowButton *win = new ArrowButton(arrow_type, parent,
                                           0, 0,
                                           button_size, button_size);
        win->setOnClick(cmd);
        item = new ButtonTool(win, ToolbarItem::SQUARE, 
                              dynamic_cast<ButtonTheme &>(*m_button_theme),
                              screen().imageControl());

    }

    if (item)
        item->renderTheme(tbar.alpha());

    return item;
}

void ToolFactory::updateThemes() {
    m_clock_theme.reconfigTheme();
    m_iconbar_theme.reconfigTheme();
    m_button_theme->reconfigTheme();
    m_workspace_theme->reconfigTheme();
}


int ToolFactory::maxFontHeight() {
    unsigned int max_height = 0;
    if (max_height < m_clock_theme.font().height())
        max_height = m_clock_theme.font().height();

    if (max_height < m_iconbar_theme.focusedText().font().height())
        max_height = m_iconbar_theme.focusedText().font().height();

    if (max_height < m_iconbar_theme.unfocusedText().font().height())
        max_height = m_iconbar_theme.unfocusedText().font().height();

    if (max_height < m_workspace_theme->font().height())
        max_height = m_workspace_theme->font().height();

    return max_height;
}

