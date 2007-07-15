// Gnome.cc for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Gnome.hh"

#include "App.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "Layer.hh"
#include "FbTk/I18n.hh"

#include <iostream>
#include <new>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::cerr;
using std::endl;
using std::list;

#ifdef DEBUG
using std::hex;
using std::dec;
#endif // DEBUG

Gnome::Gnome() {
    createAtoms();
    enableUpdate();
}

Gnome::~Gnome() {
    // destroy gnome windows
    while (!m_gnomewindows.empty()) {
        XDestroyWindow(FbTk::App::instance()->display(), m_gnomewindows.back());
        m_gnomewindows.pop_back();
    }
}


void Gnome::initForScreen(BScreen &screen) {
    Display *disp = FbTk::App::instance()->display();
    // create the GNOME window
    Window gnome_win = XCreateSimpleWindow(disp,
                                           screen.rootWindow().window(), 0, 0, 5, 5, 0, 0, 0);
    // supported WM check
    screen.rootWindow().changeProperty(m_gnome_wm_supporting_wm_check,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *) &gnome_win, 1);

    XChangeProperty(disp, gnome_win,
                    m_gnome_wm_supporting_wm_check,
                    XA_WINDOW, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

    // supported gnome atoms
    Atom gnomeatomlist[] = {
        m_gnome_wm_supporting_wm_check,
        m_gnome_wm_win_workspace_names,
        m_gnome_wm_win_client_list,
        m_gnome_wm_win_state,
        m_gnome_wm_win_hints,
        m_gnome_wm_win_layer
    };
    //list atoms that we support
    screen.rootWindow().changeProperty(m_gnome_wm_prot,
                                       XA_ATOM, 32, PropModeReplace,
                                       (unsigned char *)gnomeatomlist,
                                       (sizeof gnomeatomlist)/sizeof gnomeatomlist[0]);

    m_gnomewindows.push_back(gnome_win);

    updateClientList(screen);
    updateWorkspaceNames(screen);
    updateWorkspaceCount(screen);
    updateCurrentWorkspace(screen);

}

void Gnome::setupFrame(FluxboxWindow &win) {
    // load gnome state (take queues from the main window of the frame)
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    long flags, *data = 0;

    if (win.winClient().property(m_gnome_wm_win_state, 0, 1, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {
        flags = *data;
        setState(&win, flags);
        XFree (data);
    } else {
        updateState(win);
    }

    // load gnome layer atom
    if (win.winClient().property(m_gnome_wm_win_layer, 0, 1, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {
        flags = *data;
        setLayer(&win, flags);
        XFree (data);
    } else {
        updateLayer(win);
    }

    // load gnome workspace atom
    if (win.winClient().property(m_gnome_wm_win_workspace, 0, 1, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {
        unsigned int workspace_num = *data;
        if (win.workspaceNumber() != workspace_num)
            win.setWorkspace(workspace_num);
        XFree (data);
    } else {
        updateWorkspace(win);
    }

}


bool Gnome::propertyNotify(WinClient &winclient, Atom the_property) {
    if (the_property == m_gnome_wm_win_state) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): _WIN_STATE"<<endl;
#endif // DEBUG
        return true;
    } else if (the_property == m_gnome_wm_win_layer) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): _WIN_LAYER"<<endl;
#endif // DEBUG
        return true;
    }
    return false;
}


void Gnome::updateClientList(BScreen &screen) {
    size_t num=0;

    // count window clients in each workspace
    BScreen::Workspaces::const_iterator workspace_it =
        screen.getWorkspacesList().begin();
    BScreen::Workspaces::const_iterator workspace_it_end =
        screen.getWorkspacesList().end();
    for (; workspace_it != workspace_it_end; ++workspace_it) {
        Workspace::Windows::iterator win_it =
            (*workspace_it)->windowList().begin();
        Workspace::Windows::iterator win_it_end =
            (*workspace_it)->windowList().end();
        for (; win_it != win_it_end; ++win_it)
            num += (*win_it)->numClients();
    }

    Window *wl = new Window[num];
    if (wl == 0) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Gnome, OutOfMemoryClientList, "Fatal: Out of memory, can't allocate for GNOME client list", "")<<endl;
        return;
    }

    //add client windows to buffer
    workspace_it = screen.getWorkspacesList().begin();
    int win=0;
    for (; workspace_it != workspace_it_end; ++workspace_it) {

        // Fill in array of window ID's
        Workspace::Windows::const_iterator it =
            (*workspace_it)->windowList().begin();
        Workspace::Windows::const_iterator it_end =
            (*workspace_it)->windowList().end();
        for (; it != it_end; ++it) {
            // TODO!
            //check if the window don't want to be visible in the list
            //if (! ( (*it)->getGnomeHints() & WIN_STATE_HIDDEN) ) {
            list<WinClient *>::iterator client_it =
                (*it)->clientList().begin();
            list<WinClient *>::iterator client_it_end =
                (*it)->clientList().end();
            for (; client_it != client_it_end; ++client_it)
                wl[win++] = (*client_it)->window();

        }
    }
    //number of windows to show in client list
    num = win;
    screen.rootWindow().changeProperty(m_gnome_wm_win_client_list,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *)wl, num);

    delete[] wl;
}

