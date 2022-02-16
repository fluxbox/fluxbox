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

#include "ClientMenu.hh"

#include "Layer.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "FocusControl.hh"

#include "FbTk/MenuItem.hh"
#include "FbTk/MemFun.hh"

#include <X11/keysym.h>

namespace { // anonymous

class ClientMenuItem: public FbTk::MenuItem {
public:
    ClientMenuItem(Focusable &client, ClientMenu &menu):
        FbTk::MenuItem(client.title(), menu),
        m_client(client) {
        m_signals.join(client.titleSig(),
                       FbTk::MemFunSelectArg1(menu, &ClientMenu::titleChanged));
        m_signals.join(client.dieSig(), FbTk::MemFun(menu, &ClientMenu::clientDied));
    }

    void click(int button, int time, unsigned int mods) {
        FluxboxWindow *fbwin = m_client.fbwindow();
        if (fbwin == 0)
            return;

        // this MenuItem object can get destroyed as a result of focus(), so we
        // must get a local copy of anything we want to use here
        // AFTER ~ClientMenuItem() is called.
        FbTk::Menu *parent = menu();
        FocusControl& focus_control = m_client.screen().focusControl();

        if (WinClient *winc = dynamic_cast<WinClient*>(&m_client)) {
            fbwin->setCurrentClient(*winc, false);
        }
        m_client.focus();
        fbwin->raise();
        if ((mods & ControlMask) == 0) {
            // Ignore any focus changes due to this menu closing
            // (even in StrictMouseFocus mode)
            focus_control.ignoreAtPointer(true);
            parent->hide();
        }
    }

    const FbTk::BiDiString &label() const { return m_client.title(); }
    const FbTk::PixmapWithMask *icon() const {
        return m_client.screen().clientMenuUsePixmap() ? &m_client.icon() : 0;
    }

    bool isSelected() const {
        if (m_client.fbwindow() == 0)
            return false;
        if (m_client.fbwindow()->isFocused() == false)
            return false;
        return (&(m_client.fbwindow()->winClient()) == &m_client);
    }

    // for updating menu when receiving a signal from client
    Focusable *client() { return &m_client; }

private:
    Focusable &m_client;
    FbTk::SignalTracker m_signals;
};

} // end anonymous namespace

ClientMenu::ClientMenu(BScreen &screen, Focusables &clients,
                       bool listen_for_iconlist_changes):
    FbMenu(screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer(ResourceLayer::MENU)),
    m_list(clients) {

    if (listen_for_iconlist_changes) {
        m_slots.join(screen.iconListSig(),
                     FbTk::MemFun(*this, &ClientMenu::updateClientList));
    }

    refreshMenu();

}

void ClientMenu::refreshMenu() {
    // remove all items and then add them again
    removeAll();

    // for each fluxboxwindow add every client in them to our clientlist
    Focusables::iterator win_it = m_list.begin();
    Focusables::iterator win_it_end = m_list.end();
    for (; win_it != win_it_end; ++win_it) {
        // add every client in this fluxboxwindow to menu
        if (typeid(*win_it) == typeid(FluxboxWindow *)) {
            FluxboxWindow *win = static_cast<FluxboxWindow *>(*win_it);
            FluxboxWindow::ClientList::iterator client_it =
                win->clientList().begin();
            FluxboxWindow::ClientList::iterator client_it_end =
                win->clientList().end();
            for (; client_it != client_it_end; ++client_it)
                insertItem(new ClientMenuItem(**client_it, *this));
        } else
            insertItem(new ClientMenuItem(**win_it, *this));
    }

    updateMenu();
}

namespace {

ClientMenuItem* getMenuItem(ClientMenu& menu, Focusable& win) {
    // find the corresponding menuitem
    ClientMenuItem *cl_item = 0;
    for (size_t i = 0; i < menu.numberOfItems(); i++) {
        FbTk::MenuItem *item = menu.find(i);
        if (item && typeid(*item) == typeid(ClientMenuItem)) {
            cl_item = static_cast<ClientMenuItem *>(item);
            if (cl_item->client() == &win)
                break;
        }
    }

    return cl_item;

}

} // anonymous

void ClientMenu::titleChanged(Focusable& win) {
    // find correct menu item
    ClientMenuItem* cl_item = getMenuItem(*this, win);
    if (cl_item)
        themeReconfigured();
}

void ClientMenu::clientDied(Focusable &win) {
    // find correct menu item
    ClientMenuItem* cl_item = getMenuItem(*this, win);

    // update accordingly
    if (cl_item)
        FbTk::Menu::removeItem(cl_item);
}
