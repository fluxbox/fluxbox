// SendToMenu.cc for Fluxbox
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

// $Id$

#include "SendToMenu.hh"

#include "Window.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "Workspace.hh"

#include "FbTk/Command.hh"

class SendToCmd: public FbTk::Command {
public:
    SendToCmd(FluxboxWindow &win, int workspace):
        m_win(win),
        m_workspace(workspace) { }
    void execute() {
        m_win.screen().sendToWorkspace(m_workspace, &m_win, false);
    }
private:
    FluxboxWindow &m_win;
    const int m_workspace;
};

SendToMenu::SendToMenu(FluxboxWindow &win):
    FbMenu(win.screen().menuTheme(),
           win.screen().imageControl(), 
           *win.screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
    m_win(win) {
    // listen to:
    // workspace count signal
    // workspace names signal
    // current workspace signal
    // and window's workspace sig
    win.screen().workspaceCountSig().attach(this);
    win.screen().workspaceNamesSig().attach(this);
    win.screen().currentWorkspaceSig().attach(this);
    win.workspaceSig().attach(this);

    disableTitle();
    // build menu
    updateMenu(0);
}

void SendToMenu::update(FbTk::Subject *subj) {
    if (subj != 0) {
        // if workspace changed we enable all workspaces except the current one
        if (subj == &(m_win.screen().currentWorkspaceSig()) || 
            subj == &(m_win.workspaceSig())) {
            // enabled all workspaces
            const BScreen::Workspaces &wlist = m_win.screen().getWorkspacesList();
            for (size_t i = 0; i < wlist.size(); ++i)
                setItemEnabled(i, true);
            // disable send to on the workspace which the window exist
            setItemEnabled(m_win.workspaceNumber(), false);
            updateMenu();
            // we're done
            return;
        } else if (subj == &(theme().reconfigSig())) {
            // we got reconfig Theme signal, let base menu handle it 
            FbTk::Menu::update(subj);
            return;
        }
        
    }
    // rebuild menu

    removeAll();

    const BScreen::Workspaces &wlist = m_win.screen().getWorkspacesList();
    for (size_t i = 0; i < wlist.size(); ++i) {
        FbTk::RefCount<FbTk::Command> sendto_cmd(new SendToCmd(m_win, i));
        insert(wlist[i]->name().c_str(), sendto_cmd);

    }

    setItemEnabled(m_win.workspaceNumber(), false);

    updateMenu();
}