void Gnome::updateClientClose(WinClient &client) {
    if (!client.screen().isShuttingdown()) {
        XDeleteProperty(FbTk::App::instance()->display(), client.window(),
                        m_gnome_wm_win_workspace);
        XDeleteProperty(FbTk::App::instance()->display(), client.window(),
                        m_gnome_wm_win_layer);
        XDeleteProperty(FbTk::App::instance()->display(), client.window(),
                        m_gnome_wm_win_state);
    }
}

void Gnome::updateWorkspaceNames(BScreen &screen) {

    size_t number_of_desks = screen.getWorkspaceNames().size();
    const BScreen::WorkspaceNames &workspace_names = screen.getWorkspaceNames();
    // convert our desktop names to a char * so we can send it
    char *names[number_of_desks];

    for (size_t i = 0; i < number_of_desks; i++) {
        names[i] = new char[workspace_names[i].size() + 1];
        strcpy(names[i], workspace_names[i].c_str());
    }

    XTextProperty  text;
    if (XStringListToTextProperty(names, number_of_desks, &text)) {
        XSetTextProperty(FbTk::App::instance()->display(), screen.rootWindow().window(),
			 &text, m_gnome_wm_win_workspace_names);
        XFree(text.value);
    }

    // destroy name buffers
    for (size_t i = 0; i < number_of_desks; i++)
        delete [] names[i];
}

void Gnome::updateCurrentWorkspace(BScreen &screen) {
    long workspace = screen.currentWorkspaceID();
    screen.rootWindow().changeProperty(m_gnome_wm_win_workspace, XA_CARDINAL, 32, PropModeReplace,
                                       (unsigned char *)&workspace, 1);

    updateClientList(screen); // make sure the client list is updated too
}

void Gnome::updateWorkspaceCount(BScreen &screen) {
    long numworkspaces = screen.numberOfWorkspaces();
    screen.rootWindow().changeProperty(m_gnome_wm_win_workspace_count, XA_CARDINAL, 32, PropModeReplace,
                                       (unsigned char *)&numworkspaces, 1);
}

void Gnome::updateWorkspace(FluxboxWindow &win) {
    long val = win.workspaceNumber();
    if (win.isStuck()) {
        val = -1;
    }
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): setting workspace("<<val<<
        ") for window("<<&win<<")"<<endl;
#endif // DEBUG

    FluxboxWindow::ClientList::iterator client_it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator client_it_end = win.clientList().end();
    for (; client_it != client_it_end; ++client_it)
        (*client_it)->changeProperty(m_gnome_wm_win_workspace,
                                       XA_CARDINAL, 32, PropModeReplace,
                                       (unsigned char *)&val, 1);
}

