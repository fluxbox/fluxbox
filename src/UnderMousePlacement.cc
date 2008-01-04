// UnderMousePlacement.cc
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

#include "UnderMousePlacement.hh"

#include "FbTk/App.hh"
#include "Screen.hh"
#include "Window.hh"

bool UnderMousePlacement::placeWindow(const FluxboxWindow &win, int head,
                                      int &place_x, int &place_y) {

    int root_x, root_y, ignore_i;

    unsigned int ignore_ui;

    Window ignore_w;

    XQueryPointer(FbTk::App::instance()->display(),
                  win.screen().rootWindow().window(), &ignore_w, 
                  &ignore_w, &root_x, &root_y,
                  &ignore_i, &ignore_i, &ignore_ui);

    // 2*border = border on each side of the screen
    // not using offset ones because we won't let tabs influence the "centre"
    int win_w = win.width() + win.fbWindow().borderWidth()*2,
        win_h = win.height() + win.fbWindow().borderWidth()*2;

    int test_x = root_x - (win_w / 2);
    int test_y = root_y - (win_h / 2);

    // keep the window inside the screen
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    if (test_x < head_left)
        test_x = head_left;

    if (test_x + win_w > head_right)
        test_x = head_right - win_w;

    if (test_y < head_top)
        test_y = head_top;

    if (test_y + win_h > head_bot)
        test_y = head_bot - win_h;

    place_x = test_x;
    place_y = test_y;

    return true;
}
