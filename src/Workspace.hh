// Workspace.hh for Fluxbox
// Copyright (c) 2002 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef     WORKSPACE_HH
#define     WORKSPACE_HH

#include "ClientMenu.hh"

#include "FbTk/NotCopyable.hh"
#include "FbTk/Signal.hh"

#include <string>
#include <list>

class BScreen;
class FluxboxWindow;

/**
 * Handles a single workspace
 */
class Workspace: private FbTk::NotCopyable {
public:
    typedef std::list<FluxboxWindow *> Windows;

    Workspace(BScreen &screen, const std::string &name,
              unsigned int workspaceid = 0);
    ~Workspace();

    /// Set workspace name
    void setName(const FbTk::FbString& name);
    /// Deiconify all windows on this workspace
    void showAll();
    void hideAll(bool interrupt_moving);
    /// Iconify all windows on this workspace
    void removeAll(unsigned int dest);
    void reconfigure();
    void shutdown();

    /// Add @a win to this workspace, placing it if @a place is true
    void addWindow(FluxboxWindow &win);
    int removeWindow(FluxboxWindow *win, bool still_alive);
    void updateClientmenu();

    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }

    FbTk::Menu &menu() { return m_clientmenu; }
    const FbTk::Menu &menu() const { return m_clientmenu; }
    ///    name of this workspace
    const FbTk::FbString &name() const { return m_name; }
    /**
       @return the number of this workspace, note: obsolete, should be in BScreen
    */
    unsigned int workspaceID() const { return m_id; }

    const Windows &windowList() const { return m_windowlist; }
    Windows &windowList() { return m_windowlist; }

    size_t numberOfWindows() const;

private:
    void placeWindow(FluxboxWindow &win);

    BScreen &m_screen;

    Windows m_windowlist;
    FbTk::Signal<> m_clientlist_sig;
    ClientMenu m_clientmenu;

    FbTk::FbString m_name;  ///< name of this workspace
    unsigned int m_id;    ///< id, obsolete, this should be in BScreen

};


#endif // WORKSPACE_HH

