// RectangleUtil.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2026 Mathias Gumz (akira at fluxbox dot org)
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

#include "RectangleUtil.hh"

#include <algorithm>

namespace FbTk {
namespace RectangleUtil {

bool insideRectangle(int x, int y, int width, int height, int px, int py) {

    return
        px >= x &&
        px < (x + width) &&
        py >= y &&
        py < (y + height);
}

bool overlapRectangles(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight) {

    // x + width is one past the last column, so rectangles that only
    // share an edge don't overlap
    bool do_not_overlap =
        ax >= (bx + bwidth) ||
        bx >= (ax + awidth) ||
        ay >= (by + bheight) ||
        by >= (ay + aheight);

    return !do_not_overlap;
}

int overlapArea(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight) {

    // the overlap region, if any, spans from the larger left/top edge to
    // the smaller right/bottom edge
    const int w = std::min(ax + awidth, bx + bwidth) - std::max(ax, bx);
    const int h = std::min(ay + aheight, by + bheight) - std::max(ay, by);
    if (w <= 0 || h <= 0) {
        return 0;
    }
    return w * h;
}

namespace {

// edges, right/bottom exclusive
struct Box {
    int l, t, r, b;

    bool contains(int px, int py) const {
        return insideRectangle(l, t, r - l, b - t, px, py);
    }
};

} // end anonymous namespace

/* All region and free-part edges cut the plane into a grid of cells; each
 * cell is either wholly free or wholly not. With a handful of regions (heads)
 * the grid is tiny, so try every block of cells, with prefix sums making each
 * check O(1). Ties keep the top-left-most block.
 */
Rect largestFreeRectangle(const std::vector<Rect> &regions,
                          const std::vector<Rect> &free_parts) {

    const size_t n = std::min(regions.size(), free_parts.size());
    std::vector<Box> region(n), avail(n);
    std::vector<int> xs, ys;

    for (size_t k = 0; k < n; ++k) {
        const Rect &rr = regions[k], &fr = free_parts[k];
        region[k] = Box{ rr.x, rr.y, rr.x + rr.width, rr.y + rr.height };
        // a free part never reaches beyond its region
        avail[k] = Box{ std::max(region[k].l, fr.x),
                        std::max(region[k].t, fr.y),
                        std::min(region[k].r, fr.x + fr.width),
                        std::min(region[k].b, fr.y + fr.height) };

        const Box *bs[] = { &region[k], &avail[k] };
        for (const Box *b : bs) {
            xs.push_back(b->l); xs.push_back(b->r);
            ys.push_back(b->t); ys.push_back(b->b);
        }
    }

    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    Rect best = { 0, 0, 0, 0 };
    if (xs.size() < 2 || ys.size() < 2) {
        return best;
    }

    const size_t nx = xs.size() - 1, ny = ys.size() - 1;
    const size_t stride = nx + 1;

    // blocked[(j+1)*stride + (i+1)]: number of non-free cells in [0,i]x[0,j]
    std::vector<int> blocked(stride * (ny + 1), 0);
    for (size_t j = 0; j < ny; ++j) {
        for (size_t i = 0; i < nx; ++i) {
            // the cell's top-left corner stands for the whole cell
            const int x = xs[i], y = ys[j];
            bool in_avail = false, reserved = false;
            for (size_t k = 0; k < n; ++k) {
                if (avail[k].contains(x, y)) {
                    in_avail = true;
                } else if (region[k].contains(x, y)) {
                    reserved = true;
                }
            }
            const int cell_blocked = (in_avail && !reserved) ? 0 : 1;
            blocked[(j + 1)*stride + (i + 1)] =
                cell_blocked +
                blocked[j*stride + (i + 1)] +
                blocked[(j + 1)*stride + i] -
                blocked[j*stride + i];
        }
    }

    long best_area = 0;
    for (size_t j1 = 0; j1 < ny; ++j1) {
        for (size_t i1 = 0; i1 < nx; ++i1) {
            for (size_t j2 = j1; j2 < ny; ++j2) {
                for (size_t i2 = i1; i2 < nx; ++i2) {
                    const int nblocked =
                        blocked[(j2 + 1)*stride + (i2 + 1)] -
                        blocked[j1*stride + (i2 + 1)] -
                        blocked[(j2 + 1)*stride + i1] +
                        blocked[j1*stride + i1];
                    if (nblocked) {
                        break; // a wider block keeps the blocked cell
                    }
                    const int w = xs[i2 + 1] - xs[i1], h = ys[j2 + 1] - ys[j1];
                    if (long(w) * h > best_area) {
                        best_area = long(w) * h;
                        best = Rect{ xs[i1], ys[j1], w, h };
                    }
                }
            }
        }
    }

    return best;
}

} // namespace RectangleUtil
} // namespace FbTk
