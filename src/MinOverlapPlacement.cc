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

// $Id$

#include "MinOverlapPlacement.hh"

#include "Window.hh"
#include "Screen.hh"

ScreenPlacement::PlacementPolicy MinOverlapPlacement::s_policy = ScreenPlacement::ROWMINOVERLAPPLACEMENT;
ScreenPlacement::RowDirection MinOverlapPlacement::s_row_dir = ScreenPlacement::LEFTRIGHT;
ScreenPlacement::ColumnDirection MinOverlapPlacement::s_col_dir = ScreenPlacement::TOPBOTTOM;

MinOverlapPlacement::MinOverlapPlacement(ScreenPlacement::PlacementPolicy policy) {
    s_policy = policy;
}

bool MinOverlapPlacement::placeWindow(
        const std::list<FluxboxWindow *> &windowlist,
        const FluxboxWindow &win, int &place_x, int &place_y) {

    // view (screen + head) constraints
    int head = (signed) win.getOnHead();
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    const ScreenPlacement &screen_placement = 
        dynamic_cast<const ScreenPlacement &>(win.screen().placementStrategy());
    s_row_dir = screen_placement.rowDirection();
    s_col_dir = screen_placement.colDirection();

    int win_w = win.width() + win.fbWindow().borderWidth()*2 + win.widthOffset();
    int win_h = win.height() + win.fbWindow().borderWidth()*2 + win.heightOffset();

    // we keep a set of open spaces on the desktop, sorted by size/location
    std::set<Region> region_set;

    // initialize the set of regions to contain the entire head
    region_set.insert(Region(Region::TOPLEFT, head_left, head_top));
    region_set.insert(Region(Region::TOPRIGHT, head_right - win_w, head_top));
    region_set.insert(Region(Region::BOTTOMLEFT, head_left, head_bot - win_h));
    region_set.insert(Region(Region::BOTTOMRIGHT, head_right - win_w,
                             head_bot - win_h));

    // go through the list of windows, creating other reasonable placements
    // at the end, we'll find the one with minimum overlap
    // the size of this set is at most 2(n+2)(n+1) (n = number of windows)
    // finding overlaps is therefore O(n^3), but it can probably be improved
    std::list<FluxboxWindow *>::const_reverse_iterator it = windowlist.rbegin(),
                                                   it_end = windowlist.rend();
    for (; it != it_end; ++it) {

        // get the dimensions of the window
        int left = (*it)->x() - (*it)->xOffset();
        int top = (*it)->y() - (*it)->yOffset();
        int right = left + (*it)->width() +
            2*(*it)->frame().window().borderWidth() +
            (*it)->widthOffset();
        int bottom = top + (*it)->height() +
            2*(*it)->frame().window().borderWidth() +
            (*it)->heightOffset();

        // go through the list of regions
        // if this window overlaps that region and the new window still fits,
        // it will create new regions to test
        std::set<Region>::iterator reg_it = region_set.begin();
        for (; reg_it != region_set.end(); ++reg_it) {

            switch (reg_it->corner) {
                case Region::TOPLEFT:
                    if (right > reg_it->x && bottom > reg_it->y) {
                        if (bottom + win_h <= head_bot)
                            region_set.insert(Region(Region::TOPLEFT,
                                                     reg_it->x, bottom));
                        if (right + win_w <= head_right)
                            region_set.insert(Region(Region::TOPLEFT,
                                                     right, reg_it->y));
                    }
                    break;
                case Region::TOPRIGHT:
                    if (left < reg_it->x + win_w && bottom > reg_it->y) {
                        if (bottom + win_h <= head_bot)
                            region_set.insert(Region(Region::TOPRIGHT,
                                                     reg_it->x, bottom));
                        if (left - win_w >= head_left)
                            region_set.insert(Region(Region::TOPRIGHT,
                                                     left - win_w, reg_it->y));
                    }
                    break;
                case Region::BOTTOMRIGHT:
                    if (left < reg_it->x + win_w && top < reg_it->y + win_h) {
                        if (top - win_h >= head_top)
                            region_set.insert(Region(Region::BOTTOMRIGHT,
                                                     reg_it->x, top - win_h));
                        if (left - win_w >= head_left)
                            region_set.insert(Region(Region::BOTTOMRIGHT,
                                                     left - win_w, reg_it->y));
                    }
                    break;
                case Region::BOTTOMLEFT:
                    if (right > reg_it->x && top < reg_it->y + win_h) {
                        if (top - win_h >= head_top)
                            region_set.insert(Region(Region::BOTTOMLEFT,
                                                     reg_it->x, top - win_h));
                        if (right + win_w <= head_right)
                            region_set.insert(Region(Region::BOTTOMLEFT,
                                                     right, reg_it->y));
                    }
                    break;
            }

        }
    }

    // choose the region with minimum overlap
    int min_so_far = win_w * win_h * windowlist.size() + 1;
    std::set<Region>::iterator min_reg = region_set.end();

    std::set<Region>::iterator reg_it = region_set.begin();
    for (; reg_it != region_set.end(); ++reg_it) {

        int overlap = 0;
        it = windowlist.rbegin();
        for (; it != windowlist.rend(); ++it) {

            // get the dimensions of the window
            int left = (*it)->x() - (*it)->xOffset();
            int top = (*it)->y() - (*it)->yOffset();
            int right = left + (*it)->width() +
                2*(*it)->frame().window().borderWidth() +
                (*it)->widthOffset();
            int bottom = top + (*it)->height() +
                2*(*it)->frame().window().borderWidth() +
                (*it)->heightOffset();

            // get the coordinates of the overlap region
            int min_right = (right > reg_it->x + win_w) ?
                reg_it->x + win_w : right;
            int min_bottom = (bottom > reg_it->y + win_h) ?
                reg_it->y + win_h : bottom;
            int max_left = (left > reg_it->x) ? left : reg_it->x;
            int max_top = (top > reg_it->y) ? top : reg_it->y;

            // now compute the overlap and add to running total
            if (min_right > max_left && min_bottom > max_top)
                overlap += (min_right - max_left) * (min_bottom - max_top);

        }

        // if this placement is better, use it
        if (overlap < min_so_far) {
            min_reg = reg_it;
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
