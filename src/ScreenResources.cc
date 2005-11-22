// ScreenResources.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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



// holds screen resource handling

#include "Screen.hh"
#include <string>
using namespace std;

template <>
void FbTk::Resource<BScreen::PlacementPolicy>::setFromString(const char *str) {
    if (strcasecmp("RowSmartPlacement", str) == 0)
        *(*this) = BScreen::ROWSMARTPLACEMENT;
    else if (strcasecmp("ColSmartPlacement", str) == 0)
        *(*this) = BScreen::COLSMARTPLACEMENT;
    else if (strcasecmp("UnderMousePlacement", str) == 0)
        *(*this) = BScreen::UNDERMOUSEPLACEMENT;
    else if (strcasecmp("CascadePlacement", str) == 0)
        *(*this) = BScreen::CASCADEPLACEMENT;
    else
        setDefaultValue();
}

template <>
string FbTk::Resource<BScreen::PlacementPolicy>::getString() const {
    switch (*(*this)) {
    case BScreen::ROWSMARTPLACEMENT:
        return "RowSmartPlacement";
    case BScreen::COLSMARTPLACEMENT:
        return "ColSmartPlacement";
    case BScreen::UNDERMOUSEPLACEMENT:
        return "UnderMousePlacement";
    case BScreen::CASCADEPLACEMENT:
        return "CascadePlacement";
    }

    return "RowSmartPlacement";
}

template <>
void FbTk::Resource<BScreen::RowDirection>::setFromString(const char *str) {
    if (strcasecmp("LeftToRight", str) == 0)
        *(*this) = BScreen::LEFTRIGHT;
    else if (strcasecmp("RightToLeft", str) == 0)
        *(*this) = BScreen::RIGHTLEFT;
    else
        setDefaultValue();
    
}

template <>
string FbTk::Resource<BScreen::RowDirection>::getString() const {
    switch (*(*this)) {
    case BScreen::LEFTRIGHT:
        return "LeftToRight";
    case BScreen::RIGHTLEFT:
        return "RightToLeft";
    }

    return "LeftToRight";
}


template <>
void FbTk::Resource<BScreen::ColumnDirection>::setFromString(const char *str) {
    if (strcasecmp("TopToBottom", str) == 0)
        *(*this) = BScreen::TOPBOTTOM;
    else if (strcasecmp("BottomToTop", str) == 0)
        *(*this) = BScreen::BOTTOMTOP;
    else
        setDefaultValue();
    
}

template <>
string FbTk::Resource<BScreen::ColumnDirection>::getString() const {
    switch (*(*this)) {
    case BScreen::TOPBOTTOM:
        return "TopToBottom";
    case BScreen::BOTTOMTOP:
        return "BottomToTop";
    }

    return "TopToBottom";
}

template <>
string FbTk::Resource<FbTk::MenuTheme::MenuMode>::getString() const {
    switch (*(*this)) {
    case FbTk::MenuTheme::DELAY_OPEN:
        return string("Delay");
    case FbTk::MenuTheme::CLICK_OPEN:
        return string("Click");
    }
    return string("Delay");
}

template <>
void FbTk::Resource<FbTk::MenuTheme::MenuMode>::setFromString(const char *str) {
    if (strcasecmp(str, "Delay") == 0)
        *(*this) = FbTk::MenuTheme::DELAY_OPEN;
    else if (strcasecmp(str, "Click") == 0)
        *(*this) = FbTk::MenuTheme::CLICK_OPEN;
    else
        setDefaultValue();
}

template <>
std::string FbTk::Resource<BScreen::ResizeModel>::getString() const {
    switch (m_value) {
    case BScreen::QUADRANTRESIZE:
        return std::string("Quadrant");
    case BScreen::BOTTOMRESIZE:
        return std::string("Bottom");
    }

    return std::string("Default");
}

template<>
void FbTk::Resource<BScreen::ResizeModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "Bottom") == 0) {
        m_value = BScreen::BOTTOMRESIZE;
    } else if (strcasecmp(strval, "Quadrant") == 0) {
        m_value = BScreen::QUADRANTRESIZE;
    } else 
        m_value = BScreen::DEFAULTRESIZE;
}

template<>
std::string FbTk::Resource<BScreen::FocusModel>::getString() const {
    switch (m_value) {
    case BScreen::MOUSEFOCUS:
        return string("MouseFocus");
    case BScreen::CLICKFOCUS:
        return string("ClickFocus");
    }
    // default string
    return string("ClickFocus");
}

