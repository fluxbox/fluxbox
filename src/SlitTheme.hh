// SlitTheme.hh for fluxbox
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

#ifndef SLITTHEME_HH
#define SLITTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Texture.hh"
#include "FbTk/Color.hh"

class SlitTheme: public FbTk::Theme, public FbTk::ThemeProxy<SlitTheme> {
public:
    explicit SlitTheme(int screen_num);

    void reconfigTheme();
    bool fallback(FbTk::ThemeItem_base &item);

    const FbTk::Texture &texture() const { return *m_texture; }
    const FbTk::Color &borderColor() const { return *m_border_color; }
    int borderWidth() const { return *m_border_width; }
    int bevelWidth() const { return *m_bevel_width; }

    virtual FbTk::Signal<> &reconfigSig() { return FbTk::Theme::reconfigSig(); }

    virtual SlitTheme &operator *() { return *this; }
    virtual const SlitTheme &operator *() const { return *this; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_texture;
    FbTk::ThemeItem<int> m_border_width, m_bevel_width;
    FbTk::ThemeItem<FbTk::Color> m_border_color;
};

#endif // SLITTHEME_HH
