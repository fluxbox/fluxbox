// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2004 Henrik Kinnunen (fluxgen[at]users.sourceforge.net)
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

// $Id: Workspace.cc,v 1.97 2004/06/07 11:46:04 rathnor Exp $

#include "Workspace.hh"

#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "FbWinFrame.hh"

#include "FbTk/I18n.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/StringUtil.hh"

// use GNU extensions
#ifndef  _GNU_SOURCE
#define  _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <cstdio>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <iterator>

using namespace std;

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
        FbTk::MenuItem(client.title().c_str(), client.fbwindow() ? &client.fbwindow()->menu() : 0),
        m_client(client) {
        
    }
    void click(int button, int time) {
        if (m_client.fbwindow() == 0)
            return;
        FluxboxWindow &win = *m_client.fbwindow();

        win.screen().changeWorkspaceID(win.workspaceNumber()); 
        win.setCurrentClient(m_client);
        win.raiseAndFocus();
    }

    const std::string &label() const { return m_client.title(); }
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
                     const std::string &name, unsigned int id):
    m_screen(scrn),
    m_lastfocus(0),
    m_clientmenu(scrn.menuTheme(), scrn.imageControl(),
                 *scrn.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
    m_layermanager(layermanager),
    m_name(name),
    m_id(id) {


    m_cascade_x = new int[scrn.numHeads() + 1];
    m_cascade_y = new int[scrn.numHeads() + 1];
    for (int i=0; i < scrn.numHeads()+1; i++) {
        m_cascade_x[i] = 32 + scrn.getHeadX(i);
        m_cascade_y[i] = 32 + scrn.getHeadY(i);
    }
    menu().setInternalMenu();
    setName(name);
}


Workspace::~Workspace() {
    delete [] m_cascade_x;
    delete [] m_cascade_y;
}

void Workspace::setLastFocusedWindow(FluxboxWindow *win) {
    // make sure we have this window in the list
    if (std::find(m_windowlist.begin(), m_windowlist.end(), win) != m_windowlist.end())
        m_lastfocus = win;
    else
        m_lastfocus = 0;
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

    if (m_lastfocus == w) {
        m_lastfocus = 0;
    }

    if (w->isFocused() && still_alive)
        Fluxbox::instance()->unfocusWindow(w->winClient(), true, true);
	
    // we don't remove it from the layermanager, as it may be being moved
    Windows::iterator erase_it = remove(m_windowlist.begin(), 
                                        m_windowlist.end(), w);
    if (erase_it != m_windowlist.end())
        m_windowlist.erase(erase_it);

    updateClientmenu();

    if (m_lastfocus == w || m_windowlist.empty())
        m_lastfocus = 0;


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


void Workspace::removeAll() {
    Windows::iterator it = m_windowlist.begin();
    Windows::const_iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it)
        (*it)->iconify();
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

int Workspace::numberOfWindows() const {
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

    // go throu every group and search for matching win instancename
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
                    if ( !(*wit)->isGroupable() && (*wit)->winClient().fbwindow() == &win)
                        break; // try next name
#ifdef DEBUG
                    cerr<<__FILE__<<"("<<__FUNCTION__<<"): window ("<<*wit<<") attaching window ("<<&win<<")"<<endl;
#endif // DEBUG
                    (*wit)->attachClient(win.winClient());
                    return true; // grouping done
                
                }
                
            }

        }

    }

    return false;
}

bool Workspace::loadGroups(const std::string &filename) {
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
    menu().update();
}


void Workspace::setName(const std::string &name) {
    if (!name.empty()) {
        m_name = name;
    } else { //if name == 0 then set default name from nls
        _FB_USES_NLS;
        char tname[128];
        sprintf(tname, 
                _FBTEXT(Workspace, DefaultNameFormat, 
                        "Workspace %d", "Default workspace names, with a %d for the workspace number"),
                m_id + 1); //m_id starts at 0
        m_name = tname;
    }
	
    screen().updateWorkspaceNamesAtom();
	
    menu().setLabel(m_name.c_str());
    menu().update();
}

