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

// $Id$

#include "IconbarTheme.hh"
#include "FbTk/App.hh"

IconbarTheme::IconbarTheme(int screen_num, 
                           const std::string &name,
                           const std::string &altname):
    FbTk::Theme(screen_num),
    m_focused_texture(*this, name + ".focused", altname + ".Focused"),
    m_unfocused_texture(*this, name + ".unfocused", altname + ".Unfocused"),
    m_empty_texture(*this, name + ".empty", altname + ".Empty"),
    m_focused_border(*this, name + ".focused", altname + ".Focused"),
    m_unfocused_border(*this, name + ".unfocused", altname + ".Unfocused"),
    m_border(*this, name, altname),
    m_focused_text(*this, name + ".focused", altname + ".Focused"),
    m_unfocused_text(*this, name + ".unfocused", altname + ".Unfocused"),
    m_name(name),
    m_alpha(*this, name+".alpha", altname+".Alpha") {

    FbTk::ThemeManager::instance().loadTheme(*this);

}
IconbarTheme::~IconbarTheme() {

}


void IconbarTheme::reconfigTheme() {
    m_focused_text.update();
    m_unfocused_text.update();
}

// fallback resources
bool IconbarTheme::fallback(FbTk::ThemeItem_base &item) {
    using namespace FbTk;
    ThemeManager &tm = ThemeManager::instance();

    if (&m_focused_texture == &item) {
        return (tm.loadItem(item, "window.label.focus", "Window.Label.Focus") ||
                tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel"));

    } else if (&m_unfocused_texture == &item) {
        return (tm.loadItem(item, "window.label.unfocus", "Window.Label.Unfocus") ||
                tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel"));
    } else if (&m_empty_texture == &item) {
        return (tm.loadItem(item, m_focused_texture.name(), m_focused_texture.altName()) || 
                tm.loadItem(item, "toolbar.windowLabel", "toolbar.windowLabel") ||
                tm.loadItem(item, "toolbar", "toolbar")
            ); 
    } else if (item.name() == m_name + ".focused.borderWidth" ||
               item.name() == m_name + ".unfocused.borderWidth")
        // don't fallback for base border, for theme backwards compatibility
        return tm.loadItem(item, "borderWidth", "BorderWidth");

    else if (item.name() == m_name + ".focused.borderColor" ||
             item.name() == m_name + ".unfocused.borderColor")

        return tm.loadItem(item, "borderColor", "BorderColor");

    else if (item.name() == m_name + ".focused.font" ||
             item.name() == m_name + ".unfocused.font")

        return tm.loadItem(item, "window.font", "Window.Font");

    else if (item.name() == m_name + ".focused.textColor") {

        return tm.loadItem(item, "window.label.focus.textColor", "Window.Label.Focus.TextColor");        

    } else if (item.name() == m_name + ".unfocused.textColor") {
        return tm.loadItem(item, "window.label.unfocus.textColor", "Window.Label.Unfocus.TextColor");
    } else if (item.name() == m_name + ".alpha") {
        if (!tm.loadItem(item, "toolbar.alpha", "Toolbar.Alpha")) {
            *m_alpha = 255;
        }
        return true;
    }
 
    return false;
}
