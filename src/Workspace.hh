// Workspace.hh for Fluxbox
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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
#include "MultLayers.hh"

#include <X11/Xlib.h>

#include <string>
#include <vector>
#include <list>

class BScreen;
class FluxboxWindow;
class WinClient;

/**
	Handles a single workspace
*/
class Workspace:private FbTk::NotCopyable {
public:
    typedef std::vector<FluxboxWindow *> Windows;

    Workspace(BScreen &screen, FbTk::MultLayers &layermanager, const std::string &name,
              unsigned int workspaceid = 0);
    ~Workspace();
	
    void setLastFocusedWindow(FluxboxWindow *w);

    ///   Set workspace name
    void setName(const std::string &name);
    void showAll();
    void hideAll();
    void removeAll();
    void reconfigure();
    void update();
    void setCurrent();
    void shutdown();
    void addWindow(FluxboxWindow &win, bool place = false);
    int removeWindow(FluxboxWindow *win);
    void removeWindow(WinClient &client);
    void updateClientmenu();

    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }	

    FluxboxWindow *lastFocusedWindow() { return m_lastfocus; }
    const FluxboxWindow *lastFocusedWindow() const { return m_lastfocus; }	

    FbTk::Menu &menu() { return m_clientmenu; }
    inline const FbTk::Menu &menu() const { return m_clientmenu; }
    ///	name of this workspace
    inline const std::string &name() const { return m_name; }
    /**
       @return the number of this workspace, note: obsolete, should be in BScreen
    */
    inline unsigned int workspaceID() const { return m_id; }	

    const Windows &windowList() const { return m_windowlist; }
    Windows &windowList() { return m_windowlist; }

    bool isCurrent() const;
    bool isLastWindow(FluxboxWindow *window) const;
    int numberOfWindows() const;
    bool checkGrouping(FluxboxWindow &win);
    static bool loadGroups(const std::string &filename);
protected:
    void placeWindow(FluxboxWindow &win);

private:


    BScreen &m_screen;
    FluxboxWindow *m_lastfocus;
    FbTk::Menu m_clientmenu;

    typedef std::list<FluxboxWindow *> WindowStack;
    typedef std::vector<std::string> Group;
    typedef std::vector<Group> GroupList;
	
    static GroupList m_groups; ///< handle auto groupings

    FbTk::MultLayers &m_layermanager;
    Windows m_windowlist;

    std::string m_name;  ///< name of this workspace
    unsigned int m_id;	///< id, obsolete, this should be in BScreen
    int *m_cascade_x, *m_cascade_y; // need a cascade for each head (Xinerama)
};


#endif // WORKSPACE_HH

