// IconbarTool.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: IconbarTool.cc,v 1.21 2003/12/10 23:08:03 fluxgen Exp $

#include "IconbarTool.hh"

#include "Screen.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "FbMenu.hh"
#include "BoolMenuItem.hh"
#include "CommandParser.hh"

#include "FbTk/Menu.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/MacroCommand.hh"

#include <typeinfo>
#include <string>
#include <iterator>
#include <iostream>
using namespace std;

template<>
void FbTk::Resource<IconbarTool::Mode>::setFromString(const char *strval) {
    if (strcasecmp(strval, "None") == 0) 
        m_value = IconbarTool::NONE;
    else if (strcasecmp(strval, "Icons") == 0) 
        m_value = IconbarTool::ICONS;
    else if (strcasecmp(strval, "WorkspaceIcons") == 0) 
        m_value = IconbarTool::WORKSPACEICONS;
    else if (strcasecmp(strval, "Workspace") == 0) 
        m_value = IconbarTool::WORKSPACE;
    else if (strcasecmp(strval, "AllWindows") == 0) 
        m_value = IconbarTool::ALLWINDOWS;
    else
        setDefaultValue();
}


template<>
string FbTk::Resource<IconbarTool::Mode>::getString() {

    switch (m_value) {
    case IconbarTool::NONE:
        return string("None");
        break;
    case IconbarTool::ICONS:
        return string("Icons");
        break;
    case IconbarTool::WORKSPACEICONS:
        return string("WorkspaceIcons");
        break;
    case IconbarTool::WORKSPACE:
        return string("Workspace");
        break;
    case IconbarTool::ALLWINDOWS:
        return string("AllWindows");
        break;
    }
    // default string
    return string("Icons");
}

namespace {

class ToolbarModeMenuItem : public FbTk::MenuItem {
public:
    ToolbarModeMenuItem(const char *label, IconbarTool &handler, 
                        IconbarTool::Mode mode, 
                        FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_handler(handler), m_mode(mode) {
    }
    bool isEnabled() const { return m_handler.mode() != m_mode; }
    void click(int button, int time) {
        m_handler.setMode(m_mode);
        FbTk::MenuItem::click(button, time);
    }

private:
    IconbarTool &m_handler;
    IconbarTool::Mode m_mode;
};

void setupModeMenu(FbTk::Menu &menu, IconbarTool &handler) {
    using namespace FbTk;

    // TODO: nls
    menu.setLabel("Iconbar Mode");

    RefCount<Command> saverc_cmd(new SimpleCommand<Fluxbox>(
                                                            *Fluxbox::instance(), 
                                                            &Fluxbox::save_rc));
    
    //TODO: nls
    menu.insert(new ToolbarModeMenuItem("None", handler, 
                                        IconbarTool::NONE, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Icons", handler, 
                                        IconbarTool::ICONS, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Workspace Icons", handler, 
                                        IconbarTool::WORKSPACEICONS, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("Workspace", handler, 
                                        IconbarTool::WORKSPACE, saverc_cmd));
    menu.insert(new ToolbarModeMenuItem("All Windows", handler, 
                                        IconbarTool::ALLWINDOWS, saverc_cmd));
    menu.update();
}
                
inline bool checkAddWindow(IconbarTool::Mode mode, const FluxboxWindow &win) {

    // just add the icons that are on the this workspace
    switch (mode) {
    case IconbarTool::NONE:
        break;
    case IconbarTool::ICONS:
        if (win.isIconic())
            return true;
        break;
    case IconbarTool::WORKSPACEICONS:
        if(win.workspaceNumber() == win.screen().currentWorkspaceID() &&
           win.isIconic())
            return true;
        break;
    case IconbarTool::WORKSPACE:
        if (win.workspaceNumber() == win.screen().currentWorkspaceID())
            return true;
        break;
    case IconbarTool::ALLWINDOWS:
        return true;
        break;
    }

    return false;
}

void removeDuplicate(const IconbarTool::IconList &iconlist, std::list<FluxboxWindow *> &windowlist) {
    IconbarTool::IconList::const_iterator win_it = iconlist.begin();
    IconbarTool::IconList::const_iterator win_it_end = iconlist.end();
    std::list<FluxboxWindow *>::iterator remove_it = windowlist.end();
    for (; win_it != win_it_end; ++win_it)
        remove_it = remove(windowlist.begin(), remove_it, &(*win_it)->win());

    // remove already existing windows
    windowlist.erase(remove_it, windowlist.end());

}

}; // end anonymous namespace

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, BScreen &screen,
                         FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent),
    m_theme(theme),
    m_focused_pm(0),
    m_unfocused_pm(0),
    m_empty_pm(0),
    m_rc_mode(screen.resourceManager(), WORKSPACE,
              screen.name() + ".iconbar.mode", screen.altName() + ".Iconbar.Mode"),
    m_rc_use_pixmap(screen.resourceManager(), true,
                    screen.name() + ".iconbar.usePixmap", screen.altName() + ".Iconbar.UsePixmap"),
    m_menu(*screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())) {

    // setup mode menu
    setupModeMenu(m_menu, *this);

    using namespace FbTk;
    // setup use pixmap item to reconfig iconbar and save resource on click
    MacroCommand *save_and_reconfig = new MacroCommand();   
    RefCount<Command> reconfig(new SimpleCommand<IconbarTool>(*this, &IconbarTool::renderTheme));
    RefCount<Command> save(CommandParser::instance().parseLine("saverc"));
    save_and_reconfig->add(reconfig);
    save_and_reconfig->add(save);
    RefCount<Command> s_and_reconfig(save_and_reconfig);
    m_menu.insert(new BoolMenuItem("Show Pictures", *m_rc_use_pixmap, s_and_reconfig));
    m_menu.update();
    // must be internal menu, otherwise toolbar main menu tries to delete it.
    m_menu.setInternalMenu();

    // add iconbar menu to toolbar menu
    menu.insert(m_menu.label().c_str(), &m_menu);

    // setup signals
    theme.reconfigSig().attach(this);
    screen.clientListSig().attach(this);
    screen.iconListSig().attach(this);
    screen.currentWorkspaceSig().attach(this);

    update(0);
}

