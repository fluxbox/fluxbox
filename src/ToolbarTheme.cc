// ToolbarTheme.cc  a theme class for Toolbar
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ToolbarTheme.cc,v 1.15 2004/05/24 15:30:52 rathnor Exp $

#include "ToolbarTheme.hh"

#include "FbTk/App.hh"

#include <iostream>
using namespace std;

template<>
void FbTk::ThemeItem<bool>::load(const std::string *name, const std::string *altname) { }

template<>
void FbTk::ThemeItem<bool>::setDefaultValue() {
    *(*this) = false;
}

template<>
void FbTk::ThemeItem<bool>::setFromString(char const *strval) {
    if (strcasecmp(strval, "true")==0)
        *(*this) = true;
    else
        *(*this) = false;
}

ToolbarTheme::ToolbarTheme(int screen_num):
    FbTk::Theme(screen_num),
    m_toolbar(*this, "toolbar", "Toolbar"),
    m_border(*this, "toolbar", "Toolbar"),
    m_bevel_width(*this, "toolbar.bevelWidth", "Toolbar.BevelWidth"),
    m_shape(*this, "toolbar.shaped", "Toolbar.Shaped"),    
    m_alpha(*this, "toolbar.alpha", "Toolbar.Alpha"),
    m_height(*this, "toolbar.height", "Toolbar.Height"),
    m_button_size(*this, "toolbar.button.size", "Toolbar.Button.Size") {
    // set default value
    *m_bevel_width = 0;
    *m_shape = false;
    *m_height = 0;
    *m_button_size = -1;
}

ToolbarTheme::~ToolbarTheme() {

}

bool ToolbarTheme::fallback(FbTk::ThemeItem_base &item) {
    if (item.name() == "toolbar.alpha") {
        *m_alpha = 255;
        return true;
    }
    return false;
}

void ToolbarTheme::reconfigTheme() {
    if (*m_alpha > 255)
        *m_alpha = 255;
    else if (*m_alpha < 0)
        *m_alpha = 0;

    if (*m_bevel_width > 20)
        *m_bevel_width = 20;

    if (*m_height > 100)
        *m_height = 100;
    else if (*m_height < 0)
        *m_height = 0;

    if (*m_button_size > 100)
        *m_button_size = 100;
}
