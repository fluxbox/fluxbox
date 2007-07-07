// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

#include "Workspace.hh"

#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "FbWinFrame.hh"
#include "WindowCmd.hh"
#include "FocusControl.hh"
#include "PlacementStrategy.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/FbString.hh"

// use GNU extensions
#ifndef  _GNU_SOURCE
#define  _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#include <algorithm>
#include <iostream>
#include <iterator>

using std::string;
using std::vector;
using std::ifstream;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

namespace { // anonymous

int countTransients(const WinClient &client) {
    if (client.transientList().empty())
        return 0;
    // now go throu the entire tree and count transients
    size_t ret = client.transientList().size();
    WinClient::TransientList::const_iterator it = client.transientList().begin();
    WinClient::TransientList::const_iterator it_end = client.transientList().end();
    for (; it != it_end; ++it)
        ret += countTransients(*(*it));

    return ret;
}

class ClientMenuItem:public FbTk::MenuItem {
public:
    ClientMenuItem(WinClient &client):
        FbTk::MenuItem(client.title().c_str(), &client.screen().windowMenu()),
        m_client(client) {

    }
    FbTk::Menu *submenu() { return &m_client.screen().windowMenu(); }
    const FbTk::Menu *submenu() const { return &m_client.screen().windowMenu(); }

    void showSubmenu() {
        WindowCmd<void>::setClient(&m_client);
        FbTk::MenuItem::showSubmenu();
    }

    void click(int button, int time) {
        if (m_client.fbwindow() == 0)
            return;
        FluxboxWindow &win = *m_client.fbwindow();

        if (win.screen().currentWorkspaceID() != win.workspaceNumber() &&
            !win.isStuck()) {
            win.menu().hide();
            BScreen::FollowModel model = win.screen().getUserFollowModel();
            if (model == BScreen::IGNORE_OTHER_WORKSPACES)
                return;
            // fetch the window to the current workspace
            else if ((button == 3) ^ (model == BScreen::FETCH_ACTIVE_WINDOW ||
                win.isIconic() && model == BScreen::SEMIFOLLOW_ACTIVE_WINDOW)) {
                win.screen().sendToWorkspace(win.screen().currentWorkspaceID(), &win, true);
                return;
            }
            // warp to the workspace of the window
            win.screen().changeWorkspaceID(win.workspaceNumber());
        }
        win.setCurrentClient(m_client);
        win.raiseAndFocus();
    }

    const string &label() const { return m_client.title(); }
    bool isSelected() const {
        if (m_client.fbwindow() == 0)
            return false;
        if (m_client.fbwindow()->isFocused() == false)
            return false;
        return (&(m_client.fbwindow()->winClient()) == &m_client);

    }
private:
    WinClient &m_client;
};

};

Workspace::GroupList Workspace::m_groups;

Workspace::Workspace(BScreen &scrn, FbTk::MultLayers &layermanager,
                     const string &name, unsigned int id):
    m_screen(scrn),
    m_clientmenu(scrn.menuTheme(), scrn.imageControl(),
                 *scrn.layerManager().getLayer(Layer::MENU)),
    m_layermanager(layermanager),
    m_name(name),
    m_id(id) {

    menu().setInternalMenu();
    setName(name);

}


Workspace::~Workspace() {
}

void Workspace::addWindow(FluxboxWindow &w, bool place) {
    // we don't need to add a window that already exist in our list
    if (find(m_windowlist.begin(), m_windowlist.end(), &w) != m_windowlist.end())
        return;

    w.setWorkspace(m_id);
    // attach signals
    w.titleSig().attach(this);

    if (place)
        placeWindow(w);

    m_windowlist.push_back(&w);
    updateClientmenu();

    if (!w.isStuck()) {
        FluxboxWindow::ClientList::iterator client_it =
            w.clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end =
            w.clientList().end();
        for (; client_it != client_it_end; ++client_it)
            screen().updateNetizenWindowAdd((*client_it)->window(), m_id);
    }

}

// still_alive is true if the window will continue to exist after
// this event. Particularly, this isn't the removeWindow for
// the destruction of the window. Because if so, the focus revert
// is done in another place
int Workspace::removeWindow(FluxboxWindow *w, bool still_alive) {

    if (w == 0)
        return -1;

    // detach from signals
    w->titleSig().detach(this);

    if (w->isFocused() && still_alive)
        FocusControl::unfocusWindow(w->winClient(), true, true);

    // we don't remove it from the layermanager, as it may be being moved
    Windows::iterator erase_it = remove(m_windowlist.begin(),
                                        m_windowlist.end(), w);
    if (erase_it != m_windowlist.end())
        m_windowlist.erase(erase_it);

    updateClientmenu();

    if (!w->isStuck()) {
        FluxboxWindow::ClientList::iterator client_it =
            w->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end =
            w->clientList().end();
        for (; client_it != client_it_end; ++client_it)
            screen().updateNetizenWindowDel((*client_it)->window());
    }

    return m_windowlist.size();
}

void Workspace::showAll() {
    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it)
        (*it)->deiconify(false, false);
}


void Workspace::hideAll(bool interrupt_moving) {
    Windows::reverse_iterator it = m_windowlist.rbegin();
    Windows::reverse_iterator it_end = m_windowlist.rend();
    for (; it != it_end; ++it) {
        if (! (*it)->isStuck())
            (*it)->withdraw(interrupt_moving);
    }
}


