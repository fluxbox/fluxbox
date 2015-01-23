// IconbarTheme.cc
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

#include "IconbarTheme.hh"
#include "FbTk/App.hh"

IconbarTheme::IconbarTheme(int screen_num, 
                           const std::string &name,
                           const std::string &altname):
    FbTk::Theme(screen_num),
    m_texture(*this, name, altname),
    m_empty_texture(*this, name + ".empty", altname + ".Empty"),
    m_border(*this, name, altname),
    m_text(*this, name, altname),
    m_name(name), m_altname(altname) {

    FbTk::ThemeManager::instance().loadTheme(*this);

}
IconbarTheme::~IconbarTheme() {

}


void IconbarTheme::reconfigTheme() {
    m_text.updateTextColor();
}

// fallback resources
bool IconbarTheme::fallback(FbTk::ThemeItem_base &item) {
    using namespace FbTk;
    ThemeManager &tm = ThemeManager::instance();
    std::string base = m_name;
    base.erase(base.find_last_of("."));
    std::string altbase = m_altname;
    altbase.erase(altbase.find_last_of("."));

    if (&m_texture == &item) {
        return tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel");
    } else if (&m_empty_texture == &item) {
        return (tm.loadItem(item, "toolbar.iconbar.empty",
                            "Toolbar.Iconbar.Empty") ||
                tm.loadItem(item, m_texture.name(), m_texture.altName()) ||
                tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel")
                || tm.loadItem(item, "toolbar", "toolbar"));
    } else if (item.name() == m_name + ".borderWidth")
        // don't fallback for base border, for theme backwards compatibility
        return (tm.loadItem(item, base + ".borderWidth", altbase + ".BorderWidth") ||
                tm.loadItem(item, "window.borderWidth", "Window.BorderWidth") ||
                tm.loadItem(item, "borderWidth", "BorderWidth"));

    else if (item.name() == m_name + ".borderColor")

        return (tm.loadItem(item, base + ".borderColor", altbase + ".BorderColor") ||
                tm.loadItem(item, "window.borderColor", "Window.BorderColor") ||
                tm.loadItem(item, "borderColor", "BorderColor"));

    else if (item.name() == m_name + ".font")

        return tm.loadItem(item, "window.font", "Window.Font");

    else if (item.name() == m_name + ".justify") {
        return (tm.loadItem(item, base + ".justify", altbase + ".Justify")
                || tm.loadItem(item, "window.justify", "Window.Justify"));
    }

    return false;
}
