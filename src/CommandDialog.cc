// CommandDialog.cc for Fluxbox
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "CommandDialog.hh"

#include "Screen.hh"
#include "FbWinFrameTheme.hh"
#include "WinClient.hh"
#include "FbTk/CommandRegistry.hh"
#include "FocusControl.hh"
#include "fluxbox.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/App.hh"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <memory>
#include <stdexcept>

using std::string;
using std::vector;
using std::auto_ptr;
using std::less;
using std::out_of_range;

CommandDialog::CommandDialog(BScreen &screen,
        const string &title, const string precommand) :
    FbTk::FbWindow(screen.rootWindow().screenNumber(), 0, 0, 200, 1, ExposureMask),
    m_textbox(*this, screen.winFrameTheme().font(), ""),
    m_label(*this, screen.winFrameTheme().font(), title),
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

void CommandDialog::setText(const string &text) {
    m_textbox.setText(text);
}

void CommandDialog::show() {
    FbTk::FbWindow::show();
    m_textbox.setInputFocus();
    m_label.clear();
    Fluxbox::instance()->setShowingDialog(true);
    // resize to correct width, which should be the width of label text
    // no need to truncate label text in this dialog
    // but if label text size < 200 we set 200
    if (m_label.textWidth() < 200)
        return;
    else {
        resize(m_label.textWidth(), height());
        updateSizes();
        render();
    }
}

void CommandDialog::hide() {
    FbTk::FbWindow::hide();
    Fluxbox::instance()->setShowingDialog(false);

    // return focus to fluxbox window
    if (FocusControl::focusedFbWindow())
        FocusControl::focusedFbWindow()->focus();

}

void CommandDialog::exposeEvent(XExposeEvent &event) {
    if (event.window == window())
        clearArea(event.x, event.y, event.width, event.height);
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
    unsigned int state = FbTk::KeyUtil::instance().isolateModifierMask(event.state);
    if (state)
        return;

    KeySym ks;
    char keychar;
    XLookupString(&event, &keychar, 1, &ks, 0);

    if (ks == XK_Return) {
        hide(); // hide and return focus to a FluxboxWindow
        // create command from line
        auto_ptr<FbTk::Command> cmd(FbTk::CommandRegistry::instance().
            parseLine(m_precommand + m_textbox.text()));
        if (cmd.get())
            cmd->execute();
        // post execute
        if (*m_postcommand != 0)
            m_postcommand->execute();

        delete this; // end this
    } else if (ks == XK_Escape)
        delete this; // end this
    else if (ks == XK_Tab) {
        // try to expand a command
        tabComplete();
    }
}

void CommandDialog::tabComplete() {
    try {
        string::size_type first = m_textbox.text().find_last_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                                    "abcdefghijklmnopqrstuvwxyz"
                                                                    "0123456789",
                                                                    m_textbox.cursorPosition());
        if (first == string::npos)
            first = 0;
        string prefix = FbTk::StringUtil::toLower(m_textbox.text().substr(first, m_textbox.cursorPosition()));
        if (prefix.empty()) {
            XBell(FbTk::App::instance()->display(), 0);
            return;
        }

        FbTk::CommandRegistry::CreatorMap::const_iterator it = FbTk::CommandRegistry::instance().commandMap().begin();
        const FbTk::CommandRegistry::CreatorMap::const_iterator it_end = FbTk::CommandRegistry::instance().commandMap().end();
        vector<string> matches;
        for (; it != it_end; ++it) {
            if ((*it).first.find(prefix) == 0) {
                matches.push_back((*it).first);
            }
        }

        if (!matches.empty()) {
            // sort and apply larges match
            sort(matches.begin(), matches.end(), less<string>());
            m_textbox.setText(m_textbox.text() + matches[0].substr(prefix.size()));
        } else
            XBell(FbTk::App::instance()->display(), 0);

    } catch (out_of_range &oor) {
        XBell(FbTk::App::instance()->display(), 0);
    }
}

void CommandDialog::render() {
    Pixmap tmp = m_pixmap;
    if (!m_screen.winFrameTheme().iconbarTheme().focusedTexture().usePixmap()) {
        m_label.setBackgroundColor(m_screen.winFrameTheme().iconbarTheme().focusedTexture().color());
        m_pixmap = 0;
    } else {
        m_pixmap = m_screen.imageControl().renderImage(m_label.width(), m_label.height(),
                                                       m_screen.winFrameTheme().iconbarTheme().focusedTexture());
        m_label.setBackgroundPixmap(m_pixmap);
    }

    if (tmp)
        m_screen.imageControl().removeImage(tmp);

}

void CommandDialog::init() {


    // setup label
    // we listen to motion notify too
    m_label.setEventMask(m_label.eventMask() | ButtonPressMask | ButtonMotionMask);
    m_label.setGC(m_screen.winFrameTheme().iconbarTheme().focusedText().textGC());
    m_label.show();

    // setup text box
    FbTk::Color white("white", m_textbox.screenNumber());
    m_textbox.setBackgroundColor(white);
    FbTk::Color black("black", m_textbox.screenNumber());
    m_gc.setForeground(black);
    m_textbox.setGC(m_gc.gc());
    m_textbox.show();

    // setup this window
    setBorderWidth(1);
    setBackgroundColor(white);
    // move to center of the screen
    move((m_screen.width() - width())/2, (m_screen.height() - height())/2);

    updateSizes();
    resize(width(), m_textbox.height() + m_label.height());

    render();

    // we need ConfigureNotify from children
    FbTk::EventManager::instance()->addParent(*this, *this);
}

void CommandDialog::updateSizes() {
    m_label.moveResize(0, 0,
                       width(), m_textbox.font().height() + 2);

    m_textbox.moveResize(2, m_label.height(),
                         width() - 4, m_textbox.font().height() + 2);
}
