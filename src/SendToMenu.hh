// SendToMenu.hh for Fluxbox
// Copyright (c) 2003 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef SENDTOMENU_HH
#define SENDTOMENU_HH

#include "FbMenu.hh"

namespace FbTk {
class Observer;
}

class BScreen;

/**
 * Creates the "send to menu".
 * Displays all the workspaces for which the current window can be sent to.
 */
class SendToMenu:public FbMenu {
public:
    /// @param screen the screen on which this menu should be created on.
    explicit SendToMenu(BScreen &screen);
    virtual ~SendToMenu();
    /// @see FbTk::Menu
    void show();
private:
    /// Rebuild the menu from scratch.
    void rebuildMenu();
    /// listens to signals that makes this instance need to rebuild menu
    FbTk::Observer *m_rebuildObs;
};

#endif // SENDTOMENU_HH
