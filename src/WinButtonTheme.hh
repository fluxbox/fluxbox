// WinButtonTheme.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: WinButtonTheme.hh,v 1.3 2003/08/22 14:48:10 fluxgen Exp $

#ifndef WINBUTTONTHEME_HH
#define WINBUTTONTHEME_HH

#include "Theme.hh"
#include "FbPixmap.hh"

class FbWinFrameTheme;

class WinButtonTheme: public FbTk::Theme {
public:
    /// holds pixmap and a mask
    struct PixmapWithMask {
        /// scale both pixmap and mask to specified size
        void scale(unsigned int width, unsigned int height) {
            pixmap.scale(width, height);
            mask.scale(width, height);
        }
        FbTk::FbPixmap pixmap;
        FbTk::FbPixmap mask;
    };

    WinButtonTheme(int screen_num, const FbWinFrameTheme &frame_theme);
    ~WinButtonTheme();

    void reconfigTheme();

    inline const PixmapWithMask &closePixmap() const { return *m_close_pm; }
    inline PixmapWithMask &closePixmap() { return *m_close_pm; }
    inline PixmapWithMask &closeUnfocusPixmap() { return *m_close_unfocus_pm; }
    inline const PixmapWithMask &closePressedPixmap() const { return *m_close_pressed_pm; }
    inline PixmapWithMask &closePressedPixmap() { return *m_close_pressed_pm; }

    inline const PixmapWithMask &maximizePixmap() const { return *m_maximize_pm; }
    inline PixmapWithMask &maximizePixmap() { return *m_maximize_pm; }
    inline PixmapWithMask &maximizeUnfocusPixmap() { return *m_maximize_unfocus_pm; }
    inline const PixmapWithMask &maximizePressedPixmap() const { return *m_maximize_pressed_pm; }
    inline PixmapWithMask &maximizePressedPixmap() { return *m_maximize_pressed_pm; }

    inline const PixmapWithMask &iconifyPixmap() const { return *m_iconify_pm; }
    inline PixmapWithMask &iconifyPixmap() { return *m_iconify_pm; }
    inline PixmapWithMask &iconifyUnfocusPixmap() { return *m_iconify_unfocus_pm; }
    inline const PixmapWithMask &iconifyPressedPixmap() const { return *m_iconify_pressed_pm; }
    inline PixmapWithMask &iconifyPressedPixmap() { return *m_iconify_pressed_pm; }

    inline const PixmapWithMask &stickPixmap() const { return *m_stick_pm; }
    inline PixmapWithMask &stickPixmap() { return *m_stick_pm; }
    inline PixmapWithMask &stickUnfocusPixmap() { return *m_stick_unfocus_pm; }
    inline const PixmapWithMask &stickPressedPixmap() const { return *m_stick_pressed_pm; }
    inline PixmapWithMask &stickPressedPixmap() { return *m_stick_pressed_pm; }

    inline PixmapWithMask &stuckPixmap() { return *m_stuck_pm; }
    inline PixmapWithMask &stuckUnfocusPixmap() { return *m_stuck_unfocus_pm; }

    inline const PixmapWithMask &shadePixmap() const { return *m_shade_pm; }
    inline PixmapWithMask &shadePixmap() { return *m_shade_pm; }
    inline PixmapWithMask &shadeUnfocusPixmap() { return *m_shade_unfocus_pm; }
    inline const PixmapWithMask &shadePressedPixmap() const { return *m_shade_pressed_pm; }
    inline PixmapWithMask &shadePressedPixmap() { return *m_shade_pressed_pm; }

private:

    FbTk::ThemeItem<PixmapWithMask> m_close_pm, m_close_unfocus_pm, m_close_pressed_pm;
    FbTk::ThemeItem<PixmapWithMask> m_maximize_pm, m_maximize_unfocus_pm, m_maximize_pressed_pm;
    FbTk::ThemeItem<PixmapWithMask> m_iconify_pm, m_iconify_unfocus_pm, m_iconify_pressed_pm;
    FbTk::ThemeItem<PixmapWithMask> m_shade_pm, m_shade_unfocus_pm, m_shade_pressed_pm;
    FbTk::ThemeItem<PixmapWithMask> m_stick_pm, m_stick_unfocus_pm, m_stick_pressed_pm;    
    FbTk::ThemeItem<PixmapWithMask> m_stuck_pm, m_stuck_unfocus_pm; 

    const FbWinFrameTheme &m_frame_theme;
};

#endif // WINBUTTONTHEME_HH
