// TextBox.cc for FbTk - fluxbox toolkit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "TextBox.hh"
#include "Font.hh"
#include "EventManager.hh"
#include "App.hh"
#include "KeyUtil.hh"

#ifdef HAVE_CCTYPE
  #include <cctype>
#else
  #include <ctype.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <iostream>

namespace FbTk {


TextBox::TextBox(int screen_num,
                 const Font &font, const std::string &text):
    FbWindow(screen_num, 0, 0, 1, 1, ExposureMask | KeyPressMask | ButtonPressMask | FocusChangeMask | KeymapStateMask),
    m_font(&font),
    m_text(text),
    m_gc(0),
    m_cursor_pos(0),
    m_start_pos(0),
    m_end_pos(0),
    m_select_pos(std::string::npos),
    m_padding(0),
    m_xic(0){

    if (App::instance()->inputModule())
        m_xic = XCreateIC(App::instance()->inputModule(), XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window(), NULL);
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
    m_end_pos(0),
    m_select_pos(std::string::npos),
    m_padding(0),
    m_xic(0){
    if (App::instance()->inputModule())
        m_xic = XCreateIC(App::instance()->inputModule(), XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window(), NULL);
    FbTk::EventManager::instance()->add(*this, *this);
}

TextBox::~TextBox() {
    if (m_xic)
        XFree(m_xic);
    m_xic = 0;
}

void TextBox::setText(const FbTk::BiDiString &text) {
    m_text = text;
    m_start_pos = 0;
    cursorEnd();
    adjustStartPos();
}

void TextBox::setFont(const Font &font) {
    m_font = &font;
}

void TextBox::setPadding(int padding) {
    m_padding = padding;
    adjustPos();
    clear();
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
    adjustEndPos();
}

void TextBox::cursorEnd() {
    m_end_pos = text().size();
    adjustStartPos();
    m_cursor_pos = m_end_pos - m_start_pos;
}

void TextBox::cursorForward() {
    StringRange r = charRange(m_start_pos + cursorPosition());
    const std::string::size_type s = r.end - r.begin + 1;

    if (hasSelection()) {
        int start = std::max(m_start_pos, std::min(m_start_pos + m_cursor_pos, m_select_pos));
        int select_length = std::max(m_start_pos + m_cursor_pos, m_select_pos) - start;
        if (select_length > static_cast<signed>(m_end_pos - m_start_pos)) {
            // shift range
            m_start_pos += s;
            m_end_pos += s;
            adjustPos();
        }
    }

    if (r.end < m_end_pos)
        m_cursor_pos = r.end + 1 - m_start_pos;
    else if (m_end_pos < text().size()) {
        m_cursor_pos = r.end + 1 - m_start_pos;
        m_end_pos += s;
        adjustStartPos();
    }
}

void TextBox::cursorBackward() {

    StringRange r = charRange(m_start_pos + cursorPosition() - 1);
    const std::string::size_type s = r.end - r.begin + 1;

    if (hasSelection()) {
       int end = std::max(m_end_pos, std::min(m_end_pos - m_cursor_pos, m_select_pos));
       int select_length = end - std::min(m_end_pos - m_cursor_pos, m_select_pos);
       if (select_length > static_cast<signed>(m_end_pos - m_start_pos)) {
           // shift range
           m_start_pos -= s;
           m_end_pos -= s;
           adjustPos();
       }
    }

    if (m_start_pos || cursorPosition()) {
        if (cursorPosition())
            m_cursor_pos = r.begin - m_start_pos;
        else if (m_start_pos) {
            m_start_pos -= s;
            adjustEndPos();
        }
    }
}

void TextBox::backspace() {
    if (hasSelection())
        return deleteForward();
    if (m_start_pos || cursorPosition()) {
        FbString t = text();
        StringRange r = charRange(m_start_pos + cursorPosition() - 1);
        const std::string::size_type s = r.end - r.begin + 1;
        t.erase(r.begin, s);
        m_text.setLogical(t);
        if (cursorPosition())
            setCursorPosition(r.begin - m_start_pos);
        else
            m_start_pos -= s;
        adjustEndPos();
    }
}

void TextBox::deleteForward() {
    std::string::size_type pos = m_start_pos + m_cursor_pos;
    int length = 1;
    bool selected = false;
    if (selected = hasSelection()) {
        pos = std::min(m_start_pos + m_cursor_pos, m_select_pos);
        length = std::max(m_start_pos + m_cursor_pos, m_select_pos) - pos;
        m_cursor_pos = pos - m_start_pos;
    } else {
        StringRange r = charRange(pos);
        pos = r.begin;
        length = r.end - r.begin + 1;
    }
    if (pos < m_end_pos) {
        FbString t = text();
        t.erase(pos, length);
        m_text.setLogical(t);
        adjustEndPos();
    }
    if (selected && length > 1)
        adjustStartPos();
}

void TextBox::insertText(const std::string &val) {
    if (hasSelection())
        deleteForward();

    FbString t = text();
    std::string::size_type pos = m_start_pos + m_cursor_pos;
    t.insert(pos ? charRange(pos - 1).end + 1 : pos, val);
    m_text.setLogical(t);
    m_cursor_pos += val.size();
    m_end_pos += val.size();

    adjustPos();
}

void TextBox::killToEnd() {
    if (cursorPosition() >= 0 && cursorPosition() < static_cast<signed>(text().size())) {
        FbString t = text();
        t.erase(cursorPosition());
        setText(t);
    }
}

void TextBox::clear() {
    Display *dpy = FbTk::App::instance()->display();
    FbWindow::clear();
    // center text by default
    int center_pos = (height() - font().height())/2 + font().ascent();
    if (gc() == 0)
        setGC(DefaultGC(dpy, screenNumber()));

    int cursor_pos = font().textWidth(m_text.visual().c_str() + m_start_pos, m_cursor_pos);

    int char_length = m_end_pos - m_start_pos;
    int text_width = font().textWidth(m_text.visual().c_str() + m_start_pos, char_length);
    int char_draw_length = char_length;
    if (text_width > static_cast<signed>(width()) - 2*m_padding) {
        // draw less text
        int char_start_current = m_start_pos;
        int char_end_current = m_end_pos;
        adjustPos();
        char_draw_length = m_end_pos - m_start_pos;
        m_start_pos = char_start_current;
        m_end_pos = char_end_current;
    }

    font().drawText(*this, screenNumber(), gc(),
                    m_text.visual().c_str() + m_start_pos,
                    char_draw_length, m_padding, center_pos); // pos

    if (hasSelection()) {
        int select_pos = m_select_pos <= m_start_pos ? 0 :
                         font().textWidth(m_text.visual().c_str() + m_start_pos,
                                          m_select_pos - m_start_pos);
        int start = std::max(m_start_pos, std::min(m_start_pos + m_cursor_pos, m_select_pos));
        int length = std::max(m_start_pos + m_cursor_pos, m_select_pos) - start;
        length = std::min(length, char_draw_length);
        int x = m_padding + std::min(select_pos, cursor_pos);
        int select_width = std::min(std::abs(select_pos - cursor_pos), text_width);

        XGCValues backup;
        XGetGCValues(dpy, gc(), GCForeground|GCBackground, &backup);
        XSetForeground(dpy, gc(), backup.foreground);

        fillRectangle(gc(), x, (height()-font().height())/2, select_width, font().height());

        XColor c;
        c.pixel = backup.foreground;
        XQueryColor(dpy, DefaultColormap(dpy, screenNumber()), &c);
        XSetForeground(dpy, gc(), c.red + c.green + c.blue > 0x17ffe ?
                                     BlackPixel(dpy, screenNumber()) :
                                     WhitePixel(dpy, screenNumber()));
        font().drawText(*this, screenNumber(), gc(),
                        m_text.visual().c_str() + start, length, x, center_pos); // pos
        XSetForeground(dpy, gc(), backup.foreground);
    }


    // draw cursor position
    drawLine(gc(), m_padding + cursor_pos, height()/2 + font().ascent()/2, m_padding + cursor_pos, height()/2 - font().ascent()/2);
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
    if (event.window == window()) {
        std::string::size_type i;
        std::string::size_type click_pos = m_end_pos;
        int delta = width();
        int tmp = 0;
        for(i = m_start_pos; i <= m_end_pos; i++) {
            tmp = abs(static_cast<int>
                      (event.x - font().textWidth(m_text.visual().c_str() + m_start_pos, i - m_start_pos) - m_padding));

            if (tmp < delta) {
                delta = tmp;
                click_pos = i;
            }
        }
        m_cursor_pos = click_pos - m_start_pos;
        m_select_pos = std::string::npos; // clear select
        adjustPos();
        clear();
    }
}

void TextBox::keyPressEvent(XKeyEvent &event) {

    event.state = KeyUtil::instance().cleanMods(event.state);

    KeySym ks;
    char keychar[20];
    if (m_xic) {
        Status status;
        int count = Xutf8LookupString(m_xic, &event, keychar, sizeof(keychar), &ks, &status);
        if (status == XBufferOverflow)
            return;
        keychar[count] = '\0';
    }
    else {
        XLookupString(&event, keychar, 1, &ks, 0);
        keychar[1] = '\0';
    }

    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks)) return;


