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
    TextButton::clearArea(0, 0, width(), height());
}

void TextButton::clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposure) {
    Button::clearArea(x, y, width, height, exposure);
}


unsigned int TextButton::textWidth() const {
    return font().textWidth(text());
}

unsigned int TextButton::preferredWidth() const {
    return m_bevel + m_left_padding + m_right_padding + textWidth();
}

void TextButton::renderForeground(FbWindow &win, FbDrawable &drawable) {
    // (win should always be *this, no need to check)
    drawText(0, 0, &drawable);
}

void TextButton::drawText(int x_offset, int y_offset, FbDrawable *drawable) {

    if (drawable == 0)
        drawable = this;


    const FbString& visual = text().visual();
    unsigned int textlen = visual.size();
    unsigned int button_width = width();
    unsigned int button_height = height();
    int padding = m_left_padding + m_right_padding;

    int n_pixels = static_cast<int>(button_width) - x_offset;
    if (m_orientation == ROT90 || m_orientation == ROT270) {
        n_pixels = static_cast<int>(button_height) - y_offset;
    }
    n_pixels -= padding;

    // text is to small to render
    if (n_pixels <= bevel()) {
        return;
    }


    translateSize(m_orientation, button_width, button_height);

    // horizontal alignment, cut off text if needed
    int align_x = FbTk::doAlignment(n_pixels,
                                    bevel(), justify(), font(),
                                    visual.data(), visual.size(),
                                    textlen); // return new text len

    //
    //  we center the text vertically. to do this we have to nudge the
    //  baseline a little bit down so the "middle" of the glyph is placed
    //  on the middle of the button. we "assume", that ascent/2 is roughly
    //  the middle of the glyph. example:
    //
    //   +== ascent <--------->==                       +=====================
    //   |          |         |                         |
    //   |          |  ggggg  |                         |   ascent <--------->
    //   |          |  g  gg  |                         |          |         |
    //   | baseline <  glyph  |                         |          |  ggggg  |
    //   |          |      g  |  -- middle of button -- |          |  g  gg  |
    //   |  descent <  ggggg  |                         | baseline <  glyph  |
    //   |  height  |_________|                         |          |      g  |
    //   |                                              |  descent <  ggggg  |
    //   |                                              |   height |_________|
    //   |                                              |
    //   +=======================                       +=====================
    //
    //    ascent = 4
    //    button_height = 11
    //    baseline = (11 + 4) / 2 - 1 = 6
    //

    int baseline_x = align_x + x_offset + m_left_padding;
    int baseline_y = ((button_height + font().ascent()) / 2) - 1 + y_offset;

    // TODO: remove debug output fprintf(stderr, "%d | %d %d %d\n", height(), font().height(), font().ascent(), font().descent());

    // give it ROT0 style coords
    translateCoords(m_orientation, baseline_x, baseline_y, button_width, button_height);

    font().drawText(*drawable, screenNumber(), gc(),
                    visual.c_str(), textlen,
                    baseline_x, baseline_y, m_orientation);
}


bool TextButton::textExceeds(int x_offset) {

    const FbString& visual = text().visual();
    unsigned int textlen = visual.size();
    unsigned int button_width = width();
    unsigned int button_height = height();
    translateSize(m_orientation, button_width, button_height);

    FbTk::doAlignment(button_width - x_offset - m_left_padding - m_right_padding,
                      bevel(), justify(), font(), visual.data(), visual.size(),
                      textlen); // return new text len

    return visual.size() > textlen;
}

void TextButton::exposeEvent(XExposeEvent &event) {
    clearArea(event.x, event.y, event.width, event.height, false);
}

} // end namespace FbTk
