// TextDialog.cc
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "TextDialog.hh"

#include "Screen.hh"
#include "FbWinFrameTheme.hh"
#include "fluxbox.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/KeyUtil.hh"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <memory>

using std::string;

/**
 * This is an abstract class providing a text box dialog
 */

TextDialog::TextDialog(BScreen &screen,
        const string &title) :
    FbTk::FbWindow(screen.rootWindow().screenNumber(), 0, 0, 200, 1, ExposureMask),
    m_textbox(*this, screen.focusedWinFrameTheme()->font(), ""),
    m_label(*this, screen.focusedWinFrameTheme()->font(), title),
    m_gc(m_textbox),
    m_screen(screen),
    m_move_x(0),
    m_move_y(0),
    m_pixmap(0){
    init();
}


TextDialog::~TextDialog() {
    FbTk::EventManager::instance()->remove(*this);
    hide();
    if (m_pixmap != 0)
        m_screen.imageControl().removeImage(m_pixmap);
}


void TextDialog::setText(const string &text) {
    m_textbox.setText(text);
}

void TextDialog::show() {
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

void TextDialog::hide() {
    FbTk::FbWindow::hide();
    Fluxbox::instance()->setShowingDialog(false);
}

void TextDialog::exposeEvent(XExposeEvent &event) {
    if (event.window == window())
        clearArea(event.x, event.y, event.width, event.height);
}

void TextDialog::buttonPressEvent(XButtonEvent &event) {
    m_textbox.setInputFocus();
    m_move_x = event.x_root - x();
    m_move_y = event.y_root - y();
}

void TextDialog::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify && event.xconfigure.window != window()) {
        moveResize(event.xconfigure.x, event.xconfigure.y,
                   event.xconfigure.width, event.xconfigure.height);
    } else if (event.type == DestroyNotify)
        delete this;
}

void TextDialog::motionNotifyEvent(XMotionEvent &event) {
    int new_x = event.x_root - m_move_x;
    int new_y = event.y_root - m_move_y;
    move(new_x, new_y);
}

void TextDialog::keyPressEvent(XKeyEvent &event) {
    unsigned int state = FbTk::KeyUtil::instance().isolateModifierMask(event.state);
    if (state)
        return;

    KeySym ks;
    char keychar;
    XLookupString(&event, &keychar, 1, &ks, 0);

    if (ks == XK_Return) {
    	exec(m_textbox.text());
        delete this;
    } else if (ks == XK_Escape)
        delete this; // end this
    else if (ks == XK_Tab) {
        // try to expand a command
        tabComplete();
    }

}

void TextDialog::render() {
    Pixmap tmp = m_pixmap;
    if (!m_screen.focusedWinFrameTheme()->iconbarTheme().texture().usePixmap()) {
        m_label.setBackgroundColor(m_screen.focusedWinFrameTheme()->iconbarTheme().texture().color());
        m_pixmap = 0;
    } else {
        m_pixmap = m_screen.imageControl().renderImage(m_label.width(), m_label.height(),
                                                       m_screen.focusedWinFrameTheme()->iconbarTheme().texture());
        m_label.setBackgroundPixmap(m_pixmap);
    }

    if (tmp)
        m_screen.imageControl().removeImage(tmp);

}

void TextDialog::init() {

    // setup label
    // we listen to motion notify too
    m_label.setEventMask(m_label.eventMask() | ButtonPressMask | ButtonMotionMask);
    m_label.setGC(m_screen.focusedWinFrameTheme()->iconbarTheme().text().textGC());
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

void TextDialog::updateSizes() {
    m_label.moveResize(0, 0,
                       width(), m_textbox.font().height() + 2);

    m_textbox.moveResize(2, m_label.height(),
                         width() - 4, m_textbox.font().height() + 2);
}