void Gnome::updateState(FluxboxWindow &win) {
    //translate to gnome win state
    long state=0;
    if (win.isStuck())
        state |= WIN_STATE_STICKY;
    if (win.isIconic())
        state |= WIN_STATE_MINIMIZED;
    if (win.isShaded())
        state |= WIN_STATE_SHADED;

    FluxboxWindow::ClientList::iterator client_it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator client_it_end = win.clientList().end();
    for (; client_it != client_it_end; ++client_it) {
        (*client_it)->changeProperty(m_gnome_wm_win_state,
                                     XA_CARDINAL, 32,
                                     PropModeReplace, (unsigned char *)&state, 1);
    }
}

void Gnome::updateLayer(FluxboxWindow &win) {
    //TODO - map from flux layers to gnome ones
    // our layers are in the opposite direction to GNOME
    long layernum = Layer::DESKTOP - win.layerNum();

    FluxboxWindow::ClientList::iterator client_it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator client_it_end = win.clientList().end();
    for (; client_it != client_it_end; ++client_it)
        (*client_it)->changeProperty(m_gnome_wm_win_layer,
                                     XA_CARDINAL, 32, PropModeReplace,
                                     (unsigned char *)&layernum, 1);

}

void Gnome::updateHints(FluxboxWindow &win) {
    //TODO

}

bool Gnome::checkClientMessage(const XClientMessageEvent &ce, BScreen * screen, WinClient * const winclient) {
    if (ce.message_type == m_gnome_wm_win_workspace) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): Got workspace atom="<<ce.data.l[0]<<endl;
#endif//!DEBUG
        if ( winclient !=0 && // the message sent to client window?
             ce.data.l[0] >= 0 &&
             ce.data.l[0] < (signed)winclient->screen().numberOfWorkspaces()) {
            winclient->screen().changeWorkspaceID(ce.data.l[0]);

        } else if (screen!=0 && //the message sent to root window?
                   ce.data.l[0] >= 0 &&
                   ce.data.l[0] < (signed)screen->numberOfWorkspaces())
            screen->changeWorkspaceID(ce.data.l[0]);
        return true;
    } else if (winclient == 0)
        return false;


    if (ce.message_type == m_gnome_wm_win_state) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_STATE"<<endl;
        cerr<<__FILE__<<"("<<__LINE__<<"): Mask of members to change:"<<
            hex<<ce.data.l[0]<<dec<<endl; // mask_of_members_to_change
        cerr<<"New members:"<<ce.data.l[1]<<endl;
#endif // DEBUG

        if (winclient && winclient->fbwindow()) {
            //get new states
            int flag = ce.data.l[0] & ce.data.l[1];
            //don't update this when when we set new state
            disableUpdate();
            // convert to Fluxbox state
            setState(winclient->fbwindow(), flag);
            // enable update of atom states
            enableUpdate();
        }
    } else if (ce.message_type == m_gnome_wm_win_hints) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_HINTS"<<endl;
#endif // DEBUG

    } else if (ce.message_type == m_gnome_wm_win_layer) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_LAYER"<<endl;
#endif // DEBUG

        if (winclient && winclient->fbwindow())
            setLayer(winclient->fbwindow(), ce.data.l[0]);
    } else
        return false; //the gnome atom wasn't found or not supported

    return true; // we handled the atom
}

