// ScreenPlacement.hh
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

#ifndef SCREENPLACEMENT_HH
#define SCREENPLACEMENT_HH

#include "PlacementStrategy.hh"
#include "FbTk/Resource.hh"

#include <memory>

namespace FbTk {
    class Menu;
}
class BScreen;

/**
 * Main class for strategy handling
 * This is a bridge between screen and 
 * the real placement strategy (rowcol, undermouse etc)
 * The placeWindow function in this class is guaranteed to succeed.
 * It holds a pointer to the real placement strategy which is
 * called upon placeWindow, it also holds the placement resources
 */
class ScreenPlacement: public PlacementStrategy {
public:
    enum PlacementPolicy { 
        ROWSMARTPLACEMENT, 
        COLSMARTPLACEMENT,
        COLMINOVERLAPPLACEMENT,
        ROWMINOVERLAPPLACEMENT,
        CASCADEPLACEMENT,
        UNDERMOUSEPLACEMENT,
        AUTOTABPLACEMENT
    };

    enum RowDirection { 
        LEFTRIGHT, ///< from left to right
        RIGHTLEFT  ///< from right to left
    };
    enum ColumnDirection { 
        TOPBOTTOM,  ///< from top to bottom
        BOTTOMTOP   ///< from bottom to top
    };

    explicit ScreenPlacement(BScreen &screen);

    virtual ~ScreenPlacement() {}
    /// placeWindow is guaranteed to succeed, ignore return value
    /// @return true
    bool placeWindow(const FluxboxWindow &window, int head,
                     int &place_x, int &place_y);

    // places and show 'menu' at 'x','y'
    void placeAndShowMenu(FbTk::Menu& menu, int x, int y, bool respect_struts);

    PlacementPolicy placementPolicy() const { return *m_placement_policy; }
    RowDirection rowDirection() const { return *m_row_direction; }
    ColumnDirection colDirection() const { return *m_col_direction; }

private:
    FbTk::Resource<RowDirection> m_row_direction; ///< row direction resource
    FbTk::Resource<ColumnDirection> m_col_direction; ///< column direction resource
    FbTk::Resource<PlacementPolicy> m_placement_policy; ///< placement policy resource
    PlacementPolicy m_old_policy; ///< holds old policy, used to determine if resources has changed
    std::unique_ptr<PlacementStrategy> m_strategy; ///< main strategy
    std::unique_ptr<PlacementStrategy> m_fallback_strategy; ///< a fallback strategy if the main strategy fails
    BScreen& m_screen;
};

#endif // SCREENPLACEMENT_HH
