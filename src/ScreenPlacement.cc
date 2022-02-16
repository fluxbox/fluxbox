// ScreenPlacement.cc
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "ScreenPlacement.hh"

#include "RowSmartPlacement.hh"
#include "MinOverlapPlacement.hh"
#include "UnderMousePlacement.hh"
#include "ColSmartPlacement.hh"
#include "CascadePlacement.hh"

#include "Screen.hh"
#include "Window.hh"

#include "FbTk/Menu.hh"

#include <iostream>
#include <exception>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
using std::cerr;
using std::endl;

ScreenPlacement::ScreenPlacement(BScreen &screen):
    m_row_direction(screen.resourceManager(), LEFTRIGHT, 
                    screen.name()+".rowPlacementDirection", 
                    screen.altName()+".RowPlacementDirection"),
    m_col_direction(screen.resourceManager(), TOPBOTTOM, 
                    screen.name()+".colPlacementDirection", 
                    screen.altName()+".ColPlacementDirection"),
    m_placement_policy(screen.resourceManager(), ROWMINOVERLAPPLACEMENT, 
                       screen.name()+".windowPlacement", 
                       screen.altName()+".WindowPlacement"),
    m_old_policy(ROWSMARTPLACEMENT),
    m_screen(screen)
{
}

bool ScreenPlacement::placeWindow(const FluxboxWindow &win, int head,
                                  int &place_x, int &place_y) {


    // check the resource placement and see if has changed
    // and if so update the strategy
    if (m_old_policy != *m_placement_policy || !m_strategy.get()) {
        m_old_policy = *m_placement_policy;
        switch (*m_placement_policy) {
        case ROWSMARTPLACEMENT:
            m_strategy.reset(new RowSmartPlacement());
            break;
        case COLSMARTPLACEMENT:
            m_strategy.reset(new ColSmartPlacement());
            break;
        case ROWMINOVERLAPPLACEMENT:
        case COLMINOVERLAPPLACEMENT:
            m_strategy.reset(new MinOverlapPlacement());
            break;
        case CASCADEPLACEMENT:
            m_strategy.reset(new CascadePlacement(win.screen()));
            break;
        case UNDERMOUSEPLACEMENT:
            m_strategy.reset(new UnderMousePlacement());
            break;
        case AUTOTABPLACEMENT:
            m_strategy.reset(0);
            break;
        }
    }

    // view (screen + head) constraints
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    // start placement, top left corner
    place_x = head_left;
    place_y = head_top;

    bool placed = false;
    if (m_strategy) {
        try {
            placed = m_strategy->placeWindow(win, head, place_x, place_y);
        } catch (std::bad_cast & cast) {
            // This should not happen.
            // If for some reason we change the PlacementStrategy in Screen
            // from ScreenPlacement to something else then we might get
            // bad_cast from some placement strategies.
            cerr<<"Failed to place window: "<<cast.what()<<endl;
        }
    }

    if (!placed) {
        // Create fallback strategy, when we need it the first time
        // This strategy must succeed!
        if (m_fallback_strategy.get() == 0)
            m_fallback_strategy.reset(new CascadePlacement(win.screen()));

        m_fallback_strategy->placeWindow(win, head, place_x, place_y);
    }



    int win_w = win.normalWidth() + win.fbWindow().borderWidth()*2 + win.widthOffset(),
        win_h = win.normalHeight() + win.fbWindow().borderWidth()*2 + win.heightOffset();

    // make sure the window is inside our screen(head) area
    if (place_x + win_w - win.xOffset() > head_right)
        place_x = head_left + (head_right - head_left - win_w) / 2 +
                  win.xOffset();
    if (place_y + win_h - win.yOffset() > head_bot)
        place_y = head_top + (head_bot - head_top - win_h) / 2 + win.yOffset();

    return true;
}

