// ToolFactory.cc for Fluxbox
// Copyright (c) 2003-2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ToolFactory.cc,v 1.3 2004/01/11 16:09:50 fluxgen Exp $

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

#include "CommandParser.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "fluxbox.hh"

#include "FbTk/FbWindow.hh"

namespace {
class ShowMenuAboveToolbar: public FbTk::Command {
public:
    explicit ShowMenuAboveToolbar(Toolbar &tbar):m_tbar(tbar) { }
    void execute() {
        m_tbar.screen().hideMenus();
        // get last button pos
        const XEvent &event = Fluxbox::instance()->lastEvent();
        int x = event.xbutton.x_root - (m_tbar.menu().width() / 2);
        int y = event.xbutton.y_root - (m_tbar.menu().height() / 2);

        if (x < 0)
            x = 0;
        else if (x + m_tbar.menu().width() > m_tbar.screen().width())
            x = m_tbar.screen().width() - m_tbar.menu().width();

        if (y < 0)
            y = 0;
        else if (y + m_tbar.menu().height() > m_tbar.screen().height())
            y = m_tbar.screen().height() - m_tbar.menu().height();

        m_tbar.menu().move(x, y);
        m_tbar.menu().show();        
    }
private:
    Toolbar &m_tbar;
};

};

ToolFactory::ToolFactory(BScreen &screen):m_screen(screen),
    m_clock_theme(screen.screenNumber(), "toolbar.clock", "Toolbar.Clock"),
    m_button_theme(new ButtonTheme(screen.screenNumber(), "toolbar.button", "Toolbar.Button")),
    m_workspace_theme(new WorkspaceNameTheme(screen.screenNumber(), "toolbar.workspace", "Toolbar.Workspace")),
    m_iconbar_theme(screen.screenNumber(), "toolbar.iconbar", "Toolbar.Iconbar") {

}

ToolbarItem *ToolFactory::create(const std::string &name, const FbTk::FbWindow &parent, Toolbar &tbar) {

    unsigned int button_size = 24;
    if (tbar.theme().buttonSize() > 0)
        button_size = tbar.theme().buttonSize();

    if (name == "workspacename") {
        WorkspaceNameTool *item = new WorkspaceNameTool(parent,
                                                        *m_workspace_theme, screen());
        using namespace FbTk;
        RefCount<Command> showmenu(new ShowMenuAboveToolbar(tbar));
        item->button().setOnClick(showmenu);
        return item;
    } else if (name == "iconbar") {
        return new IconbarTool(parent, m_iconbar_theme, 
                               screen(), tbar.menu());
    } else if (name == "systemtray") {
        return new SystemTray(parent);
    } else if (name == "clock") {
        return new ClockTool(parent, m_clock_theme, screen(), tbar.menu());
    } else if (name == "nextworkspace" || 
               name == "prevworkspace") {

        FbTk::RefCount<FbTk::Command> cmd(CommandParser::instance().parseLine(name));
        if (*cmd == 0) // we need a command
            return 0;

        ArrowButton::Type arrow_type = ArrowButton::LEFT;
        if (name == "nextworkspace")
            arrow_type = ArrowButton::RIGHT;

        ArrowButton *win = new ArrowButton(arrow_type, parent,
                                           0, 0,
                                           button_size, button_size);
        win->setOnClick(cmd);
        return new ButtonTool(win, ToolbarItem::FIXED, 
                              dynamic_cast<ButtonTheme &>(*m_button_theme),
                              screen().imageControl());

    } else if (name == "nextwindow" || 
               name == "prevwindow") {

        FbTk::RefCount<FbTk::Command> cmd(CommandParser::instance().parseLine(name));
        if (*cmd == 0) // we need a command
            return 0;

        ArrowButton::Type arrow_type = ArrowButton::LEFT;
        if (name == "nextwindow")
            arrow_type = ArrowButton::RIGHT;
                    
        ArrowButton *win = new ArrowButton(arrow_type, parent,
                                           0, 0,
                                           button_size, button_size);
        win->setOnClick(cmd);
        return new ButtonTool(win, ToolbarItem::FIXED, 
                              dynamic_cast<ButtonTheme &>(*m_button_theme),
                              screen().imageControl());

    }

    return 0;
}

void ToolFactory::updateThemes() {
    m_clock_theme.setAntialias(screen().antialias());
    m_iconbar_theme.setAntialias(screen().antialias());
    m_button_theme->setAntialias(screen().antialias());
    m_workspace_theme->setAntialias(screen().antialias());
}


int ToolFactory::maxFontHeight() const {
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