IconbarTool::~IconbarTool() {
    deleteIcons();

    // remove cached images
    if (m_focused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_unfocused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_empty_pm)
        m_screen.imageControl().removeImage(m_empty_pm);

}

void IconbarTool::move(int x, int y) {
    m_icon_container.move(x, y);
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
    m_icon_container.resize(width, height);
    renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {

    m_icon_container.moveResize(x, y, width, height);
    renderTheme();
}

void IconbarTool::show() {
    m_icon_container.show();
}

void IconbarTool::hide() {
    m_icon_container.hide();
}

void IconbarTool::setMode(Mode mode) {
    if (mode == *m_rc_mode)
        return;

    *m_rc_mode = mode;

    // lock graphics update
    m_icon_container.setUpdateLock(true);

    deleteIcons();

    // update mode
    switch (mode) {
    case NONE: // nothing
        break;
    case ICONS: // all icons from all workspaces
    case WORKSPACEICONS: // all icons on current workspace
        updateIcons();
        break;
    case WORKSPACE: // all windows and all icons on current workspace
        updateWorkspace();
        break;
    case ALLWINDOWS: // all windows and all icons from all workspaces
        updateAllWindows();
        break;
    };

    // unlock graphics update
    m_icon_container.setUpdateLock(false);
    m_icon_container.update();
    m_icon_container.showSubwindows();

    renderTheme();
}

unsigned int IconbarTool::width() const {
    return m_icon_container.width();
}

unsigned int IconbarTool::height() const {
    return m_icon_container.height();
}

unsigned int IconbarTool::borderWidth() const {
    return m_icon_container.borderWidth();
}

void IconbarTool::update(FbTk::Subject *subj) {
    // ignore updates if we're shutting down
    if (m_screen.isShuttingdown())
        return;

    if (mode() == NONE) {
        if (subj != 0 && typeid(*subj) == typeid(IconbarTheme))
            renderTheme();
                
        return;
    }

    // handle window signal
    if (subj != 0 && typeid(*subj) == typeid(FluxboxWindow::WinSubject)) {
        // we handle everything except die signal here
        FluxboxWindow::WinSubject *winsubj = static_cast<FluxboxWindow::WinSubject *>(subj);
        if (subj == &(winsubj->win().focusSig())) {
            renderWindow(winsubj->win());
            return;
        } else if (subj == &(winsubj->win().workspaceSig())) {
            // we can ignore this signal if we're in ALLWINDOWS mode
            if (mode() == ALLWINDOWS)
                return;

            // workspace changed for this window, and if it's not on current workspace we remove it
            if (m_screen.currentWorkspaceID() != winsubj->win().workspaceNumber()) {
                removeWindow(winsubj->win());
                renderTheme();
            }
            return;
        } else if (subj == &(winsubj->win().dieSig())) { // die sig
            removeWindow(winsubj->win());
            renderTheme();
            return; // we don't need to update the entire list
        } else if (subj == &(winsubj->win().stateSig())) {
            if (mode() == ICONS || mode() == WORKSPACEICONS) {
                if (!winsubj->win().isIconic()) {
                    removeWindow(winsubj->win());
                    renderTheme();
                }
            } else if (mode() != WORKSPACE) {
                if (winsubj->win().isIconic()) {
                    removeWindow(winsubj->win());
                    renderTheme();
                }
            }
            return;

        } else if (subj == &(winsubj->win().titleSig())) {
            renderWindow(winsubj->win());
            return;
        } else {
            // signal not handled
            return;
        }
    }

    bool remove_all = false; // if we should readd all windows    

    if (subj != 0 && typeid(*subj) == typeid(BScreen::ScreenSubject) && mode() != ALLWINDOWS) {
        BScreen::ScreenSubject *screen_subj = static_cast<BScreen::ScreenSubject *>(subj);
        // current workspace sig
        if (&m_screen.currentWorkspaceSig() == screen_subj &&
            mode() != ALLWINDOWS && mode() != ICONS) {
            remove_all = true; // remove and readd all windows
        }/* else if (&m_screen.iconListSig() == screen_subj &&
                   (mode() == ALLWINDOWS || mode() == ICONS || mode() == WORKSPACE)) {
            remove_all = true;
        }*/
    }

    // lock graphic update
    m_icon_container.setUpdateLock(true);

    if (remove_all)
        deleteIcons();

    // ok, we got some signal that we need to update our iconbar container
    switch (mode()) {
    case NONE:
        return;
        break;
    case ICONS:        
    case WORKSPACEICONS:
        updateIcons();
        break;
    case WORKSPACE:
        updateWorkspace();
        break;
    case ALLWINDOWS:
        updateAllWindows();
        break;
    }

    // unlock container and update graphics
    renderTheme();
    m_icon_container.setUpdateLock(false);
    m_icon_container.update();
    m_icon_container.showSubwindows();
    
}

void IconbarTool::renderWindow(FluxboxWindow &win) {
    
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if (&(*icon_it)->win() == &win)
            break;
    }

    if (icon_it == m_icon_list.end())
        return;

    renderButton(*(*icon_it));
}

