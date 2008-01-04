// ToolTheme.cc
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

#include "ToolTheme.hh"

ToolTheme::ToolTheme(int screen_num, const std::string &name, const std::string &altname):
    FbTk::Theme(screen_num),
    FbTk::TextTheme(*this, name, altname),
    m_texture(*this, name, altname),
    m_border(*this, name, altname),
    m_alpha(255) {

    FbTk::ThemeManager::instance().loadTheme(*this);
}

ToolTheme::~ToolTheme() {

}

void ToolTheme::reconfigTheme() { 
    // update text theme
    updateTextColor();
}

bool ToolTheme::fallback(FbTk::ThemeItem_base &item) {
    /* Don't fallback these for theme backwards compatibility
    if (item.name().find(".borderWidth") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    } else if (item.name().find(".borderColor") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    } else
    */
    if (item.name().find(".justify") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       "toolbar.justify",
                                                       "Toolbar.Justify");
    }

    return false;
}
