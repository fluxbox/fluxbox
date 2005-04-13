// TextButton.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "TextButton.hh"
#include "Font.hh"
#include "GContext.hh"
#include <iostream>
using namespace std;

namespace FbTk {

TextButton::TextButton(const FbTk::FbWindow &parent,
                       const FbTk::Font &font,
                       const std::string &text):
    FbTk::Button(parent, 0, 0, 10, 10),
    m_font(&font),
    m_text(text),
    m_justify(FbTk::LEFT), m_bevel(1),
    m_left_padding(0),
    m_right_padding(0) {

}

void TextButton::resize(unsigned int width, unsigned int height) {
    if (this->width() == width && height == this->height())
        return;

    Button::resize(width, height);
}

void TextButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {
    if (this->width() == width && height == this->height() &&
        x == this->x() && y == this->y())
        return;

    Button::moveResize(x, y, width, height);
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
}

/// set bevel and redraw text
void TextButton::setBevel(int bevel) {
    if (m_bevel == bevel)
        return;
    m_bevel = bevel;
}

void TextButton::setTextPaddingLeft(unsigned int leftpadding) {
    m_left_padding = leftpadding;
}

void TextButton::setTextPaddingRight(unsigned int rightpadding) {
    m_right_padding = rightpadding;
}

void TextButton::setTextPadding(unsigned int padding) {
    setTextPaddingLeft(padding/2);
    setTextPaddingRight(padding/2);
}

/// clear window and redraw text
void TextButton::clear() {
    TextButton::clearArea(0, 0,
                          width(), height());
}

void TextButton::clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposure) {
    Button::clearArea(x, y, width, height, exposure);
    // TODO: do we need to check if the text overlaps the clearing area
    // and if so, then clear a rectangle that encompases all the text plus the
    // requested area?
    drawText();
}

unsigned int TextButton::textWidth() const {
    return font().textWidth(text().c_str(), text().size());
}

void TextButton::drawText(int x_offset, int y_offset) {
    unsigned int textlen = text().size();
    // do text alignment
    int align_x = FbTk::doAlignment(width() - x_offset - m_left_padding - m_right_padding,
                                    bevel(),
                                    justify(),
                                    font(),
                                    text().c_str(), text().size(),
                                    textlen); // return new text len

    // center text by default
    int center_pos = height()/2 + font().ascent()/2 - 1;

    font().drawText(*this,
                    screenNumber(),
                    gc(), // graphic context
                    text().c_str(), textlen, // string and string size
                    align_x + x_offset + m_left_padding, center_pos + y_offset); // position
    
}

void TextButton::exposeEvent(XExposeEvent &event) {
    clearArea(event.x, event.y, event.width, event.height, false);
}

}; // end namespace FbTk
