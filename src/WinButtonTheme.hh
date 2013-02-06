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

#ifndef WINBUTTONTHEME_HH
#define WINBUTTONTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/PixmapWithMask.hh"

#include <string>

class FbWinFrameTheme;

class WinButtonTheme: public FbTk::Theme,
                      public FbTk::ThemeProxy<WinButtonTheme> {
public:
    WinButtonTheme(int screen_num,
                   const std::string &extra, const std::string &altextra,
                   FbTk::ThemeProxy<FbWinFrameTheme> &frame_theme);
    ~WinButtonTheme();

    void reconfigTheme();

    const FbTk::PixmapWithMask &closePixmap() const { return *m_close_pm; }
    FbTk::PixmapWithMask &closePixmap() { return *m_close_pm; }

    const FbTk::PixmapWithMask &maximizePixmap() const { return *m_maximize_pm; }
    FbTk::PixmapWithMask &maximizePixmap() { return *m_maximize_pm; }

    const FbTk::PixmapWithMask &iconifyPixmap() const { return *m_iconify_pm; }
    FbTk::PixmapWithMask &iconifyPixmap() { return *m_iconify_pm; }

    const FbTk::PixmapWithMask &stickPixmap() const { return *m_stick_pm; }
    FbTk::PixmapWithMask &stickPixmap() { return *m_stick_pm; }

    const FbTk::PixmapWithMask &stuckPixmap() const { return *m_stuck_pm; }
    FbTk::PixmapWithMask &stuckPixmap() { return *m_stuck_pm; }

    const FbTk::PixmapWithMask &shadePixmap() const { return *m_shade_pm; }
    FbTk::PixmapWithMask &shadePixmap() { return *m_shade_pm; }

    const FbTk::PixmapWithMask &unshadePixmap() const { return *m_unshade_pm; }
    FbTk::PixmapWithMask &unshadePixmap() { return *m_unshade_pm; }

    const FbTk::PixmapWithMask &menuiconPixmap() const { return *m_menuicon_pm; }
    FbTk::PixmapWithMask &menuiconPixmap() { return *m_menuicon_pm; }

    FbTk::PixmapWithMask &titlePixmap() { return *m_title_pm; }
    const FbTk::PixmapWithMask &titlePixmap() const { return *m_title_pm; }


    FbTk::PixmapWithMask &leftHalfPixmap() { return *m_lefthalf_pm; }
    const FbTk::PixmapWithMask &leftHalfPixmap() const { return *m_lefthalf_pm; }

    FbTk::PixmapWithMask &rightHalfPixmap() { return *m_righthalf_pm; }
    const FbTk::PixmapWithMask &rightHalfPixmap() const { return *m_righthalf_pm; }


    virtual FbTk::Signal<> &reconfigSig() { return FbTk::Theme::reconfigSig(); }

    virtual WinButtonTheme &operator *() { return *this; }
    virtual const WinButtonTheme &operator *() const { return *this; }

private:

    FbTk::ThemeItem<FbTk::PixmapWithMask> m_close_pm, m_maximize_pm,
            m_iconify_pm, m_shade_pm, m_unshade_pm, m_menuicon_pm, m_title_pm,
            m_stick_pm, m_stuck_pm, m_lefthalf_pm, m_righthalf_pm;

    FbTk::ThemeProxy<FbWinFrameTheme> &m_frame_theme;
};

#endif // WINBUTTONTHEME_HH
