// IconbarTheme.hh
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

#ifndef ICONBARTHEME_HH
#define ICONBARTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Texture.hh"

#include "TextTheme.hh"
#include "BorderTheme.hh"

class IconbarTheme:public FbTk::Theme {
public:
    IconbarTheme(int screen_num, const std::string &name, const std::string &altname);
    virtual ~IconbarTheme();

    void reconfigTheme();
    bool fallback(FbTk::ThemeItem_base &item);

    TextTheme &focusedText()  { return m_focused_text; }
    TextTheme &unfocusedText() { return m_unfocused_text; }

    const BorderTheme &focusedBorder() const { return m_focused_border; }
    const BorderTheme &unfocusedBorder() const { return m_unfocused_border; }
    const BorderTheme &border() const { return m_border; }
    
    const FbTk::Texture &focusedTexture() const { return *m_focused_texture; }
    const FbTk::Texture &unfocusedTexture() const { return *m_unfocused_texture; }
    const FbTk::Texture &emptyTexture() const { return *m_empty_texture; }
    inline unsigned char alpha() const { return *m_alpha; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_focused_texture, m_unfocused_texture, m_empty_texture;
    BorderTheme m_focused_border, m_unfocused_border, m_border;
    TextTheme m_focused_text, m_unfocused_text;
    std::string m_name;
    FbTk::ThemeItem<int> m_alpha;
};

#endif  // ICONBARTHEME_HH
