// Workspace.hh for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Workspace.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef	 WORKSPACE_HH
#define	 WORKSPACE_HH

#include "NotCopyable.hh"

#include "Menu.hh"

#include <X11/Xlib.h>

#include <string>
#include <vector>
#include <list>

class BScreen;
class FluxboxWindow;

/**
	Handles a single workspace
*/
class Workspace:private FbTk::NotCopyable {
public:
    typedef std::vector<FluxboxWindow *> Windows;
    typedef std::vector<Window> Stack;

    explicit Workspace(BScreen *screen, unsigned int workspaceid = 0);
    ~Workspace();
	
    void setLastFocusedWindow(FluxboxWindow *w);
    /**
       Set workspace name
    */
    void setName(const std::string &name);
    void showAll();
    void hideAll();
    void removeAll();
    void raiseWindow(FluxboxWindow *win);
    void lowerWindow(FluxboxWindow *win);
    void reconfigure();
    void update();
    void setCurrent();
    void shutdown();
    int addWindow(FluxboxWindow *win, bool place = false);
    int removeWindow(FluxboxWindow *win);
    BScreen *getScreen() { return screen; }
    FluxboxWindow *getLastFocusedWindow() { return lastfocus; }

    const BScreen *getScreen() const { return screen; }	
    const FluxboxWindow *getLastFocusedWindow() const { return lastfocus; }	
    FbTk::Menu &menu() { return m_clientmenu; }
    inline const FbTk::Menu &menu() const { return m_clientmenu; }
    ///	name of this workspace
    inline const std::string &name() const { return m_name; }
    /**
       @return the number of this workspace, note: obsolete, should be in BScreen
    */
    inline unsigned int workspaceID() const { return m_id; }	
    /**
       @param id the window id number
       @return window that match the id, else 0
    */
    FluxboxWindow *getWindow(unsigned int id);
    const FluxboxWindow *getWindow(unsigned int id) const;
    const Windows &getWindowList() const { return m_windowlist; }
    Windows &getWindowList() { return m_windowlist; }

    bool isCurrent() const;
    bool isLastWindow(FluxboxWindow *window) const;
    int getCount() const;
    void checkGrouping(FluxboxWindow &win);
    static bool loadGroups(const std::string &filename);
protected:
    void placeWindow(FluxboxWindow *win);

private:

    void raiseAndFillStack(Stack::iterator &it, const FluxboxWindow &win);
    void lowerAndFillStack(Stack::iterator &it, const FluxboxWindow &win);

    BScreen *screen;
    FluxboxWindow *lastfocus;
    FbTk::Menu m_clientmenu;

    typedef std::list<FluxboxWindow *> WindowStack;
    typedef std::vector<std::string> Group;
    typedef std::vector<Group> GroupList;
	
    static GroupList m_groups; ///< handle auto groupings

    WindowStack stackingList;
    Windows m_windowlist;

    std::string m_name;  ///< name of this workspace
    unsigned int m_id;	///< id, obsolete, this should be in BScreen
    int cascade_x, cascade_y;
};


#endif // WORKSPACE_HH

