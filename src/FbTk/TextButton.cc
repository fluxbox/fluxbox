// TextButton.cc for Fluxbox Window Manager
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

#include "TextButton.hh"
#include "TextUtils.hh"
#include "Font.hh"
#include "GContext.hh"

namespace FbTk {

TextButton::TextButton(const FbTk::FbWindow &parent,
                       FbTk::Font &font,
                       const FbTk::BiDiString &text):
    FbTk::Button(parent, 0, 0, 10, 10),
    m_font(&font),
    m_text(text),
    m_justify(FbTk::LEFT),
    m_orientation(FbTk::ROT0),
    m_bevel(1),
    m_left_padding(0),
    m_right_padding(0) {

    setRenderer(*this);
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

bool TextButton::setOrientation(FbTk::Orientation orient) {
    if (orient == m_orientation
        || !m_font->validOrientation(orient))
        return false;
    invalidateBackground();

    if (((m_orientation == FbTk::ROT0 || m_orientation == FbTk::ROT180) &&
         (orient == FbTk::ROT90 || orient == FbTk::ROT270)) ||
        ((m_orientation == FbTk::ROT90 || m_orientation == FbTk::ROT270) &&
         (orient == FbTk::ROT0 || orient == FbTk::ROT180))) {
        // flip width and height
        m_orientation = orient;
        resize(height(), width());
    } else {
        m_orientation = orient;
    }

    return true;
}

void TextButton::setText(const FbTk::BiDiString &text) {
    if (m_text.logical() != text.logical()) {
        m_text = text;
        updateBackground(false);
        clear();
    }
}

void TextButton::setFont(FbTk::Font &font) {
    // no need to set new font if it's the same
    if (&font == m_font)
        return;
    m_font = &font;
    font.validOrientation(m_orientation); // load the orientation!
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
    if (backgroundPixmap() == ParentRelative) 
        drawText(0, 0, this);
}


unsigned int TextButton::textWidth() const {
    return font().textWidth(text());
}

void TextButton::renderForeground(FbWindow &win, FbDrawable &drawable) {
    // (win should always be *this, no need to check)
    drawText(0, 0, &drawable);
}

void TextButton::drawText(int x_offset, int y_offset, FbDrawable *drawable) {
    const FbString& visual = text().visual();
    unsigned int textlen = visual.size();
    unsigned int textw = width();
    unsigned int texth = height();
    translateSize(m_orientation, textw, texth);

    int align_x = FbTk::doAlignment(textw - x_offset - m_left_padding - m_right_padding,
                                    bevel(), justify(), font(),
                                    visual.data(), visual.size(),
                                    textlen); // return new text len

    // center text by default
    int center_pos = texth/2 + font().ascent()/2 - 1;

    int textx = align_x + x_offset + m_left_padding;
    int texty = center_pos + y_offset;

    if (drawable == 0)
        drawable = this;

    // give it ROT0 style coords
    translateCoords(m_orientation, textx, texty, textw, texth);

    font().drawText(*drawable,
                    screenNumber(),
                    gc(), // graphic context
                    visual.c_str(), textlen, // string and string size
                    textx, texty, m_orientation); // position
}


bool TextButton::textExceeds(int x_offset) {

    const FbString& visual = text().visual();
    unsigned int textlen = visual.size();
    unsigned int textw = width();
    unsigned int texth = height();
    translateSize(m_orientation, textw, texth);

    FbTk::doAlignment(textw - x_offset - m_left_padding - m_right_padding,
                      bevel(), justify(), font(), visual.data(), visual.size(),
                      textlen); // return new text len

    return visual.size()>textlen;
}

void TextButton::exposeEvent(XExposeEvent &event) {
    clearArea(event.x, event.y, event.width, event.height, false);
}

} // end namespace FbTk
