// WorkspaceMenu.cc for Fluxbox
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "WorkspaceMenu.hh"

#include "Screen.hh"
#include "Workspace.hh"
#include "WorkspaceCmd.hh"
#include "MenuCreator.hh"
#include "FbTk/CommandRegistry.hh"
#include "FbCommands.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/MultiButtonMenuItem.hh"

#include <typeinfo>

// the menu consists of (* means static)
//   - icons               * 0
//   --------------------- * 1
//   - workspaces            2
//   --------------------- * 3
//   - new workspace       * 4
//   - edit workspace name * 5
//   - remove last         * 6
//

#define IDX_AFTER_ICONS 2
#define NR_STATIC_ITEMS 6

WorkspaceMenu::WorkspaceMenu(BScreen &screen):
   FbMenu(screen.menuTheme(), 
           screen.imageControl(), 
           *screen.layerManager().getLayer(Layer::MENU)) {


    init(screen);
}

void WorkspaceMenu::update(FbTk::Subject *subj) {

    if (subj != 0 && typeid(*subj) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject &screen_subj = *static_cast<BScreen::ScreenSubject *>(subj);
        BScreen &screen = screen_subj.screen();
        if (subj == &screen.currentWorkspaceSig()) {
            FbTk::MenuItem *item = 0;
            for (unsigned int i = 0; i < screen.numberOfWorkspaces(); ++i) {
                item = find(i + IDX_AFTER_ICONS);
                if (item && item->isSelected()) {
                    setItemSelected(i + IDX_AFTER_ICONS, false);
                    updateMenu(i + IDX_AFTER_ICONS);
                    break;
                }
            }
            setItemSelected(screen.currentWorkspace()->workspaceID() + IDX_AFTER_ICONS, true);
            updateMenu(screen.currentWorkspace()->workspaceID() + IDX_AFTER_ICONS);
        } else if (subj == &screen.workspaceCountSig() || 
                   subj == &screen.workspaceNamesSig()) {
            while (numberOfItems() > NR_STATIC_ITEMS) {
                remove(IDX_AFTER_ICONS);
            }
            // for each workspace add workspace name and it's menu
            // to our workspace menu
            for (size_t workspace = 0; workspace < screen.numberOfWorkspaces(); 
                 ++workspace) {
                Workspace *wkspc = screen.getWorkspace(workspace);
                wkspc->menu().setInternalMenu();
                FbTk::MultiButtonMenuItem* mb_menu = new FbTk::MultiButtonMenuItem(5, 
                                                                                   wkspc->name().c_str(),
                                                                                   &wkspc->menu());
                FbTk::RefCount<FbTk::Command> jump_cmd(new JumpToWorkspaceCmd(wkspc->workspaceID()));
                mb_menu->setCommand(3, jump_cmd);
                insert(mb_menu, workspace + IDX_AFTER_ICONS);
            }

            updateMenu(-1);
        }
    } else {
        FbTk::Menu::update(subj);
    }
}

void WorkspaceMenu::init(BScreen &screen) {
    screen.currentWorkspaceSig().attach(this);
    screen.workspaceCountSig().attach(this);
    screen.workspaceNamesSig().attach(this);
    using namespace FbTk;
    _FB_USES_NLS;

    removeAll();

    setLabel(_FB_XTEXT(Workspace, MenuTitle, "Workspaces", "Title of main workspace menu"));
    insert(_FB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"),
           MenuCreator::createMenuType("iconmenu", screen.screenNumber()));
    insert(new FbTk::MenuSeparator());
    // for each workspace add workspace name and it's menu to our workspace menu
    for (size_t workspace = 0; workspace < screen.numberOfWorkspaces(); ++workspace) {
        Workspace *wkspc = screen.getWorkspace(workspace);
        wkspc->menu().setInternalMenu();
        FbTk::MultiButtonMenuItem* mb_menu = new FbTk::MultiButtonMenuItem(5, 
                                                                           wkspc->name().c_str(),
                                                                           &wkspc->menu());
        FbTk::RefCount<FbTk::Command> jump_cmd(new JumpToWorkspaceCmd(wkspc->workspaceID()));
        mb_menu->setCommand(2, jump_cmd);
        insert(mb_menu, workspace + IDX_AFTER_ICONS);
    }
    setItemSelected(screen.currentWorkspace()->workspaceID() + IDX_AFTER_ICONS, true);


    RefCount<Command> saverc_cmd(new FbCommands::SaveResources());

    MacroCommand *new_workspace_macro = new MacroCommand();
    RefCount<Command> addworkspace(new SimpleCommand<BScreen, int>(screen, &BScreen::addWorkspace));
    new_workspace_macro->add(addworkspace);
    new_workspace_macro->add(saverc_cmd);
    RefCount<Command> new_workspace_cmd(new_workspace_macro);

    MacroCommand *remove_workspace_macro = new MacroCommand();
    RefCount<Command> rmworkspace(new SimpleCommand<BScreen, int>(screen, &BScreen::removeLastWorkspace));
    remove_workspace_macro->add(rmworkspace);
    remove_workspace_macro->add(saverc_cmd);
    RefCount<Command> remove_last_cmd(remove_workspace_macro);

    RefCount<Command> start_edit(FbTk::CommandRegistry::instance().parseLine("setworkspacenamedialog"));

    insert(new FbTk::MenuSeparator());
    insert(_FB_XTEXT(Workspace, NewWorkspace, "New Workspace", "Add a new workspace"), 
           new_workspace_cmd);
    insert(_FB_XTEXT(Toolbar, EditWkspcName, "Edit current workspace name", "Edit current workspace name"),
           start_edit);
    insert(_FB_XTEXT(Workspace, RemoveLast, "Remove Last", "Remove the last workspace"), 
          remove_last_cmd);
    
    updateMenu();
}
