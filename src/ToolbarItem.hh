// ToolbarItem.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ToolbarItem.hh,v 1.4 2004/06/20 10:29:51 rathnor Exp $

#ifndef TOOLBARITEM_HH
#define TOOLBARITEM_HH

#include "FbTk/Subject.hh"

/// An item in the toolbar that has either fixed or realive size to the toolbar
class ToolbarItem {
public:
    /// size type in the toolbar
    enum Type { 
        FIXED,  ///< the size can not be changed
        RELATIVE ///< the size can be changed
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
    virtual unsigned int height() const = 0;
    virtual unsigned int borderWidth() const = 0;
    // some items might be there, but effectively empty, so shouldn't appear
    virtual bool active() { return true; }

    FbTk::Subject &resizeSig() { return m_resize_sig; }

    void setType(Type type) { m_type = type; }
    Type type() const { return m_type; }

    class ToolbarItemSubject : public FbTk::Subject {};

private:
    Type m_type;

    ToolbarItemSubject m_resize_sig;
};

#endif // TOOLBARITEM_HH
