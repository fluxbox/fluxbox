// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Workspace.cc,v 1.38 2002/12/04 14:16:42 rathnor Exp $

#include "Workspace.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "StringUtil.hh"

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

};

Workspace::GroupList Workspace::m_groups;

Workspace::Workspace(BScreen *scrn, unsigned int i):
    screen(scrn),
    lastfocus(0),
    m_clientmenu(*this),
    m_name(""),
    m_id(i),
    cascade_x(32), cascade_y(32) {

    setName(screen->getNameOfWorkspace(m_id));

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

int Workspace::addWindow(FluxboxWindow *w, bool place) {
    if (w == 0)
        return -1;

    if (place)
        placeWindow(w);

    w->setWorkspace(m_id);
    w->setWindowNumber(m_windowlist.size());

    stackingList.push_front(w);

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
    //add to list
    m_clientmenu.insert(w->getTitle().c_str());
    m_windowlist.push_back(w);
	
    //update menugraphics
    m_clientmenu.update();
	
    if (!w->isStuck()) 
        screen->updateNetizenWindowAdd(w->getClientWindow(), m_id);

    raiseWindow(w);

    return w->getWindowNumber();
}


int Workspace::removeWindow(FluxboxWindow *w) {
    if (w == 0)
        return -1;

    if (lastfocus == w) {
        lastfocus = 0;
    }

    stackingList.remove(w);

    if (w->isFocused()) {
        if (screen->isSloppyFocus()) {
            Fluxbox::instance()->setFocusedWindow(0); // set focused window to none
        } else if (w->isTransient() && w->getTransientFor() &&
                   w->getTransientFor()->isVisible()) {
            /* TODO: check transient
               if (w->getTransientFor() == w) { // FATAL ERROR, this should not happend
               cerr<<"w->getTransientFor() == w: aborting!"<<endl;
               abort();
               }*/
            w->getTransientFor()->setInputFocus();
        } else {
            FluxboxWindow *top = 0;
            if (stackingList.size() != 0)
                top = stackingList.front();

            if (top == 0|| !top->setInputFocus()) {
                Fluxbox::instance()->setFocusedWindow(0); // set focused window to none
                XSetInputFocus(Fluxbox::instance()->getXDisplay(),
                               screen->getToolbar()->getWindowID(),
                               RevertToParent, CurrentTime);						
            }
        }
    }
	
	

    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        if (*it == w) {
            m_windowlist.erase(it);
            break;
        }
    }

    m_clientmenu.remove(w->getWindowNumber());
    m_clientmenu.update();
	
    if (!w->isStuck())
        screen->updateNetizenWindowDel(w->getClientWindow());

    {
        Windows::iterator it = m_windowlist.begin();
        Windows::const_iterator it_end = m_windowlist.end();
        for (int i = 0; it != it_end; ++it, ++i) {
            (*it)->setWindowNumber(i);
        }
    }
	
    if (lastfocus == w || m_windowlist.empty())
        lastfocus = 0;
	
    return m_windowlist.size();
}


void Workspace::showAll(void) {
    WindowStack::iterator it = stackingList.begin();
    WindowStack::iterator it_end = stackingList.end();
    for (; it != it_end; ++it) {
        (*it)->deiconify(false, false);
    }
}


void Workspace::hideAll(void) {
    WindowStack::reverse_iterator it = stackingList.rbegin();
    WindowStack::reverse_iterator it_end = stackingList.rend();
    for (; it != it_end; ++it) {
        if (! (*it)->isStuck())
            (*it)->withdraw();
    }
}


void Workspace::removeAll(void) {
    Windows::iterator it = m_windowlist.begin();
    Windows::const_iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it) {
        (*it)->iconify();
    }
}


