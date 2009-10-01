// BoolMenuItem.hh for FbTk
// Copyright (c) 2003 - 2007 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_BOOLMENUITEM_HH
#define FBTK_BOOLMENUITEM_HH

#include "MenuItem.hh"

namespace FbTk {

template <typename> class Accessor;

/// a bool menu item
class BoolMenuItem: public FbTk::MenuItem {
public:
    BoolMenuItem(const FbTk::FbString &label, Accessor<bool> &item, 
                 FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::MenuItem(label, cmd), m_item(item) { 
        FbTk::MenuItem::setSelected(m_item);
        setToggleItem(true);
        setCloseOnClick(false);
    }
    BoolMenuItem(const FbTk::FbString &label, Accessor<bool> &item):
        FbTk::MenuItem(label), m_item(item) {
        FbTk::MenuItem::setSelected(m_item);
        setToggleItem(true);
        setCloseOnClick(false);
    }
    bool isSelected() const { return m_item; }
    // toggle state
    void click(int button, int time, unsigned int mods) {
        setSelected(!m_item);
        FbTk::MenuItem::click(button, time, mods);
    }
    void setSelected(bool value) { 
        m_item = value;
        FbTk::MenuItem::setSelected(m_item);
    }
private:
    Accessor<bool> &m_item;
};

} // end namespace FbTk

#endif // FBTK_BOOLMENUITEM_HH