    if (m_select_pos == std::string::npos && (event.state & ShiftMask) == ShiftMask) {
        m_select_pos = m_cursor_pos + m_start_pos;
    }

    if ((event.state & ControlMask) == ControlMask) {

        switch (ks) {
        case XK_Left: {
            unsigned int pos = findEmptySpaceLeft();
            if (pos < m_start_pos){
                m_start_pos  = pos;
                m_cursor_pos = 0;
            } else if (m_start_pos > 0) {
                m_cursor_pos = pos - m_start_pos;
            } else {
                m_cursor_pos = pos;
            }
            adjustPos();
        }
            break;
        case XK_Right:
            if (!m_text.logical().empty() && m_cursor_pos < m_text.logical().size()){
                unsigned int pos = findEmptySpaceRight();
                if (pos > m_start_pos)
                    pos -= m_start_pos;
                else
                    pos = 0;
                if (m_start_pos + pos <= m_end_pos)
                    m_cursor_pos = pos;
                else if (m_end_pos < text().size()) {
                    m_cursor_pos = pos;
                    m_end_pos = pos;
                }

                adjustPos();

            }
            break;

        case XK_BackSpace: {
                unsigned int pos = findEmptySpaceLeft();
                FbString t = text();
                t.erase(pos, m_cursor_pos - pos + m_start_pos);
                m_text.setLogical(t);

                if (pos < m_start_pos){
                    m_start_pos  = pos;
                    m_cursor_pos = 0;
                } else if (m_start_pos > 0) {
                    m_cursor_pos = pos - m_start_pos;
                } else {
                    m_cursor_pos = pos;
                }
                adjustPos();
            }
            break;
        case XK_Delete: {
                if (text().empty() || m_cursor_pos >= text().size())
                    break;
                unsigned int pos = findEmptySpaceRight();
                FbString t = text();
                t.erase(m_cursor_pos + m_start_pos, pos - (m_cursor_pos + m_start_pos));
                m_text.setLogical(t);
                adjustPos();
            }
            break;
        case XK_a:
            selectAll();
            break;
        }
        clear();
        return;

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
        case XK_Delete:
            deleteForward();
            break;
#define SET_SINGLE_BYTE(_B_) keychar[0] = _B_; keychar[1] = '\0'
        case XK_KP_Insert:
            SET_SINGLE_BYTE('0');
            break;
        case XK_KP_End:
            SET_SINGLE_BYTE('1');
            break;
        case XK_KP_Down:
            SET_SINGLE_BYTE('2');
            break;
        case XK_KP_Page_Down:
            SET_SINGLE_BYTE('3');
            break;
        case XK_KP_Left:
            SET_SINGLE_BYTE('4');
            break;
        case XK_KP_Begin:
            SET_SINGLE_BYTE('5');
            break;
        case XK_KP_Right:
            SET_SINGLE_BYTE('6');
            break;
        case XK_KP_Home:
            SET_SINGLE_BYTE('7');
            break;
        case XK_KP_Up:
            SET_SINGLE_BYTE('8');
            break;
        case XK_KP_Page_Up:
            SET_SINGLE_BYTE('9');
            break;
        case XK_KP_Delete:
            SET_SINGLE_BYTE(',');
            break;
#undef SET_SINGLE_BYTE
        }
    }
    if ((m_xic && !std::iscntrl(*keychar)) || std::isprint(*keychar)) {
        insertText(std::string(keychar));
        m_select_pos = std::string::npos;
    }
    if ((event.state & ShiftMask) != ShiftMask)
        m_select_pos = std::string::npos;
    clear();
}