void IconbarTool::renderTheme() {
    Pixmap tmp = m_focused_pm;
    if (!m_theme.focusedTexture().usePixmap()) {
        m_focused_pm = 0;        
    } else {
        m_focused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                           m_icon_container.height(),
                                                           m_theme.focusedTexture());
    }
        
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    tmp = m_unfocused_pm;
    if (!m_theme.unfocusedTexture().usePixmap()) {
        m_unfocused_pm = 0;        
    } else {
        m_unfocused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                             m_icon_container.height(),
                                                             m_theme.unfocusedTexture());
    }
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    // if we dont have any icons then we should render empty texture
    tmp = m_empty_pm;
    if (!m_theme.emptyTexture().usePixmap()) {
        m_empty_pm = 0;
        m_icon_container.setBackgroundColor(m_theme.emptyTexture().color());
    } else {
        m_empty_pm = m_screen.imageControl().renderImage(m_icon_container.width(),
                                                         m_icon_container.height(),
                                                         m_theme.emptyTexture());
        m_icon_container.setBackgroundPixmap(m_empty_pm);
    }

    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    m_icon_container.setBorderWidth(m_theme.border().width());
    m_icon_container.setBorderColor(m_theme.border().color());

    // update buttons
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it)
        renderButton(*(*icon_it));
}

void IconbarTool::renderButton(IconButton &button) {

    button.setPixmap(*m_rc_use_pixmap);

    if (button.win().isFocused()) { // focused texture
        button.setGC(m_theme.focusedText().textGC());     
        button.setFont(m_theme.focusedText().font());
        button.setJustify(m_theme.focusedText().justify());

        if (m_focused_pm != 0)
            button.setBackgroundPixmap(m_focused_pm);
        else
            button.setBackgroundColor(m_theme.focusedTexture().color());            

        button.setBorderWidth(m_theme.focusedBorder().width());
        button.setBorderColor(m_theme.focusedBorder().color());

    } else { // unfocused
        button.setGC(m_theme.unfocusedText().textGC());
        button.setFont(m_theme.unfocusedText().font());
        button.setJustify(m_theme.unfocusedText().justify());

        if (m_unfocused_pm != 0)
            button.setBackgroundPixmap(m_unfocused_pm);
        else
            button.setBackgroundColor(m_theme.unfocusedTexture().color());

        button.setBorderWidth(m_theme.unfocusedBorder().width());
        button.setBorderColor(m_theme.unfocusedBorder().color());
    }

    button.clear();
}

