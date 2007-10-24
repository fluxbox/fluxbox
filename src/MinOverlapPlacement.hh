// MinOverlapPlacement.hh
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef MINOVERLAPPLACEMENT_HH
#define MINOVERLAPPLACEMENT_HH

#include "ScreenPlacement.hh"

class MinOverlapPlacement: public PlacementStrategy {
public:
    MinOverlapPlacement(ScreenPlacement::PlacementPolicy policy);

    bool placeWindow(const FluxboxWindow &win, int head,
                     int &place_x, int &place_y);

private:
    class Region {
    public:

        enum Corner {
            TOPLEFT,
            TOPRIGHT,
            BOTTOMLEFT,
            BOTTOMRIGHT
        } corner; // indicates the corner of the window that will be placed

        Region(Corner _corner, int _x, int _y):
            corner(_corner), x(_x), y(_y) { };

        // do all STL set implementations use this for sorting?
        bool operator <(const Region &o) const {
            switch (MinOverlapPlacement::s_policy) {
                case ScreenPlacement::ROWMINOVERLAPPLACEMENT:
                    // if we're making rows, y-value is most important
                    if (y != o.y)
                        return ((y < o.y) ^ (s_col_dir == ScreenPlacement::BOTTOMTOP));
                    if (x != o.x)
                        return ((x < o.x) ^ (s_row_dir == ScreenPlacement::RIGHTLEFT));
                    return (corner < o.corner);
                case ScreenPlacement::COLMINOVERLAPPLACEMENT:
                    // if we're making columns, x-value is most important
                    if (x != o.x)
                        return ((x < o.x) ^ (s_row_dir == ScreenPlacement::RIGHTLEFT));
                    if (y != o.y)
                        return ((y < o.y) ^ (s_col_dir == ScreenPlacement::BOTTOMTOP));
                    return (corner < o.corner);
                default:
                    return false;
            }
        }

        // position where the top left corner of the window will be placed
        int x, y;
    };

    static ScreenPlacement::PlacementPolicy s_policy;
    static ScreenPlacement::RowDirection s_row_dir;
    static ScreenPlacement::ColumnDirection s_col_dir;
};

#endif // MINOVERLAPPLACEMENT_HH
