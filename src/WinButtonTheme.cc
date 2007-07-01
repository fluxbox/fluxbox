// WinButtonTheme.cc for Fluxbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#include "WinButtonTheme.hh"

#include "FbTk/App.hh"
#include "FbTk/Image.hh"
#include "FbTk/PixmapWithMask.hh"

#include "FbWinFrameTheme.hh"

WinButtonTheme::WinButtonTheme(int screen_num, FbWinFrameTheme &frame_theme):
    FbTk::Theme(screen_num),
    m_close_pm(*this, "window.close.pixmap", "Window.Close.Pixmap"),
    m_close_unfocus_pm(*this, "window.close.unfocus.pixmap", "Window.Close.Unfocus.Pixmap"),
    m_close_pressed_pm(*this, "window.close.pressed.pixmap", "Window.Close.Pressed.Pixmap"),
    m_maximize_pm(*this, "window.maximize.pixmap", "Window.Maximize.Pixmap"),
    m_maximize_unfocus_pm(*this, "window.maximize.unfocus.pixmap", "Window.Maximize.Unfocus.pixmap"),
    m_maximize_pressed_pm(*this, "window.maximize.pressed.pixmap", "Window.Maximize.Pressed.Pixmap"),
    m_iconify_pm(*this, "window.iconify.pixmap", "Window.Iconify.Pixmap"),
    m_iconify_unfocus_pm(*this, "window.iconify.unfocus.pixmap", "Window.Iconify.Unfocus.Pixmap"),    
    m_iconify_pressed_pm(*this, "window.iconify.pressed.pixmap", "Window.Iconify.Pressed.Pixmap"),
    m_shade_pm(*this, "window.shade.pixmap", "Window.Shade.Pixmap"),
    m_shade_unfocus_pm(*this, "window.shade.unfocus.pixmap", "Window.Shade.Unfocus.Pixmap"),
    m_shade_pressed_pm(*this, "window.shade.pressed.pixmap", "Window.Shade.Pressed.Pixmap"),
    m_unshade_pm(*this, "window.unshade.pixmap", "Window.Unshade.Pixmap"),
    m_unshade_unfocus_pm(*this, "window.unshade.unfocus.pixmap", "Window.Unshade.Unfocus.Pixmap"),
    m_unshade_pressed_pm(*this, "window.unshade.pressed.pixmap", "Window.Unshade.Pressed.Pixmap"),
    m_menuicon_pm(*this, "window.menuicon.pixmap", "Window.MenuIcon.Pixmap"),
    m_menuicon_unfocus_pm(*this, "window.menuicon.unfocus.pixmap", "Window.MenuIcon.Unfocus.Pixmap"),
    m_menuicon_pressed_pm(*this, "window.menuicon.pressed.pixmap", "Window.MenuIcon.Pressed.Pixmap"),
    m_title_focus_pm(*this, "window.title.focus.pixmap", "Window.Title.Focus.Pixmap"),
    m_title_unfocus_pm(*this, "window.title.unfocus.pixmap", "Window.Title.UnFocus.Pixmap"),
    m_stick_pm(*this, "window.stick.pixmap", "Window.Stick.Pixmap"),
    m_stick_unfocus_pm(*this, "window.stick.unfocus.pixmap", "Window.Stick.Unfocus.Pixmap"),
    m_stick_pressed_pm(*this, "window.stick.pressed.pixmap", "Window.Stick.Pressed.Pixmap"),
    m_stuck_pm(*this, "window.stuck.pixmap", "Window.Stuck.Pixmap"),
    m_stuck_unfocus_pm(*this, "window.stuck.unfocus.pixmap", "Window.Stuck.Unfocus.Pixmap"),
    m_frame_theme(frame_theme) {

}

WinButtonTheme::~WinButtonTheme() {

}

void WinButtonTheme::reconfigTheme() {
    // rescale the pixmaps to match frame theme height

    unsigned int size = m_frame_theme.titleHeight()
                        - 2 * m_frame_theme.bevelWidth();
    if (m_frame_theme.titleHeight() == 0) {
        // calculate height from font and border width to scale pixmaps
        size = m_frame_theme.font().height() + 2;

    } // else  use specified height to scale pixmaps

    // scale all pixmaps
    m_close_pm->scale(size, size);
    m_close_unfocus_pm->scale(size, size);
    m_close_pressed_pm->scale(size, size);

    m_maximize_pm->scale(size, size);
    m_maximize_unfocus_pm->scale(size, size);
    m_maximize_pressed_pm->scale(size, size);

    m_menuicon_pm->scale(size, size);
    m_menuicon_unfocus_pm->scale(size, size);
    m_menuicon_pressed_pm->scale(size, size);

    m_iconify_pm->scale(size, size);
    m_iconify_unfocus_pm->scale(size, size);
    m_iconify_pressed_pm->scale(size, size);

    m_shade_pm->scale(size, size);
    m_shade_unfocus_pm->scale(size, size);
    m_shade_pressed_pm->scale(size, size);

    m_unshade_pm->scale(size, size);
    m_unshade_unfocus_pm->scale(size, size);
    m_unshade_pressed_pm->scale(size, size);

    m_title_focus_pm->scale(size, size);
    m_title_unfocus_pm->scale(size, size);

    m_stick_pm->scale(size, size);
    m_stick_unfocus_pm->scale(size, size);
    m_stick_pressed_pm->scale(size, size);

    m_stuck_pm->scale(size, size);
    m_stuck_unfocus_pm->scale(size, size);
}

