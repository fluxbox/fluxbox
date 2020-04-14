// WindowState.cc
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "WindowState.hh"

#include "FbTk/StringUtil.hh"

#include <cstdlib>
#include <cerrno>

bool WindowState::useBorder() const {
    return !fullscreen && maximized != MAX_FULL && deco_mask & DECORM_BORDER;
}

bool WindowState::useHandle() const {
    return !fullscreen && !shaded && deco_mask & DECORM_HANDLE &&
           size_hints.isResizable();
}

bool WindowState::useTabs() const {
    return !fullscreen && deco_mask & DECORM_TAB;
}

bool WindowState::useTitlebar() const {
    return !fullscreen && deco_mask & DECORM_TITLEBAR;
}

void WindowState::saveGeometry(int new_x, int new_y,
                               unsigned int new_w, unsigned int new_h,
                               bool force) {
    if ((fullscreen || maximized == MAX_FULL) && !force)
        return;

    if (!(maximized & MAX_HORZ) || force) {
        x = new_x;
        width = new_w;
    }
    if (!(maximized & MAX_VERT) || force) {
        y = new_y;
        if (!shaded || force)
            height = new_h;
    }
}

int WindowState::queryToggleMaximized(int type) const {
    if (type == MAX_NONE)
        return maximized;

    int new_max = maximized;
    // toggle maximize vertically?
    // when _don't_ we want to toggle?
    // - type is horizontal maximize, or
    // - type is full and we are not maximized horz but already vertically
    if (type != MAX_HORZ && (type != MAX_FULL || maximized != MAX_VERT))
        new_max ^= MAX_VERT;

    // maximize horizontally?
    if (type != MAX_VERT && (type != MAX_FULL || maximized != MAX_HORZ))
        new_max ^= MAX_HORZ;

    return new_max;
}

int WindowState::getDecoMaskFromString(const std::string &str_label) {
    std::string label = FbTk::StringUtil::toLower(str_label);
    if (label == "none")
        return DECOR_NONE;
    if (label == "normal")
        return DECOR_NORMAL;
    if (label == "tiny")
        return DECOR_TINY;
    if (label == "tool")
        return DECOR_TOOL;
    if (label == "border")
        return DECOR_BORDER;
    if (label == "tab")
        return DECOR_TAB;

    int mask = -1;
    FbTk::StringUtil::extractNumber(str_label, mask);

    return mask;
}

bool SizeHints::isResizable() const {
    return max_width == 0 || max_height == 0 ||
           max_width > min_width || max_height > min_height;
}

void SizeHints::reset(const XSizeHints &sizehint) {
    if (sizehint.flags & PMinSize) {
        min_width = sizehint.min_width;
        min_height = sizehint.min_height;
    } else
        min_width = min_height = 1;

    if (sizehint.flags & PBaseSize) {
        base_width = std::max(sizehint.base_width, 0);
        base_height = std::max(sizehint.base_height, 0);

        if (!(sizehint.flags & PMinSize)) {
            min_width = base_width;
            min_height = base_height;
        }
    } else
        base_width = base_height = 0;

    if (sizehint.flags & PMaxSize) {
        max_width = sizehint.max_width;
        max_height = sizehint.max_height;
    } else
        max_width = max_height = 0; // unbounded

    if (sizehint.flags & PResizeInc) {
        width_inc = sizehint.width_inc;
        height_inc = sizehint.height_inc;
    } else
        width_inc = height_inc = 1;

    if (sizehint.flags & PAspect) {
        min_aspect_x = sizehint.min_aspect.x;
        min_aspect_y = sizehint.min_aspect.y;
        max_aspect_x = sizehint.max_aspect.x;
        max_aspect_y = sizehint.max_aspect.y;
    } else {
        min_aspect_x = max_aspect_y = 0;
        min_aspect_y = max_aspect_x = 1;
    }

    if (sizehint.flags & PWinGravity)
        win_gravity = sizehint.win_gravity;
    else
        win_gravity = NorthWestGravity;

    // some sanity checks
    if (width_inc == 0)
        width_inc = 1;
    if (height_inc == 0)
        height_inc = 1;

    if (base_width > min_width)
        min_width = base_width;
    if (base_height > min_height)
        min_height = base_height;
}

