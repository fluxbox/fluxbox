// WorkspaceMenu.cc for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: WorkspaceMenu.cc,v 1.1 2004/05/02 20:51:36 fluxgen Exp $

#include "WorkspaceMenu.hh"

#include "Screen.hh"
#include "fluxbox.hh"
#include "Workspace.hh"
#include "MenuCreator.hh"
#include "I18n.hh"

#include "FbTk/SimpleCommand.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MenuItem.hh"

WorkspaceMenu::WorkspaceMenu(BScreen &screen):
   FbMenu(screen.menuTheme(), 
           screen.imageControl(), 
           *screen.layerManager().
           getLayer(Fluxbox::instance()->getMenuLayer())) {


    init(screen);
}

void WorkspaceMenu::update(FbTk::Subject *subj) {
    if (subj != 0 && typeid(*subj) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject &screen_subj = *static_cast<BScreen::ScreenSubject *>(subj);
        BScreen &screen = screen_subj.screen();
        if (subj == &screen.currentWorkspaceSig()) {
            FbTk::MenuItem *item = 0;
            for (unsigned int i = 2; i < numberOfItems(); ++i) {
                item = find(i);
                if (item && item->isSelected()) {
                    setItemSelected(i, false);
                    FbTk::Menu::update(i);
                    break;
                }
            }
            setItemSelected(screen.currentWorkspace()->workspaceID() + 2, true);
            FbTk::Menu::update(screen.currentWorkspace()->workspaceID() + 2);
        } else if (subj == &screen.workspaceCountSig() || 
                   subj == &screen.workspaceNamesSig()) {
            while (numberOfItems() != 3) {
                remove(2);
            }
            // for each workspace add workspace name and it's menu
            // to our workspace menu
            for (size_t workspace = 0; workspace < screen.getCount(); 
                 ++workspace) {
                Workspace *wkspc = screen.getWorkspace(workspace);
                wkspc->menu().setInternalMenu();
                insert(wkspc->name().c_str(), &wkspc->menu(), numberOfItems() - 1);
            }

            FbTk::Menu::update(-1);
        }
    } else {
        FbTk::Menu::update(subj);
    }
}

void WorkspaceMenu::init(BScreen &screen) {
    screen.currentWorkspaceSig().attach(this);
    screen.workspaceCountSig().attach(this);
    screen.workspaceNamesSig().attach(this);
    I18n &i18n = *I18n::instance();
    using namespace FbTk;
    using namespace FBNLS;

    removeAll();

    setLabel(i18n.getMessage(WorkspacemenuSet, WorkspacemenuWorkspacesTitle,
                             "Workspace"));
    RefCount<Command> new_workspace(new SimpleCommand<BScreen, int>(screen, &BScreen::addWorkspace));
    RefCount<Command> remove_last(new SimpleCommand<BScreen, int>(screen, &BScreen::removeLastWorkspace));
    insert(i18n.getMessage(WorkspacemenuSet, WorkspacemenuNewWorkspace,
                            "New Workspace"), 
           new_workspace);
    insert(i18n.getMessage(WorkspacemenuSet, WorkspacemenuRemoveLast,
                           "Remove Last"), 
           remove_last);
    // for each workspace add workspace name and it's menu to our workspace menu
    for (size_t workspace = 0; workspace < screen.getCount(); ++workspace) {
        Workspace *wkspc = screen.getWorkspace(workspace);
        wkspc->menu().setInternalMenu();
        insert(wkspc->name().c_str(), &wkspc->menu());
    }
    setItemSelected(screen.currentWorkspace()->workspaceID() + 2, true);

    insert(i18n.getMessage(IconSet, IconIcons,
                           "Icons"),
           MenuCreator::createMenuType("iconmenu", screen.screenNumber()));
    FbMenu::update();
}