void Workspace::raiseWindow(FluxboxWindow *w) {
    FluxboxWindow *win = w;

    while (win->getTransientFor()) {
        win = win->getTransientFor();
        assert(win != win->getTransientFor());
    }
	
    int i = 1 + countTransients(*win);

    Stack nstack(i);
    Stack::iterator stackit = nstack.begin();
	
    *(stackit++) = win->getFrameWindow();
    screen->updateNetizenWindowRaise(win->getClientWindow());
    if (! win->isIconic()) {
        Workspace *wkspc = screen->getWorkspace(win->getWorkspaceNumber());
        wkspc->stackingList.remove(win);
        wkspc->stackingList.push_front(win);
    }	
	
    raiseAndFillStack(stackit, *win);

    screen->raiseWindows(nstack);
	
}

void Workspace::lowerWindow(FluxboxWindow *w) {
    FluxboxWindow *win = (FluxboxWindow *) 0, *bottom = w;

    while (bottom->getTransientFor()) {
        bottom = bottom->getTransientFor();
        assert(bottom != bottom->getTransientFor());
    }

    win = bottom;
    int i = 1 + countTransients(*win);


    Stack st(i);
    Stack::iterator stackit = st.begin();
    lowerAndFillStack(stackit, *win);
    (*stackit) = win->getFrameWindow();

    screen->updateNetizenWindowLower(win->getClientWindow());
    if (! win->isIconic()) {
        Workspace *wkspc = screen->getWorkspace(win->getWorkspaceNumber());
        wkspc->stackingList.remove(win);
        wkspc->stackingList.push_back(win);
    }

    XLowerWindow(BaseDisplay::getXDisplay(), st.front());
    XRestackWindows(BaseDisplay::getXDisplay(), &st[0], st.size());

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
                    //toggle tab on
                    if ((*wit)->getTab() == 0)
                        (*wit)->setTab(true);
                    if (win.getTab() == 0)
                        win.setTab(true);
                    // did we succefully set the tab?
                    if ((*wit)->getTab() == 0)
                        break; // try another window
                    (*wit)->getTab()->insert(win.getTab());

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
    screen->getToolbar()->redrawWindowLabel(True);
}


bool Workspace::isCurrent() const{
    return (m_id == screen->getCurrentWorkspaceID());
}


bool Workspace::isLastWindow(FluxboxWindow *w) const{
    return (w == m_windowlist.back());
}

void Workspace::setCurrent() {
    screen->changeWorkspaceID(m_id);
}


void Workspace::setName(const std::string &name) {
    if (name.size() != 0) {
        m_name = name;
    } else { //if name == 0 then set default name from nls
        char tname[128];
        sprintf(tname, I18n::instance()->
                getMessage(
                           FBNLS::WorkspaceSet, FBNLS::WorkspaceDefaultNameFormat,
                           "Workspace %d"), m_id + 1); //m_id starts at 0
        m_name = tname;
    }
	
    screen->updateWorkspaceNamesAtom();
	
    m_clientmenu.setLabel(m_name.c_str());
    m_clientmenu.update();
}

//------------ shutdown ---------
// Calls restore on all windows
// on the workspace and then
// clears the m_windowlist
//-------------------------------
void Workspace::shutdown() {
    // note: when the window dies it'll remove it self from the list
    while (!m_windowlist.empty()) {
        m_windowlist.back()->restore(true); // restore with remap
        delete m_windowlist.back(); //delete window (the window removes it self from m_windowlist)
    }
}


