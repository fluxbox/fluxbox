// ScreenResources.cc for Fluxbox Window Manager
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



// holds screen resource handling

#include "Screen.hh"
#include <string>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;

namespace FbTk {

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

template<>
string FbTk::Resource<BScreen::FollowModel>::getString() const {
    switch (m_value) {
    case BScreen::FOLLOW_ACTIVE_WINDOW:
    default:
        return string("Follow");
        break;
    case BScreen::FETCH_ACTIVE_WINDOW:
        return string("Current");
        break;
    case BScreen::SEMIFOLLOW_ACTIVE_WINDOW:
        return string("SemiFollow");
        break;
    case BScreen::IGNORE_OTHER_WORKSPACES:
        return string("Ignore");
        break;
    }

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
    else if (strcasecmp(strval, "SemiFollow") == 0)
        m_value = BScreen::SEMIFOLLOW_ACTIVE_WINDOW;
    else if (strcasecmp(strval, "Ignore") == 0)
        m_value = BScreen::IGNORE_OTHER_WORKSPACES;
    else
        setDefaultValue();
}

template<>
string FbTk::Resource<FbTk::GContext::LineStyle>::getString() const {
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
string FbTk::Resource<FbTk::GContext::JoinStyle>::getString() const {
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
string FbTk::Resource<FbTk::GContext::CapStyle>::getString() const {
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
} // end namespace FbTk
