// ToolbarHandler for fluxbox
// Copyright (c) 2003 Simon Bowden (rathnor at fluxbox.org)
//                and Henrik Kinnunen (fluxgen at fluxbox.org)
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

// $Id: ToolbarHandler.cc,v 1.8 2003/04/16 23:33:59 rathnor Exp $

/**
 * The ToolbarHandler class acts as a rough interface to the toolbar.
 * It deals with whether it should be there or not, so anything that
 * always needs to be accessible must come through the handler.
 */

#include "ToolbarHandler.hh"
#include "Window.hh"
#include "Screen.hh"
#include "Workspace.hh"
#include "MenuItem.hh"
#include "Menu.hh"
#include "FbCommands.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"
#include "MacroCommand.hh"
#include "IntResMenuItem.hh"
#include "BoolMenuItem.hh"

namespace {

class ToolbarModeMenuItem : public FbTk::MenuItem {
public:
    ToolbarModeMenuItem(const char *label, ToolbarHandler &handler, 
                        ToolbarHandler::ToolbarMode mode, 
                        FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_handler(handler), m_mode(mode) {
    }
    bool isEnabled() const { return m_handler.getMode() != m_mode; }
    void click(int button, int time) {
        m_handler.setMode(m_mode);
        FbTk::MenuItem::click(button, time);
    }

private:
    ToolbarHandler &m_handler;
    ToolbarHandler::ToolbarMode m_mode;
};

void setupModeMenu(FbTk::Menu &menu, ToolbarHandler &handler) {
    //I18n *i18n = I18n::instance();
    //using namespace FBNLS;
    using namespace FbTk;

    RefCount<Command> saverc_cmd(new SimpleCommand<Fluxbox>(
        *Fluxbox::instance(), 
        &Fluxbox::save_rc));
    
    //TODO: nls
    menu.insert(new ToolbarModeMenuItem("Off", handler, 
                                        ToolbarHandler::OFF, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("None", handler, 
                                        ToolbarHandler::NONE, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Icons", handler, 
                                        ToolbarHandler::ICONS, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Workspace Icons", handler, 
                                        ToolbarHandler::WORKSPACEICONS, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Workspace", handler, 
                                        ToolbarHandler::WORKSPACE, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("All Windows", handler, 
                                        ToolbarHandler::ALLWINDOWS, saverc_cmd));
    menu.update();
}
                
}; // end anonymous namespace

ToolbarHandler::ToolbarHandler(BScreen &screen, ToolbarMode mode) 
    : m_screen(screen), m_mode(mode), m_toolbar(0), m_current_workspace(0),
      m_modemenu(*screen.menuTheme(),
                 screen.getScreenNumber(), *screen.getImageControl()),
      m_toolbarmenu(*screen.menuTheme(),
                 screen.getScreenNumber(), *screen.getImageControl())
{
    m_modemenu.setInternalMenu();
    setupModeMenu(m_modemenu, *this);
    setMode(mode, false); // the atomhandler part will initialise it shortly
}

void ToolbarHandler::setMode(ToolbarMode mode, bool initialise) {
    if (mode < 0 || mode >= LASTMODE || (mode == m_mode && initialise)) 
        return;

    m_screen.saveToolbarMode(mode);
    if (mode == OFF) {
        m_mode = mode;
        m_toolbarmenu.removeAll();
        //TODO: nls
        m_toolbarmenu.insert("Mode...", &m_modemenu);
        m_toolbar.reset(0);
        m_toolbarmenu.update();

        return;
    } else if (!m_toolbar.get()) {
        m_toolbarmenu.removeAll();

        m_toolbar.reset(new Toolbar(m_screen, 
                                    *m_screen.layerManager().getLayer(m_screen.getToolbarLayerNum()), m_toolbarmenu));
        m_toolbarmenu.insert("Mode...", &m_modemenu);   
        m_toolbarmenu.update();
    }
    

    if (mode == NONE) {
        // disableIconBar will clean up
        m_toolbar->disableIconBar();
    } else {
        // rebuild it
        // be sure the iconbar is on
        m_toolbar->enableIconBar();
        m_toolbar->delAllIcons();
    }
    // reset Toolbar, and reload it (initForScreen)
    m_mode = mode;
    if (initialise)
        initForScreen(m_screen);
}

void ToolbarHandler::initForScreen(BScreen &screen) {
    if (&m_screen != &screen) 
        return;
    switch (m_mode) {
    case OFF:
        break;
    case NONE:
        break;
    case ALLWINDOWS:
    {
        BScreen::Workspaces::const_iterator workspace_it = m_screen.getWorkspacesList().begin();
        BScreen::Workspaces::const_iterator workspace_it_end = m_screen.getWorkspacesList().end();
        for (; workspace_it != workspace_it_end; ++workspace_it) {
            Workspace::Windows &wins = (*workspace_it)->getWindowList();
            Workspace::Windows::iterator wit = wins.begin();
            Workspace::Windows::iterator wit_end = wins.end();
            for (; wit != wit_end; ++wit) {
                    m_toolbar->addIcon(*wit);
/*
                FluxboxWindow::ClientList::iterator cit = (*wit)->clientList().begin();
                FluxboxWindow::ClientList::iterator cit_end = (*wit)->clientList().end();
                for (; cit != cit_end; ++cit)
                    m_toolbar->addIcon(*(*cit));
*/
            }
        }
    }
    // fall through and add icons
    case LASTMODE:
    case ICONS: 
    {
        BScreen::Icons &iconlist = m_screen.getIconList();
        BScreen::Icons::iterator iconit = iconlist.begin();
        BScreen::Icons::iterator iconit_end = iconlist.end();
        for(; iconit != iconit_end; ++iconit) {
            m_toolbar->addIcon(*iconit);
        }
    }
    break;
    case WORKSPACE:
    {
        Workspace::Windows &wins = m_screen.getCurrentWorkspace()->getWindowList();
        Workspace::Windows::iterator wit = wins.begin();
        Workspace::Windows::iterator wit_end = wins.end();
        for (; wit != wit_end; ++wit) {
            m_toolbar->addIcon(*wit);
        }
    }
    // fall through and add icons for this workspace
    case WORKSPACEICONS:
    {
        m_current_workspace = m_screen.getCurrentWorkspaceID();
        
        BScreen::Icons &wiconlist = m_screen.getIconList();
        BScreen::Icons::iterator iconit = wiconlist.begin();
        BScreen::Icons::iterator iconit_end = wiconlist.end();
        for(; iconit != iconit_end; ++iconit) {
            if ((*iconit)->getWorkspaceNumber() == m_current_workspace)
                m_toolbar->addIcon(*iconit);
        }
    }
    break;
    }
}

void ToolbarHandler::setupWindow(FluxboxWindow &win) {
    if (&win.getScreen() != &m_screen)
        return;

    switch (m_mode) {
    case OFF:
    case NONE:
        break;
    case WORKSPACE:
        if (win.getWorkspaceNumber() == m_current_workspace)    
            m_toolbar->addIcon(&win);
        break;
    case WORKSPACEICONS:
        if (win.getWorkspaceNumber() != m_current_workspace) 
            break;
        // else fall through and add the icon
    case LASTMODE:
    case ICONS:
        if (win.isIconic()) {
            m_toolbar->addIcon(&win);
        }
        break;
    case ALLWINDOWS:
        m_toolbar->addIcon(&win);
        break;
    }
}

void ToolbarHandler::updateWindowClose(FluxboxWindow &win) {
    if (&win.getScreen() != &m_screen) 
        return;

    // check status of window (in current workspace, etc) and remove if necessary
    switch (m_mode) {
    case OFF:
    case NONE:
        break;
    case WORKSPACEICONS:
        if (win.getWorkspaceNumber() != m_current_workspace) 
            break;
        // else fall through and remove the icon
    case LASTMODE:
    case ICONS:
        if (win.isIconic()) {
            m_toolbar->delIcon(&win);
        }
        break;
    case WORKSPACE:
        if (win.getWorkspaceNumber() == m_current_workspace)
            m_toolbar->delIcon(&win);
        break;
    case ALLWINDOWS:
        m_toolbar->delIcon(&win);
        break;
    }
}

void ToolbarHandler::updateState(FluxboxWindow &win) {
    if (&win.getScreen() != &m_screen)
        return;

    // this function only relevant for icons
    switch (m_mode) {
    case OFF:
    case NONE:
    case WORKSPACE:
    case ALLWINDOWS:
        break;
    case WORKSPACEICONS:
        if (win.getWorkspaceNumber() != m_current_workspace) break;
        // else fall through and do the same as icons (knowing it is the right ws)
    case LASTMODE:
    case ICONS:
        // if the window is iconic (it mustn't have been before), then add it
        // else remove it
        if (win.isIconic()) {
            if (!m_toolbar->containsIcon(win)) {
                m_toolbar->addIcon(&win);
            }
        } else {
            m_toolbar->delIcon(&win);
        }
        break;
    }
}
        

void ToolbarHandler::updateWorkspace(FluxboxWindow &win) {
    if (&win.getScreen() != &m_screen) 
        return;

    // don't care about current workspace except if in workspace mode
    if (!(m_mode == WORKSPACE || (m_mode == WORKSPACEICONS && win.isIconic()))) return;
    
    if (win.getWorkspaceNumber() == m_current_workspace) {
        //!! TODO
        // this shouldn't be needed, but is until Workspaces get fixed so that
        // you only move between them, you don't 'add' and 'remove'
        // alternatively, fix reassocaiteWindow so that the iconic stuff is
        // done elsewhere
        if (!m_toolbar->containsIcon(win))
            m_toolbar->addIcon(&win);
    } else {
        // relies on the fact that this runs but does nothing if window isn't contained.
        m_toolbar->delIcon(&win);
    }
}

void ToolbarHandler::updateCurrentWorkspace(BScreen &screen) {
    if (&screen != &m_screen) return;
    // if only displaying current workspace, update list
    // otherwise ignore it
    if (m_mode != WORKSPACE && m_mode != WORKSPACEICONS) return;
    m_toolbar->delAllIcons();
    initForScreen(m_screen);
}

