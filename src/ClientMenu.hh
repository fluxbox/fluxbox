// ClientMenu.hh
// Copyright (c) 2007-2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef CLIENTMENU_HH
#define CLIENTMENU_HH

#include "FbMenu.hh"

#include "FbTk/Signal.hh"

class BScreen;
class FluxboxWindow;
class Focusable;

/**
 * A menu holding a set of client menus.
 * @see WorkspaceMenu
 */
class ClientMenu: public FbMenu {
public:

    typedef std::list<FluxboxWindow *> Focusables;

    /**
     * @param screen the screen to show this menu on
     * @param client a list of clients to show in this menu
     * @param listen_for_iconlist_changes Listen for list changes from the \c screen.
     */
    ClientMenu(BScreen &screen, 
               Focusables &clients, bool listen_for_iconlist_changes);

    /// refresh the entire menu
    void refreshMenu();

    /// Called when window title changed.
    void titleChanged(Focusable& win);

    /// Called when a client dies. Removes the corresponding menu item
    void clientDied(Focusable& win);

private:

    void updateClientList(BScreen& screen) {
        refreshMenu();
    }

    Focusables &m_list; ///< clients in the menu
    FbTk::SignalTracker m_slots; ///< track all the slots
};

#endif // CLIENTMENU_HH
