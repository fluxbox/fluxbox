// FbWinFrameTheme.cc for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWinFrameTheme.cc,v 1.10 2003/08/25 16:37:50 fluxgen Exp $

#include "FbWinFrameTheme.hh"
#include "App.hh"

#include <X11/cursorfont.h>

#include <iostream>

FbWinFrameTheme::FbWinFrameTheme(int screen_num): 
    FbTk::Theme(screen_num),
    m_label_focus(*this, "window.label.focus", "Window.Label.Focus"),
    m_label_unfocus(*this, "window.label.unfocus", "Window.Label.Unfocus"),

    m_title_focus(*this, "window.title.focus", "Window.Title.Focus"),
    m_title_unfocus(*this, "window.title.unfocus", "Window.Title.Unfocus"),

    m_handle_focus(*this, "window.handle.focus", "Window.Handle.Focus"),
    m_handle_unfocus(*this, "window.handle.unfocus", "Window.Handle.Unfocus"),

    m_button_focus(*this, "window.button.focus", "Window.Button.Focus"),
    m_button_unfocus(*this, "window.button.unfocus", "Window.Button.Unfocus"),
    m_button_pressed(*this, "window.button.pressed", "Window.Button.Pressed"),
    
    m_grip_focus(*this, "window.grip.focus", "Window.Grip.Focus"),
    m_grip_unfocus(*this, "window.grip.unfocus", "Window.Grip.Unfocus"),
  
    m_label_focus_color(*this, "window.label.focus.textColor", "Window.Label.Focus.TextColor"),
    m_label_unfocus_color(*this, "window.label.unfocus.textColor", "Window.Label.Unfocus.TextColor"),
    
    m_frame_focus_color(*this, "window.frame.focusColor", "Window.Frame.FocusColor"), 
    m_frame_unfocus_color(*this, "window.frame.unfocusColor", "Window.Frame.UnfocusColor"),

    m_button_focus_color(*this, "window.button.focus.picColor", "Window.Button.Focus.PicColor"),
    m_button_unfocus_color(*this, "window.button.unfocus.picColor", "Window.Button.Unfocus.PicColor"),

    m_font(*this, "window.font", "Window.Font"),
    m_textjustify(*this, "window.justify", "Window.Justify"),
    m_shape_place(*this, "window.roundCorners", "Window.RoundCorners"),

    m_alpha(*this, "window.alpha", "Window.Alpha"),
    m_title_height(*this, "window.title.height", "Window.Title.Height"),
    m_border(*this, "window", "Window") { // for window.border*

    *m_title_height = 0;
    // set defaults
    m_font->load("fixed");
    *m_alpha = 255;

    // create GCs
    Display *disp = FbTk::App::instance()->display();
    Window rootwin = RootWindow(disp, screen_num);
    m_label_text_focus_gc = XCreateGC(disp, rootwin, 0, 0);
    m_label_text_unfocus_gc = XCreateGC(disp, rootwin, 0, 0);
    m_button_pic_focus_gc = XCreateGC(disp, rootwin, 0, 0);
    m_button_pic_unfocus_gc = XCreateGC(disp, rootwin, 0, 0);
    // create cursors
    m_cursor_move = XCreateFontCursor(disp, XC_fleur);
    m_cursor_lower_left_angle = XCreateFontCursor(disp, XC_ll_angle);
    m_cursor_lower_right_angle = XCreateFontCursor(disp, XC_lr_angle);

}

FbWinFrameTheme::~FbWinFrameTheme() {
    // destroy GCs
    Display *disp = FbTk::App::instance()->display();
    XFreeGC(disp, m_label_text_focus_gc);
    XFreeGC(disp, m_label_text_unfocus_gc);
    XFreeGC(disp, m_button_pic_focus_gc);
    XFreeGC(disp, m_button_pic_unfocus_gc);
}

bool FbWinFrameTheme::fallback(FbTk::ThemeItem_base &item) {
    if (item.name() == "window.borderWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    else if (item.name() == "window.borderColor")
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");

    return false;
}

void FbWinFrameTheme::reconfigTheme() {
    if (*m_alpha > 255)
        *m_alpha = 255;
    else if (*m_alpha < 0)
        *m_alpha = 0;

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;
    Display *disp = FbTk::App::instance()->display();

    gcv.foreground = m_label_focus_color->pixel();
    XChangeGC(disp, m_label_text_focus_gc, gc_value_mask, &gcv);

    gcv.foreground = m_label_unfocus_color->pixel();
    XChangeGC(disp, m_label_text_unfocus_gc, gc_value_mask, &gcv);

    gcv.foreground = m_button_focus_color->pixel();
    XChangeGC(disp, m_button_pic_focus_gc, gc_value_mask, &gcv);

    gcv.foreground = m_button_unfocus_color->pixel();
    XChangeGC(disp, m_button_pic_unfocus_gc, gc_value_mask, &gcv);

    // notify listeners
    m_theme_change.notify();
}