template<>
void FbTk::Resource<BScreen::FocusModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "MouseFocus") == 0) 
        m_value = BScreen::MOUSEFOCUS;
    else if (strcasecmp(strval, "ClickToFocus") == 0) 
        m_value = BScreen::CLICKFOCUS;
    else
        setDefaultValue();
}

template<>

std::string FbTk::Resource<BScreen::TabFocusModel>::getString() const {
    switch (m_value) {
    case BScreen::MOUSETABFOCUS:
        return string("SloppyTabFocus");
    case BScreen::CLICKTABFOCUS:
        return string("ClickToTabFocus");
    }
    // default string
    return string("ClickToTabFocus");
}

template<>
void FbTk::Resource<BScreen::TabFocusModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "SloppyTabFocus") == 0 )
        m_value = BScreen::MOUSETABFOCUS;
    else if (strcasecmp(strval, "ClickToTabFocus") == 0) 
        m_value = BScreen::CLICKTABFOCUS;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<BScreen::FollowModel>::getString() const {
    switch (m_value) {
    case BScreen::FOLLOW_ACTIVE_WINDOW:
        return std::string("Follow");
        break;
    case BScreen::FETCH_ACTIVE_WINDOW:
        return std::string("Current");
        break;
    }

    return std::string("Ignore");
}

template<>
void FbTk::Resource<BScreen::FollowModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "Follow") == 0)
        m_value = BScreen::FOLLOW_ACTIVE_WINDOW;
    else if (strcasecmp(strval, "Current") == 0 || 
             strcasecmp(strval, "CurrentWorkspace") == 0 ||
             strcasecmp(strval, "Fetch") == 0)
        m_value = BScreen::FETCH_ACTIVE_WINDOW;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<FbTk::GContext::LineStyle>::getString() const {
    switch(m_value) {
    case FbTk::GContext::LINESOLID:
        return "LineSolid";
        break;
    case FbTk::GContext::LINEONOFFDASH:
        return "LineOnOffDash";
        break;
    case FbTk::GContext::LINEDOUBLEDASH:
        return "LineDoubleDash";
        break;
    };
    return "LineSolid";
}

template<>
void FbTk::Resource<FbTk::GContext::LineStyle>
::setFromString(char const *strval) { 

    if (strcasecmp(strval, "LineSolid") == 0 )
        m_value = FbTk::GContext::LINESOLID;
    else if (strcasecmp(strval, "LineOnOffDash") == 0 )
        m_value = FbTk::GContext::LINEONOFFDASH;
    else if (strcasecmp(strval, "LineDoubleDash") == 0) 
        m_value = FbTk::GContext::LINEDOUBLEDASH;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<FbTk::GContext::JoinStyle>::getString() const {
    switch(m_value) {
    case FbTk::GContext::JOINMITER:
        return "JoinMiter";
        break;
    case FbTk::GContext::JOINBEVEL:
        return "JoinBevel";
        break;
    case FbTk::GContext::JOINROUND:
        return "JoinRound";
        break;
    };
    return "JoinMiter";
}

template<>
void FbTk::Resource<FbTk::GContext::JoinStyle>
::setFromString(char const *strval) { 

    if (strcasecmp(strval, "JoinRound") == 0 )
        m_value = FbTk::GContext::JOINROUND;
    else if (strcasecmp(strval, "JoinMiter") == 0 )
        m_value = FbTk::GContext::JOINMITER;
    else if (strcasecmp(strval, "JoinBevel") == 0) 
        m_value = FbTk::GContext::JOINBEVEL;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<FbTk::GContext::CapStyle>::getString() const {
    switch(m_value) {
    case FbTk::GContext::CAPNOTLAST:
        return "CapNotLast";
        break;
    case FbTk::GContext::CAPBUTT:
        return "CapButt";
        break;
    case FbTk::GContext::CAPROUND:
        return "CapRound";
        break;
    case FbTk::GContext::CAPPROJECTING:
        return "CapProjecting";
        break;
    };
    return "CapNotLast";
}

template<>
void FbTk::Resource<FbTk::GContext::CapStyle>
::setFromString(char const *strval) { 

    if (strcasecmp(strval, "CapNotLast") == 0 )
        m_value = FbTk::GContext::CAPNOTLAST;
    else if (strcasecmp(strval, "CapProjecting") == 0 )
        m_value = FbTk::GContext::CAPPROJECTING;
    else if (strcasecmp(strval, "CapRound") == 0) 
        m_value = FbTk::GContext::CAPROUND;
    else if (strcasecmp(strval, "CapButt" ) == 0)
        m_value = FbTk::GContext::CAPBUTT;
    else
        setDefaultValue();
}

