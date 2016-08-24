// MinOverlapPlacement.cc
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
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR (*it)
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR (*it)WISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR (*it)
// DEALINGS IN THE SOFTWARE.

#include "MinOverlapPlacement.hh"

#include "FocusControl.hh"
#include "Window.hh"
#include "Screen.hh"

namespace {

inline void getWindowDimensions(const FluxboxWindow& win, int& left, int& top, int& right, int& bottom) {

    const int bw = 2 * win.frame().window().borderWidth();
    left = win.x() - win.xOffset();
    top = win.y() - win.yOffset();
    right = left + win.width() + bw + win.widthOffset();
    bottom = top + win.height() + bw + win.heightOffset();
}

class Area {
public:

    enum Corner {
        TOPLEFT,
        TOPRIGHT,
        BOTTOMLEFT,
        BOTTOMRIGHT
    } corner; // indicates the corner of the window that will be placed

    Area(Corner _corner, int _x, int _y):
        corner(_corner), x(_x), y(_y) { };

    // do all STL set implementations use this for sorting?
    bool operator <(const Area &o) const {
        switch (s_policy) {
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

    static ScreenPlacement::RowDirection s_row_dir;
    static ScreenPlacement::ColumnDirection s_col_dir;
    static ScreenPlacement::PlacementPolicy s_policy;
};

ScreenPlacement::RowDirection Area::s_row_dir = ScreenPlacement::LEFTRIGHT;
ScreenPlacement::ColumnDirection Area::s_col_dir = ScreenPlacement::TOPBOTTOM;
ScreenPlacement::PlacementPolicy Area::s_policy = ScreenPlacement::ROWMINOVERLAPPLACEMENT;

} // end of anonymous namespace


bool MinOverlapPlacement::placeWindow(const FluxboxWindow &win, int head,
                                      int &place_x, int &place_y) {

    int left;
    int top;
    int right;
    int bottom;

    std::list<FluxboxWindow *> windowlist;
    const std::list<Focusable *> focusables =
            win.screen().focusControl().focusedOrderWinList().clientList();
    std::list<Focusable *>::const_iterator foc_it = focusables.begin(),
                                           foc_it_end = focusables.end();
    unsigned int workspace = win.workspaceNumber();
    for (; foc_it != foc_it_end; ++foc_it) {
        // make sure it's a FluxboxWindow
        if (*foc_it == (*foc_it)->fbwindow() &&
            (workspace == (*foc_it)->fbwindow()->workspaceNumber() ||
             (*foc_it)->fbwindow()->isStuck()))
            windowlist.push_back((*foc_it)->fbwindow());
    }

    // view (screen + head) constraints
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    int win_w = win.normalWidth() + win.fbWindow().borderWidth()*2 +
                win.widthOffset();
    int win_h = win.normalHeight() + win.fbWindow().borderWidth()*2 +
                win.heightOffset();

    // we keep a set of open spaces on the desktop, sorted by size/location
    std::set<Area> areas;

    // setup stuff in order to make Area::operator< work
    const ScreenPlacement& p = win.screen().placementStrategy();
    Area::s_policy = p.placementPolicy();
    Area::s_row_dir = p.rowDirection();
    Area::s_col_dir = p.colDirection();


    // initialize the set of areas to contain the entire head
    areas.insert(Area(Area::TOPLEFT, head_left, head_top));
    areas.insert(Area(Area::TOPRIGHT, head_right - win_w, head_top));
    areas.insert(Area(Area::BOTTOMLEFT, head_left, head_bot - win_h));
    areas.insert(Area(Area::BOTTOMRIGHT, head_right - win_w, head_bot - win_h));

    // go through the list of windows, creating other reasonable placements
    // at the end, we'll find the one with minimum overlap
    // the size of this set is at most 2(n+2)(n+1) (n = number of windows)
    // finding overlaps is therefore O(n^3), but it can probably be improved
    const std::list<FluxboxWindow* >& const_windowlist = windowlist;
    std::list<FluxboxWindow *>::const_reverse_iterator it = const_windowlist.rbegin(),
                                                   it_end = const_windowlist.rend();
    for (; it != it_end; ++it) {
        if (*it == &win) continue;
        if ((*it)->layerNum() != win.layerNum() ){ continue; } //windows are in different layers - skip it
        
        getWindowDimensions(*(*it), left, top, right, bottom);

        // go through the list of regions
        // if this window overlaps that region and the new window still fits,
        // it will create new regions to test
        std::set<Area>::iterator ar_it = areas.begin();
        for (; ar_it != areas.end(); ++ar_it) {

            switch (ar_it->corner) {
                case Area::TOPLEFT:
                    if (right > ar_it->x && bottom > ar_it->y) {
                        if (bottom + win_h <= head_bot)
                            areas.insert(Area(Area::TOPLEFT, ar_it->x, bottom));
                        if (right + win_w <= head_right)
                            areas.insert(Area(Area::TOPLEFT, right, ar_it->y));
                    }
                    break;
                case Area::TOPRIGHT:
                    if (left < ar_it->x + win_w && bottom > ar_it->y) {
                        if (bottom + win_h <= head_bot)
                            areas.insert(Area(Area::TOPRIGHT, ar_it->x, bottom));
                        if (left - win_w >= head_left)
                            areas.insert(Area(Area::TOPRIGHT, left - win_w, ar_it->y));
                    }
                    break;
                case Area::BOTTOMRIGHT:
                    if (left < ar_it->x + win_w && top < ar_it->y + win_h) {
                        if (top - win_h >= head_top)
                            areas.insert(Area(Area::BOTTOMRIGHT, ar_it->x, top - win_h));
                        if (left - win_w >= head_left)
                            areas.insert(Area(Area::BOTTOMRIGHT, left - win_w, ar_it->y));
                    }
                    break;
                case Area::BOTTOMLEFT:
                    if (right > ar_it->x && top < ar_it->y + win_h) {
                        if (top - win_h >= head_top)
                            areas.insert(Area(Area::BOTTOMLEFT, ar_it->x, top - win_h));
                        if (right + win_w <= head_right)
                            areas.insert(Area(Area::BOTTOMLEFT, right, ar_it->y));
                    }
                    break;
            }

        }
    }

    // choose the region with minimum overlap
    int min_so_far = win_w * win_h * windowlist.size() + 1;
    std::set<Area>::iterator min_reg = areas.end();

    std::set<Area>::iterator ar_it = areas.begin();
    for (; ar_it != areas.end(); ++ar_it) {

        int overlap = 0;
        it = const_windowlist.rbegin();
        for (; it != it_end; ++it) {

            getWindowDimensions(*(*it), left, top, right, bottom);

            // get the coordinates of the overlap region
            int min_right = std::min(right, ar_it->x + win_w);
            int min_bottom = std::min(bottom, ar_it->y + win_h);
            int max_left = std::max(left, ar_it->x);
            int max_top = std::max(top, ar_it->y);

            // now compute the overlap and add to running total
            if (min_right > max_left && min_bottom > max_top)
                overlap += (min_right - max_left) * (min_bottom - max_top);

        }

        // if this placement is better, use it
        if (overlap < min_so_far) {
            min_reg = ar_it;
            min_so_far = overlap;
            if (overlap == 0) // can't do better than this
                break;
        }

    }

    // place window
    place_x = min_reg->x + win.xOffset();
    place_y = min_reg->y + win.yOffset();

    return true;
}
