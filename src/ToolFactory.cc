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

#include "ToolFactory.hh"

// Tools
#include "ButtonTool.hh"
#include "ClockTool.hh"
#ifdef USE_SYSTRAY
#include "SystemTray.hh"
#endif
#include "IconbarTool.hh"
#include "WorkspaceNameTool.hh"
#include "ArrowButton.hh"

// Themes
#include "WorkspaceNameTheme.hh"
#include "ButtonTheme.hh"

#include "FbTk/CommandParser.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "Toolbar.hh"
#include "fluxbox.hh"

namespace {
class ShowMenuAboveToolbar: public FbTk::Command<void> {
public:
    explicit ShowMenuAboveToolbar(Toolbar &tbar):m_tbar(tbar) { }
    void execute() {

        const XEvent& e= Fluxbox::instance()->lastEvent();

        m_tbar.screen()
            .placementStrategy()
            .placeAndShowMenu(m_tbar.menu(), e.xbutton.x_root, e.xbutton.y_root, false);
    }
private:
    Toolbar &m_tbar;
};

}

ToolFactory::ToolFactory(BScreen &screen):m_screen(screen),
    m_clock_theme(screen.screenNumber(), "toolbar.clock", "Toolbar.Clock"),
    m_button_theme(new ButtonTheme(screen.screenNumber(), "toolbar.button", "Toolbar.Button", 
                                   "toolbar.clock", "Toolbar.Clock")),
    m_workspace_theme(new WorkspaceNameTheme(screen.screenNumber(), "toolbar.workspace", "Toolbar.Workspace")),
    m_systray_theme(new ButtonTheme(screen.screenNumber(), "toolbar.systray", "Toolbar.Systray",
                                    "toolbar.clock", "Toolbar.Clock")),
    m_iconbar_theme(screen.screenNumber(), "toolbar.iconbar", "Toolbar.Iconbar"),
    m_focused_iconbar_theme(screen.screenNumber(), "toolbar.iconbar.focused", "Toolbar.Iconbar.Focused"),
    m_unfocused_iconbar_theme(screen.screenNumber(), "toolbar.iconbar.unfocused", "Toolbar.Iconbar.Unfocused") {

}

ToolbarItem *ToolFactory::create(const std::string &name, const FbTk::FbWindow &parent, Toolbar &tbar) {
    ToolbarItem * item = 0;

    unsigned int button_size = 24;
    if (tbar.theme()->buttonSize() > 0)
        button_size = tbar.theme()->buttonSize();

    if (name == "workspacename") {
        WorkspaceNameTool *witem = new WorkspaceNameTool(parent,
                                                        *m_workspace_theme, screen());
        using namespace FbTk;
        RefCount<Command<void> > showmenu(new ShowMenuAboveToolbar(tbar));
        witem->button().setOnClick(showmenu);
        item = witem;
    } else if (name == "iconbar") {
        item = new IconbarTool(parent, m_iconbar_theme, m_focused_iconbar_theme, m_unfocused_iconbar_theme, screen(), tbar.menu());
    } else if (name == "systemtray") {
#ifdef USE_SYSTRAY
        item = new SystemTray(parent, dynamic_cast<ButtonTheme &>(*m_systray_theme), screen());
#endif
    } else if (name == "clock") {
        item = new ClockTool(parent, m_clock_theme, screen(), tbar.menu());
    } else {

        std::string cmd_str = name;
        if (name == "prevwindow" || name == "nextwindow") {
            cmd_str += " (workspace=[current])";
        }

        FbTk::RefCount<FbTk::Command<void> > cmd(FbTk::CommandParser<void>::instance().parse(cmd_str));
        if (cmd == 0) // we need a command
            return 0;

        // TODO maybe direction of arrows should depend on toolbar layout ?
        FbTk::FbDrawable::TriangleType arrow_type = FbTk::FbDrawable::RIGHT;
        if (name.find("prev") != std::string::npos)
            arrow_type = FbTk::FbDrawable::LEFT;

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
    m_focused_iconbar_theme.reconfigTheme();
    m_unfocused_iconbar_theme.reconfigTheme();
    m_button_theme->reconfigTheme();
    m_workspace_theme->reconfigTheme();
}


int ToolFactory::maxFontHeight() {
    unsigned int max_height = 0;
    if (max_height < m_clock_theme.font().height())
        max_height = m_clock_theme.font().height();

    if (max_height < m_focused_iconbar_theme.text().font().height())
        max_height = m_focused_iconbar_theme.text().font().height();

    if (max_height < m_unfocused_iconbar_theme.text().font().height())
        max_height = m_unfocused_iconbar_theme.text().font().height();

    if (max_height < m_workspace_theme->font().height())
        max_height = m_workspace_theme->font().height();

    return max_height;
}