void ScreenPlacement::placeAndShowMenu(FbTk::Menu& menu, int x, int y, bool respect_struts) {

    int head = m_screen.getHead(x, y);

    menu.setScreen(m_screen.getHeadX(head),
        m_screen.getHeadY(head),
        m_screen.getHeadWidth(head),
        m_screen.getHeadHeight(head));

    menu.updateMenu(); // recalculate the size

    x = x - (menu.width() / 2);
    if (menu.isTitleVisible())
        y = y - (menu.titleWindow().height() / 2);

    // adjust (x, y) to fit on the screen
    if (!respect_struts) {

        int bw = 2 * menu.fbwindow().borderWidth();
        std::pair<int, int> pos = m_screen.clampToHead(head, x, y, menu.width() + bw, menu.height() + bw);
        x = pos.first;
        y = pos.second;

    } else { // do not cover toolbar if no title

        int top = static_cast<signed>(m_screen.maxTop(head));
        int bottom = static_cast<signed>(m_screen.maxBottom(head));
        int left = static_cast<signed>(m_screen.maxLeft(head));
        int right = static_cast<signed>(m_screen.maxRight(head));

        if (y < top)
            y = top;
        else if (y + static_cast<signed>(menu.height()) >= bottom)
            y = bottom - menu.height() - 1 - menu.fbwindow().borderWidth();

        if (x < left)
            x = left;
        else if (x + static_cast<signed>(menu.width()) >= right)
            x = right - static_cast<int>(menu.width()) - 1;
    }

    menu.move(x, y);
    menu.show();
    menu.grabInputFocus();
}

////////////////////// Placement Resources
namespace FbTk {

template <>
std::string FbTk::Resource<ScreenPlacement::PlacementPolicy>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::ROWSMARTPLACEMENT:
        return "RowSmartPlacement";
    case ScreenPlacement::COLSMARTPLACEMENT:
        return "ColSmartPlacement";
    case ScreenPlacement::ROWMINOVERLAPPLACEMENT:
        return "RowMinOverlapPlacement";
    case ScreenPlacement::COLMINOVERLAPPLACEMENT:
        return "ColMinOverlapPlacement";
    case ScreenPlacement::UNDERMOUSEPLACEMENT:
        return "UnderMousePlacement";
    case ScreenPlacement::CASCADEPLACEMENT:
        return "CascadePlacement";
    case ScreenPlacement::AUTOTABPLACEMENT:
        return "AutotabPlacement";
    }

    return "RowSmartPlacement";
}

template <>
void FbTk::Resource<ScreenPlacement::PlacementPolicy>::setFromString(const char *str) {
    if (strcasecmp("RowSmartPlacement", str) == 0)
        *(*this) = ScreenPlacement::ROWSMARTPLACEMENT;
    else if (strcasecmp("ColSmartPlacement", str) == 0)
        *(*this) = ScreenPlacement::COLSMARTPLACEMENT;
    else if (strcasecmp("RowMinOverlapPlacement", str) == 0)
        *(*this) = ScreenPlacement::ROWMINOVERLAPPLACEMENT;
    else if (strcasecmp("ColMinOverlapPlacement", str) == 0)
        *(*this) = ScreenPlacement::COLMINOVERLAPPLACEMENT;
    else if (strcasecmp("UnderMousePlacement", str) == 0)
        *(*this) = ScreenPlacement::UNDERMOUSEPLACEMENT;
    else if (strcasecmp("CascadePlacement", str) == 0)
        *(*this) = ScreenPlacement::CASCADEPLACEMENT;
    else if (strcasecmp("AutotabPlacement", str) == 0)
        *(*this) = ScreenPlacement::AUTOTABPLACEMENT;
    else
        setDefaultValue();
}


template <>
std::string FbTk::Resource<ScreenPlacement::RowDirection>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::LEFTRIGHT:
        return "LeftToRight";
    case ScreenPlacement::RIGHTLEFT:
        return "RightToLeft";
    }

    return "LeftToRight";
}


template <>
void FbTk::Resource<ScreenPlacement::RowDirection>::setFromString(const char *str) {
    if (strcasecmp("LeftToRight", str) == 0)
        *(*this) = ScreenPlacement::LEFTRIGHT;
    else if (strcasecmp("RightToLeft", str) == 0)
        *(*this) = ScreenPlacement::RIGHTLEFT;
    else
        setDefaultValue();
}

template <>
std::string FbTk::Resource<ScreenPlacement::ColumnDirection>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::TOPBOTTOM:
        return "TopToBottom";
    case ScreenPlacement::BOTTOMTOP:
        return "BottomToTop";
    }

    return "TopToBottom";
}


template <>
void FbTk::Resource<ScreenPlacement::ColumnDirection>::setFromString(const char *str) {
    if (strcasecmp("TopToBottom", str) == 0)
        *(*this) = ScreenPlacement::TOPBOTTOM;
    else if (strcasecmp("BottomToTop", str) == 0)
        *(*this) = ScreenPlacement::BOTTOMTOP;
    else
        setDefaultValue();
}

} // end namespace FbTk
