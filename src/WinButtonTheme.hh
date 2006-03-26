// WinButtonTheme.hh for Fluxbox - an X11 Window manager
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

#ifndef WINBUTTONTHEME_HH
#define WINBUTTONTHEME_HH

#include "Theme.hh"
#include "FbTk/PixmapWithMask.hh"

class FbWinFrameTheme;

class WinButtonTheme: public FbTk::Theme {
public:
    WinButtonTheme(int screen_num, FbWinFrameTheme &frame_theme);
    ~WinButtonTheme();

    void reconfigTheme();

    const FbTk::PixmapWithMask &closePixmap() const { return *m_close_pm; }
    FbTk::PixmapWithMask &closePixmap() { return *m_close_pm; }
    FbTk::PixmapWithMask &closeUnfocusPixmap() { return *m_close_unfocus_pm; }
    const FbTk::PixmapWithMask &closePressedPixmap() const { return *m_close_pressed_pm; }
    FbTk::PixmapWithMask &closePressedPixmap() { return *m_close_pressed_pm; }

    const FbTk::PixmapWithMask &maximizePixmap() const { return *m_maximize_pm; }
    FbTk::PixmapWithMask &maximizePixmap() { return *m_maximize_pm; }
    FbTk::PixmapWithMask &maximizeUnfocusPixmap() { return *m_maximize_unfocus_pm; }
    const FbTk::PixmapWithMask &maximizePressedPixmap() const { return *m_maximize_pressed_pm; }
    FbTk::PixmapWithMask &maximizePressedPixmap() { return *m_maximize_pressed_pm; }

    const FbTk::PixmapWithMask &iconifyPixmap() const { return *m_iconify_pm; }
    FbTk::PixmapWithMask &iconifyPixmap() { return *m_iconify_pm; }
    FbTk::PixmapWithMask &iconifyUnfocusPixmap() { return *m_iconify_unfocus_pm; }
    const FbTk::PixmapWithMask &iconifyPressedPixmap() const { return *m_iconify_pressed_pm; }
    FbTk::PixmapWithMask &iconifyPressedPixmap() { return *m_iconify_pressed_pm; }

    const FbTk::PixmapWithMask &stickPixmap() const { return *m_stick_pm; }
    FbTk::PixmapWithMask &stickPixmap() { return *m_stick_pm; }
    FbTk::PixmapWithMask &stickUnfocusPixmap() { return *m_stick_unfocus_pm; }
    const FbTk::PixmapWithMask &stickPressedPixmap() const { return *m_stick_pressed_pm; }
    FbTk::PixmapWithMask &stickPressedPixmap() { return *m_stick_pressed_pm; }

    FbTk::PixmapWithMask &stuckPixmap() { return *m_stuck_pm; }
    FbTk::PixmapWithMask &stuckUnfocusPixmap() { return *m_stuck_unfocus_pm; }

    const FbTk::PixmapWithMask &shadePixmap() const { return *m_shade_pm; }
    FbTk::PixmapWithMask &shadePixmap() { return *m_shade_pm; }
    FbTk::PixmapWithMask &shadeUnfocusPixmap() { return *m_shade_unfocus_pm; }
    const FbTk::PixmapWithMask &shadePressedPixmap() const { return *m_shade_pressed_pm; }
    FbTk::PixmapWithMask &shadePressedPixmap() { return *m_shade_pressed_pm; }

    const FbTk::PixmapWithMask &unshadePixmap() const { return *m_unshade_pm; }
    FbTk::PixmapWithMask &unshadePixmap() { return *m_unshade_pm; }
    FbTk::PixmapWithMask &unshadeUnfocusPixmap() { return *m_unshade_unfocus_pm; }
    const FbTk::PixmapWithMask &unshadePressedPixmap() const { return *m_unshade_pressed_pm; }
    FbTk::PixmapWithMask &unshadePressedPixmap() { return *m_unshade_pressed_pm; }

    const FbTk::PixmapWithMask &menuiconPixmap() const { return *m_menuicon_pm; }
    FbTk::PixmapWithMask &menuiconPixmap() { return *m_menuicon_pm; }
    FbTk::PixmapWithMask &menuiconUnfocusPixmap() { return *m_menuicon_unfocus_pm; }
    const FbTk::PixmapWithMask &menuiconPressedPixmap() const { return *m_menuicon_pressed_pm; }
    FbTk::PixmapWithMask &menuiconPressedPixmap() { return *m_menuicon_pressed_pm; }
    
    FbTk::PixmapWithMask &titleFocusPixmap() { return *m_title_focus_pm; }
    const FbTk::PixmapWithMask &titleFocusPixmap() const { return *m_title_focus_pm; }
    FbTk::PixmapWithMask &titleUnfocusPixmap() { return *m_title_unfocus_pm; }
    const FbTk::PixmapWithMask &titleUnfocusPixmap() const { return *m_title_unfocus_pm; }

private:

    FbTk::ThemeItem<FbTk::PixmapWithMask> m_close_pm, m_close_unfocus_pm, m_close_pressed_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_maximize_pm, m_maximize_unfocus_pm, m_maximize_pressed_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_iconify_pm, m_iconify_unfocus_pm, m_iconify_pressed_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_shade_pm, m_shade_unfocus_pm, m_shade_pressed_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_unshade_pm, m_unshade_unfocus_pm, m_unshade_pressed_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_menuicon_pm, m_menuicon_unfocus_pm, m_menuicon_pressed_pm;
    // why this? we need this for the background of the appicon in WinButtons
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_title_focus_pm, m_title_unfocus_pm;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_stick_pm, m_stick_unfocus_pm, m_stick_pressed_pm;    
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_stuck_pm, m_stuck_unfocus_pm; 

    FbWinFrameTheme &m_frame_theme;
};

#endif // WINBUTTONTHEME_HH
