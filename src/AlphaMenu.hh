// AlphaMenu.hh for Fluxbox
// Copyright (c) 2007 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef ALPHAMENU_HH
#define ALPHAMENU_HH

#include "ToggleMenu.hh"
#include "FbTk/MenuItem.hh"
#include "ObjectResource.hh"

class AlphaObject {
public:

    virtual int getFocusedAlpha() const = 0;
    virtual int getUnfocusedAlpha() const = 0;
    virtual bool getUseDefaultAlpha() const = 0;

    virtual void setFocusedAlpha(int alpha) = 0;
    virtual void setUnfocusedAlpha(int alpha) = 0;
    virtual void setUseDefaultAlpha(bool use_default) = 0;

    virtual ~AlphaObject() {};
};


class AlphaMenu : public ToggleMenu {
public:
    AlphaMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
              FbTk::XLayer &layer, AlphaObject &object);

    // we override these to update the menu when the active window changes
    void move(int x, int y);
    void show();

    ObjectResource<AlphaObject, int> m_focused_alpha_resource;
    ObjectResource<AlphaObject, int> m_unfocused_alpha_resource;

};

class AlphaMenuSelectItem : public FbTk::MenuItem {

public:
    AlphaMenuSelectItem(const FbTk::FbString &label, AlphaObject *object, AlphaMenu &parent):
        FbTk::MenuItem(label), m_object(object), m_parent(parent) {
        setToggleItem(true);
    }

    bool isSelected() const { return m_object->getUseDefaultAlpha(); }
    void click(int button, int time) {
        bool newval = !m_object->getUseDefaultAlpha();
        m_object->setUseDefaultAlpha(newval);
        // items 1 and 2 (the focused/unfocused values) are only enabled if we don't use default values
        m_parent.setItemEnabled(1, !newval);
        m_parent.setItemEnabled(2, !newval);
        m_parent.show(); // cheat to refreshing the window
        FbTk::MenuItem::click(button, time);
    }

    void updateLabel() {
        bool val = m_object->getUseDefaultAlpha();
        m_parent.setItemEnabled(1, !val);
        m_parent.setItemEnabled(2, !val);
        m_parent.updateMenu();
    }

private:
    AlphaObject *m_object;
    AlphaMenu &m_parent;
};

#endif // ALPHAMENU_HH
