// TextBox.hh for FbTk - fluxbox toolkit
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

#ifndef FBTK_TEXTBOX_HH
#define FBTK_TEXTBOX_HH

#include "FbWindow.hh"
#include "EventHandler.hh"
#include "FbString.hh"

namespace FbTk {

class Font;

class TextBox:public FbWindow, public EventHandler {
public:
    TextBox(int screen_num, const Font &font, const std::string &text);
    TextBox(const FbWindow &parent, const Font &font, const std::string &text);
    virtual ~TextBox();

    void setText(const FbTk::BiDiString &text);
    void setPadding(int padding);
    void setFont(const Font &font);
    void setGC(GC gc);
    void setCursorPosition(int cursor);
    void setInputFocus();
    void cursorEnd();
    void cursorHome();
    void cursorForward();
    void cursorBackward();
    void deleteForward();
    void insertText(const std::string &val);
    void backspace();
    void killToEnd();

    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);
    void clear();

    void exposeEvent(XExposeEvent &event);
    void buttonPressEvent(XButtonEvent &event);
    void keyPressEvent(XKeyEvent &event);
    void handleEvent(XEvent &event);

    const FbString &text() const { return m_text.logical(); }
    const Font &font() const { return *m_font; }
    GC gc() const { return m_gc; }
    int cursorPosition() const { return m_cursor_pos; }
    int textStartPos() const { return m_start_pos; }

    bool hasSelection() const {
        return (m_select_pos != std::string::npos) && (m_select_pos != m_cursor_pos + m_start_pos); }
    void select(std::string::size_type pos, int length);
    void selectAll();

    unsigned int findEmptySpaceLeft();
    unsigned int findEmptySpaceRight();

private:
    void adjustEndPos();
    void adjustStartPos();

    void adjustPos();

    typedef struct { std::string::size_type begin, end; } StringRange;
    StringRange charRange(std::string::size_type pos) const;

    const FbTk::Font *m_font;
    BiDiString m_text;
    GC m_gc;
    std::string::size_type m_cursor_pos, m_start_pos, m_end_pos, m_select_pos;
    int m_padding;
    XIC m_xic;
};

} // end namespace FbTk

#endif // FBTK_TEXTBOX_HH
