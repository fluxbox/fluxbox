// IconButtonTheme.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: IconButtonTheme.cc,v 1.1 2003/08/11 15:49:56 fluxgen Exp $

#include "IconButtonTheme.hh"

#include "FbTk/App.hh"

IconButtonTheme::IconButtonTheme(int screen_num):
    FbTk::Theme(screen_num),
    m_selected_texture(*this, "iconButton.selected", "IconButton.Selected"),
    m_unselected_texture(*this, "iconButton.unselected", "IconButton.Unselected"),
    m_selected_font(*this, "iconButton.selectedFont", "IconButton.SelectedFont"),
    m_unselected_font(*this, "iconButton.unselectedFont", "IconButton.UnselectedFont"),
    m_selected_color(*this, "iconButton.selectedTextColor", "IconButton.SelectedTextColor"),
    m_unselected_color(*this, "iconButton.unselectedTextColor", "IconButton.UnselectedTextColor") {

    // load default font
    selectedFont().load("fixed");
    unselectedFont().load("fixed");

    // create graphic context
    Display *disp = FbTk::App::instance()->display();
    m_selected_gc = XCreateGC(disp, RootWindow(disp, screen_num), 0, 0);
    m_unselected_gc = XCreateGC(disp, RootWindow(disp, screen_num), 0, 0);
}

IconButtonTheme::~IconButtonTheme() {
    Display *disp = FbTk::App::instance()->display();
    XFreeGC(disp, m_selected_gc);
    XFreeGC(disp, m_unselected_gc);
}

void IconButtonTheme::reconfigTheme() {

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;
    Display *disp = FbTk::App::instance()->display();

    gcv.foreground = m_selected_color->pixel();    
    XChangeGC(disp, m_selected_gc, gc_value_mask, &gcv);

    gcv.foreground = m_unselected_color->pixel();    
    XChangeGC(disp, m_unselected_gc, gc_value_mask, &gcv);

    m_theme_change.notify();
}

