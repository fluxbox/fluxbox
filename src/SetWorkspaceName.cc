// SetWorkspaceName.cc for Fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: SetWorkspaceName.cc,v 1.2 2003/08/27 18:05:12 fluxgen Exp $

#include "SetWorkspaceName.hh"

#include "Screen.hh"
#include "Workspace.hh"
#include "WinClient.hh"
#include "FbWinFrameTheme.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/App.hh"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <iostream>
using namespace std;

SetWorkspaceName::SetWorkspaceName(BScreen &screen):
    FbWindow(screen.rootWindow().screenNumber(),
             0, 0, 1, 1, 0),
    m_textbox(*this, m_font, screen.currentWorkspace()->name()),
    m_label(*this, m_font, "Set workspace name:"),
    m_font("fixed"),
    m_gc(m_textbox),
    m_screen(screen),
    m_move_x(0),
    m_move_y(0) {


    m_label.setGC(screen.winFrameTheme().labelTextFocusGC());
    m_label.setBackgroundColor(screen.winFrameTheme().labelFocusTexture().color());
    m_label.moveResize(0, 0,
                       200, m_font.height() + 2);
    m_label.setEventMask(m_label.eventMask() | ButtonPressMask | ButtonMotionMask); // we listen to motion notify too
    m_label.show();

    m_textbox.setBackgroundColor(FbTk::Color("white", m_textbox.screenNumber()));

    FbTk::Color black("black", m_textbox.screenNumber());
    m_gc.setForeground(black);

    m_textbox.setGC(m_gc.gc());

    m_textbox.moveResize(0, m_label.height(),
                         200, m_font.height() + 2);    
    m_textbox.show();

    resize(200, m_textbox.height() + m_label.height());


    // move to center of the screen
    move((screen.width() - width())/2, (screen.height() - height())/2);

    // we need ConfigureNotify from children
    FbTk::EventManager::instance()->addParent(*this, *this);
}

SetWorkspaceName::~SetWorkspaceName() {
    FbTk::EventManager::instance()->remove(*this);
    hide();
}

void SetWorkspaceName::show() {
    FbTk::FbWindow::show();
    m_textbox.setInputFocus();
    m_textbox.setText(m_screen.currentWorkspace()->name());
    m_textbox.clear();
    m_label.clear();
}

void SetWorkspaceName::hide() {
    FbTk::FbWindow::hide();

    // return focus to fluxbox window
    if (Fluxbox::instance()->getFocusedWindow() &&
        Fluxbox::instance()->getFocusedWindow()->fbwindow())
        Fluxbox::instance()->getFocusedWindow()->fbwindow()->setInputFocus();

}

void SetWorkspaceName::buttonPressEvent(XButtonEvent &event) {
    m_textbox.setInputFocus();
    m_move_x = event.x_root - x();
    m_move_y = event.y_root - y();
}

void SetWorkspaceName::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify && event.xconfigure.window != window()) {
        moveResize(event.xconfigure.x, event.xconfigure.y,
                   event.xconfigure.width, event.xconfigure.height);
    } else if (event.type == DestroyNotify)
        delete this;
}

void SetWorkspaceName::motionNotifyEvent(XMotionEvent &event) {
    int new_x = event.x_root - m_move_x;
    int new_y = event.y_root - m_move_y;
    move(new_x, new_y);
}

void SetWorkspaceName::keyPressEvent(XKeyEvent &event) {
    if (event.state)
        return;

    KeySym ks;
    char keychar[1];
    XLookupString(&event, keychar, 1, &ks, 0);

    if (ks == XK_Return) {
        m_screen.currentWorkspace()->setName(m_textbox.text());
        m_screen.updateWorkspaceNamesAtom();
        Fluxbox::instance()->save_rc();
        delete this; // end this
    } else if (ks == XK_Escape)
        delete this; // end this
}
