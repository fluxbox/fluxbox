// Theme.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Theme.cc,v 1.16 2003/08/28 15:46:13 fluxgen Exp $

#include "Theme.hh"

#include "XrmDatabaseHelper.hh"
#include "Font.hh"
#include "Color.hh"
#include "Texture.hh"
#include "App.hh"
#include "Image.hh"
#include "PixmapWithMask.hh"
#include "StringUtil.hh"

#include <cstdio>
#include <memory>
#include <iostream>
using namespace std;

namespace FbTk {

// create default handlers for Color, Font, Texture, int and string
template <>
void FbTk::ThemeItem<std::string>::load() { }

template <>
void FbTk::ThemeItem<std::string>::setDefaultValue() { 
    *(*this) = ""; 
}

template <>
void FbTk::ThemeItem<std::string>::setFromString(const char *str) { 
    *(*this) = (str ? str : ""); 
}

template <>
void FbTk::ThemeItem<int>::load() { }

template <>
void FbTk::ThemeItem<int>::setDefaultValue() {
    *(*this) = 0;
}

template <>
void FbTk::ThemeItem<int>::setFromString(const char *str) {
    if (str == 0)
        return;
    sscanf(str, "%d", &m_value);
}

template <>
void ThemeItem<FbTk::Font>::setDefaultValue() {
    if (!m_value.load("fixed")) {
        cerr<<"FbTk::ThemeItem<FbTk::Font>: Warning! Failed to load default value 'fixed'"<<endl;
    }
}

template <>
void ThemeItem<FbTk::Font>::setFromString(const char *str) {
    if (m_value.load(str) == false) {
        cerr<<"FbTk::Theme: Error loading font "<<
            ((m_value.isAntialias() || m_value.utf8()) ? "(" : "")<<

            (m_value.isAntialias() ? "antialias" : "")<<
            (m_value.utf8() ? " utf8" : "")<<

            ((m_value.isAntialias() || m_value.utf8()) ? ") " : "")<<
            "for \""<<name()<<"\" or \""<<altName()<<"\": "<<str<<endl;

        cerr<<"FbTk::Theme: Setting default value"<<endl;
        setDefaultValue();
    }
    
}

// do nothing
template <>
void ThemeItem<FbTk::Font>::load() {
}

template <>
void ThemeItem<FbTk::Texture>::setDefaultValue() {
    m_value.setType(FbTk::Texture::FLAT | FbTk::Texture::SOLID);
}

template <>
void ThemeItem<FbTk::Texture>::setFromString(const char *str) {
    m_value.setFromString(str);
    if (m_value.type() == 0) // failed to set value
        setDefaultValue();
}


template <>
void ThemeItem<FbTk::Texture>::load() {
    string color_name(ThemeManager::instance().
                      resourceValue(name()+".color", altName()+".Color"));
    string colorto_name(ThemeManager::instance().
                        resourceValue(name()+".colorTo", altName()+".ColorTo"));
    string pixmap_name(ThemeManager::instance().
                       resourceValue(name()+".pixmap", altName()+".Pixmap"));


    // set default value if we failed to load color
    if (!m_value.color().setFromString(color_name.c_str(), m_tm.screenNum()))
        m_value.color().setFromString("darkgray", m_tm.screenNum());

    if (!m_value.colorTo().setFromString(colorto_name.c_str(), m_tm.screenNum()))
        m_value.colorTo().setFromString("white", m_tm.screenNum());
           

    std::auto_ptr<PixmapWithMask> pm(Image::load(pixmap_name, m_tm.screenNum()));
    if (pm.get() == 0) {
        cerr<<"Resource("<<name()+".pixmap"<<"): Failed to load image: "<<pixmap_name<<endl;
        m_value.pixmap() = 0;
    } else
        m_value.pixmap() = pm->pixmap().release();
}


// not used
template <>
void FbTk::ThemeItem<PixmapWithMask>::
load() { }

template <>
void FbTk::ThemeItem<PixmapWithMask>::
setDefaultValue() {
    // create empty pixmap
    (*this)->pixmap() = 0;
    (*this)->mask() = 0;
}

template <>
void FbTk::ThemeItem<PixmapWithMask>::
setFromString(const char *str) {
    if (str == 0)
        setDefaultValue();
    else {
        std::auto_ptr<FbTk::PixmapWithMask> pm(Image::load(str, m_tm.screenNum()));
        if (pm.get() == 0)
            setDefaultValue();
        else {
            (*this)->pixmap() = pm->pixmap().release();
            (*this)->mask() = pm->mask().release();
        }
    } 
}

template <>
void ThemeItem<FbTk::Color>::setDefaultValue() {
    m_value.setFromString("white", m_tm.screenNum());
}

template <>
void ThemeItem<FbTk::Color>::setFromString(const char *str) {
    if (!m_value.setFromString(str, m_tm.screenNum())) {
        cerr<<"FbTk::Theme: Error loading color value for \""<<name()<<"\" or \""<<altName()<<"\"."<<endl;
        setDefaultValue();
    }
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

bool ThemeManager::load(const std::string &filename) {
    
    if (!m_database.load(FbTk::StringUtil::expandFilename(filename).c_str()))
        return false;

    //get list and go throu all the resources and load them
    ThemeList::iterator theme_it = m_themelist.begin();
    const ThemeList::iterator theme_it_end = m_themelist.end();
    for (; theme_it != theme_it_end; ++theme_it) {
        loadTheme(**theme_it);
    }
    // notify all themes that we reconfigured
    theme_it = m_themelist.begin();
    for (; theme_it != theme_it_end; ++theme_it) {
        // send reconfiguration signal to theme and listeners
        (*theme_it)->reconfigTheme();
        (*theme_it)->reconfigSig().notify();
    }
    return true;
}

void ThemeManager::loadTheme(Theme &tm) {
    std::list<ThemeItem_base *>::iterator i = tm.itemList().begin();
    std::list<ThemeItem_base *>::iterator i_end = tm.itemList().end();
    for (; i != i_end; ++i) {
        ThemeItem_base *resource = *i;
        if (!loadItem(*resource)) {
            // try fallback resource in theme
            if (!tm.fallback(*resource)) {
                cerr<<"Failed to read theme item: "<<resource->name()<<endl;
                cerr<<"Setting default value"<<endl;
                resource->setDefaultValue();                
            }
        }
    }
    // send reconfiguration signal to theme and listeners
}

bool ThemeManager::loadItem(ThemeItem_base &resource) {
    return loadItem(resource, resource.name(), resource.altName());
}

/// handles resource item loading with specific name/altname
bool ThemeManager::loadItem(ThemeItem_base &resource, const std::string &name, const std::string &alt_name) {
    XrmValue value;
    char *value_type;

    if (XrmGetResource(*m_database, name.c_str(),
                       alt_name.c_str(), &value_type, &value)) {
        resource.setFromString(value.addr);
        resource.load(); // load additional stuff by the ThemeItem
    } else
        return false;

    return true;
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

/*
void ThemeManager::listItems() {
    ThemeList::iterator it = m_themelist.begin();
    ThemeList::iterator it_end = m_themelist.end();
    for (; it != it_end; ++it) {
        std::list<ThemeItem_base *>::iterator item = (*it)->itemList().begin();
        std::list<ThemeItem_base *>::iterator item_end = (*it)->itemList().end();
        for (; item != item_end; ++item) {
            cerr<<(*item)->name()<<":"<<endl;
            if (typeid(**item) == typeid(ThemeItem<Texture>)) {
                cerr<<(*item)->name()<<".pixmap:"<<endl;
                cerr<<(*item)->name()<<".color:"<<endl;
                cerr<<(*item)->name()<<".colorTo:"<<endl;
            }
        }
    }
             
}
*/
}; // end namespace FbTk
