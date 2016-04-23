// ColSmartPlacement.cc
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

#include "ColSmartPlacement.hh"

#include "FocusControl.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "Window.hh"

bool ColSmartPlacement::placeWindow(const FluxboxWindow &win, int head,
                                    int &place_x, int &place_y) {

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

    // xinerama head constraints
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    bool placed = false;
    int next_x, next_y;
    const ScreenPlacement &screen_placement = win.screen().placementStrategy();

    bool top_bot = screen_placement.colDirection() == ScreenPlacement::TOPBOTTOM;
    bool left_right = screen_placement.rowDirection() == ScreenPlacement::LEFTRIGHT;

    int test_x;

    int win_w = win.width() + win.fbWindow().borderWidth()*2 + win.widthOffset();
    int win_h = win.height() + win.fbWindow().borderWidth()*2 + win.heightOffset();

    int x_off = win.xOffset();
    int y_off = win.yOffset();

    if (left_right)
        test_x = head_left;
    else
        test_x = head_right - win_w;

    int change_y = 1;
    if (screen_placement.colDirection() == ScreenPlacement::BOTTOMTOP)
        change_y = -1;

    while (!placed &&
           (left_right ? test_x + win_w <= head_right
            : test_x >= head_left)) {

        if (left_right)
            next_x = head_right; // it will get shrunk
        else 
            next_x = head_left-1;

        int test_y;
        if (top_bot)
            test_y = head_top;
        else
            test_y = head_bot - win_h;

        while (!placed && 
               (top_bot ? test_y + win_h <= head_bot
                : test_y >= head_top)) {
            placed = true;

            next_y = test_y + change_y;

            std::list<FluxboxWindow *>::const_iterator it = windowlist.begin();
            std::list<FluxboxWindow *>::const_iterator it_end = windowlist.end();
            for (; it != it_end && placed; ++it) {
                if (*it == &win) continue;
                if ((*it)->layerNum() != win.layerNum() ){ continue; } //windows are in different layers - skip it
                
                int bw = 2 * (*it)->fbWindow().borderWidth();
                int curr_x = (*it)->x() - (*it)->xOffset();
                int curr_y = (*it)->y() - (*it)->yOffset();
                int curr_w = (*it)->width()  + bw + (*it)->widthOffset();
                int curr_h = (*it)->height() + bw + (*it)->heightOffset();

                if (curr_x < test_x + win_w &&
                    curr_x + curr_w > test_x &&
                    curr_y < test_y + win_h &&
                    curr_y + curr_h > test_y) {
                    // this window is in the way
                    placed = false;

                    // we find the next y that we can go to (a window will be in the way
                    // all the way to its bottom)
                    if (top_bot) {
                        if (curr_y + curr_h > next_y)
                            next_y = curr_y + curr_h;
                    } else {
                        if (curr_y - win_h < next_y)
                            next_y = curr_y - win_h;
                    }

                    // but we can only go to the nearest x, since that is where the 
                    // next time current windows in the way will change
                    if (left_right) {
                        if (curr_x + curr_w < next_x) 
                            next_x = curr_x + curr_w;
                    } else {
                        if (curr_x - win_w > next_x)
                            next_x = curr_x - win_w;
                    }
                }
            }

            if (placed) {
                place_x = test_x + x_off;
                place_y = test_y + y_off;
            }

            test_y = next_y;
        } // end while

        test_x = next_x;
    } // end while

    return placed;
}
