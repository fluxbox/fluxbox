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
    m_focused_texture(*this,
            name + (name == "window.label" ? ".focus" : ".focused"),
            altname + (name == "window.label" ? ".Focus" : ".Focused")),
    m_unfocused_texture(*this,
            name + (name == "window.label" ? ".unfocus" : ".unfocused"),
            altname + (name == "window.label" ? ".Unfocus" : ".Unfocused")),
    m_empty_texture(*this, name + ".empty", altname + ".Empty"),
    m_focused_border(*this,
            name + (name == "window.label" ? ".focus" : ".focused"),
            altname + (name == "window.label" ? ".Focus" : ".Focused")),
    m_unfocused_border(*this,
            name + (name == "window.label" ? ".unfocus" : ".unfocused"),
            altname + (name == "window.label" ? ".Unfocus" : ".Unfocused")),
    m_border(*this, name, altname),
    m_focused_text(*this,
            name + (name == "window.label" ? ".focus" : ".focused"),
            altname + (name == "window.label" ? ".Focus" : ".Focused")),
    m_unfocused_text(*this,
            name + (name == "window.label" ? ".unfocus" : ".unfocused"),
            altname + (name == "window.label" ? ".Unfocus" : ".Unfocused")),
    m_name(name), m_altname(altname) {

    FbTk::ThemeManager::instance().loadTheme(*this);

}
IconbarTheme::~IconbarTheme() {

}


void IconbarTheme::reconfigTheme() {
    m_focused_text.updateTextColor();
    m_unfocused_text.updateTextColor();
}

// fallback resources
bool IconbarTheme::fallback(FbTk::ThemeItem_base &item) {
    using namespace FbTk;
    ThemeManager &tm = ThemeManager::instance();
    std::string focus = (m_name == "window.label" ? ".focus" : ".focused");
    std::string un = (m_name == "window.label" ? ".unfocus" : ".unfocused");

    if (&m_focused_texture == &item || &m_unfocused_texture == &item) {
        return tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel");
    } else if (&m_empty_texture == &item) {
        return (tm.loadItem(item, m_focused_texture.name(),
                m_focused_texture.altName()) ||
                tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel")
                || tm.loadItem(item, "toolbar", "toolbar")); 
    } else if (item.name() == m_name + focus + ".borderWidth" ||
               item.name() == m_name + un + ".borderWidth")
        // don't fallback for base border, for theme backwards compatibility
        return (tm.loadItem(item, m_name + ".borderWidth",
                            m_altname + ".BorderWidth") ||
                tm.loadItem(item, "window.borderWidth", "Window.BorderWidth") ||
                tm.loadItem(item, "borderWidth", "BorderWidth"));

    else if (item.name() == m_name + focus + ".borderColor" ||
             item.name() == m_name + un + ".borderColor")

        return (tm.loadItem(item, m_name + ".borderColor",
                            m_altname + ".BorderColor") ||
                tm.loadItem(item, "window.borderColor", "Window.BorderColor") ||
                tm.loadItem(item, "borderColor", "BorderColor"));

    else if (item.name() == m_name + focus + ".font" ||
             item.name() == m_name + un + ".font")

        return tm.loadItem(item, "window.font", "Window.Font");

    else if (item.name() == m_name + focus + ".justify" ||
             item.name() == m_name + un + ".justify") {
        return (tm.loadItem(item, m_name + ".justify", m_altname + ".Justify")
                || tm.loadItem(item, "window.justify", "Window.Justify"));
    }

    return false;
}
