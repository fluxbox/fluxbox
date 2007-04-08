// ClientMenu.hh
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "ClientMenu.hh"

#include "Layer.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WindowCmd.hh"

#include "FbTk/MenuItem.hh"

namespace { // anonymous

class ClientMenuItem: public FbTk::MenuItem {
public:
    ClientMenuItem(Focusable &client, ClientMenu &menu):
        FbTk::MenuItem(client.title().c_str(), menu),
        m_client(client) { client.titleSig().attach(&menu); }
    ~ClientMenuItem() { m_client.titleSig().detach(menu()); }

    void click(int button, int time) {
        FluxboxWindow *fbwin = m_client.fbwindow();
        if (fbwin == 0)
            return;
        m_client.focus();
        fbwin->raise();
    }

    const std::string &label() const { return m_client.title(); }
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

private:
    Focusable &m_client;
};

}; // end anonymous namespace

ClientMenu::ClientMenu(BScreen &screen, Focusables &clients,
                       FbTk::Subject *refresh):
    FbMenu(screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer(Layer::MENU)),
    m_list(clients),
    m_refresh_sig(refresh) {

    if (refresh)
        refresh->attach(this);
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
                insert(new ClientMenuItem(**client_it, *this));
        } else
            insert(new ClientMenuItem(**win_it, *this));
    }

    updateMenu();
}

void ClientMenu::update(FbTk::Subject *subj) {
    if (subj == m_refresh_sig)
        refreshMenu();
    else
        FbTk::Menu::update(subj);
}
