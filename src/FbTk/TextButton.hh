// TextButton.hh for Fluxbox Window Manager
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

#ifndef FBTK_TEXTBUTTON_HH
#define FBTK_TEXTBUTTON_HH

#include "Button.hh"
#include "FbString.hh"

namespace FbTk {

class Font;

/// Displays a text on a button
class TextButton: public FbTk::Button, FbTk::FbWindowRenderer {
public:
    TextButton(const FbTk::FbWindow &parent, 
               FbTk::Font &font, const FbTk::BiDiString &text);

    void setJustify(FbTk::Justify just);
    bool setOrientation(FbTk::Orientation orient);
    void setText(const FbTk::BiDiString &text);
    void setFont(FbTk::Font &font);
    void setTextPadding(unsigned int padding);
    void setTextPaddingLeft(unsigned int leftpadding);
    void setTextPaddingRight(unsigned int rightpadding);

    /// clears window and redraw text
    void clear();
    /// clears area and redraws text
    void clearArea(int x, int y,
                   unsigned int width, unsigned int height,
                   bool exposure = false);

    void exposeEvent(XExposeEvent &event);

    //void renderForeground(FbDrawable &drawable);
    void renderForeground(FbWindow &win, FbDrawable &drawable);

    FbTk::Justify justify() const { return m_justify; }
    const BiDiString &text() const { return m_text; }
    FbTk::Font &font() const { return *m_font; }
    FbTk::Orientation orientation() const { return m_orientation; }
    unsigned int textWidth() const;
    int bevel() const { return m_bevel; }

    virtual unsigned int preferredWidth() const;

protected:
    virtual void drawText(int x_offset, int y_offset, FbDrawable *drawable_override);
    // return true if the text will be truncated
    bool textExceeds(int x_offset);

private:
    FbTk::Font *m_font;
    BiDiString m_text;
    FbTk::Justify m_justify;
    FbTk::Orientation m_orientation;

    int m_bevel;
    unsigned int m_left_padding; ///< space between buttonborder and text
    unsigned int m_right_padding; ///< space between buttonborder and text

};

} // end namespace FbTk

#endif // FBTK_TEXTBUTTON_HH
