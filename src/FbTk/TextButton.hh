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

// $Id: TextButton.hh,v 1.5 2004/01/08 22:03:13 fluxgen Exp $

#ifndef FBTK_TEXTBUTTON_HH
#define FBTK_TEXTBUTTON_HH

#include "Button.hh"
#include "Text.hh"
#include "FbPixmap.hh"

#include <string>

namespace FbTk {

class Font;

/// Displays a text on a button
class TextButton: public FbTk::Button {
public:
    TextButton(const FbTk::FbWindow &parent, 
               const FbTk::Font &font, const std::string &text);

    void setJustify(FbTk::Justify just);
    void setText(const std::string &text);
    void setFont(const FbTk::Font &font);
    void setBevel(int bevel);

    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    /// clears window and redraw text
    void clear();
    /// clears area and redraws text
    void clearArea(int x, int y,
                   unsigned int width, unsigned int height,
                   bool exposure = false);

    void exposeEvent(XExposeEvent &event);

    inline FbTk::Justify justify() const { return m_justify; }
    inline const std::string &text() const { return m_text; }
    inline const FbTk::Font &font() const { return *m_font; }
    unsigned int textWidth() const;
    int bevel() const { return m_bevel; }

protected:
    virtual void drawText(int x_offset = 0, int y_offset = 0);

private:
    const FbTk::Font *m_font;
    std::string m_text;
    FbTk::Justify m_justify;
    int m_bevel;
    FbTk::FbPixmap m_buffer; ///< for background buffer
};

} // end namespace FbTk

#endif // FBTK_TEXTBUTTON_HH
