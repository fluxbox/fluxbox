// MenuTheme.cc for FbTk
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: MenuTheme.cc,v 1.9 2003/08/16 12:23:17 fluxgen Exp $

#include "MenuTheme.hh"

#include "Color.hh"
#include "Texture.hh"
#include "Font.hh"
#include "App.hh"
#include "StringUtil.hh"

#include <cstdio>

namespace FbTk {

MenuTheme::MenuTheme(int screen_num):
    FbTk::Theme(screen_num), 
    t_text(*this, "menu.title.textColor", "Menu.Title.TextColor"),
    f_text(*this, "menu.frame.textColor", "Menu.Frame.TextColor"),
    h_text(*this, "menu.hilite.textColor", "Menu.Hilite.TextColor"),
    d_text(*this, "menu.frame.disableColor", "Menu.Frame.DisableColor"),
    title(*this, "menu.title", "Menu.Title"),
    frame(*this, "menu.frame", "Menu.Frame"),
    hilite(*this, "menu.hilite", "Menu.Hilite"),
    titlefont(*this, "menu.title.font", "Menu.Title.Font"),
    framefont(*this, "menu.frame.font", "Menu.Frame.Font"),
    framefont_justify(*this, "menu.frame.justify", "Menu.Frame.Justify"),
    titlefont_justify(*this, "menu.title.justify", "Menu.Title.Justify"),
    bullet_pos(*this, "menu.bullet.position", "Menu.Bullet.Position"),
    m_bullet(*this, "menu.bullet", "Menu.Bullet"),
    m_border_width(*this, "borderWidth", "BorderWidth"),
    m_bevel_width(*this, "bevelWidth", "BevelWidth"),
    m_border_color(*this, "borderColor", "BorderColor"),
    m_display(FbTk::App::instance()->display()),
    m_alpha(255)
{ 
    // set default values
    *m_border_width = 0;
    *m_bevel_width = 0;

    Window rootwindow = RootWindow(m_display, screen_num);

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;
    gcv.foreground = t_text->pixel();

    t_text_gc = XCreateGC(m_display, rootwindow, gc_value_mask, &gcv);

    gcv.foreground = f_text->pixel();

    f_text_gc = XCreateGC(m_display, rootwindow, gc_value_mask, &gcv);

    gcv.foreground = h_text->pixel();
    h_text_gc =	XCreateGC(m_display, rootwindow, gc_value_mask, &gcv);

    gcv.foreground = d_text->pixel();
    d_text_gc =	XCreateGC(m_display, rootwindow, gc_value_mask, &gcv);

    gcv.foreground = hilite->color().pixel();
    hilite_gc =	XCreateGC(m_display, rootwindow, gc_value_mask, &gcv);
}

MenuTheme::~MenuTheme() {
    XFreeGC(m_display, t_text_gc);
    XFreeGC(m_display, f_text_gc);
    XFreeGC(m_display, h_text_gc);
    XFreeGC(m_display, d_text_gc);
    XFreeGC(m_display, hilite_gc);
}

void MenuTheme::reconfigTheme() {
    // clamp to "normal" size
    if (*m_bevel_width > 20)
        *m_bevel_width = 20;
    if (*m_border_width > 20)
        *m_border_width = 20;

    XGCValues gcv;
    unsigned long gc_value_mask = GCForeground;
    
    gcv.foreground = t_text->pixel();
	
    XChangeGC(m_display, t_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = f_text->pixel();	
	
    XChangeGC(m_display, f_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = h_text->pixel();
    XChangeGC(m_display, h_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = d_text->pixel();
    XChangeGC(m_display, d_text_gc,
              gc_value_mask, &gcv);

    gcv.foreground = hilite->color().pixel();
    XChangeGC(m_display, hilite_gc,
              gc_value_mask, &gcv);

    // notify any listeners
    m_theme_change_sig.notify();
}


template <>
void ThemeItem<MenuTheme::BulletType>::setDefaultValue() {
    m_value = MenuTheme::EMPTY;
}

template <>
void ThemeItem<MenuTheme::BulletType>::setFromString(const char *str) {

    // do nothing
    if (StringUtil::strcasestr(str, "empty") != 0)
        m_value = MenuTheme::EMPTY;
    else if (StringUtil::strcasestr(str, "square") != 0)
        m_value = MenuTheme::SQUARE;
    else if (StringUtil::strcasestr(str, "triangle") != 0)
        m_value = MenuTheme::TRIANGLE;
    else if (StringUtil::strcasestr(str, "diamond") != 0)
        m_value = MenuTheme::DIAMOND;
    else
        setDefaultValue();
}

template <>
void ThemeItem<MenuTheme::BulletType>::load() {
    // do nothing, we don't have anything extra to load
}

template <>
void ThemeItem<unsigned int>::setDefaultValue() {
    m_value = 0;
}

template <>
void ThemeItem<unsigned int>::setFromString(const char *str) {
    sscanf(str, "%d", &m_value);
}

template <>
void ThemeItem<unsigned int>::load() {
}


}; // end namespace  FbTk
