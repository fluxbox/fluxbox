// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
#include "FbTk/MemFun.hh"

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

using std::string;

Workspace::Workspace(BScreen &scrn, const string &name, unsigned int id):
    m_screen(scrn),
    m_clientmenu(scrn, m_windowlist, false),
    m_name(name),
    m_id(id) {

    m_clientlist_sig.connect(FbTk::MemFun(m_clientmenu,
                                          &ClientMenu::refreshMenu));

    menu().setInternalMenu();
    setName(name);


}


Workspace::~Workspace() {
}

void Workspace::addWindow(FluxboxWindow &w) {
    // we don't need to add a window that already exist in our list
    if (find(m_windowlist.begin(), m_windowlist.end(), &w) != m_windowlist.end())
        return;

    w.setWorkspace(m_id);

    m_windowlist.push_back(&w);
    m_clientlist_sig.emit();

}

// still_alive is true if the window will continue to exist after
// this event. Particularly, this isn't the removeWindow for
// the destruction of the window. Because if so, the focus revert
// is done in another place
int Workspace::removeWindow(FluxboxWindow *w, bool still_alive) {

    if (w == 0)
        return -1;

    // if w is focused and alive, remove the focus ... except if it
    // is a transient window. removing the focus from such a window
    // leads in a wild race between BScreen::reassociateWindow(),
    // BScreen::changeWorkspaceID(), FluxboxWindow::focus() etc. which
    // finally leads to crash.
    if (w->isFocused() && !w->isTransient() && still_alive)
        FocusControl::unfocusWindow(w->winClient(), true, true);

    m_windowlist.remove(w);
    m_clientlist_sig.emit();

    return m_windowlist.size();
}

void Workspace::showAll() {
    Windows::iterator it = m_windowlist.begin();
    Windows::iterator it_end = m_windowlist.end();
    for (; it != it_end; ++it)
        (*it)->show();
}


void Workspace::hideAll(bool interrupt_moving) {
    Windows::reverse_iterator it = m_windowlist.rbegin();
    Windows::reverse_iterator it_end = m_windowlist.rend();
    for (; it != it_end; ++it) {
        if (! (*it)->isStuck())
            (*it)->hide(interrupt_moving);
    }
}


void Workspace::removeAll(unsigned int dest) {
    Windows tmp_list(m_windowlist);
    Windows::iterator it = tmp_list.begin();
    Windows::iterator it_end = tmp_list.end();
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

void Workspace::setName(const string &name) {
    if (!name.empty() && name != "") {
        if (name == m_name)
            return;
        m_name = name;
    } else { //if name == 0 then set default name from nls
        _FB_USES_NLS;
        char tname[128];
        snprintf(tname, sizeof(tname),
                _FB_XTEXT(Workspace, DefaultNameFormat,
                        "Workspace %d", "Default workspace names, with a %d for the workspace number").c_str(),
                m_id + 1); //m_id starts at 0
        m_name = tname;
    }

    screen().updateWorkspaceName(m_id);

    menu().setLabel(FbTk::BiDiString(m_name));
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
    m_clientlist_sig.emit();
}