void TextBox::handleEvent(XEvent &event) {
    if (event.type == KeymapNotify) {
        XRefreshKeyboardMapping(&event.xmapping);
    } else if (event.type == FocusIn && m_xic) {
        XSetICFocus(m_xic);
    } else if (event.type == FocusOut && m_xic) {
        XUnsetICFocus(m_xic);
    }
}

void TextBox::setCursorPosition(int pos) {
    m_cursor_pos = pos < 0 ? 0 : pos;
    if (m_cursor_pos > text().size() - 2*m_padding)
        cursorEnd();
}

void TextBox::adjustEndPos() {
    m_end_pos = text().size();
    m_start_pos = std::min(m_start_pos, m_end_pos);
    int text_width = font().textWidth(text().c_str() + m_start_pos, m_end_pos - m_start_pos);
    while (text_width > (static_cast<signed>(width()) - 2*m_padding)) {
        m_end_pos--;
        text_width = font().textWidth(text().c_str() + m_start_pos, m_end_pos - m_start_pos);
    }
}

void TextBox::adjustStartPos() {

    const char* visual = m_text.visual().c_str();

    int text_width = font().textWidth(visual, m_end_pos);
    if (m_cursor_pos >= 0 && text_width < (static_cast<signed>(width()) - 2*m_padding))
        return;

    int start_pos = 0;
    while (text_width > (static_cast<signed>(width()) - 2 * m_padding)) {
        start_pos++;
        text_width = font().textWidth(visual + start_pos, m_end_pos - start_pos);
    }

    // adjust cursorPosition() according relative to change to m_start_pos
    m_cursor_pos -= start_pos - m_start_pos;
    m_start_pos = start_pos;
}