void Workspace::placeWindow(FluxboxWindow *win) {
    Bool placed = False;
    int borderWidth4x = screen->getBorderWidth2x() * 2,
#ifdef SLIT
        slit_x = screen->getSlit()->x() - screen->getBorderWidth(),
        slit_y = screen->getSlit()->y() - screen->getBorderWidth(),
        slit_w = screen->getSlit()->width() + borderWidth4x,
        slit_h = screen->getSlit()->height() + borderWidth4x,
#endif // SLIT
        toolbar_x = screen->getToolbar()->x() - screen->getBorderWidth(),
        toolbar_y = screen->getToolbar()->y() - screen->getBorderWidth(),
        toolbar_w = screen->getToolbar()->width() + borderWidth4x,
        toolbar_h = screen->getToolbar()->height() + borderWidth4x,
        place_x = 0, place_y = 0, change_x = 1, change_y = 1;

    if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
        change_y = -1;
    if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
        change_x = -1;

#ifdef XINERAMA
    int head = 0,
        head_x = 0,
        head_y = 0;
    int head_w, head_h;
    if (screen->hasXinerama()) {
        head = screen->getCurrHead();
        head_x = screen->getHeadX(head);
        head_y = screen->getHeadY(head);
        head_w = screen->getHeadWidth(head);
        head_h = screen->getHeadHeight(head);

    } else { // no xinerama
        head_w = screen->getWidth();
        head_h = screen->getHeight();
    }

#endif // XINERAMA

    int win_w = win->getWidth() + screen->getBorderWidth2x(),
        win_h = win->getHeight() + screen->getBorderWidth2x();

    if (win->hasTab()) {
        if ((! win->isShaded()) &&
            screen->getTabPlacement() == Tab::PLEFT ||
            screen->getTabPlacement() == Tab::PRIGHT)
            win_w += (screen->isTabRotateVertical())
                ? screen->getTabHeight()
                : screen->getTabWidth();
        else // tab placement top or bottom or win is shaded
            win_h += screen->getTabHeight();
    }

    register int test_x, test_y, curr_x, curr_y, curr_w, curr_h;

    switch (screen->getPlacementPolicy()) {
    case BScreen::ROWSMARTPLACEMENT: {
#ifdef XINERAMA
        test_y = head_y;
#else // !XINERAMA
        test_y = 0;
#endif // XINERAMA
        if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
#ifdef XINERAMA
            test_y = (head_y + head_h) - win_h - test_y;
#else // !XINERAMA
        test_y = screen->getHeight() - win_h - test_y;
#endif // XINERAMA

        while (((screen->getColPlacementDirection() == BScreen::BOTTOMTOP) ?
#ifdef XINERAMA
                test_y >= head_y : test_y + win_h <= (head_y + head_h)
#else // !XINERAMA
                test_y > 0 : test_y + win_h < (signed) screen->getHeight()
#endif // XINERAMA
                ) && ! placed) {

#ifdef XINERAMA
            test_x = head_x;
#else // !XINERAMA
            test_x = 0;
#endif // XINERAMA
            if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
#ifdef XINERAMA
                test_x = (head_x + head_w) - win_w - test_x;
#else // !XINERAMA
            test_x = screen->getWidth() - win_w - test_x;
#endif // XINERAMA

            while (((screen->getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
#ifdef XINERAMA
                    test_x >= head_x : test_x + win_w <= (head_x + head_w)
#else // !XINERAMA
                    test_x > 0 : test_x + win_w < (signed) screen->getWidth()
#endif // XINERAMA
                    ) && ! placed) {

                placed = True;

                Windows::iterator it = m_windowlist.begin();
                Windows::iterator it_end = m_windowlist.end();

                for (; it != it_end && placed; ++it) {
                    curr_x = (*it)->getXFrame();
                    curr_y = (*it)->getYFrame();
                    curr_w = (*it)->getWidth() + screen->getBorderWidth2x();
                    curr_h =
                        (((*it)->isShaded())
                         ? (*it)->getTitleHeight()
                         : (*it)->getHeight()) +
                        screen->getBorderWidth2x();	

                    if ((*it)->hasTab()) {
                        if (! (*it)->isShaded()) { // not shaded window
                            switch(screen->getTabPlacement()) {
                            case Tab::PTOP:
                                curr_y -= screen->getTabHeight();
                            case Tab::PBOTTOM:
                                curr_h += screen->getTabHeight();
                                break;
                            case Tab::PLEFT:
                                curr_x -= (screen->isTabRotateVertical())
                                    ? screen->getTabHeight()
                                    : screen->getTabWidth();
                            case Tab::PRIGHT:
                                curr_w += (screen->isTabRotateVertical())
                                    ? screen->getTabHeight()
                                    : screen->getTabWidth();
                                break;
                            case Tab::PNONE:
                                break;
                            }
                        } else { // shaded window
                            if (screen->getTabPlacement() == Tab::PTOP)
                                curr_y -= screen->getTabHeight();
                            curr_h += screen->getTabHeight();
                        }
                    } // tab cheking done

                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        placed = False;
                    }
                }

                if ((toolbar_x < test_x + win_w &&
                     toolbar_x + toolbar_w > test_x &&
                     toolbar_y < test_y + win_h &&
                     toolbar_y + toolbar_h > test_y)
#ifdef SLIT
                    ||
                    (slit_x < test_x + win_w &&
                     slit_x + slit_w > test_x &&
                     slit_y < test_y + win_h &&
                     slit_y + slit_h > test_y)
#endif // SLIT
                    )
                    placed = False;

                if (placed) {
                    place_x = test_x;
                    place_y = test_y;

                    break;
                }

                test_x += change_x;
            }

            test_y += change_y;
        }

        break; }

    case BScreen::COLSMARTPLACEMENT: {
#ifdef XINERAMA
        test_x = head_x;
#else // !XINERAMA
        test_x = 0;
#endif // XINERAMA
        if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
#ifdef XINERAMA
            test_x = (head_x + head_w) - win_w - test_x;
#else // !XINERAMA
        test_x = screen->getWidth() - win_w - test_x;
#endif // XINERAMA

        while (((screen->getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
#ifdef XINERAMA
                test_x >= 0 : test_x + win_w <= (head_x + head_w)
#else // !XINERAMA
                test_x > 0 : test_x + win_w < (signed) screen->getWidth()
#endif // XINERAMA
                ) && ! placed) {

#ifdef XINERAMA
            test_y = head_y;
#else // !XINERAMA
            test_y = 0;
#endif // XINERAMA
            if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
#ifdef XINERAMA
                test_y = (head_y + head_h) - win_h - test_y;
#else // !XINERAMA
            test_y = screen->getHeight() - win_h - test_y;
#endif // XINERAMA

            while (((screen->getColPlacementDirection() == BScreen::BOTTOMTOP) ?
#ifdef XINERAMA
                    test_y >= head_y : test_y + win_h <= (head_y + head_h)
#else // !XINERAMA
                    test_y > 0 : test_y + win_h < (signed) screen->getHeight()
#endif // XINERAMA
                    ) && ! placed) {
                placed = True;

                Windows::iterator it = m_windowlist.begin();
                Windows::iterator it_end = m_windowlist.end();
                for (; it != it_end && placed; ++it) {
                    curr_x = (*it)->getXFrame();
                    curr_y = (*it)->getYFrame();
                    curr_w = (*it)->getWidth() + screen->getBorderWidth2x();
                    curr_h =
                        (((*it)->isShaded())
                         ? (*it)->getTitleHeight()
                         : (*it)->getHeight()) +
                        screen->getBorderWidth2x();;

                    if ((*it)->hasTab()) {
                        if (! (*it)->isShaded()) { // not shaded window
                            switch(screen->getTabPlacement()) {
                            case Tab::PTOP:
                                curr_y -= screen->getTabHeight();
                            case Tab::PBOTTOM:
                                curr_h += screen->getTabHeight();
                                break;
                            case Tab::PLEFT:
                                curr_x -= (screen->isTabRotateVertical())
                                    ? screen->getTabHeight()
                                    : screen->getTabWidth();
                            case Tab::PRIGHT:
                                curr_w += (screen->isTabRotateVertical())
                                    ? screen->getTabHeight()
                                    : screen->getTabWidth();
                                break;
                            default:
#ifdef DEBUG
                                cerr << __FILE__ << ":" <<__LINE__ << ": " <<
                                    "Unsupported Placement" << endl;
#endif // DEBUG
                                break;
                            }
                        } else { // shaded window
                            if (screen->getTabPlacement() == Tab::PTOP)
                                curr_y -= screen->getTabHeight();
                            curr_h += screen->getTabHeight();
                        }
                    } // tab cheking done

                    if (curr_x < test_x + win_w &&
                        curr_x + curr_w > test_x &&
                        curr_y < test_y + win_h &&
                        curr_y + curr_h > test_y) {
                        placed = False;
                    }
                }

                if ((toolbar_x < test_x + win_w &&
                     toolbar_x + toolbar_w > test_x &&
                     toolbar_y < test_y + win_h &&
                     toolbar_y + toolbar_h > test_y)
#ifdef SLIT
                    ||
                    (slit_x < test_x + win_w &&
                     slit_x + slit_w > test_x &&
                     slit_y < test_y + win_h &&
                     slit_y + slit_h > test_y)
#endif // SLIT
                    )
                    placed = False;

                if (placed) {
                    place_x = test_x;
                    place_y = test_y;
                }

                test_y += change_y;
            }

            test_x += change_x;
        }

        break; }
    }

    // cascade placement or smart placement failed
    if (! placed) {
#ifdef XINERAMA
        if ((cascade_x > (head_w / 2)) ||
            (cascade_y > (head_h / 2)))
#else // !XINERAMA
            if (((unsigned) cascade_x > (screen->getWidth() / 2)) ||
                ((unsigned) cascade_y > (screen->getHeight() / 2)))
#endif // XINERAMA

                cascade_x = cascade_y = 32;
#ifdef XINERAMA
        place_x = head_x + cascade_x;
        place_y = head_y + cascade_y;
#else // !XINERAMA
        place_x = cascade_x;
        place_y = cascade_y;
#endif // XINERAMA
        cascade_x += win->getTitleHeight();
        cascade_y += win->getTitleHeight();
    }

#ifdef XINERAMA
    if (place_x + win_w > (head_x + head_w))
        place_x = head_x + ((head_w - win_w) / 2);
    if (place_y + win_h > (head_y + head_h))
        place_y = head_y + ((head_h - win_h) / 2);
#else // !XINERAMA
    if (place_x + win_w > (signed) screen->getWidth())
        place_x = (((signed) screen->getWidth()) - win_w) / 2;
    if (place_y + win_h > (signed) screen->getHeight())
        place_y = (((signed) screen->getHeight()) - win_h) / 2;
#endif // XINERAMA

    // fix window placement, think of tabs
    if (win->hasTab()) {
        if (screen->getTabPlacement() == Tab::PTOP) 
            place_y += screen->getTabHeight();
        else if (screen->getTabPlacement() == Tab::PLEFT)
            place_x += (screen->isTabRotateVertical())
                ? screen->getTabHeight()
                : screen->getTabWidth();
    }

    win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}


void Workspace::raiseAndFillStack(Stack::iterator &stackit, const FluxboxWindow &w) {
    if (w.getTransients().empty())
        return;

    std::list<FluxboxWindow *>::const_iterator it = w.getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = w.getTransients().end();
    for (; it != it_end; ++it) {		
        *stackit++ = (*it)->getFrameWindow();
		
        screen->updateNetizenWindowRaise((*it)->getClientWindow());

        if (! (*it)->isIconic()) {
            Workspace *wkspc = screen->getWorkspace((*it)->getWorkspaceNumber());
            wkspc->stackingList.remove((*it));
            wkspc->stackingList.push_front((*it));
        }

		
    }
	
    it = w.getTransients().begin();
    for (; it != it_end; ++it)
        raiseAndFillStack(stackit, *(*it));
	

}

void Workspace::lowerAndFillStack(Stack::iterator &stackit, const FluxboxWindow &win) {
    if (win.getTransients().empty()) // nothing to lower and stack
        return;

    std::list<FluxboxWindow *>::const_reverse_iterator it = win.getTransients().rbegin();
    std::list<FluxboxWindow *>::const_reverse_iterator it_end = win.getTransients().rend();
    for (; it != it_end; ++it)
        lowerAndFillStack(stackit, *(*it));
	
    it = win.getTransients().rbegin();
	
    for (; it != it_end; ++it) {
        (*stackit) = (*it)->getFrameWindow();
        ++stackit;
        screen->updateNetizenWindowLower((*it)->getClientWindow());
        if (! (*it)->isIconic()) {
            Workspace *wkspc = screen->getWorkspace((*it)->getWorkspaceNumber());
            wkspc->stackingList.remove((*it));
            wkspc->stackingList.push_back((*it));
        }
    }

}
