// FbMenu.hh for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef TOGGLEMENU_HH
#define TOGGLEMENU_HH

#include "FbMenu.hh"

/**
 *  menu that redraws the entire menu at button release
 *  so that each toggle item gets updated
 */
class ToggleMenu: public FbMenu {
public:
    ToggleMenu(class FbTk::ThemeProxy<FbTk::MenuTheme> &tm,
               FbTk::ImageControl &imgctrl, FbTk::Layer &layer):
        FbMenu(tm, imgctrl, layer) { }

    virtual ~ToggleMenu() {}
    void buttonReleaseEvent(XButtonEvent &ev) {


        // do redraw of other items
        FbMenu::buttonReleaseEvent(ev);

        // since this menu consist of toggle menu items 
        // that relate to each other, we need to redraw
        // the items each time we get a button release event
        // so that the last toggled item gets redrawn as 
        // not toggled.
        if (ev.window == frameWindow()) {
            // force full foreground update
            frameWindow().updateBackground(false);
        }
        clearWindow();
    }

};

#endif // TOGGLEMENU_HH
