// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen[at]users.sourceforge.net)
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

// $Id: Workspace.cc,v 1.51 2003/04/14 14:59:15 fluxgen Exp $

#include "Workspace.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "StringUtil.hh"
#include "SimpleCommand.hh"
#include "WinClient.hh"

// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
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

int countTransients(const FluxboxWindow &win) {
    if (win.getTransients().size() == 0)
        return 0;
    // now go throu the entire tree and count transients
    size_t ret = win.getTransients().size();	
    std::list<FluxboxWindow *>::const_iterator it = win.getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win.getTransients().end();
    for (; it != it_end; ++it)
        ret += countTransients(*(*it));

    return ret;
}

class ClientMenuItem:public FbTk::MenuItem {
public:
    ClientMenuItem(WinClient &client, Workspace &space):
        FbTk::MenuItem(client.title().c_str()),
        m_client(client), m_space(space) {
        
    }
    void click(int button, int time) {
        if (m_client.fbwindow() == 0)
            return;
        FluxboxWindow &win = *m_client.fbwindow();
        BScreen &scr = win.getScreen();
        // determine workspace change
        for (size_t i=0; i<scr.getCount(); i++) {
            if (scr.getWorkspace(i) == &m_space) {
                scr.changeWorkspaceID(i); 
                break;
            }
        }

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
    Workspace &m_space;
};

};

Workspace::GroupList Workspace::m_groups;

Workspace::Workspace(BScreen &scrn, FbTk::MultLayers &layermanager, unsigned int i):
    screen(scrn),
    lastfocus(0),
    m_clientmenu(*scrn.menuTheme(), scrn.getScreenNumber(), *scrn.getImageControl()),
    m_layermanager(layermanager),
    m_name(""),
    m_id(i),
    cascade_x(32), cascade_y(32) {

    m_clientmenu.setInternalMenu();
    setName(screen.getNameOfWorkspace(m_id));

}


Workspace::~Workspace() {

}

void Workspace::setLastFocusedWindow(FluxboxWindow *win) {
    // make sure we have this window in the list
    if (std::find(m_windowlist.begin(), m_windowlist.end(), win) != m_windowlist.end())
        lastfocus = win;
    else
        lastfocus = 0;
}

int Workspace::addWindow(FluxboxWindow &w, bool place) {
    // we don't need to add a window that already exist in our list
    if (find(m_windowlist.begin(), m_windowlist.end(), &w) !=
        m_windowlist.end())
        return -1;

    w.setWorkspace(m_id);
    w.setWindowNumber(m_windowlist.size());

    if (place)
        placeWindow(w);


    //insert window after the currently focused window	
    //FluxboxWindow *focused = Fluxbox::instance()->getFocusedWindow();	

    //if there isn't any window that's focused, just add it to the end of the list
    /*
      if (focused == 0) {
      m_windowlist.push_back(w);
      //Add client to clientmenu
      m_clientmenu.insert(w->getTitle().c_str());
      } else {
      Windows::iterator it = m_windowlist.begin();
      size_t client_insertpoint=0;
      for (; it != m_windowlist.end(); ++it, ++client_insertpoint) {
      if (*it == focused) {
      ++it;				
      break;
      }
      }

      m_windowlist.insert(it, w);
      //Add client to clientmenu
      m_clientmenu.insert(w->getTitle().c_str(), client_insertpoint);
		

      }
    */

    // find focused window position
    /*    Windows::iterator insert_point_it = m_windowlist.begin();
          for (;insert_point_it != m_windowlist.end(); ++insert_point_it) {
          if ((*insert_point_it)->isFocused()) {
          break;
          }
          }
          // if we found focused window, insert our window directly after it
          if (insert_point_it != m_windowlist.end())
          m_windowlist.insert(insert_point_it, w);
          else // we didn't find it, so we just add it to stack
    */
    m_windowlist.push_back(&w);
    updateClientmenu();

	
    if (!w.isStuck()) 
        screen.updateNetizenWindowAdd(w.getClientWindow(), m_id);

    return w.getWindowNumber();
}


