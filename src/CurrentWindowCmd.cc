// CurrentWindowCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "CurrentWindowCmd.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "Screen.hh"
#include "WinClient.hh"

#include "FocusControl.hh"

void WindowHelperCmd::execute() {
    m_win = 0;
    if (WindowCmd<void>::window() || FocusControl::focusedFbWindow())
        real_execute();
}

void WindowHelperCmd::execute(FluxboxWindow &win) {
    m_win = &win;
    real_execute();
}

FluxboxWindow &WindowHelperCmd::fbwindow() {
    // will exist from execute above
    if (m_win)
        return *m_win;
    FluxboxWindow *tmp = WindowCmd<void>::window();
    if (tmp) return *tmp;
    return *FocusControl::focusedFbWindow();
}

void CurrentWindowCmd::real_execute() {
    (fbwindow().*m_action)();
}

void SetHeadCmd::real_execute() {
    fbwindow().setOnHead(m_head);
}

void SendToWorkspaceCmd::real_execute() {
    fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow(), false);
}

void SendToNextWorkspaceCmd::real_execute() {
    const int ws_nr =
        ( fbwindow().workspaceNumber() + m_delta ) %
          fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow(), false);
}

void SendToPrevWorkspaceCmd::real_execute() {
    int ws_nr = (fbwindow().workspaceNumber() - m_delta );
    if ( ws_nr < 0 )
        ws_nr += fbwindow().screen().numberOfWorkspaces();

    ws_nr = ws_nr % fbwindow().screen().numberOfWorkspaces();

    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow(), false);
}

void TakeToWorkspaceCmd::real_execute() {
    fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow());
}

void TakeToNextWorkspaceCmd::real_execute() {
    unsigned int ws_nr =
        ( fbwindow().workspaceNumber() + m_delta) %
          fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow());
}

void TakeToPrevWorkspaceCmd::real_execute() {
    int ws_nr = (fbwindow().workspaceNumber() - m_delta);
    if ( ws_nr < 0 )
        ws_nr += fbwindow().screen().numberOfWorkspaces();

    ws_nr = ws_nr % fbwindow().screen().numberOfWorkspaces();

    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow());
}

void GoToTabCmd::real_execute() {
    int num = m_tab_num;
    if (num < 0) num += fbwindow().numClients() + 1;
    if (num < 1) num = 1;
    if (num > fbwindow().numClients()) num = fbwindow().numClients();

    FluxboxWindow::ClientList::iterator it = fbwindow().clientList().begin();

    while (--num > 0) ++it;

    (*it)->focus();
}

void StartMovingCmd::real_execute() {
    const XEvent &last = Fluxbox::instance()->lastEvent();
    if (last.type == ButtonPress) {
        const XButtonEvent &be = last.xbutton;
        fbwindow().startMoving(be.x_root, be.y_root);
    }
}

void StartResizingCmd::real_execute() {
    const XEvent &last = Fluxbox::instance()->lastEvent();
    if (last.type == ButtonPress) {
        const XButtonEvent &be = last.xbutton;
        int x = be.x_root - fbwindow().x()
                - fbwindow().frame().window().borderWidth();
        int y = be.y_root - fbwindow().y()
                - fbwindow().frame().window().borderWidth();
        fbwindow().startResizing(x, y, fbwindow().getResizeDirection(x, y, m_mode));
    }
}

MoveCmd::MoveCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveCmd::real_execute() {
  fbwindow().move(
      fbwindow().x() + m_step_size_x,
      fbwindow().y() + m_step_size_y);
}

ResizeCmd::ResizeCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeCmd::real_execute() {

    int w = std::max<int>(static_cast<int>(fbwindow().width() +
                                      m_step_size_x * fbwindow().winClient().width_inc),
                     fbwindow().frame().titlebarHeight() * 2 + 10);
    int h = std::max<int>(static_cast<int>(fbwindow().height() +
                                      m_step_size_y * fbwindow().winClient().height_inc),
                     fbwindow().frame().titlebarHeight() + 10);
    fbwindow().resize(w, h);
}

MoveToCmd::MoveToCmd(const int step_size_x, const int step_size_y, const unsigned int refc) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y), m_refc(refc) { }

void MoveToCmd::real_execute() {
    int x = 0;
    int y = 0;

    const int head = fbwindow().screen().getHead(fbwindow().fbWindow());

    if (m_refc & MoveToCmd::LOWER)
        y = fbwindow().screen().maxBottom(head) - fbwindow().height() - 2 * fbwindow().frame().window().borderWidth() - m_step_size_y;
    if (m_refc & MoveToCmd::UPPER)
        y = fbwindow().screen().maxTop(head) + m_step_size_y;
    if (m_refc & MoveToCmd::RIGHT)
        x = fbwindow().screen().maxRight(head) - fbwindow().width() - 2 * fbwindow().frame().window().borderWidth() - m_step_size_x;
    if (m_refc & MoveToCmd::LEFT)
        x = fbwindow().screen().maxLeft(head) + m_step_size_x;

    if (m_refc & MoveToCmd::IGNORE_X)
        x = fbwindow().x();
    if (m_refc & MoveToCmd::IGNORE_Y)
        y = fbwindow().y();

    fbwindow().move(x, y);
}


ResizeToCmd::ResizeToCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeToCmd::real_execute() {
    if (m_step_size_x > 0 && m_step_size_y > 0)
        fbwindow().resize(m_step_size_x, m_step_size_y);
}

FullscreenCmd::FullscreenCmd() { }
void FullscreenCmd::real_execute() {
    fbwindow().setFullscreen(!fbwindow().isFullscreen());
}

SetAlphaCmd::SetAlphaCmd(int focused, bool relative,
                         int unfocused, bool un_relative) :
    m_focus(focused), m_unfocus(unfocused),
    m_relative(relative), m_un_relative(un_relative) { }

void SetAlphaCmd::real_execute() {
    if (m_focus == 256 && m_unfocus == 256) {
        // made up signal to return to default
        fbwindow().setUseDefaultAlpha(true);
        return;
    }

    int new_alpha;
    if (m_relative) {
        new_alpha = fbwindow().getFocusedAlpha() + m_focus;
        if (new_alpha < 0) new_alpha = 0;
        if (new_alpha > 255) new_alpha = 255;
        fbwindow().setFocusedAlpha(new_alpha);
    } else
        fbwindow().setFocusedAlpha(m_focus);

    if (m_un_relative) {
        new_alpha = fbwindow().getUnfocusedAlpha() + m_unfocus;
        if (new_alpha < 0) new_alpha = 0;
        if (new_alpha > 255) new_alpha = 255;
        fbwindow().setUnfocusedAlpha(new_alpha);
    } else
        fbwindow().setUnfocusedAlpha(m_unfocus);
}
