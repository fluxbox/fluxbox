// SendToMenu.cc for Fluxbox
// Copyright (c) 2003 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "SendToMenu.hh"

#include "Window.hh"
#include "Screen.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "Layer.hh"

#include "FbTk/MultiButtonMenuItem.hh"
#include "FbTk/Command.hh"
#include "FbTk/MemFun.hh"

class SendToCmd: public FbTk::Command<void> {
public:
    SendToCmd(int workspace, bool follow):
        m_workspace(workspace),
        m_follow(follow) { }
    void execute() {
        if (FbMenu::window() != 0)
            FbMenu::window()->screen().sendToWorkspace(m_workspace, FbMenu::window(), m_follow);
    }
private:
    const int m_workspace;
    const bool m_follow;
};

SendToMenu::SendToMenu(BScreen &screen):
    FbMenu(screen.menuTheme(),
           screen.imageControl(), 
           *screen.layerManager().getLayer(ResourceLayer::MENU)) {
    // listen to:
    // workspace count signal
    // workspace names signal
    // current workspace signal

    join(screen.workspaceNamesSig(),
         FbTk::MemFun(*this, &SendToMenu::rebuildMenuForScreen));

    join(screen.currentWorkspaceSig(),
         FbTk::MemFun(*this, &SendToMenu::rebuildMenuForScreen));

    // setup new signal system
    join(screen.workspaceCountSig(),
         FbTk::MemFun(*this, &SendToMenu::rebuildMenuForScreen));

    // no title for this menu, it should be a submenu in the window menu.
    disableTitle();
    // setup menu items
    rebuildMenu();
}

SendToMenu::~SendToMenu() {

}

void SendToMenu::rebuildMenu() {
    // rebuild menu

    removeAll();
    BScreen *screen = Fluxbox::instance()->findScreen(screenNumber());
    const BScreen::Workspaces &wlist = screen->getWorkspacesList();
    for (size_t i = 0; i < wlist.size(); ++i) {
        FbTk::RefCount<FbTk::Command<void> > sendto_cmd(new SendToCmd(i, false));
        FbTk::RefCount<FbTk::Command<void> > sendto_follow_cmd(new SendToCmd(i, true));

        FbTk::MultiButtonMenuItem* item = new FbTk::MultiButtonMenuItem(3, wlist[i]->name());
        item->setCommand(1, sendto_cmd);
        item->setCommand(2, sendto_follow_cmd);
        item->setCommand(3, sendto_cmd);
        insertItem(item);
    }

    updateMenu();
}

void SendToMenu::show() {
    if (FbMenu::window() != 0) {
        for (unsigned int i=0; i < numberOfItems(); ++i)
            setItemEnabled(i, true);
        // update the workspace for the current window
        setItemEnabled(FbMenu::window()->workspaceNumber(), false);
        updateMenu();
    }
    FbTk::Menu::show();
}

