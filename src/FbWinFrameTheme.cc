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

#include "FbWinFrameTheme.hh"
#include "IconbarTheme.hh"

#include "FbTk/App.hh"
#include "FbTk/Util.hh"

#include <X11/cursorfont.h>

FbWinFrameTheme::FbWinFrameTheme(int screen_num, const std::string &extra,
                                 const std::string &altextra):
    FbTk::Theme(screen_num),
    m_title(*this, "window.title" + extra, "Window.Title" + altextra),
    m_handle(*this, "window.handle" + extra, "Window.Handle" + altextra),
    m_button(*this, "window.button" + extra, "Window.Button" + altextra),
    m_button_pressed(*this, "window.button.pressed", "Window.Button.Pressed"),
    m_grip(*this, "window.grip" + extra, "Window.Grip" + altextra),
    m_button_color(*this, "window.button" + extra + ".picColor",
                   "Window.Button" + altextra + ".PicColor"),
    m_font(*this, "window.font", "Window.Font"),
    m_shape_place(*this, "window.roundCorners", "Window.RoundCorners"),
    m_title_height(*this, "window.title.height", "Window.Title.Height"),
    m_bevel_width(*this, "window.bevelWidth", "Window.BevelWidth"),
    m_handle_width(*this, "window.handleWidth", "Window.handleWidth"),
    m_border(*this, "window" + extra, "Window" + altextra),
    m_button_pic_gc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_alpha(255),
    m_iconbar_theme(screen_num, "window.label" + extra,
                    "Window.Label" + altextra) {

    *m_title_height = 0;
    m_font->load(FbTk::Font::DEFAULT_FONT);

    // create cursors
    Display *disp = FbTk::App::instance()->display();
    m_cursor_move = XCreateFontCursor(disp, XC_fleur);
    m_cursor_lower_left_angle = XCreateFontCursor(disp, XC_bottom_left_corner);
    m_cursor_lower_right_angle = XCreateFontCursor(disp, XC_bottom_right_corner);
    m_cursor_upper_right_angle = XCreateFontCursor(disp, XC_top_right_corner);
    m_cursor_upper_left_angle = XCreateFontCursor(disp, XC_top_left_corner);
    m_cursor_left_side = XCreateFontCursor(disp, XC_left_side);
    m_cursor_top_side = XCreateFontCursor(disp, XC_top_side);
    m_cursor_right_side = XCreateFontCursor(disp, XC_right_side);
    m_cursor_bottom_side = XCreateFontCursor(disp, XC_bottom_side);

    FbTk::ThemeManager::instance().loadTheme(*this);
    reconfigTheme();
}

FbWinFrameTheme::~FbWinFrameTheme() {

}

bool FbWinFrameTheme::fallback(FbTk::ThemeItem_base &item) {
    if (item.name() == "window.focus.borderWidth" ||
        item.name() == "window.unfocus.borderWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "window.borderWidth", "Window.BorderWidth") ||
               FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    else if (item.name() == "window.focus.borderColor" || item.name() == "window.unfocus.borderColor")
        return FbTk::ThemeManager::instance().loadItem(item, "window.borderColor", "Window.BorderColor") ||
               FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    else if (item.name() == "window.bevelWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "bevelWidth", "bevelWidth");
    else if (item.name() == "window.handleWidth")
        return FbTk::ThemeManager::instance().loadItem(item, "handleWidth", "HandleWidth");

    return false;
}

void FbWinFrameTheme::reconfigTheme() {
    *m_bevel_width = FbTk::Util::clamp(*m_bevel_width, 0, 20);
    if (*m_handle_width < 0)
        *m_handle_width = 1;
    *m_handle_width = FbTk::Util::clamp(*m_handle_width, 0, 200);
    m_button_pic_gc.setForeground(*m_button_color);
    m_iconbar_theme.reconfigTheme();
}

