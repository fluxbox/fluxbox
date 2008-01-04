// SlitTheme.cc for fluxbox
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

#include "SlitTheme.hh"

SlitTheme::SlitTheme(int screen_num):FbTk::Theme(screen_num),
                                   m_texture(*this, "slit", "Slit"),
                                   m_border_width(*this, "slit.borderWidth", "Slit.borderWidth"),
                                   m_bevel_width(*this, "slit.bevelWidth", "slit.bevelWidth"),
                                   m_border_color(*this, "slit.borderColor", "Slit.BorderColor") { 
    m_texture.setDefaultValue();
    m_border_width.setDefaultValue();
    m_bevel_width.setDefaultValue();
    m_border_color.setDefaultValue();
    // default texture type
    m_texture->setType(FbTk::Texture::SOLID);
    FbTk::ThemeManager::instance().loadTheme(*this);
}


void SlitTheme::reconfigTheme() {
}

bool SlitTheme::fallback(FbTk::ThemeItem_base &item) {
    if (&item == &m_texture) {
        return FbTk::ThemeManager::instance().loadItem(item,
                                                       "toolbar",
                                                       "Toolbar");
    } else if (item.name().find(".borderWidth") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item,
                                                       "borderWidth",
                                                       "BorderWidth");
    } else if (item.name().find(".borderColor") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item,
                                                       "borderColor",
                                                       "BorderColor");
    }

    return false;
}


