// holds screen resource handling

#include "Screen.hh"
#include <string>
using namespace std;

template <>
void FbTk::Resource<BScreen::PlacementPolicy>::setDefaultValue() {
    *(*this) = BScreen::ROWSMARTPLACEMENT;
}

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
string FbTk::Resource<BScreen::PlacementPolicy>::getString() {
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
void FbTk::Resource<BScreen::RowDirection>::setDefaultValue() {
    *(*this) = BScreen::LEFTRIGHT;
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
string FbTk::Resource<BScreen::RowDirection>::getString() {
    switch (*(*this)) {
    case BScreen::LEFTRIGHT:
        return "LeftToRight";
    case BScreen::RIGHTLEFT:
        return "RightToLeft";
    }

    return "LeftToRight";
}


template <>
void FbTk::Resource<BScreen::ColumnDirection>::setDefaultValue() {
    *(*this) = BScreen::TOPBOTTOM;
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
string FbTk::Resource<BScreen::ColumnDirection>::getString() {
    switch (*(*this)) {
    case BScreen::TOPBOTTOM:
        return "TopToBottom";
    case BScreen::BOTTOMTOP:
        return "BottomToTop";
    }

    return "TopToBottom";
}

template <>
void FbTk::Resource<FbTk::MenuTheme::MenuMode>::setDefaultValue() {
    *(*this) = FbTk::MenuTheme::DELAY_OPEN;
}

template <>
string FbTk::Resource<FbTk::MenuTheme::MenuMode>::getString() {
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
std::string FbTk::Resource<BScreen::FocusModel>::
getString() {
    switch (m_value) {
    case BScreen::SLOPPYFOCUS:
        return string("SloppyFocus");
    case BScreen::SEMISLOPPYFOCUS:
        return string("SemiSloppyFocus");
    case BScreen::CLICKTOFOCUS:
        return string("ClickToFocus");
    }
    // default string
    return string("ClickToFocus");
}

template<>
void FbTk::Resource<BScreen::FocusModel>::
setFromString(char const *strval) {
    // auto raise options here for backwards read compatibility
    // they are not supported for saving purposes. Nor does the "AutoRaise" 
    // part actually do anything
    if (strcasecmp(strval, "SloppyFocus") == 0 
        || strcasecmp(strval, "AutoRaiseSloppyFocus") == 0) 
        m_value = BScreen::SLOPPYFOCUS;
    else if (strcasecmp(strval, "SemiSloppyFocus") == 0
        || strcasecmp(strval, "AutoRaiseSemiSloppyFocus") == 0) 
        m_value = BScreen::SEMISLOPPYFOCUS;
    else if (strcasecmp(strval, "ClickToFocus") == 0) 
        m_value = BScreen::CLICKTOFOCUS;
    else
        setDefaultValue();
}

template<>
void FbTk::Resource<FbTk::GContext::LineStyle>::setDefaultValue() {
    *(*this) = FbTk::GContext::LINESOLID;
}

template<>
std::string FbTk::Resource<FbTk::GContext::LineStyle>::getString() {
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
void FbTk::Resource<FbTk::GContext::JoinStyle>::setDefaultValue() {
    *(*this) = FbTk::GContext::JOINMITER;
}

template<>
std::string FbTk::Resource<FbTk::GContext::JoinStyle>::getString() {
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
void FbTk::Resource<FbTk::GContext::CapStyle>::setDefaultValue() {
    *(*this) = FbTk::GContext::CAPNOTLAST;
}

template<>
std::string FbTk::Resource<FbTk::GContext::CapStyle>::getString() {
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

