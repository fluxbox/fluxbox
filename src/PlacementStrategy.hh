// PlacementStrategy.hh
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

#ifndef PLACEMENTSTRATEGY_HH
#define PLACEMENTSTRATEGY_HH

class FluxboxWindow;

struct PlacementStrategy {
    /**
     * Calculates a placement for @win and returns suggested placement in @place_x and @place_y
     * @param win the window that needs to be placed
     * @param place_x x placement of specific strategy
     * @param place_y y placement of specific strategy
     * @return true if the strategy found a placement for the window
     */
    virtual bool placeWindow(const FluxboxWindow &win, int head,
                             int &place_x, int &place_y) = 0;

    virtual ~PlacementStrategy() { }
};

#endif // PLACEMENTSTRATEGY_HH