void Workspace::removeAll(unsigned int dest) {
    Windows tmp_list(m_windowlist);
    Windows::iterator it = tmp_list.begin();
    Windows::const_iterator it_end = tmp_list.end();
    for (; it != it_end; ++it)
        m_screen.sendToWorkspace(dest, *it, false);
}


void Workspace::reconfigure() {
    menu().reconfigure();

    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        if ((*it)->winClient().validateClient())
            (*it)->reconfigure();
    }
}

size_t Workspace::numberOfWindows() const {
    return m_windowlist.size();
}

namespace {
// helper class for checkGrouping
class FindInGroup {
public:
    FindInGroup(const FluxboxWindow &w):m_w(w) { }
    bool operator ()(const string &name) const {
        return (name == m_w.winClient().getWMClassName());
    }
private:
    const FluxboxWindow &m_w;
};

};

//Note: this function doesn't check if the window is groupable
bool Workspace::checkGrouping(FluxboxWindow &win) {
    if (win.numClients() == 0)
        return false;
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): Checking grouping. ("<<win.title()<<")"<<endl;
#endif // DEBUG
    if (!win.isGroupable()) { // make sure this window can hold a tab
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): window can't use a tab"<<endl;
#endif // DEBUG
        return false;
    }

    string instance_name = win.winClient().getWMClassName();

    // go through every group and search for matching win instancename
    GroupList::iterator g(m_groups.begin());
    GroupList::iterator g_end(m_groups.end());
    for (; g != g_end; ++g) {
        Group::iterator name((*g).begin());
        Group::iterator name_end((*g).end());
        for (; name != name_end; ++name) {

            if ((*name) != instance_name)
                continue;

            // find a window with the specific name
            Windows::iterator wit(m_windowlist.begin());
            Windows::iterator wit_end(m_windowlist.end());
            for (; wit != wit_end; ++wit) {
#ifdef DEBUG
                cerr<<__FILE__<<" check group with : "<<(*wit)->winClient().getWMClassName()<<endl;
#endif // DEBUG
                if (find_if((*g).begin(),
                            (*g).end(),
                            FindInGroup(*(*wit))) != (*g).end()) {
                    // make sure the window is groupable
                    // and don't group with ourself
                    if ( !(*wit)->isGroupable() || (*wit)->winClient().fbwindow() == &win)
                        break; // try next name
#ifdef DEBUG
                    cerr<<__FILE__<<"("<<__FUNCTION__<<"): window ("<<*wit<<") attaching window ("<<&win<<")"<<endl;
#endif // DEBUG
                    WinClient &client = win.winClient();
                    (*wit)->attachClient(client);
                    if (client.screen().focusControl().focusNew())
                        (*wit)->setCurrentClient(client);
                    return true; // grouping done

                }

            }

        }

    }

    return false;
}

bool Workspace::loadGroups(const string &filename) {
    string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbTk::StringUtil::removeTrailingWhitespace(real_filename);
    ifstream infile(real_filename.c_str());
    if (!infile)
        return false;

    m_groups.clear(); // erase old groups

    // load new groups
    while (!infile.eof()) {
        string line;
        vector<string> names;
        getline(infile, line);
        FbTk::StringUtil::stringtok(names, line);
        m_groups.push_back(names);
    }

    return true;
}

void Workspace::update(FbTk::Subject *subj) {
    updateClientmenu();
}


void Workspace::setName(const string &name) {
    if (!name.empty() && name != "") {
        if (name == m_name)
            return;
        m_name = name;
    } else { //if name == 0 then set default name from nls
        _FB_USES_NLS;
        char tname[128];
        sprintf(tname,
                _FB_XTEXT(Workspace, DefaultNameFormat,
                        "Workspace %d", "Default workspace names, with a %d for the workspace number").c_str(),
                m_id + 1); //m_id starts at 0
        m_name = tname;
    }

    screen().updateWorkspaceName(m_id);

    menu().setLabel(m_name);
    menu().updateMenu();
}

/**
 Calls restore on all windows
 on the workspace and then
 clears the m_windowlist
*/
void Workspace::shutdown() {
    // note: when the window dies it'll remove it self from the list
    while (!m_windowlist.empty()) {
        //delete window (the window removes it self from m_windowlist)
        delete m_windowlist.back();
    }
}

void Workspace::updateClientmenu() {
    // remove all items and then add them again
    menu().removeAll();
    // for each fluxboxwindow add every client in them to our clientlist
    Windows::iterator win_it = m_windowlist.begin();
    Windows::iterator win_it_end = m_windowlist.end();
    for (; win_it != win_it_end; ++win_it) {
        // add every client in this fluxboxwindow to menu
        FluxboxWindow::ClientList::iterator client_it =
            (*win_it)->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end =
            (*win_it)->clientList().end();
        for (; client_it != client_it_end; ++client_it)
            menu().insert(new ClientMenuItem(*(*client_it)));
    }

    menu().updateMenu();
}

void Workspace::placeWindow(FluxboxWindow &win) {
    int place_x, place_y;
    // we ignore the return value,
    // the screen placement strategy is guaranteed to succeed.
    screen().placementStrategy().placeWindow(m_windowlist,
                                             win,
                                             place_x, place_y);

    win.moveResize(place_x, place_y, win.width(), win.height());
}