int Workspace::removeWindow(FluxboxWindow *w) {

    if (w == 0)
        return -1;

    if (lastfocus == w) {
        lastfocus = 0;
    }

    if (w->isFocused()) {
        if (screen.isSloppyFocus()) {
            Fluxbox::instance()->setFocusedWindow(0); // set focused window to none
        } else if (w->isTransient() && w->getTransientFor() &&
                   w->getTransientFor()->isVisible()) {
            w->getTransientFor()->setInputFocus();
        } else {
            FluxboxWindow *top = 0;

            // this bit is pretty dodgy at present
            // it gets the next item down, then scans through our windowlist to see if it is 
            // in this workspace. If not, goes down more
            /* //!! TODO! FbTk::XLayerItem *item = 0, *lastitem = w->getLayerItem();
               do {
               item = m_layermanager.getItemBelow(*lastitem);
               Windows::iterator it = m_windowlist.begin();
               Windows::iterator it_end = m_windowlist.end();
               for (; it != it_end; ++it) {
               if ((*it)->getLayerItem() == item) {
               // found one!
               top = *it;
               }
               }

               lastitem = item;
                
               } while (item && !top);
            
               if (!top) {
               // look upwards
               lastitem = w->getLayerItem();
               do {
               item = m_layermanager.getItemAbove(*lastitem);
               Windows::iterator it = m_windowlist.begin();
               Windows::iterator it_end = m_windowlist.end();
               for (; it != it_end; ++it) {
               if ((*it)->getLayerItem() == item) {
               // found one!
               top = *it;
               }
               }
               lastitem = item;
               } while (item && !top);

               }
            */
            if (top == 0|| !top->setInputFocus()) {
                Fluxbox::instance()->setFocusedWindow(0); // set focused window to none
            }
        }
    }
	
    // we don't remove it from the layermanager, as it may be being moved
    Windows::iterator erase_it = remove(m_windowlist.begin(), 
                                        m_windowlist.end(), w);
    if (erase_it != m_windowlist.end())
        m_windowlist.erase(erase_it);

    updateClientmenu();

    if (lastfocus == w || m_windowlist.empty())
        lastfocus = 0;

    return m_windowlist.size();
}

void Workspace::removeWindow(WinClient &client) {
    if (client.m_win == 0)
        return;

    if (client.m_win->numClients() == 0) {
        Windows::iterator erase_it = remove(m_windowlist.begin(), 
                                            m_windowlist.end(), client.m_win);
        if (erase_it != m_windowlist.end())
            m_windowlist.erase(erase_it);
    }

    updateClientmenu();
}

void Workspace::showAll() {
    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        (*it)->deiconify(false, false);
    }
}


void Workspace::hideAll() {
    Windows::reverse_iterator it = m_windowlist.rbegin();
    Windows::reverse_iterator it_end = m_windowlist.rend();
    for (; it != it_end; ++it) {
        if (! (*it)->isStuck())
            (*it)->withdraw();
    }
}


void Workspace::removeAll() {
    Windows::iterator it = m_windowlist.begin();
    Windows::const_iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        (*it)->iconify();
    }
}


void Workspace::reconfigure() {
    m_clientmenu.reconfigure();

    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        if ((*it)->validateClient())
            (*it)->reconfigure();
    }
}


const FluxboxWindow *Workspace::getWindow(unsigned int index) const {
    if (index < m_windowlist.size())
        return m_windowlist[index];
    return 0;
}

FluxboxWindow *Workspace::getWindow(unsigned int index) {
    if (index < m_windowlist.size())
        return m_windowlist[index];
    return 0;
}


int Workspace::getCount() const {
    return m_windowlist.size();
}

namespace {
// helper class for checkGrouping
class FindInGroup {
public:
    FindInGroup(const FluxboxWindow &w):m_w(w) { }
    bool operator ()(const string &name) {
        return (name == m_w.instanceName());
    }
private:
    const FluxboxWindow &m_w;
};

};

