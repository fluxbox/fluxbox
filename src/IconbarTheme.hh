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

#ifndef ICONBARTHEME_HH
#define ICONBARTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/BorderTheme.hh"
#include "FbTk/Texture.hh"
#include "FbTk/TextTheme.hh"

class IconbarTheme: public FbTk::Theme, public FbTk::ThemeProxy<IconbarTheme> {
public:
    IconbarTheme(int screen_num, const std::string &name, const std::string &altname);
    virtual ~IconbarTheme();

    void reconfigTheme();
    bool fallback(FbTk::ThemeItem_base &item);

    FbTk::TextTheme &text()  { return m_text; }
    const FbTk::BorderTheme &border() const { return m_border; }
    const FbTk::Texture &texture() const { return *m_texture; }
    const FbTk::Texture &emptyTexture() const { return *m_empty_texture; }

    virtual FbTk::Signal<> &reconfigSig() { return FbTk::Theme::reconfigSig(); }

    virtual IconbarTheme &operator *() { return *this; }
    virtual const IconbarTheme &operator *() const { return *this; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_texture, m_empty_texture;
    FbTk::BorderTheme m_border;
    FbTk::TextTheme m_text;
    std::string m_name, m_altname;
};

#endif  // ICONBARTHEME_HH
