// WinButtonTheme.cc for Fluxbox - an X11 Window manager
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

// $Id: WinButtonTheme.cc,v 1.1 2003/04/28 22:30:34 fluxgen Exp $

#include "WinButtonTheme.hh"

#include "App.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include <X11/xpm.h>
#endif // HAVE_XPM

// not used
template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
load() { }

template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
setDefaultValue() {
    // create empty pixmap
    (*this)->pixmap = FbTk::FbPixmap(); // pixmap
    (*this)->mask = FbTk::FbPixmap(); // mask
    (*this)->pixmap_scaled = FbTk::FbPixmap();
    (*this)->mask_scaled = FbTk::FbPixmap();
}

template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
setFromString(const char *str) {
    if (str == 0)
        setDefaultValue();
    else {
#ifdef HAVE_XPM
        XpmAttributes xpm_attr;
        xpm_attr.valuemask = 0;
        Display *dpy = FbTk::App::instance()->display();
        Pixmap pm = 0, mask = 0;
        int retvalue = XpmReadFileToPixmap(dpy,
                                           //!! TODO need the right root window
                                           RootWindow(dpy, 0), 
                                           const_cast<char *>(str),
                                           &pm,
                                           &mask, &xpm_attr);
        if (retvalue == 0) { // success
            (*this)->pixmap = pm;
            (*this)->mask = mask;            
        } else {  // failure        
            setDefaultValue();
        }
#else
        setDefaultValue();
#endif // HAVE_XPM

    } 
}

WinButtonTheme::WinButtonTheme(int screen_num):
    FbTk::Theme(screen_num),
    m_close_pm(*this, "window.close.pixmap", "Window.Close.Pixmap"),
    m_close_pressed_pm(*this, "window.close.pressed.pixmap", "Window.Close.Pressed.Pixmap"),
    m_maximize_pm(*this, "window.maximize.pixmap", "Window.Maximize.Pixmap"),
    m_maximize_pressed_pm(*this, "window.maximize.pressed.pixmap", "Window.Maximize.Pressed.Pixmap"),
    m_iconify_pm(*this, "window.iconify.pixmap", "Window.Iconify.Pixmap"),
    m_iconify_pressed_pm(*this, "window.iconify.pressed.pixmap", "Window.Iconify.Pressed.Pixmap"),
    m_shade_pm(*this, "window.shade.pixmap", "Window.Shade.Pixmap"),
    m_shade_pressed_pm(*this, "window.shade.pressed.pixmap", "Window.Shade.Pressed.Pixmap"),
    m_stick_pm(*this, "window.stick.pixmap", "Window.Stick.Pixmap"),
    m_stick_pressed_pm(*this, "window.stick.pressed.pixmap", "Window.Stick.Pressed.Pixmap") {

}

WinButtonTheme::~WinButtonTheme() {

}

void WinButtonTheme::reconfigTheme() {
    reconfigSig().notify();
}

