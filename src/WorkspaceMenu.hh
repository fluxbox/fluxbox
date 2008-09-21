// WorkspaceMenu.hh for Fluxbox
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef WORKSPACEMENU_HH
#define WORKSPACEMENU_HH

#include "FbMenu.hh"
#include "FbTk/Signal.hh"

class BScreen;

/**
 * A menu specific for workspace.
 * Contains some simple workspace commands
 * such as new/delete workspace and edit
 * workspace name.
 * It also contains client menus for all clients.
 */
class WorkspaceMenu: public FbMenu, private FbTk::SignalTracker {
public:
    explicit WorkspaceMenu(BScreen &screen);
    virtual ~WorkspaceMenu() { }

private:
    /// initialize menu for the screen
    void init(BScreen &screen);
    /// Called when workspace info was changed 
    /// ( number of workspace, workspace names etc )
    void workspaceInfoChanged(BScreen& screen);
    /// Called when workspace was switched.
    void workspaceChanged(BScreen& screen);
};

#endif //  WORKSPACEMENU_HH
