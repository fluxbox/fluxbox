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

// $Id: MenuTheme.cc,v 1.14 2004/01/08 22:07:58 fluxgen Exp $

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
    m_border_width(*this, "menu.borderWidth", "Menu.BorderWidth"),
    m_bevel_width(*this, "menu.bevelWidth", "Menu.BevelWidth"),
    m_border_color(*this, "menu.borderColor", "Menu.BorderColor"),
    m_bullet_pixmap(*this, "menu.submenu.pixmap", "Menu.Submenu.Pixmap"),
    m_selected_pixmap(*this, "menu.selected.pixmap", "Menu.Selected.Pixmap"),
    m_unselected_pixmap(*this, "menu.unselected.pixmap", "Menu.Unselected.Pixmap"),
    m_display(FbTk::App::instance()->display()),
    t_text_gc(RootWindow(m_display, screen_num)),
    f_text_gc(RootWindow(m_display, screen_num)),
    h_text_gc(RootWindow(m_display, screen_num)),
    d_text_gc(RootWindow(m_display, screen_num)),
    hilite_gc(RootWindow(m_display, screen_num)),
    m_alpha(255),
    m_menumode(DELAY_OPEN),
    m_delayopen(0), // no delay as default
    m_delayclose(0) // no delay as default
{ 

    // set default values
    *m_border_width = 0;
    *m_bevel_width = 0;
    *m_border_width = 0;

    t_text_gc.setForeground(*t_text);
    f_text_gc.setForeground(*f_text);
    h_text_gc.setForeground(*h_text);
    d_text_gc.setForeground(*d_text);
    hilite_gc.setForeground(hilite->color());

}

MenuTheme::~MenuTheme() {

}

void MenuTheme::reconfigTheme() {
    // clamp to "normal" size
    if (*m_bevel_width > 20)
        *m_bevel_width = 20;
    if (*m_border_width > 20)
        *m_border_width = 20;

    m_bullet_pixmap->scale(frameFont().height(), frameFont().height());
    m_selected_pixmap->scale(frameFont().height(), frameFont().height());
    m_unselected_pixmap->scale(frameFont().height(), frameFont().height());

    t_text_gc.setForeground(*t_text);
    f_text_gc.setForeground(*f_text);
    h_text_gc.setForeground(*h_text);
    d_text_gc.setForeground(*d_text);
    hilite_gc.setForeground(hilite->color());
}

bool MenuTheme::fallback(ThemeItem_base &item) {
    if (item.name() == "menu.borderWidth") {
        return ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    } else if (item.name() == "menu.borderColor") {
        return ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    } else if (item.name() == "menu.bevelWidth") {
        return ThemeManager::instance().loadItem(item, "bevelWidth", "BevelWidth");
    }

    return false;
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
