// IconMenu.cc for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "IconMenu.hh"

#include "Screen.hh"
#include "IconMenuItem.hh"
#include "Layer.hh"
#include "FbTk/I18n.hh"

#include <typeinfo>

static void updateItems(FbTk::Menu &menu, BScreen &screen) {
    menu.removeAll();
    BScreen::Icons::iterator it = screen.iconList().begin();
    BScreen::Icons::iterator it_end = screen.iconList().end();
    for (; it != it_end; ++it) {
        FluxboxWindow::ClientList::iterator client_it = (*it)->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end = (*it)->clientList().end();
        for (; client_it != client_it_end; ++client_it)
            menu.insert(new IconMenuItem(**client_it));
    }
    menu.updateMenu();
}

IconMenu::IconMenu(BScreen &screen):
    FbMenu(screen.menuTheme(), 
           screen.imageControl(), 
           *screen.layerManager().
           getLayer(Layer::MENU)) {

    _FB_USES_NLS;
    setLabel(_FB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"));
    screen.iconListSig().attach(this);
    updateItems(*this, screen);
}

void IconMenu::update(FbTk::Subject *subj) {
    if (subj == 0)
        FbTk::Menu::update(subj);
    else if (typeid(*subj) == typeid(BScreen::ScreenSubject)) {
        BScreen &screen = static_cast<BScreen::ScreenSubject *>(subj)->screen();
        updateItems(*this, screen);
    } else 
        FbTk::Menu::update(subj);
}