void IconbarTool::deleteIcons() {
    m_icon_container.removeAll();
    while (!m_icon_list.empty()) {
        delete m_icon_list.back();
        m_icon_list.pop_back();
    }
}

void IconbarTool::removeWindow(FluxboxWindow &win) {
    // got window die signal, lets find and remove the window
    IconList::iterator it = m_icon_list.begin();
    IconList::iterator it_end = m_icon_list.end();
    for (; it != it_end; ++it) {
        if (&(*it)->win() == &win)
            break;
    }
    // did we find it?
    if (it == m_icon_list.end())
        return;
    
    // detach from all signals
    win.focusSig().detach(this);
    win.dieSig().detach(this);
    win.workspaceSig().detach(this);
    win.stateSig().detach(this);
    win.titleSig().detach(this);


    // remove from list and render theme again
    IconButton *button = *it;

    m_icon_container.removeItem(m_icon_container.find(*it));
    m_icon_list.erase(it);

    delete button;

}

void IconbarTool::addWindow(FluxboxWindow &win) {
    // we just want windows that has clients
    if (win.clientList().size() == 0)
        return;

    IconButton *button = new IconButton(m_icon_container, m_theme.focusedText().font(), win);
    button->setPixmap(*m_rc_use_pixmap);
    m_icon_container.insertItem(button);    
    m_icon_list.push_back(button);

    // dont forget to detach signal in removeWindow
    win.focusSig().attach(this);
    win.dieSig().attach(this);
    win.workspaceSig().attach(this);
    win.stateSig().attach(this);
    win.titleSig().attach(this);
}

void IconbarTool::updateIcons() {
    std::list<FluxboxWindow *> itemlist;
    // add icons to the itemlist    
    BScreen::Icons::iterator icon_it = m_screen.getIconList().begin();
    BScreen::Icons::iterator icon_it_end = m_screen.getIconList().end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if (mode() == ICONS)
            itemlist.push_back(*icon_it);
        else if (mode() == WORKSPACEICONS && (*icon_it)->workspaceNumber() == m_screen.currentWorkspaceID())
            itemlist.push_back(*icon_it);
    }
    removeDuplicate(m_icon_list, itemlist);
    addList(itemlist);    
}

void IconbarTool::updateWorkspace() {
    std::list<FluxboxWindow *> itemlist;
    // add current workspace windows
    Workspace &space = *m_screen.currentWorkspace();
    Workspace::Windows::iterator win_it = space.windowList().begin();
    Workspace::Windows::iterator win_it_end = space.windowList().end();
    for (; win_it != win_it_end; ++win_it) {
        if (checkAddWindow(mode(), **win_it))
            itemlist.push_back(*win_it);
    }    
    // add icons from current workspace
    BScreen::Icons::iterator icon_it =  m_screen.getIconList().begin();
    BScreen::Icons::iterator icon_it_end =  m_screen.getIconList().end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if ((*icon_it)->workspaceNumber() == m_screen.currentWorkspaceID())
            itemlist.push_back(*icon_it);
    }

    removeDuplicate(m_icon_list, itemlist);
    addList(itemlist);
}


void IconbarTool::updateAllWindows() {
    std::list<FluxboxWindow *> full_list;
    // for each workspace add clients to full list
    BScreen::Workspaces::iterator workspace_it = m_screen.getWorkspacesList().begin();
    BScreen::Workspaces::iterator workspace_it_end = m_screen.getWorkspacesList().end();
    for (; workspace_it != workspace_it_end; ++workspace_it) {
        full_list.insert(full_list.end(),
                         (*workspace_it)->windowList().begin(),
                         (*workspace_it)->windowList().end());
    }
    // add icons
    full_list.insert(full_list.end(),
                     m_screen.getIconList().begin(),
                     m_screen.getIconList().end());

    removeDuplicate(m_icon_list, full_list);
    addList(full_list);
}

void IconbarTool::addList(std::list<FluxboxWindow *> &winlist) {
    // ok, now we should have a list of icons that we need to add
    std::list<FluxboxWindow *>::iterator it = winlist.begin();
    std::list<FluxboxWindow *>::iterator it_end = winlist.end();
    for (; it != it_end; ++it)
        addWindow(**it);
}


