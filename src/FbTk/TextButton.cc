// TextButton.cc for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen[at]fluxbox.org)
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

// $Id: TextButton.cc,v 1.1 2003/08/18 12:15:39 fluxgen Exp $

#include "TextButton.hh"
#include "Font.hh"

#include <iostream>
using namespace std;

namespace FbTk {

TextButton::TextButton(const FbTk::FbWindow &parent, 
                       const FbTk::Font &font, 
                       const std::string &text):
    FbTk::Button(parent, 0, 0, 10, 10),
    m_font(&font),
    m_text(text),
    m_justify(FbTk::LEFT), m_bevel(1) {

}

void TextButton::setJustify(FbTk::Justify just) {
    m_justify = just;
}

void TextButton::setText(const std::string &text) {
    m_text = text;
}

void TextButton::setFont(const FbTk::Font &font) {
    // no need to set new font if it's the same
    if (&font == m_font)
        return;
    m_font = &font;
    clear(); // redraw text with new font
}

/// set bevel and redraw text
void TextButton::setBevel(int bevel) {
    if (m_bevel == bevel)
        return;
    m_bevel = bevel;
}

/// clear window and redraw text 
void TextButton::clear() {
    FbTk::Button::clear();
    drawText();
}

unsigned int TextButton::textWidth() const {
    return font().textWidth(text().c_str(), text().size());
}

void TextButton::drawText(int x_offset, int y_offset) {
    unsigned int textlen = text().size();
    // do text alignment
    int align_x = FbTk::doAlignment(width() - x_offset,
                                    bevel(),
                                    justify(),
                                    font(),
                                    text().c_str(), text().size(),
                                    textlen // return new text len
                                    );

    // center text by default
    int center_pos = height()/2 + font().ascent()/2;

    font().drawText(window(), // drawable
                    screenNumber(),
                    gc(), // graphic context
                    text().c_str(), textlen, // string and string size
                    align_x + x_offset, center_pos + y_offset); // position
}

}; // end namespace FbTk
