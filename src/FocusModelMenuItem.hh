// FocusModelMenuItem.hh for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef FOCUSMODELMENUITEM_HH
#define FOCUSMODELMENUITEM_HH

#include "Screen.hh"

#include "FbTk/MenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Command.hh"

class FocusModelMenuItem : public FbTk::MenuItem {
public:
    FocusModelMenuItem(const char *label, BScreen &screen, 
                       BScreen::FocusModel model, 
                       FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_screen(screen), m_focusmodel(model) {
    }
    bool isEnabled() const { return m_screen.getFocusModel() != m_focusmodel; }

    void click(int button, int time) {
        m_screen.saveFocusModel(m_focusmodel);
        FbTk::MenuItem::click(button, time);
    }

private:
    BScreen &m_screen;
    BScreen::FocusModel m_focusmodel;
};

#endif // FOCUSMODELMENUITEM_HH
