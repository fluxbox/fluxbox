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
#include "FocusControl.hh"
#include "PlacementStrategy.hh"

#include "FbTk/I18n.hh"
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

using std::string;
using std::vector;
using std::ifstream;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

Workspace::GroupList Workspace::m_groups;

Workspace::Workspace(BScreen &scrn, const string &name, unsigned int id):
    m_screen(scrn),
    m_clientmenu(scrn, m_windowlist, &m_clientlist_sig),
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

    if (place)
        placeWindow(w);

    m_windowlist.push_back(&w);
    m_clientlist_sig.notify();

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

    if (w->isFocused() && still_alive)
        FocusControl::unfocusWindow(w->winClient(), true, true);

    m_windowlist.remove(w);
    m_clientlist_sig.notify();

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
    Windows::iterator it = m_windowlist.begin();
    Windows::const_iterator it_end = m_windowlist.end();
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
                    (*wit)->attachClient(win.winClient());
                    (*wit)->raise();
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

void Workspace::setName(const string &name) {
    if (!name.empty() && name != "") {
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

    screen().updateWorkspaceNamesAtom();

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
    m_clientlist_sig.notify();
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
