// ThemeItems.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

/// @file ThemeItems.cc implements common theme items

#ifndef THEMEITEMS_HH
#define THEMEITEMS_HH

#include "Theme.hh"
#include "Color.hh"
#include "Texture.hh"
#include "Font.hh"
#include "GContext.hh"
#include "PixmapWithMask.hh"
#include "Image.hh"
#include "Shape.hh"
#include "StringUtil.hh"

#include <string>
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

#include <iostream>
#include <memory>

namespace FbTk {

using std::string;
using std::cerr;
using std::endl;


// create default handlers for Color, Font, Texture, int and string
template <>
void ThemeItem<string>::load(const string *name, const string *altname) { }

template <>
void ThemeItem<string>::setDefaultValue() {
    *(*this) = "";
}

template <>
void ThemeItem<string>::setFromString(const char *str) {
    *(*this) = (str ? str : "");
}

template <>
void ThemeItem<int>::load(const std::string *name, const std::string *altname) { }

template<>
void ThemeItem<int>::setDefaultValue() {
    *(*this) = 0;
}

template <>
void ThemeItem<int>::setFromString(const char *str) {
    if (str == 0) {
        setDefaultValue();
        return;
    }

    if (sscanf(str, "%d", &m_value) < 1)
        setDefaultValue();
}
template<>
void ThemeItem<bool>::load(const std::string *name, const std::string *altname) { }

template<>
void ThemeItem<bool>::setDefaultValue() {
    *(*this) = false;
}

template<>
void ThemeItem<bool>::setFromString(char const *strval) {
    if (strcasecmp(strval, "true")==0)
        *(*this) = true;
    else
        *(*this) = false;
}

template <>
void ThemeItem<unsigned int>::setDefaultValue() {
    m_value = 0;
}

template <>
void ThemeItem<unsigned int>::setFromString(const char *str) {
    sscanf(str, "%u", &m_value);
}

template <>
void ThemeItem<unsigned int>::load(const std::string *name, const std::string *altname) {
}

template <>
void ThemeItem<Font>::setDefaultValue() {
    if (!m_value.load("__DEFAULT__")) {
        cerr<<"ThemeItem<Font>: Warning! Failed to load default value 'fixed'"<<endl;
    } else {
        m_value.setHalo(false);
        m_value.setShadow(false);
    }
}

template <>
void ThemeItem<Font>::setFromString(const char *str) {

    if (str == 0 || m_value.load(str) == false) {
        if (ThemeManager::instance().verbose()) {
            cerr<<"Theme: Error loading font "<<
                ((m_value.utf8()) ? "(utf8)" : "")<<
                "for \""<<name()<<"\" or \""<<altName()<<"\": "<<str<<endl;

            cerr<<"Theme: Setting default value"<<endl;
        }
        setDefaultValue();
    }
}

template <>
void ThemeItem<Font>::load(const string *o_name, const string *o_altname) {
    const string &m_name = o_name ? *o_name : name();
    const string &m_altname = o_altname ? *o_altname : altName();

    string effect(ThemeManager::instance().resourceValue(m_name+".effect", m_altname+".Effect"));
    if (effect == "halo") {
        Color halo_color(ThemeManager::instance().resourceValue(m_name+".halo.color", m_altname+".Halo.Color").c_str(), 
                theme().screenNum());

        m_value.setHalo(true);
        m_value.setHaloColor(halo_color);
    } else if (effect == "shadow" ) {
        Color shadow_color(ThemeManager::instance().resourceValue(m_name+".shadow.color", m_altname+".Shadow.Color").c_str(), 
                theme().screenNum());
        
        m_value.setShadow(true);
        m_value.setShadowColor(shadow_color);
        m_value.setShadowOffX(atoi(ThemeManager::instance().resourceValue(m_name+".shadow.x", m_altname+".Shadow.X").c_str()));
        m_value.setShadowOffY(atoi(ThemeManager::instance().resourceValue(m_name+".shadow.y", m_altname+".Shadow.Y").c_str()));
    }
}


template <>
void ThemeItem<Texture>::load(const string *o_name, const string *o_altname) {
    const string &m_name = (o_name==0)?name():*o_name;
    const string &m_altname = (o_altname==0)?altName():*o_altname;

    string color_name(ThemeManager::instance().
                      resourceValue(m_name+".color", m_altname+".Color"));
    string colorto_name(ThemeManager::instance().
                        resourceValue(m_name+".colorTo", m_altname+".ColorTo"));
    string pixmap_name(ThemeManager::instance().
                       resourceValue(m_name+".pixmap", m_altname+".Pixmap"));


    // set default value if we failed to load color
    if (!m_value.color().setFromString(color_name.c_str(),
                                       m_tm.screenNum()))
        m_value.color().setFromString("darkgray", m_tm.screenNum());

    if (!m_value.colorTo().setFromString(colorto_name.c_str(),
                                         m_tm.screenNum()))
        m_value.colorTo().setFromString("white", m_tm.screenNum());


    if ((m_value.type() & Texture::SOLID) != 0 && (m_value.type() & Texture::FLAT) == 0)
        m_value.calcHiLoColors(m_tm.screenNum());

    StringUtil::removeFirstWhitespace(pixmap_name);
    StringUtil::removeTrailingWhitespace(pixmap_name);
    if (pixmap_name.empty()) {
        m_value.pixmap() = 0;
        return;
    }

    std::unique_ptr<PixmapWithMask> pm(Image::load(pixmap_name, m_tm.screenNum()));

    if (pm.get() == 0) {
        if (ThemeManager::instance().verbose()) {
            cerr<<"Resource("<<m_name+".pixmap"
                <<"): Failed to load image: "<<pixmap_name<<endl;
        }
        m_value.pixmap() = 0;
    } else
        m_value.pixmap() = pm->pixmap().release();

}

template <>
void ThemeItem<Texture>::setDefaultValue() {
    m_value.setType(Texture::DEFAULT_LEVEL | Texture::DEFAULT_TEXTURE);
    load(); // one might forget to add line something:  so we try to load something.*:  too
}

template <>
void ThemeItem<Texture>::setFromString(const char *str) {
    m_value.setFromString(str);
    if (m_value.type() == 0) // failed to set value
        setDefaultValue();
}



// not used
template <>
void ThemeItem<PixmapWithMask>::load(const string *name, const string *altname) { }

template <>
void ThemeItem<PixmapWithMask>::setDefaultValue() {
    // create empty pixmap
    (*this)->pixmap() = 0;
    (*this)->mask() = 0;
}

template <>
void ThemeItem<PixmapWithMask>::
setFromString(const char *str) {
    if (str == 0)
        setDefaultValue();
    else {
        string filename(str);

        StringUtil::removeFirstWhitespace(filename);
        StringUtil::removeTrailingWhitespace(filename);

        std::unique_ptr<PixmapWithMask> pm(Image::load(filename, m_tm.screenNum()));
        if (pm.get() == 0)
            setDefaultValue();
        else {
            (*this)->pixmap() = pm->pixmap().release();
            (*this)->mask() = pm->mask().release();
        }
    }
}


template <>
void ThemeItem<Color>::setDefaultValue() {
    m_value.setFromString("white", m_tm.screenNum());
}

template <>
void ThemeItem<Color>::setFromString(const char *str) {
    if (!m_value.setFromString(str, m_tm.screenNum())) {
        if (ThemeManager::instance().verbose())
            cerr<<"Theme: Error loading color value for \""<<name()<<"\" or \""<<altName()<<"\"."<<endl;
        setDefaultValue();
    }
}

// does nothing
template <>
void ThemeItem<Color>::load(const string *name, const string *altname) { }

template<>
void ThemeItem<GContext::LineStyle>::setDefaultValue() {
    *(*this) = GContext::LINESOLID;
}

template<>
void ThemeItem<GContext::LineStyle>::setFromString(char const *strval) { 

    if (strcasecmp(strval, "LineSolid") == 0 )
        m_value = GContext::LINESOLID;
    else if (strcasecmp(strval, "LineOnOffDash") == 0 )
        m_value = GContext::LINEONOFFDASH;
    else if (strcasecmp(strval, "LineDoubleDash") == 0)
        m_value = GContext::LINEDOUBLEDASH;
    else
        setDefaultValue();
}

template<>
void ThemeItem<GContext::LineStyle>::load(const string *name, const string *altname) { }


template<>
void ThemeItem<GContext::JoinStyle>::setDefaultValue() {
    *(*this) = GContext::JOINMITER;
}

template<>
void ThemeItem<GContext::JoinStyle>::setFromString(char const *strval) { 

    if (strcasecmp(strval, "JoinRound") == 0 )
        m_value = GContext::JOINROUND;
    else if (strcasecmp(strval, "JoinMiter") == 0 )
        m_value = GContext::JOINMITER;
    else if (strcasecmp(strval, "JoinBevel") == 0) 
        m_value = GContext::JOINBEVEL;
    else
        setDefaultValue();
}

template<>
void ThemeItem<GContext::JoinStyle>::load(const string *name, const string *altname) { }

template<>
void ThemeItem<GContext::CapStyle>::setDefaultValue() {
    *(*this) = GContext::CAPNOTLAST;
}

template<>
void ThemeItem<GContext::CapStyle>::setFromString(char const *strval) { 

    if (strcasecmp(strval, "CapNotLast") == 0 )
        m_value = GContext::CAPNOTLAST;
    else if (strcasecmp(strval, "CapProjecting") == 0 )
        m_value = GContext::CAPPROJECTING;
    else if (strcasecmp(strval, "CapRound") == 0) 
        m_value = GContext::CAPROUND;
    else if (strcasecmp(strval, "CapButt" ) == 0)
        m_value = GContext::CAPBUTT;
    else
        setDefaultValue();
}

template<>
void ThemeItem<GContext::CapStyle>::load(const string *name, const string *altname) { }

template <>
void ThemeItem<Shape::ShapePlace>::load(const string *name, const string *altname) { }

template <>
void ThemeItem<Shape::ShapePlace>::setDefaultValue() {
    *(*this) = Shape::NONE;
}

template <>
void ThemeItem<Shape::ShapePlace>::setFromString(const char *str) {
    int places = 0;

    if (StringUtil::strcasestr(str, "topleft") != 0)
        places |= Shape::TOPLEFT;
    if (StringUtil::strcasestr(str, "topright") != 0)
        places |= Shape::TOPRIGHT;
    if (StringUtil::strcasestr(str, "bottomleft") != 0)
        places |= Shape::BOTTOMLEFT;
    if (StringUtil::strcasestr(str, "bottomright") != 0)
        places |= Shape::BOTTOMRIGHT;

    *(*this) = static_cast<Shape::ShapePlace>(places);
}

} // end namespace FbTk

#endif // THEMEITEMS_HH
