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

#include "WorkspaceMenu.hh"

#include "Screen.hh"
#include "Workspace.hh"
#include "WorkspaceCmd.hh"
#include "MenuCreator.hh"
#include "FbTk/CommandParser.hh"
#include "FbCommands.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/MultiButtonMenuItem.hh"
#include "FbTk/MemFun.hh"

namespace {

// the menu consists of (* means static)
//   - icons               * 0
//   --------------------- * 1
//   - workspaces            2
//   --------------------- * 3
//   - new workspace       * 4
//   - edit workspace name * 5
//   - remove last         * 6
//

const unsigned int IDX_AFTER_ICONS = 2;
const unsigned int NR_STATIC_ITEMS = 6;

void add_workspaces(WorkspaceMenu& menu, BScreen& screen) {
    for (size_t i = 0; i < screen.numberOfWorkspaces(); ++i) {
        Workspace* w = screen.getWorkspace(i);
        w->menu().setInternalMenu();
        FbTk::MultiButtonMenuItem* submenu = new FbTk::MultiButtonMenuItem(5, FbTk::BiDiString(w->name()), &w->menu());
        FbTk::RefCount<FbTk::Command<void> > jump_cmd(new JumpToWorkspaceCmd(w->workspaceID()));
        submenu->setCommand(3, jump_cmd);
        menu.insertItem(submenu, i + IDX_AFTER_ICONS);
    }
}

} // end of anonymous namespace

WorkspaceMenu::WorkspaceMenu(BScreen &screen):
   FbMenu(screen.menuTheme(), 
           screen.imageControl(), 
           *screen.layerManager().getLayer(ResourceLayer::MENU)) {

    init(screen);
}

void WorkspaceMenu::workspaceInfoChanged( BScreen& screen ) {
    while (numberOfItems() > NR_STATIC_ITEMS) {
        remove(IDX_AFTER_ICONS);
    }
    ::add_workspaces(*this, screen);
    updateMenu();
}

void WorkspaceMenu::workspaceChanged(BScreen& screen) {
    FbTk::MenuItem *item = 0;
    for (unsigned int i = 0; i < screen.numberOfWorkspaces(); ++i) {
        item = find(i + IDX_AFTER_ICONS);
        if (item && item->isSelected()) {
            setItemSelected(i + IDX_AFTER_ICONS, false);
            updateMenu();
            break;
        }
    }
    setItemSelected(screen.currentWorkspace()->workspaceID() + IDX_AFTER_ICONS, true);
    updateMenu();
}

void WorkspaceMenu::init(BScreen &screen) {

    join(screen.currentWorkspaceSig(),
         FbTk::MemFun(*this, &WorkspaceMenu::workspaceChanged));
    join(screen.workspaceCountSig(),
         FbTk::MemFun(*this, &WorkspaceMenu::workspaceInfoChanged));
    join(screen.workspaceNamesSig(),
         FbTk::MemFun(*this, &WorkspaceMenu::workspaceInfoChanged));

    using namespace FbTk;
    _FB_USES_NLS;

    removeAll();

    setLabel(_FB_XTEXT(Workspace, MenuTitle, "Workspaces", "Title of main workspace menu"));
    insertSubmenu(_FB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"),
           MenuCreator::createMenuType("iconmenu", screen.screenNumber()));
    insertItem(new FbTk::MenuSeparator());

    ::add_workspaces(*this, screen);
    setItemSelected(screen.currentWorkspace()->workspaceID() + IDX_AFTER_ICONS, true);

    RefCount<Command<void> > saverc_cmd(new FbCommands::SaveResources());

    MacroCommand *new_workspace_macro = new MacroCommand();
    RefCount<Command<void> > addworkspace(new SimpleCommand<BScreen>(screen, (SimpleCommand<BScreen>::Action)&BScreen::addWorkspace));
    new_workspace_macro->add(addworkspace);
    new_workspace_macro->add(saverc_cmd);
    RefCount<Command<void> > new_workspace_cmd(new_workspace_macro);

    MacroCommand *remove_workspace_macro = new MacroCommand();
    RefCount<Command<void> > rmworkspace(new SimpleCommand<BScreen>(screen, (SimpleCommand<BScreen>::Action)&BScreen::removeLastWorkspace));
    remove_workspace_macro->add(rmworkspace);
    remove_workspace_macro->add(saverc_cmd);
    RefCount<Command<void> > remove_last_cmd(remove_workspace_macro);

    RefCount<Command<void> > start_edit(FbTk::CommandParser<void>::instance().parse("setworkspacenamedialog"));

    insertItem(new FbTk::MenuSeparator());
    insertCommand(_FB_XTEXT(Workspace, NewWorkspace, "New Workspace", "Add a new workspace"), 
           new_workspace_cmd);
    insertCommand(_FB_XTEXT(Toolbar, EditWkspcName, "Edit current workspace name", "Edit current workspace name"),
           start_edit);
    insertCommand(_FB_XTEXT(Workspace, RemoveLast, "Remove Last", "Remove the last workspace"), 
          remove_last_cmd);

    updateMenu();
}
