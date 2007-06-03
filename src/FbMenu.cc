// FbMenu.cc for fluxbox
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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


#include "FbMenu.hh"

#include "fluxbox.hh"
#include "MenuTheme.hh"
#include "Screen.hh"
#include "Shape.hh"

FbMenu::FbMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
           FbTk::XLayer &layer):
    FbTk::Menu(tm, imgctrl), 
    m_layeritem(fbwindow(), layer),
    m_shape(new Shape(fbwindow(), tm.shapePlaces())) {
    
}

FbMenu::~FbMenu() {

}

void FbMenu::updateMenu(int index) {
    FbTk::Menu::updateMenu(index);
    m_shape->update();
}

void FbMenu::clearWindow() {
    FbTk::Menu::clearWindow();
    m_shape->update();
}

void FbMenu::reconfigure() {
    m_shape->setPlaces(dynamic_cast<const MenuTheme&>(theme()).shapePlaces());
    FbTk::Menu::reconfigure();
}

void FbMenu::buttonReleaseEvent(XButtonEvent &be) {
    BScreen *screen = Fluxbox::instance()->findScreen(screenNumber());
    if (be.window == titleWindow() && isMoving() && screen) {
        // menu stopped moving, so update head
        int head = screen->getHead(be.x_root, be.y_root);
        setScreen(screen->getHeadX(head),
                  screen->getHeadY(head),
                  screen->getHeadWidth(head),
                  screen->getHeadHeight(head));
    }

    // now get on with the show
    FbTk::Menu::buttonReleaseEvent(be);
}
