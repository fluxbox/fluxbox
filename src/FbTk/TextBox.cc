// TextBox.cc for FbTk - fluxbox toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: TextBox.cc,v 1.6 2004/01/08 22:02:52 fluxgen Exp $

#include "TextBox.hh"
#include "Font.hh"
#include "EventManager.hh"
#include "App.hh"
#include "KeyUtil.hh"

#include <cctype>
#include <X11/keysym.h>
#include <X11/Xutil.h>

namespace FbTk {

TextBox::TextBox(int screen_num,
                 const Font &font, const std::string &text):
    FbWindow(screen_num, 0, 0, 1, 1, ExposureMask | KeyPressMask | ButtonPressMask),
    m_font(&font),
    m_text(text),
    m_gc(0),
    m_cursor_pos(0),
    m_start_pos(0),
    m_end_pos(0) {

    FbTk::EventManager::instance()->add(*this, *this);
}

TextBox::TextBox(const FbWindow &parent, 
                 const Font &font, const std::string &text):
    FbWindow(parent, 0, 0, 1, 1, ExposureMask | KeyPressMask | ButtonPressMask),
    m_font(&font),
    m_text(text),
    m_gc(0),
    m_cursor_pos(0),
    m_start_pos(0),
    m_end_pos(0) {

    FbTk::EventManager::instance()->add(*this, *this);
}

TextBox::~TextBox() {

}

void TextBox::setText(const std::string &text) {
    m_text = text;
    cursorEnd();
    adjustStartPos();
}

void TextBox::setFont(const Font &font) {
    m_font = &font;
}

void TextBox::setGC(GC gc) {
    m_gc = gc;
}

void TextBox::setInputFocus() {
    XSetInputFocus(FbTk::App::instance()->display(),
                   window(),
                   RevertToParent,
                   CurrentTime);
}

void TextBox::cursorHome() {
    m_start_pos = m_cursor_pos = 0;
}

void TextBox::cursorEnd() {
    m_cursor_pos = m_end_pos = text().size();
}

void TextBox::cursorForward() {
    if (m_start_pos + cursorPosition() < m_end_pos)
        m_cursor_pos++;
    else if (m_end_pos < text().size()) {
        m_cursor_pos++;
        m_end_pos++;
        adjustStartPos();
    }
}

void TextBox::cursorBackward() {
    if (cursorPosition())
        m_cursor_pos--;
    else if (m_start_pos) {
        m_start_pos--;
        adjustEndPos();
    }
}

void TextBox::backspace() {
    if (m_start_pos || cursorPosition()) {
        m_text.erase(m_start_pos + cursorPosition() - 1, 1);
        if (cursorPosition())
            setCursorPosition(cursorPosition() - 1);
        else
            m_start_pos--;
        adjustEndPos();
    }
}

void TextBox::deleteForward() {
    if (m_start_pos + m_cursor_pos < m_end_pos) {
        m_text.erase(m_start_pos + m_cursor_pos, 1);
        adjustEndPos();
    }
}

void TextBox::insertText(const std::string &val) {
    m_text.insert(m_start_pos + cursorPosition(), val);
    m_cursor_pos += val.size();
    m_end_pos += val.size();
    if (m_start_pos + cursorPosition() < m_end_pos)
        adjustEndPos();
    else
        adjustStartPos();
}

void TextBox::killToEnd() {
    if (cursorPosition() >= 0 && cursorPosition() < static_cast<signed>(text().size())) {
        m_text.erase(cursorPosition());
        setText(m_text);
    }
}

void TextBox::clear() {
    FbWindow::clear();
    // center text by default
    int center_pos = (height() + font().ascent())/2;
    if (gc() == 0)
        setGC(DefaultGC(FbTk::App::instance()->display(), screenNumber()));

    font().drawText(window(), screenNumber(), 
                    gc(),                     
                    text().c_str() + m_start_pos, 
                    m_end_pos - m_start_pos, 
                    0, center_pos); // pos

    // draw cursor position
    int cursor_pos = font().textWidth(text().c_str() + m_start_pos, m_cursor_pos) + 1;
    drawLine(gc(), cursor_pos, 0, cursor_pos, font().height());
}

void TextBox::moveResize(int x, int y,
                         unsigned int width, unsigned int height) {
    FbWindow::moveResize(x, y, width, height);
    clear();
}

void TextBox::resize(unsigned int width, unsigned int height) {
    FbWindow::resize(width, height);
    clear();
}

void TextBox::exposeEvent(XExposeEvent &event) {
    clear();
}

void TextBox::buttonPressEvent(XButtonEvent &event) {
    setInputFocus();
}

void TextBox::keyPressEvent(XKeyEvent &event) {
    // strip numlock and scrolllock mask
    event.state = KeyUtil::instance().cleanMods(event.state);

    KeySym ks;
    char keychar[1];
    XLookupString(&event, keychar, 1, &ks, 0);
    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks)) return;

    if (event.state) { // handle keybindings without state
        if (event.state == ControlMask) {

            switch (ks) {
            case XK_b:
                cursorBackward();
                break;
            case XK_f:
                cursorForward();
                break;
            case XK_a:
                cursorHome();
                break;
            case XK_e:
                cursorEnd();
                break;
            case XK_d:
                deleteForward();
                break;
            case XK_k:
                killToEnd();
                break;
            }
        } else if (event.state == ShiftMask) {
            if (isprint(keychar[0])) {
                std::string val;
                val += keychar[0];
                insertText(val);
            }
        }

    } else { // no state

        switch (ks) {
        case XK_BackSpace:
            backspace();
            break;
        case XK_Home:
            cursorHome();
            break;
        case XK_End:
            cursorEnd();
            break;
        case XK_Left:
            cursorBackward();
            break;
        case XK_Right:
            cursorForward();
            break;
        default:
            if (isprint(keychar[0])) {
                std::string val;
                val += keychar[0];
                insertText(val);
            }
        }
    }
    clear();
}

void TextBox::setCursorPosition(int pos) {
    m_cursor_pos = pos < 0 ? 0 : pos;
    if (m_cursor_pos > text().size())
        cursorEnd();
}

void TextBox::adjustEndPos() {
    m_end_pos = text().size();
    int text_width = font().textWidth(text().c_str() + m_start_pos, m_end_pos - m_start_pos);
    while (text_width > static_cast<signed>(width())) {
        m_end_pos--;
        text_width = font().textWidth(text().c_str() + m_start_pos, m_end_pos - m_start_pos);
    }
}

void TextBox::adjustStartPos() {
    int text_width = font().textWidth(text().c_str() + m_start_pos, m_end_pos - m_start_pos);
    if (text_width < static_cast<signed>(width()))
        return;

    int start_pos = 0;
    text_width = font().textWidth(text().c_str() + start_pos, m_end_pos - start_pos);
    while (text_width > static_cast<signed>(width())) {
        start_pos++;
        text_width = font().textWidth(text().c_str() + start_pos, m_end_pos - start_pos);
    }
    // adjust cursorPosition() according relative to change to m_start_pos
    m_cursor_pos -= start_pos - m_start_pos;
    m_start_pos = start_pos;
}



}; // end namespace FbTk
