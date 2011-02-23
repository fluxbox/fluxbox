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

#ifndef ALPHAMENU_HH
#define ALPHAMENU_HH

#include "ToggleMenu.hh"
#include "WindowCmd.hh"
#include "WindowMenuAccessor.hh"
#include "FbTk/MenuItem.hh"

namespace FbTk {
    class IntMenuItem;
}

class AlphaMenu : public ToggleMenu {
public:
    AlphaMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm,
              FbTk::ImageControl &imgctrl, FbTk::Layer &layer);

    // we override these to update the menu when the active window changes
    void move(int x, int y);
    void show();
private:
    FbTk::IntMenuItem* m_focused_alpha_item;
    FbTk::IntMenuItem* m_unfocused_alpha_item;
};

class AlphaMenuSelectItem : public FbTk::MenuItem {

public:
    AlphaMenuSelectItem(const FbTk::FbString &label, AlphaMenu &parent):
        FbTk::MenuItem(label), m_parent(parent) {
        setToggleItem(true);
        setCloseOnClick(false);
    }

    bool isSelected() const {
        static ConstWindowMenuAccessor<bool> s_is_default(&FluxboxWindow::getUseDefaultAlpha, true);
        return s_is_default;
    }
    void click(int button, int time, unsigned int mods) {
        static WindowCmd<void> s_set_default(&FluxboxWindow::setDefaultAlpha);
        s_set_default.execute();
        m_parent.show(); // cheat to refreshing the window
        FbTk::MenuItem::click(button, time, mods);
    }

private:
    AlphaMenu &m_parent;
};

#endif // ALPHAMENU_HH
