// Resources.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$


// holds main resource functions

#include "FbTk/StringUtil.hh"
#include "FbTk/Resource.hh"
#include "fluxbox.hh"

#include "Layer.hh"

#include <stdio.h>
#include <string>

using namespace std;
using namespace FbTk;

//-----------------------------------------------------------------
//---- accessors for int, bool, and some enums with Resource ------
//-----------------------------------------------------------------

template<>
void FbTk::Resource<int>::
setFromString(const char* strval) {
    int val;
    if (sscanf(strval, "%d", &val)==1)
        *this = val;
}

template<>
void FbTk::Resource<std::string>::
setFromString(const char *strval) {
    *this = strval;
}

template<>
void FbTk::Resource<bool>::
setFromString(char const *strval) {
    *this = (bool)!strcasecmp(strval, "true");
}

template<>
void FbTk::Resource<Fluxbox::TitlebarList>::
setFromString(char const *strval) {
    vector<std::string> val;
    StringUtil::stringtok(val, strval);
    int size=val.size();
    //clear old values
    m_value.clear();
		
    for (int i=0; i<size; i++) {
        if (strcasecmp(val[i].c_str(), "Maximize")==0)
            m_value.push_back(Fluxbox::MAXIMIZE);
        else if (strcasecmp(val[i].c_str(), "Minimize")==0)
            m_value.push_back(Fluxbox::MINIMIZE);
        else if (strcasecmp(val[i].c_str(), "Shade")==0)
            m_value.push_back(Fluxbox::SHADE);
        else if (strcasecmp(val[i].c_str(), "Stick")==0)
            m_value.push_back(Fluxbox::STICK);
        else if (strcasecmp(val[i].c_str(), "MenuIcon")==0)
            m_value.push_back(Fluxbox::MENUICON);
        else if (strcasecmp(val[i].c_str(), "Close")==0)
            m_value.push_back(Fluxbox::CLOSE);
    }
}

template<>
void FbTk::Resource<Fluxbox::TabsAttachArea>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "Titlebar")==0)
        m_value= Fluxbox::ATTACH_AREA_TITLEBAR;
    else
        m_value= Fluxbox::ATTACH_AREA_WINDOW;
}

template<>
void FbTk::Resource<unsigned int>::
setFromString(const char *strval) {	
    if (sscanf(strval, "%ul", &m_value) != 1)
        setDefaultValue();
}

template<>
void FbTk::Resource<long long>::
setFromString(const char *strval) {	
    if (sscanf(strval, "%llu", &m_value) != 1)
        setDefaultValue();
}


//-----------------------------------------------------------------
//---- manipulators for int, bool, and some enums with Resource ---
//-----------------------------------------------------------------
template<>
std::string FbTk::Resource<bool>::
getString() const {
    return std::string(**this == true ? "true" : "false");
}

template<>
std::string FbTk::Resource<int>::
getString() const {
    char strval[256];
    sprintf(strval, "%d", **this);
    return std::string(strval);
}

template<>
std::string FbTk::Resource<std::string>::
getString() const { return **this; }


template<>
std::string FbTk::Resource<Fluxbox::TitlebarList>::
getString() const {
    string retval;
    int size=m_value.size();
    for (int i=0; i<size; i++) {
        switch (m_value[i]) {
        case Fluxbox::SHADE:
            retval.append("Shade");
            break;
        case Fluxbox::MINIMIZE:
            retval.append("Minimize");
            break;
        case Fluxbox::MAXIMIZE:
            retval.append("Maximize");
            break;
        case Fluxbox::CLOSE:
            retval.append("Close");
            break;
        case Fluxbox::STICK:
            retval.append("Stick");
            break;
        case Fluxbox::MENUICON:
            retval.append("MenuIcon");
            break;
        default:
            break;
        }
        retval.append(" ");
    }

    return retval;
}

template<>
std::string FbTk::Resource<Fluxbox::TabsAttachArea>::
getString() const {
    if (m_value == Fluxbox::ATTACH_AREA_TITLEBAR)
        return "Titlebar";
    else
        return "Window";
}

template<>
string FbTk::Resource<unsigned int>::
getString() const {
    char tmpstr[128];
    sprintf(tmpstr, "%ul", m_value);
    return string(tmpstr);
}

template<>
string FbTk::Resource<long long>::
getString() const {
    char tmpstr[128];
    sprintf(tmpstr, "%llu", (unsigned long long) m_value);
    return string(tmpstr);
}

template<>
void FbTk::Resource<Layer>::
setFromString(const char *strval) {
    int tempnum = 0;
    if (sscanf(strval, "%d", &tempnum) == 1)
        m_value = tempnum;
    else if (strcasecmp(strval, "Menu") == 0)
        m_value = ::Layer::MENU;
    else if (strcasecmp(strval, "AboveDock") == 0)
        m_value = ::Layer::ABOVE_DOCK;
    else if (strcasecmp(strval, "Dock") == 0)
        m_value = ::Layer::DOCK;
    else if (strcasecmp(strval, "Top") == 0)
        m_value = ::Layer::TOP;
    else if (strcasecmp(strval, "Normal") == 0)
        m_value = ::Layer::NORMAL;
    else if (strcasecmp(strval, "Bottom") == 0)
        m_value = ::Layer::BOTTOM;
    else if (strcasecmp(strval, "Desktop") == 0)
        m_value = ::Layer::DESKTOP;
    else 
        setDefaultValue();
}


template<>
std::string FbTk::Resource<Layer>::
getString() const {
    switch (m_value.getNum()) {
    case Layer::MENU:
        return std::string("Menu");
    case Layer::ABOVE_DOCK:
        return std::string("AboveDock");
    case Layer::DOCK:
        return std::string("Dock");
    case Layer::TOP:
        return std::string("Top");
    case Layer::NORMAL:
        return std::string("Normal");
    case Layer::BOTTOM:
        return std::string("Bottom");
    case Layer::DESKTOP:
        return std::string("Desktop");
    default:
        char tmpstr[128];
        sprintf(tmpstr, "%d", m_value.getNum());
        return std::string(tmpstr);
    }
}

template<>
void FbTk::Resource<long>::
setFromString(const char *strval) {   
    if (sscanf(strval, "%ld", &m_value) != 1)
        setDefaultValue();
}
 
template<>
string FbTk::Resource<long>::
getString() const {
    char tmpstr[128];
    sprintf(tmpstr, "%ld", m_value);
    return string(tmpstr);
}
