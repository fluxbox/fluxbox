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

// $Id: CurrentWindowCmd.cc,v 1.2 2003/07/26 13:44:00 rathnor Exp $

#include "CurrentWindowCmd.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Screen.hh"

CurrentWindowCmd::CurrentWindowCmd(Action act):m_action(act) { }

void CurrentWindowCmd::execute() {
    Fluxbox *fb = Fluxbox::instance();
    if (fb->getFocusedWindow() != 0)
        (*fb->getFocusedWindow().*m_action)();
}


void KillWindowCmd::real_execute() {
    XKillClient(FbTk::App::instance()->display(), window().clientWindow());
}

void SendToWorkspaceCmd::real_execute() {
    if (m_workspace_num >= 0 && m_workspace_num < window().screen().getNumberOfWorkspaces())
        window().screen().sendToWorkspace(m_workspace_num, &window());
}

void WindowHelperCmd::execute() {
    if (Fluxbox::instance()->getFocusedWindow())
        real_execute();
}

FluxboxWindow &WindowHelperCmd::window() {
    return *Fluxbox::instance()->getFocusedWindow();
}

MoveLeftCmd::MoveLeftCmd(int step_size):MoveHelper(step_size) { }
void MoveLeftCmd::real_execute() {
    window().move(window().x() - stepSize(), window().y());
}

MoveRightCmd::MoveRightCmd(int step_size):MoveHelper(step_size) { }
void MoveRightCmd::real_execute() {
    window().move(window().x() + stepSize(), window().y());
}

MoveDownCmd::MoveDownCmd(int step_size):MoveHelper(step_size) { }
void MoveDownCmd::real_execute() {
    window().move(window().x(), window().y() + stepSize());
}

MoveUpCmd::MoveUpCmd(int step_size):MoveHelper(step_size) { }
void MoveUpCmd::real_execute() {
    window().move(window().x(), window().y() - stepSize());
}

