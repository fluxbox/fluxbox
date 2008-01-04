// ButtonTheme.cc
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

#include "ButtonTheme.hh"
#include "FbTk/App.hh"

//!! TODO: still missing *.pressed.picColor
ButtonTheme::ButtonTheme(int screen_num, 
                         const std::string &name, 
                         const std::string &alt_name,
                         const std::string &extra_fallback,
                         const std::string &extra_fallback_alt):
    ToolTheme(screen_num, name, alt_name),
    m_pic_color(*this, name + ".picColor", alt_name + ".PicColor"),
    m_pressed_texture(*this, name + ".pressed", alt_name + ".Pressed"),
    m_gc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_scale(*this, name + ".scale", alt_name + ".Scale"),
    m_name(name),
    m_fallbackname(extra_fallback), m_altfallbackname(extra_fallback_alt) {

    FbTk::ThemeManager::instance().loadTheme(*this);
}

bool ButtonTheme::fallback(FbTk::ThemeItem_base &item) {

/* Don't fallback these for theme backwards compatibility
    if (item.name().find(".borderWidth") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    }

    if (item.name().find(".borderColor") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    }
*/
    if (item.name() == name()) {
        // default to the toolbar label style
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       m_fallbackname,
                                                       m_altfallbackname);

    } else if (item.name().find(".picColor") != std::string::npos) {
        // if we've fallen back to alternate name, and it doesn't have a picColor, 
        // try its text color instead
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       m_fallbackname + ".picColor",
                                                       m_altfallbackname + ".picColor") ||
            FbTk::ThemeManager::instance().loadItem(item, 
                                                    m_fallbackname + ".textColor",
                                                    m_altfallbackname + ".TextColor");
    } else if (item.name().find(".pressed") != std::string::npos) {
        // copy texture
        *m_pressed_texture = texture();
        // invert the bevel if it has one!
        unsigned long type = m_pressed_texture->type();
        unsigned long bevels = (FbTk::Texture::SUNKEN | FbTk::Texture::RAISED);
        if ((type & bevels) != 0) {
            type ^= bevels;
            m_pressed_texture->setType(type);
        }
        return true;
    }

    return ToolTheme::fallback(item);
}

void ButtonTheme::reconfigTheme() {
    m_gc.setForeground(*m_pic_color);
}



