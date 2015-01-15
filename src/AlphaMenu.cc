// AlphaMenu.cc for Fluxbox
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

#include "AlphaMenu.hh"

#include "Window.hh"
#include "Screen.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "Layer.hh"
#include "FbTk/IntMenuItem.hh"

#include "FbTk/I18n.hh"
#include "Window.hh"

AlphaMenu::AlphaMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm,
                     FbTk::ImageControl &imgctrl, FbTk::Layer &layer):
    ToggleMenu(tm, imgctrl, layer)
{

    static WindowMenuAccessor<int> m_focused_alpha((WindowMenuAccessor<int>::Getter)&FluxboxWindow::getFocusedAlpha,
            (WindowMenuAccessor<int>::Setter)&FluxboxWindow::setFocusedAlpha, 255);
    static WindowMenuAccessor<int> m_unfocused_alpha((WindowMenuAccessor<int>::Getter)&FluxboxWindow::getUnfocusedAlpha,
            (WindowMenuAccessor<int>::Setter)&FluxboxWindow::setUnfocusedAlpha, 255);

    _FB_USES_NLS;

    // build menu...

    const FbTk::FbString focused_alpha_label = 
        _FB_XTEXT(Configmenu, FocusedAlpha,
                  "Focused Window Alpha",
                  "Transparency level of the focused window");

    m_focused_alpha_item =
        new FbTk::IntMenuItem(focused_alpha_label, m_focused_alpha, 0, 255, *this);
    insertItem(m_focused_alpha_item);

    const FbTk::FbString unfocused_alpha_label =
        _FB_XTEXT(Configmenu, UnfocusedAlpha,
                  "Unfocused Window Alpha",
                  "Transparency level of unfocused windows");

    m_unfocused_alpha_item =
        new FbTk::IntMenuItem(unfocused_alpha_label, m_unfocused_alpha, 0, 255, *this);
    insertItem(m_unfocused_alpha_item);

    const FbTk::FbString usedefault_label = _FB_XTEXT(Windowmenu, DefaultAlpha,
                                                      "Use Defaults",
                                                      "Default transparency settings for this window");
    FbTk::MenuItem *usedefault_item =
        new AlphaMenuSelectItem(usedefault_label, *this);
    insertItem(usedefault_item);

    updateMenu();
}


void AlphaMenu::move(int x, int y) {
    FbTk::Menu::move(x, y);

    if (isVisible()) {
        m_focused_alpha_item->updateLabel();
        m_unfocused_alpha_item->updateLabel();
        frameWindow().updateBackground(false);
        FbTk::Menu::clearWindow();
    }
}

void AlphaMenu::show() {
    m_focused_alpha_item->updateLabel();
    m_unfocused_alpha_item->updateLabel();
    frameWindow().updateBackground(false);
    FbTk::Menu::clearWindow();

    FbTk::Menu::show();
}