void Gnome::setState(FluxboxWindow *win, int state) {
#ifdef DEBUG
    cerr<<"Gnome: state=0x"<<hex<<state<<dec<<endl;
#endif // DEBUG

    if (state & WIN_STATE_STICKY) {
#ifdef DEBUG
        cerr<<"Gnome state: Sticky"<<endl;
#endif // DEBUG
        if (!win->isStuck())
            win->stick();
    } else if (win->isStuck())
        win->stick();

    if (state & WIN_STATE_MINIMIZED) {
#ifdef DEBUG
        cerr<<"Gnome state: Minimized"<<endl;
#endif // DEBUG
        if (win->isIconic())
            win->iconify();
    } else if (win->isIconic())
        win->deiconify(true, true);

    if (state & WIN_STATE_SHADED) {
#ifdef DEBUG
        cerr<<"Gnome state: Shade"<<endl;
#endif // DEBUG
        if (!win->isShaded())
            win->shade();
    } else if (win->isShaded())
        win->shade();

    if (state & WIN_STATE_HIDDEN)
    {
        win->setFocusHidden(! win->isFocusHidden());
        win->setIconHidden(! win->isIconHidden());
    }


    /*
    if (state & WIN_STATE_MAXIMIZED_VERT)
        cerr<<"Maximize Vert"<<endl;
    if (state & WIN_STATE_MAXIMIZED_HORIZ)
        cerr<<"Maximize Horiz"<<endl;

    if (state & WIN_STATE_HID_WORKSPACE)
        cerr<<"HID Workspace"<<endl;
    if (state & WIN_STATE_HID_TRANSIENT)
        cerr<<"HID Transient"<<endl;
    if (state & WIN_STATE_FIXED_POSITION)
        cerr<<"Fixed Position"<<endl;
    if (state & WIN_STATE_ARRANGE_IGNORE)
        cerr<<"Arrange Ignore"<<endl;
    */
}

void Gnome::setLayer(FluxboxWindow *win, int layer) {
    if (!win) return;


    switch (layer) {
    case WIN_LAYER_DESKTOP:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_DESKTOP)"<<endl;
#endif // DEBUG
        layer = Layer::DESKTOP;
        break;
    case WIN_LAYER_BELOW:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_BELOW)"<<endl;
#endif // DEBUG
        layer = Layer::BOTTOM;
        break;
    case WIN_LAYER_NORMAL:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_NORMAL)"<<endl;
#endif // DEBUG
        layer = Layer::NORMAL;
        break;
    case WIN_LAYER_ONTOP:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_ONTOP)"<<endl;
#endif // DEBUG
        layer = Layer::TOP;
        break;
    case WIN_LAYER_DOCK:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_DOCK)"<<endl;
#endif // DEBUG
        layer = Layer::DOCK;
        break;
    case WIN_LAYER_ABOVE_DOCK:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_ABOVE_DOCK)"<<endl;
#endif // DEBUG
        layer = Layer::ABOVE_DOCK;
        break;
    case WIN_LAYER_MENU:
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", WIN_LAYER_MENU)"<<endl;
#endif // DEBUG
        layer = Layer::MENU;
        break;
    default:
        // our windows are in the opposite direction to gnome
        layer = Layer::DESKTOP - layer;
#ifdef DEBUG
        cerr<<"Gnome::setLayer("<<win->title()<<", "<<layer<<")"<<endl;
#endif // DEBUG
        break;
    }

    win->moveToLayer(layer);

}

void Gnome::createAtoms() {
    Display *disp = FbTk::App::instance()->display();
    m_gnome_wm_win_layer = XInternAtom(disp, "_WIN_LAYER", False);
    m_gnome_wm_win_state = XInternAtom(disp, "_WIN_STATE", False);
    m_gnome_wm_win_hints = XInternAtom(disp, "_WIN_HINTS", False);
    m_gnome_wm_win_app_state = XInternAtom(disp, "_WIN_APP_STATE", False);
    m_gnome_wm_win_expanded_size = XInternAtom(disp, "_WIN_EXPANDED_SIZE", False);
    m_gnome_wm_win_icons = XInternAtom(disp, "_WIN_ICONS", False);
    m_gnome_wm_win_workspace = XInternAtom(disp, "_WIN_WORKSPACE", False);
    m_gnome_wm_win_workspace_count = XInternAtom(disp, "_WIN_WORKSPACE_COUNT", False);
    m_gnome_wm_win_workspace_names = XInternAtom(disp, "_WIN_WORKSPACE_NAMES", False);
    m_gnome_wm_win_client_list = XInternAtom(disp, "_WIN_CLIENT_LIST", False);
    m_gnome_wm_prot = XInternAtom(disp, "_WIN_PROTOCOLS", False);
    m_gnome_wm_supporting_wm_check = XInternAtom(disp, "_WIN_SUPPORTING_WM_CHECK", False);
}
