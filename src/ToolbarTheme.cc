// ToolbarTheme.cc  a theme class for Toolbar
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ToolbarTheme.cc,v 1.3 2003/06/24 16:26:56 fluxgen Exp $

#include "ToolbarTheme.hh"

#include "App.hh"
#include <iostream>
using namespace std;

ToolbarTheme::ToolbarTheme(int screen_num):
    FbTk::Theme(screen_num),
    m_label_textcolor(*this, 
                      "toolbar.label.textColor", "Toolbar.Label.TextColor"),
    m_window_textcolor(*this,
                       "toolbar.windowLabel.textColor", 
                       "Toolbar.WindowLabel.TextColor"),
    m_clock_textcolor(*this,
                      "toolbar.clock.textColor", "Toolbar.Clock.TextColor"),
    m_button_color(*this, 
                   "toolbar.button.picColor", "Toolbar.Button.PicColor"),
    m_border_color(*this,
                   "toolbar.borderColor", "toolbar.borderColor"),
    m_toolbar(*this, "toolbar", "Toolbar"),
    m_label(*this, "toolbar.label", "Toolbar.Label"),
    m_window(*this, "toolbar.windowLabel", "Toolbar.WindowLabel"),
    m_button(*this, "toolbar.button", "Toolbar.Button"),
    m_pressed_button(*this, 
                     "toolbar.button.pressed", "Toolbar.Button.Pressed"),
    m_clock(*this, "toolbar.clock", "Toolbar.Clock"),
    m_font(*this, "toolbar.font", "Toolbar.Font"),
    m_justify(*this, "toolbar.justify", "Toolbar.Justify"),
    m_border_width(*this, "toolbar.borderWidth", "Toolbar.BorderWidth"),
    m_bevel_width(*this, "toolbar.bevelWidth", "Toolbar.BevelWidth"),
    m_display(FbTk::App::instance()->display()){

    Window rootwindow = RootWindow(m_display, screen_num);

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;
    
    gcv.foreground = m_label_textcolor->pixel();

    m_label_text_gc =
        XCreateGC(m_display, rootwindow,
                  gc_value_mask, &gcv);

    gcv.foreground = m_window_textcolor->pixel();
    m_window_text_gc =
        XCreateGC(m_display, rootwindow,
                  gc_value_mask, &gcv);

    gcv.foreground = m_clock_textcolor->pixel();
    m_clock_text_gc =
        XCreateGC(m_display, rootwindow,
                  gc_value_mask, &gcv);

    gcv.foreground = m_button_color->pixel();
    m_button_pic_gc =
        XCreateGC(m_display, rootwindow,
                  gc_value_mask, &gcv);
    // load from current database
    FbTk::ThemeManager::instance().loadTheme(*this);
}

ToolbarTheme::~ToolbarTheme() {
    XFreeGC(m_display, m_button_pic_gc);
    XFreeGC(m_display, m_clock_text_gc);
    XFreeGC(m_display, m_label_text_gc);
    XFreeGC(m_display, m_window_text_gc);
}

void ToolbarTheme::reconfigTheme() {

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;

    
    gcv.foreground = m_label_textcolor->pixel();
    XChangeGC(m_display, m_label_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = m_window_textcolor->pixel();
    XChangeGC(m_display, m_window_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = m_clock_textcolor->pixel();
    XChangeGC(m_display, m_clock_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = m_button_color->pixel();
    XChangeGC(m_display, m_button_pic_gc,
              gc_value_mask, &gcv);

    // notify listeners
    m_theme_change_sig.notify();
}