unsigned int TextBox::findEmptySpaceLeft(){

    // found the first left space symbol
    int pos = text().rfind(' ', (m_start_pos + m_cursor_pos) > 0 ? 
                                 m_start_pos + m_cursor_pos - 1 : 0);

    // do we have one more space symbol near?
    int next_pos = -1;
    while (pos > 0 && (next_pos = text().rfind(' ', pos - 1)) > -1){
        if (next_pos + 1 < pos)
            break;
        pos = next_pos;
    }
    if (pos < 0)
        pos = 0;

    return pos;

}
unsigned int TextBox::findEmptySpaceRight(){

    // found the first right space symbol
    int pos = text().find(' ', m_start_pos + m_cursor_pos);

    // do we have one more space symbol near?
    int next_pos = -1;
    while (pos > -1 && pos < static_cast<signed>(text().size()) && (next_pos = text().find(' ', pos + 1)) > -1 ){

        if (next_pos - 1 > pos)
            break;
        pos = next_pos;
    }
    if (pos < 0)
        pos = text().size() - 1;

    return pos + 1; // (+1) - sets cursor at the right.

}
void TextBox::adjustPos(){
    if (m_start_pos + cursorPosition() < m_end_pos)
        adjustEndPos();
    else
        adjustStartPos();

}


void TextBox::select(std::string::size_type pos, int length)
{
    if (length < 0) {
        length = -length;
        pos = pos >= length ? pos - length : 0;
    }

    if (length > 0 && pos < text().size()) {
        m_select_pos = pos;
        pos = std::min(text().size(), pos + length);

        if (pos > m_start_pos)
            pos -= m_start_pos;
        else
            pos = 0;
        if (m_start_pos + pos <= m_end_pos)
            m_cursor_pos = pos;
        else if (m_end_pos < text().size()) {
            m_cursor_pos = pos;
            m_end_pos = pos;
        }

        adjustPos();
    } else {
        m_select_pos = std::string::npos;
    }
    clear();
}

void TextBox::selectAll() {
    select(0, m_text.visual().size());
}

TextBox::StringRange TextBox::charRange(std::string::size_type pos) const {
    auto isUtf8Head = [](char c) {
        const char indicator = (1<<7)|(1<<6); // first byte starts 11, following 10
        return (c & indicator) == indicator;
    };

    StringRange range = {pos, pos};

    if (!m_xic)
        return range;

    FbString t = text();

    if (pos < 0 || pos >= t.size() || t.at(pos) > 0) // invalid pos or ASCII
        return range;

    while (range.begin > 0 && t.at(range.begin) < 0 && !isUtf8Head(t.at(range.begin)))
        --range.begin;

    if (isUtf8Head(t.at(range.end))) // if pos is a utf-8 head, move into the range
        ++range.end;
    while (range.end < t.size() - 1 && t.at(range.end + 1) < 0 && !isUtf8Head(t.at(range.end + 1)))
        ++range.end;

    return range;
}

} // end namespace FbTk
