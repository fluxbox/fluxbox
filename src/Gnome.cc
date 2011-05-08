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

#include "Gnome.hh"

#include "FbTk/App.hh"
#include "FbTk/I18n.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "Layer.hh"
#include "Debug.hh"

#include <iostream>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::cerr;
using std::endl;
using std::list;
using std::hex;
using std::dec;

Gnome::Gnome() {
    setName("gnome");
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
    long flags;
    bool exists;
    flags=win.winClient().cardinalProperty(m_gnome_wm_win_state,&exists);
    if (exists) {
        setState(&win, flags);
    } else {
        updateState(win);
    }

    // load gnome layer atom
    flags=win.winClient().cardinalProperty(m_gnome_wm_win_layer,&exists);
    if (exists) {
        setLayer(&win, flags);
    } else {
        updateLayer(win);
    }

    // load gnome workspace atom
    flags=win.winClient().cardinalProperty(m_gnome_wm_win_workspace,&exists);
    if (exists)
    {
        unsigned int workspace_num = flags;
        if (win.workspaceNumber() != workspace_num)
            win.setWorkspace(workspace_num);
    } else {
        updateWorkspace(win);
    }

}


bool Gnome::propertyNotify(WinClient &winclient, Atom the_property) {
    if (the_property == m_gnome_wm_win_state) {
        fbdbg<<"("<<__FUNCTION__<<"): _WIN_STATE"<<endl;
        return true;
    } else if (the_property == m_gnome_wm_win_layer) {
        fbdbg<<"("<<__FUNCTION__<<"): _WIN_LAYER"<<endl;
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
    char** names = new char*[number_of_desks];

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
        delete[] names[i];

    delete[] names;
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

    fbdbg<<"setting workspace("<<val<<") for window("<<&win<<")"<<endl;

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
    long layernum = ResourceLayer::DESKTOP - win.layerNum();

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

        fbdbg<<"Got workspace atom="<<ce.data.l[0]<<endl;

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

        fbdbg<<"_WIN_STATE"<<endl;
        fbdbg<<"Mask of members to change:"<<
            hex<<ce.data.l[0]<<dec<<endl; // mask_of_members_to_change
        fbdbg<<"New members:"<<ce.data.l[1]<<endl;


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
        fbdbg<<"_WIN_HINTS"<<endl;
    } else if (ce.message_type == m_gnome_wm_win_layer) {
        fbdbg<<"_WIN_LAYER"<<endl;
        if (winclient && winclient->fbwindow())
            setLayer(winclient->fbwindow(), ce.data.l[0]);
    } else
        return false; //the gnome atom wasn't found or not supported

    return true; // we handled the atom
}

void Gnome::setState(FluxboxWindow *win, int state) {
    fbdbg<<"Gnome: state=0x"<<hex<<state<<dec<<endl;

    if (state & WIN_STATE_STICKY) {

        fbdbg<<"Gnome state: Sticky"<<endl;

        if (!win->isStuck())
            win->stick();
    } else if (win->isStuck())
        win->stick();

    if (state & WIN_STATE_MINIMIZED) {
        fbdbg<<"Gnome state: Minimized"<<endl;

        if (win->isIconic())
            win->iconify();
    } else if (win->isIconic())
        win->deiconify(true);

    if (state & WIN_STATE_SHADED) {

        fbdbg<<"Gnome state: Shade"<<endl;

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

    const FbTk::FbString& title = win->title().logical();
    switch (layer) {
    case WIN_LAYER_DESKTOP:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_DESKTOP)"<<endl;
        layer = ResourceLayer::DESKTOP;
        break;
    case WIN_LAYER_BELOW:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_BELOW)"<<endl;
        layer = ResourceLayer::BOTTOM;
        break;
    case WIN_LAYER_NORMAL:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_NORMAL)"<<endl;
        layer = ResourceLayer::NORMAL;
        break;
    case WIN_LAYER_ONTOP:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_ONTOP)"<<endl;
        layer = ResourceLayer::TOP;
        break;
    case WIN_LAYER_DOCK:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_DOCK)"<<endl;
        layer = ResourceLayer::DOCK;
        break;
    case WIN_LAYER_ABOVE_DOCK:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_ABOVE_DOCK)"<<endl;
        layer = ResourceLayer::ABOVE_DOCK;
        break;
    case WIN_LAYER_MENU:
        fbdbg<<"Gnome::setLayer("<<title<<", WIN_LAYER_MENU)"<<endl;
        layer = ResourceLayer::MENU;
        break;
    default:
        // our windows are in the opposite direction to gnome
        layer = ResourceLayer::DESKTOP - layer;
        fbdbg<<"Gnome::setLayer("<<win->title().logical()<<", "<<layer<<")"<<endl;

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
