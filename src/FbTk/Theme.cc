// Theme.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at linuxmail.org)
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

// $Id: Theme.cc,v 1.4 2003/05/08 15:10:57 fluxgen Exp $

#include "Theme.hh"

#include "../XrmDatabaseHelper.hh"
#include "Font.hh"
#include "Color.hh"
#include "Texture.hh"
#include "App.hh"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include <X11/xpm.h>
#endif // HAVE_XPM

#include <iostream>

using namespace std;
namespace FbTk {

// create default handlers for Color, Font, and Texture

template <>
void ThemeItem<FbTk::Font>::setDefaultValue() {
    if (!m_value.load("fixed")) {
        cerr<<"FbTk::ThemeItem<FbTk::Font>: Warning! Failed to load default value 'fixed'"<<endl;
    }
}

template <>
void ThemeItem<FbTk::Font>::setFromString(const char *str) {
    if (m_value.load(str) == false) {
        cerr<<"Failed to load font: "<<str<<endl;
        cerr<<"Setting default value"<<endl;
        setDefaultValue();
    }
    
}

// do nothing
template <>
void ThemeItem<FbTk::Font>::load() {
}


template <>
void ThemeItem<FbTk::Texture>::setFromString(const char *str) {
    m_value.setFromString(str);
}

template <>
void ThemeItem<FbTk::Texture>::setDefaultValue() {
    m_value.setType(0);
}

template <>
void ThemeItem<FbTk::Texture>::load() {
    string color_name(ThemeManager::instance().
                      resourceValue(name()+".color", altName()+".Color"));
    string colorto_name(ThemeManager::instance().
                        resourceValue(name()+".colorTo", altName()+".ColorTo"));
    string pixmap_name(ThemeManager::instance().
                       resourceValue(name()+".pixmap", altName()+".Pixmap"));

    m_value.color().setFromString(color_name.c_str(), m_tm.screenNum());
    m_value.colorTo().setFromString(colorto_name.c_str(), m_tm.screenNum());

#ifdef HAVE_XPM
    XpmAttributes xpm_attr;
    xpm_attr.valuemask = 0;
    Display *dpy = FbTk::App::instance()->display();
    Pixmap pm = 0, mask = 0;
    int retvalue = XpmReadFileToPixmap(dpy,
                                       RootWindow(dpy, m_tm.screenNum()), 
                                       const_cast<char *>(pixmap_name.c_str()),
                                       &pm,
                                       &mask, &xpm_attr);
    if (retvalue == 0) { // success
        m_value.pixmap() = pm;
        if (mask != 0)
            XFreePixmap(dpy, mask);
    } else { // failure
#ifdef DEBUG
        cerr<<"Couldn't load pixmap: "<<pixmap_name<<endl;
#endif // DEBUG
        // create empty pixmap
        m_value.pixmap() = FbTk::FbPixmap();
    }
#endif // HAVE_XPM

}


template <>
void ThemeItem<FbTk::Color>::setDefaultValue() {
    m_value.setFromString("white", m_tm.screenNum());
}

template <>
void ThemeItem<FbTk::Color>::setFromString(const char *str) {
    if (!m_value.setFromString(str, m_tm.screenNum()))
        setDefaultValue();
}

// does nothing
template <>
void ThemeItem<FbTk::Color>::load() { }

Theme::Theme(int screen_num):m_screen_num(screen_num) {

    if (!ThemeManager::instance().registerTheme(*this)) {
        // should it be fatal or not?
        cerr<<"FbTk::Theme Warning: Failed to register Theme"<<endl;
    }
}

Theme::~Theme() {
    if (!ThemeManager::instance().unregisterTheme(*this)) {
#ifdef DEBUG
        cerr<<"Warning: Theme not registered!"<<endl;
#endif // DEBUG
    }
}

ThemeManager &ThemeManager::instance() {
    static ThemeManager tm;
    return tm;
}

ThemeManager::ThemeManager():
    m_max_screens(ScreenCount(FbTk::App::instance()->display())) {

}

bool ThemeManager::registerTheme(Theme &tm) {
    // valid screen num?
    if (m_max_screens < tm.screenNum() || tm.screenNum() < 0)
        return false;
    // TODO: use find and return false if it's already there
    // instead of unique 
    m_themelist.push_back(&tm);
    m_themelist.unique(); 
    return true;
}

bool ThemeManager::unregisterTheme(Theme &tm) {
    m_themelist.remove(&tm);
    return true;
}

bool ThemeManager::load(const char *filename) {
	
    if (!m_database.load(filename))
        return false;

    //get list and go throu all the resources and load them
    ThemeList::iterator theme_it = m_themelist.begin();
    const ThemeList::iterator theme_it_end = m_themelist.end();
    for (; theme_it != theme_it_end; ++theme_it) {
        loadTheme(**theme_it);
    }

    return true;
}

void ThemeManager::loadTheme(Theme &tm) {
	
    XrmValue value;
    char *value_type;
    
    std::list<ThemeItem_base *>::iterator i = tm.itemList().begin();
    std::list<ThemeItem_base *>::iterator i_end = tm.itemList().end();
    for (; i != i_end; ++i) {
        ThemeItem_base *resource = *i;
        if (XrmGetResource(*m_database, resource->name().c_str(),
                           resource->altName().c_str(), &value_type, &value)) {
            resource->setFromString(value.addr);
            resource->load(); // load additional stuff by the ThemeItem
        } else {
            cerr<<"Failed to read theme item: "<<resource->name()<<endl;
            cerr<<"Setting default value"<<endl;
            resource->setDefaultValue();
        }
    }
    // send reconfiguration signal to theme
    tm.reconfigTheme();

}

std::string ThemeManager::resourceValue(const std::string &name, const std::string &altname) {
    XrmValue value;
    char *value_type;
	
    if (*m_database != 0 && XrmGetResource(*m_database, name.c_str(),
                                           altname.c_str(), &value_type, &value) && value.addr != 0) {
        return string(value.addr);
    }
    return "";
}


}; // end namespace FbTk
