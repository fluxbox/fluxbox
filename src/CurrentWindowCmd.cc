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

// $Id: CurrentWindowCmd.cc,v 1.6 2003/09/10 14:07:48 fluxgen Exp $

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

MoveCmd::MoveCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveCmd::real_execute() {
  fbwindow().move(
      fbwindow().x() + m_step_size_x,
      fbwindow().y() + m_step_size_y );
}

ResizeCmd::ResizeCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeCmd::real_execute() {
  fbwindow().resize(
      fbwindow().width() + m_step_size_x * fbwindow().winClient().width_inc,
      fbwindow().height() + m_step_size_y * fbwindow().winClient().height_inc );
}
