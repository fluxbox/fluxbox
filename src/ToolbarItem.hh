// ToolbarItem.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef TOOLBARITEM_HH
#define TOOLBARITEM_HH

#include "FbTk/Signal.hh"
#include "FbTk/Orientation.hh"

/// An item in the toolbar that has either fixed or relative size to the toolbar
class ToolbarItem {
public:
    /// size type in the toolbar
    enum Type {
        FIXED,  ///< the size can not be changed
        RELATIVE, ///< the size can be changed
        SQUARE ///< the size is fixed relative to the parent, and in both dimensions
    };

    explicit ToolbarItem(Type type);
    virtual ~ToolbarItem();

    virtual void move(int x, int y) = 0;
    virtual void resize(unsigned int width, unsigned int height) = 0;
    virtual void moveResize(int x, int y,
                            unsigned int width, unsigned int height) = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual unsigned int width() const = 0;
    virtual unsigned int preferredWidth() const { return width(); }
    virtual unsigned int height() const = 0;
    virtual unsigned int borderWidth() const = 0;
    // some items might be there, but effectively empty, so shouldn't appear
    virtual bool active() { return true; }

    // Tools should NOT listen to theme changes - they'll get notified by
    // the toolbar instead. Otherwise there are ordering problems.
    virtual void renderTheme(int alpha) = 0;

    // insist implemented, even if blank
    virtual void parentMoved() = 0; // called when moved from hiding

    // just update theme items that affect the size
    virtual void updateSizing() = 0;

    FbTk::Signal<> &resizeSig() { return m_resize_sig; }

    void setType(Type type) { m_type = type; }
    Type type() const { return m_type; }

    FbTk::Orientation orientation() const { return m_orientation; }
    virtual void setOrientation(FbTk::Orientation orient) { m_orientation = orient; }

    /// Edge length of an icon in a toolbar slot 'slot' pixels high,
    /// shared by the systray and the iconbar so their icons match:
    /// the common icon theme size nearest to 3/4 of the slot, so clients
    /// can use a pixel-exact image instead of a blurry rescale. Snapping
    /// down instead left 16 pixel icons in 28 pixel slots. The icon
    /// keeps at least 1 pixel above and below; slots too small for a
    /// theme size get that maximum. A tie picks the smaller size.
    /// Inline, as IconButton is built even without the toolbar sources.
    static unsigned int iconSize(unsigned int slot) {
        static const unsigned int sizes[] = { 16, 22, 24, 32, 48, 64 };

        const unsigned int target = slot * 3 / 4;
        const unsigned int limit = (slot > 2) ? slot - 2 : 1;
        unsigned int best = 0;
        unsigned int best_diff = 0;
        for (unsigned int size : sizes) {
            if (size > limit) {
                break;
            }
            const unsigned int diff = (size > target) ? size - target : target - size;
            if (best == 0 || diff < best_diff) {
                best = size;
                best_diff = diff;
            }
        }
        return (best != 0) ? best : limit;
    }

private:
    Type m_type;

    FbTk::Orientation m_orientation;

    FbTk::Signal<> m_resize_sig;
};

#endif // TOOLBARITEM_HH