/**
 Calls restore on all windows
 on the workspace and then
 clears the m_windowlist
*/
void Workspace::shutdown() {
    // note: when the window dies it'll remove it self from the list
    while (!m_windowlist.empty()) {
        // restore with remap on all clients in that window
        m_windowlist.back()->restore(true); 
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

    menu().update();
}

void Workspace::placeWindow(FluxboxWindow &win) {

    bool placed = false;

    // restrictions
    int head = (signed) screen().getCurrHead();
    int head_left = (signed) screen().maxLeft(head);
    int head_right = (signed) screen().maxRight(head);
    int head_top = (signed) screen().maxTop(head);
    int head_bot = (signed) screen().maxBottom(head);

    int place_x = head_left, place_y = head_top, change_x = 1, change_y = 1;

    if (screen().getColPlacementDirection() == BScreen::BOTTOMTOP)
        change_y = -1;
    if (screen().getRowPlacementDirection() == BScreen::RIGHTLEFT)
        change_x = -1;

    int win_w = win.width() + win.fbWindow().borderWidth()*2,
        win_h = win.height() + win.fbWindow().borderWidth()*2;


    int test_x, test_y, curr_x, curr_y, curr_w, curr_h;

    switch (screen().getPlacementPolicy()) {
    case BScreen::UNDERMOUSEPLACEMENT: {
        int root_x, root_y, ignore_i;

        unsigned int ignore_ui;

        Window ignore_w;

        XQueryPointer(FbTk::App::instance()->display(),
                      screen().rootWindow().window(), &ignore_w, 
                      &ignore_w, &root_x, &root_y,
                      &ignore_i, &ignore_i, &ignore_ui);

        test_x = root_x - (win_w / 2);
        test_y = root_y - (win_h / 2);

        // keep the window inside the screen

        if (test_x < head_left)
            test_x = head_left;

        if (test_x > head_right)
            test_x = head_right;

        if (test_y < head_top)
            test_y = head_top;

        if (test_y > head_bot)
            test_y = head_bot;

        place_x = test_x;
        place_y = test_y;

        placed = true;

        break; 
    } // end case UNDERMOUSEPLACEMENT

    case BScreen::ROWSMARTPLACEMENT: {
        int next_x, next_y;
        bool top_bot = screen().getColPlacementDirection() == BScreen::TOPBOTTOM;
        bool left_right = screen().getRowPlacementDirection() == BScreen::LEFTRIGHT;

        if (top_bot)
            test_y = head_top;
        else
            test_y = head_bot - win_h;

        while (!placed && 
               (top_bot ? test_y + win_h <= head_bot
                        : test_y >= head_top)) {

            if (left_right)
                test_x = head_left;
            else
                test_x = head_right - win_w;

            // The trick here is that we set it to the furthest away one,
            // then the code brings it back down to the safest one that
            // we can go to (i.e. the next untested area)
            if (top_bot)
                next_y = head_bot;  // will be shrunk
            else
                next_y = head_top-1;

            while (!placed &&
                   (left_right ? test_x + win_w <= head_right
                    : test_x >= head_left)) {

                placed = true;

                next_x = test_x + change_x;

                Windows::iterator win_it = m_windowlist.begin();
                const Windows::iterator win_it_end = m_windowlist.end();

                for (; win_it != win_it_end && placed; ++win_it) {
                    FluxboxWindow &window = **win_it;

                    curr_x = window.x();
                    curr_y = window.y();
                    curr_w = window.width() + window.fbWindow().borderWidth()*2;
                    curr_h = window.height() + window.fbWindow().borderWidth()*2;

                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        // this window is in the way
                        placed = false;

                        // we find the next x that we can go to (a window will be in the way
                        // all the way to its far side)
                        if (left_right) {
                            if (curr_x + curr_w > next_x) 
                                next_x = curr_x + curr_w;
                        } else {
                            if (curr_x - win_w < next_x)
                                next_x = curr_x - win_w;
                        }

                        // but we can only go to the nearest y, since that is where the 
                        // next time current windows in the way will change
                        if (top_bot) {
                            if (curr_y + curr_h < next_y)
                                next_y = curr_y + curr_h;
                        } else {
                            if (curr_y - win_h > next_y)
                                next_y = curr_y - win_h;
                        }
                    }
                }


                if (placed) {
                    place_x = test_x;
                    place_y = test_y;

                    break;
                }

                test_x = next_x;
            } // end while

            test_y = next_y;
        } // end while

        break; 
    } // end case ROWSMARTPLACEMENT

    case BScreen::COLSMARTPLACEMENT: {
        int next_x, next_y;
        bool top_bot = screen().getColPlacementDirection() == BScreen::TOPBOTTOM;
        bool left_right = screen().getRowPlacementDirection() == BScreen::LEFTRIGHT;

        if (left_right)
            test_x = head_left;
        else
            test_x = head_right - win_w;

        while (!placed &&
               (left_right ? test_x + win_w <= head_right
                : test_x >= head_left)) {
                
            if (left_right)
                next_x = head_right; // it will get shrunk
            else 
                next_x = head_left-1;

            if (top_bot)
                test_y = head_top;
            else
                test_y = head_bot - win_h;

            while (!placed && 
                   (top_bot ? test_y + win_h <= head_bot
                            : test_y >= head_top)) {
                placed = True;

                next_y = test_y + change_y;

                Windows::iterator it = m_windowlist.begin();
                Windows::iterator it_end = m_windowlist.end();
                for (; it != it_end && placed; ++it) {
                    curr_x = (*it)->x();
                    curr_y = (*it)->y();
                    curr_w = (*it)->width()  + (*it)->fbWindow().borderWidth()*2;
                    curr_h = (*it)->height() + (*it)->fbWindow().borderWidth()*2;

                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        // this window is in the way
                        placed = False;

                        // we find the next y that we can go to (a window will be in the way
                        // all the way to its bottom)
                        if (top_bot) {
                            if (curr_y + curr_h > next_y)
                                next_y = curr_y + curr_h;
                        } else {
                            if (curr_y - win_h < next_y)
                                next_y = curr_y - win_h;
                        }

                        // but we can only go to the nearest x, since that is where the 
                        // next time current windows in the way will change
                        if (left_right) {
                            if (curr_x + curr_w < next_x) 
                                next_x = curr_x + curr_w;
                        } else {
                            if (curr_x - win_w > next_x)
                                next_x = curr_x - win_w;
                        }
                    }
                }

                if (placed) {
                    place_x = test_x;
                    place_y = test_y;
                }

                test_y = next_y;
            } // end while

            test_x = next_x;
        } // end while

        break; 
    } // end COLSMARTPLACEMENT

    }

    // cascade placement or smart placement failed
    if (! placed) {

        if ((m_cascade_x[head] > ((head_left + head_right) / 2)) ||
            (m_cascade_y[head] > ((head_top + head_bot) / 2))) {
            m_cascade_x[head] = head_left + 32;
            m_cascade_y[head] = head_top + 32;
        }

        place_x = m_cascade_x[head];
        place_y = m_cascade_y[head];

        // just one borderwidth, so they can share a borderwidth (looks better)
        int titlebar_height = win.titlebarHeight() + win.fbWindow().borderWidth();
        if (titlebar_height < 4) // make sure it is not insignificant
            titlebar_height = 32;
        m_cascade_x[head] += titlebar_height;
        m_cascade_y[head] += titlebar_height;
    }

    if (place_x + win_w > head_right)
        place_x = (head_right - win_w) / 2;
    if (place_y + win_h > head_bot)
        place_y = (head_bot - win_h) / 2;


    win.moveResize(place_x, place_y, win.width(), win.height());
}
