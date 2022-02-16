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

private:
    Type m_type;

    FbTk::Orientation m_orientation;

    FbTk::Signal<> m_resize_sig;
};

#endif // TOOLBARITEM_HH
