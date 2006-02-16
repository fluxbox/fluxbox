// TextTheme.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef TEXTTHEME_HH
#define TEXTTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Font.hh"
#include "FbTk/Color.hh"
#include "FbTk/Text.hh"
#include "FbTk/GContext.hh"

class TextTheme {
public:
    TextTheme(FbTk::Theme &theme, const std::string &name, const std::string &altname);
    virtual ~TextTheme();

    void update();

    FbTk::Font &font() { return *m_font; }
    const FbTk::Font &font() const { return *m_font; }
    const FbTk::Color &textColor() const { return *m_text_color; }
    FbTk::Justify justify() const { return *m_justify; }
    GC textGC() const { return m_text_gc.gc(); }
private:
    FbTk::ThemeItem<FbTk::Font> m_font;
    FbTk::ThemeItem<FbTk::Color> m_text_color;
    FbTk::ThemeItem<FbTk::Justify> m_justify;
    FbTk::GContext m_text_gc;
};

#endif // TEXTTHEME_HH
