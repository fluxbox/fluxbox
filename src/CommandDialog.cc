// CommandDialog.cc for Fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: CommandDialog.cc,v 1.1 2003/12/19 03:53:21 fluxgen Exp $

#include "CommandDialog.hh"

#include "Screen.hh"
#include "FbWinFrameTheme.hh"
#include "WinClient.hh"
#include "CommandParser.hh"
#include "fluxbox.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/App.hh"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <iostream>
#include <memory>
using namespace std;

//!!
//!! TODO: Fix so it uses the same font as in the theme
//!!
CommandDialog::CommandDialog(BScreen &screen, const std::string &title):
    FbWindow(screen.rootWindow().screenNumber(),
             0, 0, 1, 1, 0),
    m_font("fixed"),
    m_textbox(*this, m_font, ""),
    m_label(*this, m_font, title),    
    m_gc(m_textbox),
    m_screen(screen),
    m_move_x(0),
    m_move_y(0),
    m_pixmap(0) {
    init();

}
CommandDialog::CommandDialog(BScreen &screen, const std::string &title, const std::string &precommand):
    FbWindow(screen.rootWindow().screenNumber(),
             0, 0, 1, 1, 0),
    m_font("fixed"),
    m_textbox(*this, m_font, ""),
    m_label(*this, m_font, title),    
    m_gc(m_textbox),
    m_screen(screen),
    m_move_x(0),
    m_move_y(0),
    m_pixmap(0),
    m_precommand(precommand) {
    init();

}

CommandDialog::~CommandDialog() {
    FbTk::EventManager::instance()->remove(*this);
    hide();
    if (m_pixmap != 0)
        m_screen.imageControl().removeImage(m_pixmap);
}

void CommandDialog::setText(const std::string &text) {
    m_textbox.setText(text);
}

void CommandDialog::show() {
    FbTk::FbWindow::show();
    m_textbox.setInputFocus();
    m_textbox.clear();
    m_label.clear();
}

void CommandDialog::hide() {
    FbTk::FbWindow::hide();

    // return focus to fluxbox window
    if (Fluxbox::instance()->getFocusedWindow() &&
        Fluxbox::instance()->getFocusedWindow()->fbwindow())
        Fluxbox::instance()->getFocusedWindow()->fbwindow()->setInputFocus();

}

void CommandDialog::buttonPressEvent(XButtonEvent &event) {
    m_textbox.setInputFocus();
    m_move_x = event.x_root - x();
    m_move_y = event.y_root - y();
}

void CommandDialog::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify && event.xconfigure.window != window()) {
        moveResize(event.xconfigure.x, event.xconfigure.y,
                   event.xconfigure.width, event.xconfigure.height);
    } else if (event.type == DestroyNotify)
        delete this;
}

void CommandDialog::motionNotifyEvent(XMotionEvent &event) {
    int new_x = event.x_root - m_move_x;
    int new_y = event.y_root - m_move_y;
    move(new_x, new_y);
}

void CommandDialog::keyPressEvent(XKeyEvent &event) {
    if (event.state)
        return;

    KeySym ks;
    char keychar[1];
    XLookupString(&event, keychar, 1, &ks, 0);

    if (ks == XK_Return) {
        // create command from line
        std::auto_ptr<FbTk::Command> cmd(CommandParser::instance().
            parseLine(m_precommand + m_textbox.text()));
        if (cmd.get())
            cmd->execute();
        delete this; // end this
    } else if (ks == XK_Escape)
        delete this; // end this
}

void CommandDialog::render() {
    Pixmap tmp = m_pixmap;
    if (!m_screen.winFrameTheme().labelFocusTexture().usePixmap()) {
        m_label.setBackgroundColor(m_screen.winFrameTheme().labelFocusTexture().color());
        m_pixmap = 0;
    } else {
        m_pixmap = m_screen.imageControl().renderImage(m_label.width(), m_label.height(),
                                                       m_screen.winFrameTheme().labelFocusTexture());
        m_label.setBackgroundPixmap(m_pixmap);
    }

    if (tmp)
        m_screen.imageControl().removeImage(tmp);

}

void CommandDialog::init() {

    m_label.moveResize(0, 0,
                       200, m_font.height() + 2);
    // we listen to motion notify too
    m_label.setEventMask(m_label.eventMask() | ButtonPressMask | ButtonMotionMask); 

    m_label.setGC(m_screen.winFrameTheme().labelTextFocusGC());
    render();
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
    move((m_screen.width() - width())/2, (m_screen.height() - height())/2);

    // we need ConfigureNotify from children
    FbTk::EventManager::instance()->addParent(*this, *this);
}