//Note: this function doesn't check if the window is groupable
void Workspace::checkGrouping(FluxboxWindow &win) {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): Checking grouping. ("<<win.instanceName()<<"/"<<
        win.className()<<")"<<endl;	
#endif // DEBUG
    if (!win.isGroupable()) { // make sure this window can hold a tab
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): window can't use a tab"<<endl;
#endif // DEBUG
        return;
    }

    // go throu every group and search for matching win instancename
    GroupList::iterator g(m_groups.begin());
    GroupList::iterator g_end(m_groups.end());
    for (; g != g_end; ++g) {
        Group::iterator name((*g).begin());
        Group::iterator name_end((*g).end());
        for (; name != name_end; ++name) {

            if ((*name) != win.instanceName())
                continue;

            // find a window with the specific name
            Windows::iterator wit(m_windowlist.begin());
            Windows::iterator wit_end(m_windowlist.end());
            for (; wit != wit_end; ++wit) {
#ifdef DEBUG
                cerr<<__FILE__<<" check group with : "<<(*wit)->instanceName()<<endl;
#endif // DEBUG
                if (find_if((*g).begin(), (*g).end(), FindInGroup(*(*wit))) != (*g).end()) {
                    // make sure the window is groupable
                    if ( !(*wit)->isGroupable())
                        break; // try next name
                    cerr<<__FILE__<<"("<<__FUNCTION__<<") TODO attach client here!"<<endl;
                    return; // grouping done
                }
            }

        }

    }
}

bool Workspace::loadGroups(const std::string &filename) {
    ifstream infile(filename.c_str());
    if (!infile)
        return false;
    m_groups.clear(); // erase old groups

    // load new groups
    while (!infile.eof()) {
        string line;
        vector<string> names;
        getline(infile, line);
        StringUtil::stringtok(names, line);
        m_groups.push_back(names);
    }
	
    return true;
}

void Workspace::update() {
    m_clientmenu.update();
}


bool Workspace::isCurrent() const{
    return (m_id == screen.getCurrentWorkspaceID());
}


bool Workspace::isLastWindow(FluxboxWindow *w) const{
    return (w == m_windowlist.back());
}

void Workspace::setCurrent() {
    screen.changeWorkspaceID(m_id);
}


void Workspace::setName(const std::string &name) {
    if (name.size() != 0) {
        m_name = name;
    } else { //if name == 0 then set default name from nls
        char tname[128];
        sprintf(tname, I18n::instance()->
                getMessage(
                           FBNLS::WorkspaceSet, 
                           FBNLS::WorkspaceDefaultNameFormat,
                           "Workspace %d"), m_id + 1); //m_id starts at 0
        m_name = tname;
    }
	
    screen.updateWorkspaceNamesAtom();
	
    m_clientmenu.setLabel(m_name.c_str());
    m_clientmenu.update();
}

/**
 Calls restore on all windows
 on the workspace and then
 clears the m_windowlist
*/
void Workspace::shutdown() {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): windowlist:"<<endl;
    copy(m_windowlist.begin(), m_windowlist.end(),
         ostream_iterator<FluxboxWindow *>(cerr, " \n"));
    cerr<<endl;
#endif // DEBUG
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
    m_clientmenu.removeAll();
    // for each fluxboxwindow add every client in them to our clientlist    
    Windows::iterator win_it = m_windowlist.begin();
    Windows::iterator win_it_end = m_windowlist.end();
    for (; win_it != win_it_end; ++win_it) {
        // add every client in this fluxboxwindow to menu
        FluxboxWindow::ClientList::iterator client_it = 
            (*win_it)->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end = 
            (*win_it)->clientList().end();
        for (; client_it != client_it_end; ++client_it) {
            /*  FbTk::RefCount<FbTk::Command> 
                raise_and_focus(new RaiseFocusAndSetWorkspace(*this, 
                                                              *(*client_it)));
                                                              */
            m_clientmenu.insert(new ClientMenuItem(*(*client_it), *this));
        }
    }
    
    m_clientmenu.update();
}

