// IconButtonTheme.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: IconButtonTheme.hh,v 1.1 2003/08/11 15:49:56 fluxgen Exp $

#ifndef ICONBUTTONTHEME_HH
#define ICONBUTTONTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Font.hh"
#include "FbTk/Color.hh"
#include "FbTk/Texture.hh"
#include "FbTk/Subject.hh"

class IconButtonTheme: public FbTk::Theme {
public:
    explicit IconButtonTheme(int screen_num);
    virtual ~IconButtonTheme();

    void reconfigTheme();

    const FbTk::Texture &selectedTexture() const { return *m_selected_texture; }
    const FbTk::Texture &unselectedTexture() const { return *m_unselected_texture; }

    FbTk::Font &selectedFont() { return *m_selected_font; }
    const FbTk::Font &selectedFont() const { return *m_selected_font; }
    FbTk::Font &unselectedFont() { return *m_unselected_font; }
    const FbTk::Font &unselectedFont() const { return *m_unselected_font; }

    GC selectedGC() const { return m_selected_gc; }
    GC unselectedGC() const { return m_unselected_gc; }

    FbTk::Subject &themeChangeSig() { return m_theme_change; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_selected_texture, m_unselected_texture;
    FbTk::ThemeItem<FbTk::Font> m_selected_font, m_unselected_font;
    FbTk::ThemeItem<FbTk::Color> m_selected_color, m_unselected_color;
    GC m_selected_gc, m_unselected_gc;

    FbTk::Subject m_theme_change;
};

#endif // ICONBUTTONTHEME_HH