namespace {

void closestPointToAspect(unsigned int &ret_x, unsigned int &ret_y,
                          unsigned int point_x, unsigned int point_y,
                          unsigned int aspect_x, unsigned int aspect_y) {
    double u = static_cast<double>(point_x * aspect_x + point_y * aspect_y) /
               static_cast<double>(aspect_x * aspect_x + aspect_y * aspect_y);

    ret_x = static_cast<unsigned int>(u * aspect_x);
    ret_y = static_cast<unsigned int>(u * aspect_y);
}

unsigned int increaseToMultiple(unsigned int val, unsigned int inc) {
    return val % inc ? val + inc - (val % inc) : val;
}

unsigned int decreaseToMultiple(unsigned int val, unsigned int inc) {
    return val % inc ? val - (val % inc) : val;
}

} // end of anonymous namespace

/**
 * Changes width and height to the nearest (lower) value
 * that conforms to it's size hints.
 *
 * display_* give the values that would be displayed
 * to the user when resizing.
 * We use pointers for display_* since they are optional.
 *
 * See ICCCM section 4.1.2.3
 */
void SizeHints::apply(unsigned int &width, unsigned int &height,
                                  bool make_fit) const {

    /* aspect ratios are applied exclusive to the base size
     *
     * min_aspect_x      width      max_aspect_x
     * ------------  <  -------  <  ------------
     * min_aspect_y      height     max_aspect_y
     *
     * The trick is how to get back to the aspect ratio with minimal
     * change - do we modify x, y or both?
     * A: we minimise the distance between the current point, and
     *    the target aspect ratio (consider them as x,y coordinates)
     *  Consider that the aspect ratio is a line, and the current
     *  w/h is a point, so we're just using the formula for
     *  shortest distance from a point to a line!
     */

    // make respective to base_size
    unsigned int w = width - base_width, h = height - base_height;

    if (min_aspect_y > 0 && w * min_aspect_y < min_aspect_x * h) {
        closestPointToAspect(w, h, w, h, min_aspect_x, min_aspect_y);
        // new w must be > old w, new h must be < old h
        w = increaseToMultiple(w, width_inc);
        h = decreaseToMultiple(h, height_inc);
    } else if (max_aspect_x > 0 && w * max_aspect_y > max_aspect_x * h) {
        closestPointToAspect(w, h, w, h, max_aspect_x, max_aspect_y);
        // new w must be < old w, new h must be > old h
        w = decreaseToMultiple(w, width_inc);
        h = increaseToMultiple(h, height_inc);
    }

    // Check minimum size
    if (w + base_width < min_width) {
        w = increaseToMultiple(min_width - base_width, width_inc);
        // need to check maximum aspect again
        if (max_aspect_x > 0 && w * max_aspect_y > max_aspect_x * h)
            h = increaseToMultiple(w * max_aspect_y / max_aspect_x, height_inc);
    }

    if (h + base_height < min_height) {
        h = increaseToMultiple(min_height - base_height, height_inc);
        // need to check minimum aspect again
        if (min_aspect_y > 0 && w * min_aspect_y < min_aspect_x * h)
            w = increaseToMultiple(h * min_aspect_x / min_aspect_y, width_inc);
    }

    unsigned int max_w = (make_fit && (width < max_width || max_width == 0)) ?  width : max_width;
    unsigned int max_h = (make_fit && (height < max_height || max_height == 0)) ?  height : max_height;

    // Check maximum size
    if (max_w > 0 && w + base_width > max_w)
        w = max_w - base_width;

    if (max_h > 0 && h + base_height > max_h)
        h = max_h - base_height;

    w = decreaseToMultiple(w, width_inc);
    h = decreaseToMultiple(h, height_inc);

    // need to check aspects one more time
    if (min_aspect_y > 0 && w * min_aspect_y < min_aspect_x * h)
        h = decreaseToMultiple(w * min_aspect_y / min_aspect_x, height_inc);

    if (max_aspect_x > 0 && w * max_aspect_y > max_aspect_x * h)
        w = decreaseToMultiple(h * max_aspect_x / max_aspect_y, width_inc);

    width = w + base_width;
    height = h + base_height;
}

// check if the given width and height satisfy the size hints
bool SizeHints::valid(unsigned int w, unsigned int h) const {
    if (w < min_width || h < min_height)
        return false;

    if (w > max_width || h > max_height)
        return false;

    if ((w - base_width) % width_inc != 0)
        return false;

    if ((h - base_height) % height_inc != 0)
        return false;

    if (min_aspect_x * (h - base_height) > (w - base_width) * min_aspect_y)
        return false;

    if (max_aspect_x * (h - base_height) < (w - base_width) * max_aspect_y)
        return false;

    return true;
}

void SizeHints::displaySize(unsigned int &i, unsigned int &j,
        unsigned int width, unsigned int height) const {
    i = (width - base_width) / width_inc;
    j = (height - base_height) / height_inc;
}