void Workspace::placeWindow(FluxboxWindow &win) {

    bool placed = false;

    int place_x = 0, place_y = 0, change_x = 1, change_y = 1;

    if (screen.getColPlacementDirection() == BScreen::BOTTOMTOP)
        change_y = -1;
    if (screen.getRowPlacementDirection() == BScreen::RIGHTLEFT)
        change_x = -1;


    int win_w = win.getWidth() + screen.getBorderWidth2x(),
        win_h = win.getHeight() + screen.getBorderWidth2x();

    int test_x, test_y, curr_x, curr_y, curr_w, curr_h;

    switch (screen.getPlacementPolicy()) {
    case BScreen::ROWSMARTPLACEMENT: {

        test_y = 0;

        if (screen.getColPlacementDirection() == BScreen::BOTTOMTOP)
            test_y = screen.getHeight() - win_h - test_y;


        while (((screen.getColPlacementDirection() == BScreen::BOTTOMTOP) ?
                test_y > 0 : test_y + win_h < (signed) screen.getHeight()) && 
               ! placed) {

            test_x = 0;

            if (screen.getRowPlacementDirection() == BScreen::RIGHTLEFT)
                test_x = screen.getWidth() - win_w - test_x;


            while (((screen.getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
                    test_x > 0 : test_x + win_w < (signed) screen.getWidth()) && ! placed) {

                placed = true;

                Windows::iterator win_it = m_windowlist.begin();
                const Windows::iterator win_it_end = m_windowlist.end();

                for (; win_it != win_it_end && placed; ++win_it) {
                    FluxboxWindow &window = **win_it;

                    curr_x = window.getXFrame();
                    curr_y = window.getYFrame();
                    curr_w = window.getWidth() + screen.getBorderWidth2x();
                    curr_h = window.isShaded() ? window.getTitleHeight() :
                        window.getHeight() + screen.getBorderWidth2x();

                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        placed = false;
                    }
                }


                if (placed) {
                    place_x = test_x;
                    place_y = test_y;

                    break;
                }

                test_x += change_x;
            } // end while

            test_y += change_y;
        } // end while

        break; 
    } // end case ROWSMARTPLACEMENT

    case BScreen::COLSMARTPLACEMENT: {
        test_x = 0;

        if (screen.getRowPlacementDirection() == BScreen::RIGHTLEFT)

            test_x = screen.getWidth() - win_w - test_x;


        while (((screen.getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
                test_x > 0 : test_x + win_w < (signed) screen.getWidth()) && 
               !placed) {

            test_y = 0;
            if (screen.getColPlacementDirection() == BScreen::BOTTOMTOP)
                test_y = screen.getHeight() - win_h - test_y;

            while (((screen.getColPlacementDirection() == BScreen::BOTTOMTOP) ?
                    test_y > 0 : test_y + win_h < (signed) screen.getHeight()) && 
                   !placed) {
                placed = True;

                Windows::iterator it = m_windowlist.begin();
                Windows::iterator it_end = m_windowlist.end();
                for (; it != it_end && placed; ++it) {
                    curr_x = (*it)->getXFrame();
                    curr_y = (*it)->getYFrame();
                    curr_w = (*it)->getWidth() + screen.getBorderWidth2x();
                    curr_h =
                        (((*it)->isShaded())
                         ? (*it)->getTitleHeight()
                         : (*it)->getHeight()) +
                        screen.getBorderWidth2x();;


                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        placed = False;
                    }
                }


                if (placed) {
                    place_x = test_x;
                    place_y = test_y;
                }

                test_y += change_y;
            } // end while

            test_x += change_x;
        } // end while

        break; 
    } // end COLSMARTPLACEMENT

    }

    // cascade placement or smart placement failed
    if (! placed) {

        if (((unsigned) cascade_x > (screen.getWidth() / 2)) ||
            ((unsigned) cascade_y > (screen.getHeight() / 2)))
            cascade_x = cascade_y = 32;

        place_x = cascade_x;
        place_y = cascade_y;

        cascade_x += win.getTitleHeight();
        cascade_y += win.getTitleHeight();
    }

    if (place_x + win_w > (signed) screen.getWidth())
        place_x = (((signed) screen.getWidth()) - win_w) / 2;
    if (place_y + win_h > (signed) screen.getHeight())
        place_y = (((signed) screen.getHeight()) - win_h) / 2;


    win.moveResize(place_x, place_y, win.getWidth(), win.getHeight());
}
