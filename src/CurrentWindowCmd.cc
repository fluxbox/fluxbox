// CurrentWindowCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: CurrentWindowCmd.cc,v 1.3 2003/07/28 15:06:33 rathnor Exp $

#include "CurrentWindowCmd.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"

CurrentWindowCmd::CurrentWindowCmd(Action act):m_action(act) { }

void CurrentWindowCmd::execute() {
    WinClient *client = Fluxbox::instance()->getFocusedWindow();
    if (client && client->fbwindow())
        (client->fbwindow()->*m_action)();
}


void KillWindowCmd::real_execute() {
    winclient().sendClose(true);
}

void SendToWorkspaceCmd::real_execute() {
    if (m_workspace_num >= 0 && m_workspace_num < fbwindow().screen().getNumberOfWorkspaces())
        fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow());
}

void WindowHelperCmd::execute() {
    WinClient *client = Fluxbox::instance()->getFocusedWindow();
    if (client && client->fbwindow()) // guarantee that fbwindow() exists too
        real_execute();
}

WinClient &WindowHelperCmd::winclient() {
    // will exist from execute above
    return *Fluxbox::instance()->getFocusedWindow();
}

FluxboxWindow &WindowHelperCmd::fbwindow() {
    // will exist from execute above
    return *Fluxbox::instance()->getFocusedWindow()->fbwindow();
}

MoveLeftCmd::MoveLeftCmd(int step_size):MoveHelper(step_size) { }
void MoveLeftCmd::real_execute() {
    fbwindow().move(fbwindow().x() - stepSize(),
                    fbwindow().y());
}

MoveRightCmd::MoveRightCmd(int step_size):MoveHelper(step_size) { }
void MoveRightCmd::real_execute() {
    fbwindow().move(fbwindow().x() + stepSize(),
                    fbwindow().y());
}

MoveDownCmd::MoveDownCmd(int step_size):MoveHelper(step_size) { }
void MoveDownCmd::real_execute() {
    fbwindow().move(fbwindow().x(), fbwindow().y() + stepSize());
}

MoveUpCmd::MoveUpCmd(int step_size):MoveHelper(step_size) { }
void MoveUpCmd::real_execute() {
    fbwindow().move(fbwindow().x(), fbwindow().y() - stepSize());
}

