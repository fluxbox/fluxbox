// ToolTheme.hh
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

#ifndef TOOLTHEME_HH
#define TOOLTHEME_HH


#include "TextTheme.hh"
#include "BorderTheme.hh"

#include "FbTk/Texture.hh"

#include <X11/Xlib.h>
#include <string>

class ToolbarTheme;

/// Handles toolbar item theme for text and texture
class ToolTheme: public FbTk::Theme, public TextTheme {
public:
    ToolTheme(int screen_num, const std::string &name, const std::string &altname);
    virtual ~ToolTheme();

    bool fallback(FbTk::ThemeItem_base &item);

    void reconfigTheme();
    // textures
    const FbTk::Texture &texture() const { return *m_texture; }
    const BorderTheme &border() const { return m_border; }
    inline unsigned char alpha() const { return m_alpha; }
    inline void setAlpha(unsigned char alpha) { m_alpha = alpha; }

protected:
    FbTk::ThemeItem<FbTk::Texture> &textureTheme() { return m_texture; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_texture;
    BorderTheme m_border;
    unsigned char m_alpha;
};

#endif // TOOLTHEME_HH
