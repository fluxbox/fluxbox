// ThemeItems.cc for FbTk - Fluxbox ToolKit
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

// $Id: ThemeItems.hh,v 1.4 2004/01/02 21:59:52 fluxgen Exp $

/// @file implements common theme items

#ifndef THEMEITEMS_HH
#define THEMEITEMS_HH

#include "Theme.hh"
#include "Color.hh"
#include "Texture.hh"
#include "Font.hh"
#include "PixmapWithMask.hh"
#include "Image.hh"

#include <string>
#include <cstdio>
#include <iostream>
namespace FbTk {

using namespace std;

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
    if (str == 0) {
        setDefaultValue();
        return;
    }

    if (sscanf(str, "%d", &m_value) < 1)
        setDefaultValue();
}

template <>
void ThemeItem<FbTk::Font>::setDefaultValue() {
    if (!m_value.load("fixed")) {
        cerr<<"FbTk::ThemeItem<FbTk::Font>: Warning! Failed to load default value 'fixed'"<<endl;
    }
}

template <>
void ThemeItem<FbTk::Font>::setFromString(const char *str) {

    if (str == 0 || m_value.load(str) == false) {
        if (FbTk::ThemeManager::instance().verbose()) {
            cerr<<"FbTk::Theme: Error loading font "<<
                ((m_value.isAntialias() || m_value.utf8()) ? "(" : "")<<

                (m_value.isAntialias() ? "antialias" : "")<<
                (m_value.utf8() ? " utf8" : "")<<

                ((m_value.isAntialias() || m_value.utf8()) ? ") " : "")<<
                "for \""<<name()<<"\" or \""<<altName()<<"\": "<<str<<endl;

            cerr<<"FbTk::Theme: Setting default value"<<endl;
        }
        setDefaultValue();
    }
    
}

// do nothing
template <>
void ThemeItem<FbTk::Font>::load() {
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
    if (!m_value.color().setFromString(color_name.c_str(),
                                       m_tm.screenNum()))
        m_value.color().setFromString("darkgray", m_tm.screenNum());

    if (!m_value.colorTo().setFromString(colorto_name.c_str(),
                                         m_tm.screenNum()))
        m_value.colorTo().setFromString("white", m_tm.screenNum());
           
    StringUtil::removeFirstWhitespace(pixmap_name);
    StringUtil::removeTrailingWhitespace(pixmap_name);
    if (pixmap_name.empty()) {
        m_value.pixmap() = 0;
        return;
    }
    std::auto_ptr<PixmapWithMask> pm(Image::load(pixmap_name,
                                                 m_tm.screenNum()));
    if (pm.get() == 0) {
        if (FbTk::ThemeManager::instance().verbose()) {
            cerr<<"Resource("<<name()+".pixmap"
                <<"): Failed to load image: "<<pixmap_name<<endl;
        }
        m_value.pixmap() = 0;
    } else
        m_value.pixmap() = pm->pixmap().release();

}

template <>
void ThemeItem<FbTk::Texture>::setDefaultValue() {
    m_value.setType(FbTk::Texture::FLAT | FbTk::Texture::SOLID);
    load(); // one might forget to add line something:  so we try to load something.*:  too
}

template <>
void ThemeItem<FbTk::Texture>::setFromString(const char *str) {
    m_value.setFromString(str);
    if (m_value.type() == 0) // failed to set value
        setDefaultValue();
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
        std::string filename(str);

        StringUtil::removeFirstWhitespace(filename);
        StringUtil::removeTrailingWhitespace(filename);

        std::auto_ptr<FbTk::PixmapWithMask> pm(Image::load(filename, m_tm.screenNum()));
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
        if (FbTk::ThemeManager::instance().verbose())
            cerr<<"FbTk::Theme: Error loading color value for \""<<name()<<"\" or \""<<altName()<<"\"."<<endl;
        setDefaultValue();
    }
}

// does nothing
template <>
void ThemeItem<FbTk::Color>::load() { }

};

#endif // THEMEITEMS_HH
