// ToolbarHandler for fluxbox
// Copyright (c) 2003 Simon Bowden (rathnor at fluxbox.org)
//                and Henrik Kinnunen (fluxgen at fluxbox.org)
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

// $Id: ToolbarHandler.hh,v 1.9 2003/10/06 06:22:43 rathnor Exp $

#ifndef TOOLBARHANDLER_HH
#define TOOLBARHANDLER_HH

#include "AtomHandler.hh"
#include "Menu.hh"
#include "Toolbar.hh"
#include "Resource.hh"

class BScreen;
class FluxboxWindow;

class ToolbarHandler : public AtomHandler {
public:
    enum ToolbarMode {
        OFF=0,
        NONE,
        ICONS,
	WORKSPACEICONS,
        WORKSPACE,
        ALLWINDOWS,
        LASTMODE
    };

    explicit ToolbarHandler(BScreen &screen);
    ~ToolbarHandler() { }

    void setMode(ToolbarMode mode, bool initialise = true);

    inline const Toolbar *toolbar() const { return m_toolbar.get(); }
    inline Toolbar *toolbar() { return m_toolbar.get(); }
  

    void initForScreen(BScreen &screen);
    void setupFrame(FluxboxWindow &win);
    // TODO: add setupClient and configure option to show groups or indiv. clients
    void setupClient(WinClient &winclient) {}
    
    void updateState(FluxboxWindow &win);
    void updateFrameClose(FluxboxWindow &win);
    void updateClientClose(WinClient &winclient) {}
    void updateWorkspace(FluxboxWindow &win);
    void updateCurrentWorkspace(BScreen &screen);

    // these ones don't affect us
    void updateWorkspaceNames(BScreen &screen) {}
    void updateWorkspaceCount(BScreen &screen) {}
    void updateClientList(BScreen &screen) {}
    void updateHints(FluxboxWindow &win) {}
    void updateLayer(FluxboxWindow &win) {}

    bool checkClientMessage(const XClientMessageEvent &ce, 
                            BScreen * screen, WinClient * const winclient) { return false; }

    bool propertyNotify(WinClient &winclient, Atom the_atom) { return false; }

    inline FbTk::Menu &getModeMenu() { return m_modemenu; }
    inline const FbTk::Menu &getModeMenu() const { return m_modemenu; }
    inline FbTk::Menu &getToolbarMenu() { return m_toolbarmenu; }
    inline const FbTk::Menu &getToolbarMenu() const { return m_toolbarmenu; }

    inline BScreen &screen() { return m_screen; }
    inline const BScreen &screen() const { return m_screen; }
    ToolbarMode mode() const { return m_mode; }
private:
    BScreen &m_screen;
    std::auto_ptr<Toolbar> m_toolbar;
    unsigned int m_current_workspace;
    ToolbarMode m_mode;
    FbMenu m_modemenu;
    FbMenu m_toolbarmenu;
};

#endif // TOOLBARHANDLER_HH
