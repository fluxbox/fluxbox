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

// $Id: SendToMenu.cc,v 1.1 2003/11/27 21:57:17 fluxgen Exp $

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
        m_win.screen().sendToWorkspace(m_workspace, &m_win);
    }
private:
    FluxboxWindow &m_win;
    const int m_workspace;
};

SendToMenu::SendToMenu(FluxboxWindow &win):
    FbMenu(*win.screen().menuTheme(), win.screen().screenNumber(),
           win.screen().imageControl(), 
           *win.screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
    m_win(win) {

    win.screen().workspaceCountSig().attach(this);
    win.screen().workspaceNamesSig().attach(this);

    disableTitle();
    update(0);
}

void SendToMenu::update(FbTk::Subject *subj) {
    // rebuild menu

    removeAll();

    const BScreen::Workspaces &wlist = m_win.screen().getWorkspacesList();
    for (size_t i = 0; i < wlist.size(); ++i) {
        FbTk::RefCount<FbTk::Command> sendto_cmd(new SendToCmd(m_win, i));
        insert(wlist[i]->name().c_str(), sendto_cmd);
    }

    FbMenu::update();
}
