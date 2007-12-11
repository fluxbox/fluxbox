// FbWinFrameTheme.cc for Fluxbox Window Manager
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

// $Id$

#include "FbWinFrameTheme.hh"
#include "App.hh"

#include "IconbarTheme.hh"

#include <X11/cursorfont.h>

FbWinFrameTheme::FbWinFrameTheme(int screen_num):
    FbTk::Theme(screen_num),
    m_title_focus(*this, "window.title.focus", "Window.Title.Focus"),
    m_title_unfocus(*this, "window.title.unfocus", "Window.Title.Unfocus"),

    m_handle_focus(*this, "window.handle.focus", "Window.Handle.Focus"),
    m_handle_unfocus(*this, "window.handle.unfocus", "Window.Handle.Unfocus"),

    m_button_focus(*this, "window.button.focus", "Window.Button.Focus"),
    m_button_unfocus(*this, "window.button.unfocus", "Window.Button.Unfocus"),
    m_button_pressed(*this, "window.button.pressed", "Window.Button.Pressed"),

    m_grip_focus(*this, "window.grip.focus", "Window.Grip.Focus"),
    m_grip_unfocus(*this, "window.grip.unfocus", "Window.Grip.Unfocus"),

    m_button_focus_color(*this, "window.button.focus.picColor", "Window.Button.Focus.PicColor"),
    m_button_unfocus_color(*this, "window.button.unfocus.picColor", "Window.Button.Unfocus.PicColor"),

    m_font(*this, "window.font", "Window.Font"),
    m_shape_place(*this, "window.roundCorners", "Window.RoundCorners"),
    m_title_height(*this, "window.title.height", "Window.Title.Height"),
    m_bevel_width(*this, "window.bevelWidth", "Window.BevelWidth"),
    m_handle_width(*this, "window.handleWidth", "Window.handleWidth"),
    m_border(*this, "window", "Window"), // for window.border*
    m_button_pic_focus_gc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_button_pic_unfocus_gc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_focused_alpha(255),
    m_unfocused_alpha(255),
    m_iconbar_theme(screen_num, "window.label", "Window.Label") {

    *m_title_height = 0;
    // set defaults
    m_font->load("__DEFAULT__");

    // create cursors
    Display *disp = FbTk::App::instance()->display();
    m_cursor_move = XCreateFontCursor(disp, XC_fleur);
    m_cursor_lower_left_angle = XCreateFontCursor(disp, XC_ll_angle);
    m_cursor_lower_right_angle = XCreateFontCursor(disp, XC_lr_angle);
    m_cursor_upper_right_angle = XCreateFontCursor(disp, XC_ur_angle);
    m_cursor_upper_left_angle = XCreateFontCursor(disp, XC_ul_angle);
    m_cursor_left_side = XCreateFontCursor(disp, XC_left_side);
    m_cursor_top_side = XCreateFontCursor(disp, XC_top_side);
    m_cursor_right_side = XCreateFontCursor(disp, XC_right_side);
    m_cursor_bottom_side = XCreateFontCursor(disp, XC_bottom_side);

    reconfigTheme();
}

FbWinFrameTheme::~FbWinFrameTheme() {

}

bool FbWinFrameTheme::fallback(FbTk::ThemeItem_base &item) {
    if (item.name() == "window.borderWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    else if (item.name() == "window.borderColor")
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    else if (item.name() == "window.bevelWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "bevelWidth", "bevelWidth");
    else if (item.name() == "window.handleWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "handleWidth", "HandleWidth");

    return false;
}

void FbWinFrameTheme::reconfigTheme() {
    if (*m_bevel_width > 20)
        *m_bevel_width = 20;
    else if (*m_bevel_width < 0)
        *m_bevel_width = 0;

    if (*m_handle_width > 200)
        *m_handle_width = 200;
    else if (*m_handle_width < 0)
        *m_handle_width = 1;

    m_button_pic_focus_gc.setForeground(*m_button_focus_color);
    m_button_pic_unfocus_gc.setForeground(*m_button_unfocus_color);

    m_iconbar_theme.reconfigTheme();
}

