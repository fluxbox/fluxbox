// Window.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Window.cc for Blackbox - an X11 Window manager
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

// $Id$

#include "Window.hh"

#include "WinClient.hh"
#include "fluxbox.hh"
#include "Keys.hh"
#include "Screen.hh"
#include "FbWinFrameTheme.hh"
#include "FbAtoms.hh"
#include "RootTheme.hh"
#include "Workspace.hh"
#include "FbWinFrame.hh"
#include "WinButton.hh"
#include "WinButtonTheme.hh"
#include "WindowCmd.hh"
#include "Remember.hh"
#include "MenuCreator.hh"
#include "StringUtil.hh"
#include "FocusControl.hh"
#include "Layer.hh"
#include "IconButton.hh"
#include "ScreenPlacement.hh"

#include "FbTk/Compose.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Select2nd.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

//use GNU extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#include <iostream>
#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif
#include <functional>
#include <algorithm>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::bind2nd;
using std::mem_fun;
using std::equal_to;
using std::max;
using std::swap;

using namespace FbTk;

#ifdef DEBUG
using std::dec;
using std::hex;
#endif // DEBUG

namespace {

// X event scanner for enter/leave notifies - adapted from twm
typedef struct scanargs {
    Window w;
    Bool leave, inferior, enter;
} scanargs;

// look for valid enter or leave events (that may invalidate the earlier one we are interested in)
static Bool queueScanner(Display *, XEvent *e, char *args) {
    if (e->type == LeaveNotify &&
        e->xcrossing.window == ((scanargs *) args)->w &&
        e->xcrossing.mode == NotifyNormal) {
        ((scanargs *) args)->leave = true;
        ((scanargs *) args)->inferior = (e->xcrossing.detail == NotifyInferior);
    } else if (e->type == EnterNotify &&
               e->xcrossing.mode == NotifyUngrab)
        ((scanargs *) args)->enter = true;

    return false;
}

/// returns the deepest transientFor, asserting against a close loop
WinClient *getRootTransientFor(WinClient *client) {
    while (client && client->transientFor()) {
        assert(client != client->transientFor());
        client = client->transientFor();
    }
    return client;
}


/// raise window and do the same for each transient of the current window
void raiseFluxboxWindow(FluxboxWindow &win) {
    if (win.oplock)
        return;

    if (win.isIconic())
        return;

    win.oplock = true;

    // we need to lock actual restacking so that raising above active transient
    // won't do anything nasty
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().lock();

    win.layerItem().raise();

    // for each transient do raise

    WinClient::TransientList::const_iterator it = win.winClient().transientList().begin();
    WinClient::TransientList::const_iterator it_end = win.winClient().transientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && !(*it)->fbwindow()->isIconic())
            // TODO: should we also check if it is the active client?
            raiseFluxboxWindow(*(*it)->fbwindow());
    }

    win.oplock = false;


    if (!win.winClient().transientList().empty())
        win.screen().layerManager().unlock();

}

/// lower window and do the same for each transient it holds
void lowerFluxboxWindow(FluxboxWindow &win) {
    if (win.oplock)
        return;

    if (win.isIconic())
        return;

    win.oplock = true;

    // we need to lock actual restacking so that raising above active transient
    // won't do anything nasty
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().lock();

    // lower the windows from the top down, so they don't change stacking order
    WinClient::TransientList::const_reverse_iterator it = win.winClient().transientList().rbegin();
    WinClient::TransientList::const_reverse_iterator it_end = win.winClient().transientList().rend();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && !(*it)->fbwindow()->isIconic())
            // TODO: should we also check if it is the active client?
            lowerFluxboxWindow(*(*it)->fbwindow());
    }

    win.layerItem().lower();

    win.oplock = false;
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().unlock();

}

/// raise window and do the same for each transient it holds
void tempRaiseFluxboxWindow(FluxboxWindow &win) {
    if (win.oplock) return;
    win.oplock = true;

    if (!win.isIconic()) {
        win.layerItem().tempRaise();
    }

    // for each transient do raise
    WinClient::TransientList::const_iterator it = win.winClient().transientList().begin();
    WinClient::TransientList::const_iterator it_end = win.winClient().transientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && !(*it)->fbwindow()->isIconic())
            // TODO: should we also check if it is the active client?
            tempRaiseFluxboxWindow(*(*it)->fbwindow());
    }
    win.oplock = false;

}

class SetClientCmd:public FbTk::Command {
public:
    explicit SetClientCmd(WinClient &client):m_client(client) {
    }
    void execute() {
        m_client.focus();
    }
private:
    WinClient &m_client;
};

};


int FluxboxWindow::s_num_grabs = 0;

FluxboxWindow::FluxboxWindow(WinClient &client, FbWinFrameTheme &tm,
                             FbTk::XLayer &layer):
    Focusable(client.screen(), this),
    oplock(false),
    m_hintsig(*this),
    m_statesig(*this),
    m_layersig(*this),
    m_workspacesig(*this),
    m_themelistener(*this),
    m_creation_time(0),
    moving(false), resizing(false), shaded(false), iconic(false),
    stuck(false), m_initialized(false), fullscreen(false),
    maximized(MAX_NONE),
    m_attaching_tab(0),
    display(FbTk::App::instance()->display()),
    m_button_grab_x(0), m_button_grab_y(0),
    m_last_move_x(0), m_last_move_y(0),
    m_last_resize_h(1), m_last_resize_w(1),
    m_workspace_number(0),
    m_current_state(0),
    m_old_decoration_mask(0),
    m_client(&client),
    m_toggled_decos(false),
    m_icon_hidden(false),
    m_focus_hidden(false),
    m_old_pos_x(0), m_old_pos_y(0),
    m_old_width(1),  m_old_height(1),
    m_last_button_x(0),  m_last_button_y(0),
    m_frame(client.screen(), tm, client.screen().imageControl(), layer, 0, 0, 100, 100),
    m_placed(false),
    m_layernum(layer.getLayerNum()),
    m_old_layernum(0),
    m_parent(client.screen().rootWindow()),
    m_resize_corner(RIGHTBOTTOM) {

    tm.reconfigSig().attach(&m_themelistener);

    init();

    if (!isManaged())
        return;

    // add the window to the focus list
    // always add to front on startup to keep the focus order the same
    if (screen().focusControl().focusNew() || Fluxbox::instance()->isStartup())
        screen().focusControl().addFocusWinFront(*this);
    else
        screen().focusControl().addFocusWinBack(*this);

    Fluxbox::instance()->keys()->registerWindow(frame().window().window(),
                                                *this, Keys::ON_WINDOW);

}


FluxboxWindow::~FluxboxWindow() {
    if (WindowCmd<void>::window() == this)
        WindowCmd<void>::setWindow(0);
    if ( Fluxbox::instance()->keys() != 0 ) {
        Fluxbox::instance()->keys()->
            unregisterWindow(frame().window().window());
    }


#ifdef DEBUG
    const char* title = m_client ? m_client->title().c_str() : "" ;
    cerr<<__FILE__<<"("<<__LINE__<<"): starting ~FluxboxWindow("<<this<<","<<title<<")"<<endl;
    cerr<<__FILE__<<"("<<__LINE__<<"): num clients = "<<numClients()<<endl;
    cerr<<__FILE__<<"("<<__LINE__<<"): curr client = "<<m_client<<endl;
    cerr<<__FILE__<<"("<<__LINE__<<"): m_labelbuttons.size = "<<m_labelbuttons.size()<<endl;
#endif // DEBUG

    if (moving)
        stopMoving(true);
    if (resizing)
        stopResizing(true);
    if (m_attaching_tab)
        attachTo(0, 0, true);

    // no longer a valid window to do stuff with
    Fluxbox::instance()->removeWindowSearchGroup(frame().window().window());
    Fluxbox::instance()->removeWindowSearchGroup(frame().tabcontainer().window());

    Client2ButtonMap::iterator it = m_labelbuttons.begin();
    Client2ButtonMap::iterator it_end = m_labelbuttons.end();
    for (; it != it_end; ++it)
        frame().removeTab((*it).second);

    m_labelbuttons.clear();

    m_timer.stop();

    // notify die
    m_diesig.notify();

    if (m_client != 0 && !m_screen.isShuttingdown())
        delete m_client; // this also removes client from our list
    m_client = 0;

    if (m_clientlist.size() > 1) {
        cerr<<__FILE__<<"(~FluxboxWindow()) WARNING! clientlist > 1"<<endl;
        while (!m_clientlist.empty()) {
            detachClient(*m_clientlist.back());
        }
    }

    if (!screen().isShuttingdown())
        screen().focusControl().removeWindow(*this);

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ~FluxboxWindow("<<this<<")"<<endl;
#endif // DEBUG
}


void FluxboxWindow::init() {
    m_attaching_tab = 0;

    assert(m_client);
    m_client->setFluxboxWindow(this);
    m_client->setGroupLeftWindow(None); // nothing to the left.

    if (Fluxbox::instance()->haveShape()) {
        Shape::setShapeNotify(winClient());
    }

    //!! TODO init of client should be better
    // we don't want to duplicate code here and in attachClient
    m_clientlist.push_back(m_client);
#ifdef DEBUG
    cerr<<__FILE__<<": FluxboxWindow::init(this="<<this<<", client="<<hex<<
        m_client->window()<<", frame = "<<frame().window().window()<<dec<<")"<<endl;

#endif // DEBUG

    Fluxbox &fluxbox = *Fluxbox::instance();

    // setup cursors for resize grips
    frame().gripLeft().setCursor(frame().theme().lowerLeftAngleCursor());
    frame().gripRight().setCursor(frame().theme().lowerRightAngleCursor());

    associateClient(*m_client);

    frame().setLabelButtonFocus(*m_labelbuttons[m_client]);

    // redirect events from frame to us
    frame().setEventHandler(*this);

    frame().resize(m_client->width(), m_client->height());

    m_blackbox_attrib.workspace = m_workspace_number = m_screen.currentWorkspaceID();

    m_blackbox_attrib.flags = m_blackbox_attrib.attrib = m_blackbox_attrib.stack = 0;
    m_blackbox_attrib.premax_x = m_blackbox_attrib.premax_y = 0;
    m_blackbox_attrib.premax_w = m_blackbox_attrib.premax_h = 0;

    // set default decorations but don't apply them
    setDecorationMask(getDecoMaskFromString(screen().defaultDeco()), false);

    functions.resize = functions.move = functions.iconify = functions.maximize
    = functions.close = functions.tabable = true;

    updateMWMHintsFromClient(*m_client);

    //!!
    // fetch client size and placement
    XWindowAttributes wattrib;
    if (! m_client->getAttrib(wattrib) ||
        !wattrib.screen  || // no screen? ??
        wattrib.override_redirect || // override redirect
        m_client->initial_state == WithdrawnState) // Slit client
        return;

    // save old border width so we can restore it later
    m_client->old_bw = wattrib.border_width;
    m_client->x = wattrib.x; m_client->y = wattrib.y;

    m_timer.setTimeout(fluxbox.getAutoRaiseDelay());
    FbTk::RefCount<FbTk::Command> raise_cmd(new FbTk::SimpleCommand<FluxboxWindow>(*this,
                                                                                   &FluxboxWindow::raise));
    m_timer.setCommand(raise_cmd);
    m_timer.fireOnce(true);

    Fluxbox::instance()->saveWindowSearchGroup(frame().window().window(), this);
    Fluxbox::instance()->saveWindowSearchGroup(frame().tabcontainer().window(), this);

    /**************************************************/
    /* Read state above here, apply state below here. */
    /**************************************************/

    if (m_client->transientFor() && m_client->transientFor()->fbwindow() &&
        m_client->transientFor()->fbwindow()->isStuck())
        stick();

    // adjust the window decorations based on transience and window sizes
    if (m_client->isTransient() && !screen().decorateTransient()) {
        decorations.maximize =  functions.maximize = false;
        decorations.handle = false;
    }

    if ((m_client->normal_hint_flags & PMinSize) &&
        (m_client->normal_hint_flags & PMaxSize) &&
        m_client->max_width != 0 && m_client->max_width <= m_client->min_width &&
        m_client->max_height != 0 && m_client->max_height <= m_client->min_height) {
        decorations.maximize = decorations.handle =
            functions.resize = functions.maximize = false;
        decorations.tab = false; //no tab for this window
    }

    associateClientWindow(true, 
                          wattrib.x, wattrib.y, 
                          wattrib.width, wattrib.height, 
                          m_client->gravity(), m_client->old_bw);

    setWindowType(m_client->getWindowType());

    if (fluxbox.isStartup())
        m_placed = true;
    else if (m_client->isTransient() ||
        m_client->normal_hint_flags & (PPosition|USPosition)) {

        int real_x = frame().x();
        int real_y = frame().y();

        if (real_x >= 0 &&
            real_y >= 0 &&
            real_x <= (signed) screen().width() &&
            real_y <= (signed) screen().height())
            m_placed = true;

    } else
        setOnHead(screen().getCurrHead());

    Fluxbox::instance()->attachSignals(*this);

    // this window is managed, we are now allowed to modify actual state
    m_initialized = true;

    applyDecorations(true);

    restoreAttributes();

    if (m_workspace_number >= screen().numberOfWorkspaces())
        m_workspace_number = screen().currentWorkspaceID();

/*
    if (wattrib.width <= 0)
        wattrib.width = 1;
    if (wattrib.height <= 0)
        wattrib.height = 1;
*/

    // if we're a transient then we should be on the same layer as our parent
    if (m_client->isTransient() &&
        m_client->transientFor()->fbwindow() &&
        m_client->transientFor()->fbwindow() != this)
        layerItem().setLayer(m_client->transientFor()->fbwindow()->layerItem().getLayer());
    else // if no parent then set default layer
        moveToLayer(m_layernum, m_layernum != ::Layer::NORMAL);
#ifdef DEBUG
    cerr<<"FluxboxWindow::init("<<title()<<") transientFor: "<<
        m_client->transientFor()<<endl;
    if (m_client->transientFor() && m_client->transientFor()->fbwindow()) {
        cerr<<"FluxboxWindow::init("<<title()<<") transientFor->title(): "<<
            m_client->transientFor()->fbwindow()->title()<<endl;
    }
#endif // DEBUG

    int real_width = frame().width();
    int real_height = frame().height() - frame().titlebarHeight() - frame().handleHeight();
    m_client->applySizeHints(real_width, real_height);
    real_height += frame().titlebarHeight() + frame().handleHeight();

    if (m_placed)
        moveResize(frame().x(), frame().y(), real_width, real_height);

    if (!m_placed) placeWindow(getOnHead());
    screen().getWorkspace(m_workspace_number)->addWindow(*this);

    setFocusFlag(false); // update graphics before mapping

    if (stuck) {
        stuck = false;
        stick();
    }

    if (shaded) { // start shaded
        shaded = false;
        shade();
    }

    if (iconic) {
        iconic = false;
        iconify();
    } else if (m_workspace_number == screen().currentWorkspaceID()) {
        iconic = true;
        deiconify(false);
        // check if we should prevent this window from gaining focus
        if (!allowsFocusFromClient() || Fluxbox::instance()->isStartup())
            m_focused = false;
    }

    if (fullscreen) {
        fullscreen = false;
        setFullscreen(true);
    }

    if (maximized) {
        int tmp = maximized;
        maximized = MAX_NONE;
        setMaximizedState(tmp);
    }


    struct timeval now;
    gettimeofday(&now, NULL);
    m_creation_time = now.tv_sec;

    sendConfigureNotify();

    setupWindow();

    FbTk::App::instance()->sync(false);

}

/// attach a client to this window and destroy old window
void FluxboxWindow::attachClient(WinClient &client, int x, int y) {
    //!! TODO: check for isGroupable in client
    if (client.fbwindow() == this)
        return;

    menu().hide();

    // reparent client win to this frame
    frame().setClientWindow(client);
    bool was_focused = false;
    WinClient *focused_win = 0;

    // get the current window on the end of our client list
    Window leftwin = None;
    if (!clientList().empty())
        leftwin = clientList().back()->window();

    client.setGroupLeftWindow(leftwin);

    if (client.fbwindow() != 0) {
        FluxboxWindow *old_win = client.fbwindow(); // store old window

        if (FocusControl::focusedFbWindow() == old_win)
            was_focused = true;

        ClientList::iterator client_insert_pos = getClientInsertPosition(x, y);
        FbTk::TextButton *button_insert_pos = NULL;
        if (client_insert_pos != m_clientlist.end())
            button_insert_pos = m_labelbuttons[*client_insert_pos];

        // make sure we set new window search for each client
        ClientList::iterator client_it = old_win->clientList().begin();
        ClientList::iterator client_it_end = old_win->clientList().end();
        for (; client_it != client_it_end; ++client_it) {
            // reparent window to this
            frame().setClientWindow(**client_it);

            moveResizeClient(**client_it,
                             frame().clientArea().x(),
                             frame().clientArea().y(),
                             frame().clientArea().width(),
                             frame().clientArea().height());

            // create a labelbutton for this client and
            // associate it with the pointer
            associateClient(*(*client_it));

            //null if we want the new button at the end of the list
            if (x >= 0 && button_insert_pos)
                frame().moveLabelButtonLeftOf(*m_labelbuttons[*client_it], *button_insert_pos);

            (*client_it)->saveBlackboxAttribs(m_blackbox_attrib);
        }

        // add client and move over all attached clients
        // from the old window to this list
        m_clientlist.splice(client_insert_pos, old_win->m_clientlist);
        updateClientLeftWindow();
        old_win->m_client = 0;

        delete old_win;

    } else { // client.fbwindow() == 0
        associateClient(client);

        moveResizeClient(client,
                         frame().clientArea().x(),
                         frame().clientArea().y(),
                         frame().clientArea().width(),
                         frame().clientArea().height());

        // right now, this block only happens with new windows or on restart
        bool focus_new = screen().focusControl().focusNew();
        bool is_startup = Fluxbox::instance()->isStartup();

        // we use m_focused as a signal to focus the window when mapped
        if (focus_new && !is_startup)
            m_focused = true;
        focused_win = (focus_new || is_startup) ? &client : m_client;

        client.saveBlackboxAttribs(m_blackbox_attrib);
        m_clientlist.push_back(&client);
    }

    // make sure that the state etc etc is updated for the new client
    // TODO: one day these should probably be neatened to only act on the
    // affected clients if possible
    m_statesig.notify();
    m_workspacesig.notify();
    m_layersig.notify();

    if (was_focused) {
        // don't ask me why, but client doesn't seem to keep focus in new window
        // and we don't seem to get a FocusIn event from setInputFocus
        client.focus();
        FocusControl::setFocusedWindow(&client);
    } else {
        if (!focused_win)
            focused_win = screen().focusControl().lastFocusedWindow(*this);
        if (focused_win) {
            setCurrentClient(*focused_win, false);
            if (isIconic() && m_focused)
                deiconify();
        }
    }
    frame().reconfigure();
}


/// detach client from window and create a new window for it
bool FluxboxWindow::detachClient(WinClient &client) {
    if (client.fbwindow() != this || numClients() <= 1)
        return false;

    Window leftwin = None;
    ClientList::iterator client_it, client_it_after;
    client_it = client_it_after =
        find(clientList().begin(), clientList().end(), &client);

    if (client_it != clientList().begin())
        leftwin = (*(--client_it))->window();

    if (++client_it_after != clientList().end())
        (*client_it_after)->setGroupLeftWindow(leftwin);

    removeClient(client);
    screen().createWindow(client);
    return true;
}

void FluxboxWindow::detachCurrentClient() {
    // should only operate if we had more than one client
    if (numClients() <= 1)
        return;
    WinClient &client = *m_client;
    detachClient(*m_client);
    if (client.fbwindow() != 0)
        client.fbwindow()->show();
}

/// removes client from client list, does not create new fluxboxwindow for it
bool FluxboxWindow::removeClient(WinClient &client) {
    if (client.fbwindow() != this || numClients() == 0)
        return false;

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<")["<<this<<"]"<<endl;
#endif // DEBUG

    // if it is our active client, deal with it...
    if (m_client == &client) {
        WinClient *next_client = screen().focusControl().lastFocusedWindow(*this, m_client);
        if (next_client != 0)
            setCurrentClient(*next_client, false);
    }

    menu().hide();

    m_clientlist.remove(&client);

    if (m_client == &client) {
        if (m_clientlist.empty())
            m_client = 0;
        else
            // this really shouldn't happen
            m_client = m_clientlist.back();
    }

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(client.window());

    IconButton *label_btn = m_labelbuttons[&client];
    if (label_btn != 0) {
        frame().removeTab(label_btn);
        label_btn = 0;
    }

    m_labelbuttons.erase(&client);
    frame().reconfigure();
    updateClientLeftWindow();

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<")["<<this<<"] numClients = "<<numClients()<<endl;
#endif // DEBUG

    return true;
}

/// returns WinClient of window we're searching for
WinClient *FluxboxWindow::findClient(Window win) {
    ClientList::iterator it = find_if(clientList().begin(),
                                      clientList().end(),
                                      Compose(bind2nd(equal_to<Window>(), win),
                                              mem_fun(&WinClient::window)));
    return (it == clientList().end() ? 0 : *it);
}

/// raise and focus next client
void FluxboxWindow::nextClient() {
    if (numClients() <= 1)
        return;

    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(),
                                   m_client);
    if (it == m_clientlist.end())
        return;

    ++it;
    if (it == m_clientlist.end())
        it = m_clientlist.begin();

    setCurrentClient(**it, isFocused());
}

void FluxboxWindow::prevClient() {
    if (numClients() <= 1)
        return;

    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(),
                                   m_client);
    if (it == m_clientlist.end())
        return;

    if (it == m_clientlist.begin())
        it = m_clientlist.end();
    --it;

    setCurrentClient(**it, isFocused());
}


void FluxboxWindow::moveClientLeft() {
    if (m_clientlist.size() == 1 ||
        *m_clientlist.begin() == &winClient())
        return;

    // move client in clientlist to the left
    ClientList::iterator oldpos = find(m_clientlist.begin(), m_clientlist.end(), &winClient());
    ClientList::iterator newpos = oldpos; newpos--;
    swap(*newpos, *oldpos);
    frame().moveLabelButtonLeft(*m_labelbuttons[&winClient()]);

    updateClientLeftWindow();

}

void FluxboxWindow::moveClientRight() {
    if (m_clientlist.size() == 1 ||
        *m_clientlist.rbegin() == &winClient())
        return;

    ClientList::iterator oldpos = find(m_clientlist.begin(), m_clientlist.end(), &winClient());
    ClientList::iterator newpos = oldpos; newpos++;
    swap(*newpos, *oldpos);
    frame().moveLabelButtonRight(*m_labelbuttons[&winClient()]);

    updateClientLeftWindow();
}

//list<*WinClient>::iterator FluxboxWindow::getClientInsertPosition(int x, int y) {
FluxboxWindow::ClientList::iterator FluxboxWindow::getClientInsertPosition(int x, int y) {

    int dest_x = 0, dest_y = 0;
    Window labelbutton = 0;
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               parent().window(), frame().tabcontainer().window(),
                               x, y, &dest_x, &dest_y,
                               &labelbutton))
        return m_clientlist.end();

    Client2ButtonMap::iterator it =
        find_if(m_labelbuttons.begin(),
                m_labelbuttons.end(),
                Compose(bind2nd(equal_to<Window>(), labelbutton),
                        Compose(mem_fun(&TextButton::window),
                                Select2nd<Client2ButtonMap::value_type>())));


    // label button not found
    if (it == m_labelbuttons.end())
        return m_clientlist.end();

    Window child_return=0;
    // make x and y relative to our labelbutton
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               frame().tabcontainer().window(), labelbutton,
                               dest_x, dest_y, &x, &y,
                               &child_return))
        return m_clientlist.end();

    ClientList::iterator client = find(m_clientlist.begin(),
                                       m_clientlist.end(),
                                       it->first);
    if (x > static_cast<signed>((*it).second->width()) / 2)
        client++;

    return client;

}



void FluxboxWindow::moveClientTo(WinClient &win, int x, int y) {
    int dest_x = 0, dest_y = 0;
    Window labelbutton = 0;
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               parent().window(), frame().tabcontainer().window(),
                               x, y, &dest_x, &dest_y,
                               &labelbutton))
        return;

    Client2ButtonMap::iterator it =
        find_if(m_labelbuttons.begin(),
                m_labelbuttons.end(),
                Compose(bind2nd(equal_to<Window>(), labelbutton),
                        Compose(mem_fun(&TextButton::window),
                                Select2nd<Client2ButtonMap::value_type>())));

    // label button not found
    if (it == m_labelbuttons.end())
        return;

    Window child_return = 0;
    //make x and y relative to our labelbutton
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               frame().tabcontainer().window(), labelbutton,
                               dest_x, dest_y, &x, &y,
                               &child_return))
        return;
    if (x > static_cast<signed>((*it).second->width()) / 2)
        moveClientRightOf(win, *it->first);
    else
        moveClientLeftOf(win, *it->first);

}


void FluxboxWindow::moveClientLeftOf(WinClient &win, WinClient &dest) {

	frame().moveLabelButtonLeftOf(*m_labelbuttons[&win], *m_labelbuttons[&dest]);

	ClientList::iterator it = find(m_clientlist.begin(),
                                  m_clientlist.end(),
                                  &win);
	ClientList::iterator new_pos = find(m_clientlist.begin(),
				       m_clientlist.end(),
				       &dest);

	// make sure we found them
	if (it == m_clientlist.end() || new_pos==m_clientlist.end()) {
		return;
	}
	//moving a button to the left of itself results in no change
	if( new_pos == it) {
		return;
	}
	//remove from list
	m_clientlist.erase(it);
	//insert on the new place
	m_clientlist.insert(new_pos, &win);

	updateClientLeftWindow();
}


void FluxboxWindow::moveClientRightOf(WinClient &win, WinClient &dest) {
    frame().moveLabelButtonRightOf(*m_labelbuttons[&win], *m_labelbuttons[&dest]);

    ClientList::iterator it = find(m_clientlist.begin(),
                                   m_clientlist.end(),
                                   &win);
    ClientList::iterator new_pos = find(m_clientlist.begin(),
                                        m_clientlist.end(),
                                        &dest);

    // make sure we found them
    if (it == m_clientlist.end() || new_pos==m_clientlist.end())
        return;

    //moving a button to the right of itself results in no change
    if (new_pos == it)
        return;

    //remove from list
    m_clientlist.erase(it);
    //need to insert into the next position
    new_pos++;
    //insert on the new place
    if (new_pos == m_clientlist.end())
        m_clientlist.push_back(&win);
    else
        m_clientlist.insert(new_pos, &win);

    updateClientLeftWindow();
}

/// Update LEFT window atom on all clients.
void FluxboxWindow::updateClientLeftWindow() {
    if (clientList().empty())
        return;

    // It should just update the affected clients but that
    // would require more complex code and we're assuming
    // the user dont have alot of windows grouped so this
    // wouldn't be too time consuming and it's easier to
    // implement.
    ClientList::iterator it = clientList().begin();
    ClientList::iterator it_end = clientList().end();
    // set no left window on first tab
    (*it)->setGroupLeftWindow(0);
    WinClient *last_client = *it;
    ++it;
    for (; it != it_end; ++it) {
        (*it)->setGroupLeftWindow(last_client->window());
        last_client = *it;
    }
}

bool FluxboxWindow::setCurrentClient(WinClient &client, bool setinput) {
    // make sure it's in our list
    if (client.fbwindow() != this)
        return false;

    IconButton *button = m_labelbuttons[&client];
    // in case the window is being destroyed, but this should never happen
    if (!button)
        return false;

    WinClient *old = m_client;
    m_client = &client;
    m_client->raise();
    m_client->focusSig().notify();
    titleSig().notify();

#ifdef DEBUG
    cerr<<"FluxboxWindow::"<<__FUNCTION__<<": labelbutton[client] = "<<
        button<<endl;
#endif // DEBUG
    // frame focused doesn't necessarily mean input focused
    frame().setLabelButtonFocus(*button);
    frame().setShapingClient(&client, false);

    bool ret = setinput && focus();
    if (setinput)
        // restore old client until focus event comes
        m_client = old;
    return ret;
}

bool FluxboxWindow::isGroupable() const {
    if (isResizable() && isMaximizable() && !winClient().isTransient())
        return true;
    return false;
}

void FluxboxWindow::associateClientWindow(bool use_attrs,
                                          int x, int y,
                                          unsigned int width, unsigned int height,
                                          int gravity, unsigned int client_bw) {
    m_client->updateTitle();

    frame().setShapingClient(m_client, false);

    if (use_attrs)
        frame().moveResizeForClient(x, y,
                                    width, height, gravity, client_bw);
    else
        frame().resizeForClient(m_client->width(), m_client->height());

    frame().setActiveGravity(m_client->gravity(), m_client->old_bw);
    frame().setClientWindow(*m_client);
}


void FluxboxWindow::grabButtons() {

    // needed for click to focus
    XGrabButton(display, Button1, AnyModifier,
                frame().window().window(), True, ButtonPressMask,
                GrabModeSync, GrabModeSync, None, None);
    XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask,
                  frame().window().window());

}


void FluxboxWindow::reconfigure() {

    applyDecorations();

    setFocusFlag(m_focused);

    moveResize(frame().x(), frame().y(), frame().width(), frame().height());

    m_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay());

    updateButtons();
    frame().reconfigure();

    menu().reconfigure();

    Client2ButtonMap::iterator it = m_labelbuttons.begin(),
                               it_end = m_labelbuttons.end();
    for (; it != it_end; ++it)
        it->second->setPixmap(screen().getTabsUsePixmap());

}

/// update current client title and title in our frame
void FluxboxWindow::updateTitleFromClient(WinClient &client) {
    if (&client == m_client) {
        frame().setFocusTitle(client.title());
        titleSig().notify();
    }
}

void FluxboxWindow::updateMWMHintsFromClient(WinClient &client) {
    const WinClient::MwmHints *hint = client.getMwmHint();

    if (!hint) return;

    if (!m_toggled_decos && hint->flags & MwmHintsDecorations) {
        if (hint->decorations & MwmDecorAll) {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.close = decorations.menu = true;
        } else {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.close = decorations.tab = false;
            decorations.menu = true;
            if (hint->decorations & MwmDecorBorder)
                decorations.border = true;
            if (hint->decorations & MwmDecorHandle)
                decorations.handle = true;
            if (hint->decorations & MwmDecorTitle) {
                //only tab on windows with titlebar
                decorations.titlebar = decorations.tab = true;
            }
            if (hint->decorations & MwmDecorMenu)
                decorations.menu = true;
            if (hint->decorations & MwmDecorIconify)
                decorations.iconify = true;
            if (hint->decorations & MwmDecorMaximize)
                decorations.maximize = true;
        }
    }

    // functions.tabable is ours, not special one
    // note that it means this window is "tabbable"
    if (hint->flags & MwmHintsFunctions) {
        if (hint->functions & MwmFuncAll) {
            functions.resize = functions.move = functions.iconify =
                functions.maximize = functions.close = true;
        } else {
            functions.resize = functions.move = functions.iconify =
                functions.maximize = functions.close = false;

            if (hint->functions & MwmFuncResize)
                functions.resize = true;
            if (hint->functions & MwmFuncMove)
                functions.move = true;
            if (hint->functions & MwmFuncIconify)
                functions.iconify = true;
            if (hint->functions & MwmFuncMaximize)
                functions.maximize = true;
            if (hint->functions & MwmFuncClose)
                functions.close = true;
        }
    }
}

void FluxboxWindow::updateRememberStateFromClient(WinClient &client) {
#ifdef REMEMBER
    Remember* rem= const_cast<Remember*>(static_cast<const Remember*>(Fluxbox::instance()->getAtomHandler("remember")));
    Application* app= 0;
    if ( rem && (app= (const_cast<Remember*>(rem))->find(client)) ) {
        if ( !m_toggled_decos && rem->isRemembered(client, Remember::REM_DECOSTATE) )
            setDecorationMask(app->decostate);
    }
#endif // REMEMBER
}

void FluxboxWindow::updateFunctions() {
    if (!m_client)
        return;
    bool changed = false;
    if (m_client->isClosable() != functions.close) {
        functions.close = m_client->isClosable();
        changed = true;
    }

    if (changed)
        setupWindow();
}

void FluxboxWindow::move(int x, int y) {
    moveResize(x, y, frame().width(), frame().height());
}

void FluxboxWindow::resize(unsigned int width, unsigned int height) {
    // don't set window as placed, since we're only resizing
    bool placed = m_placed;
    moveResize(frame().x(), frame().y(), width, height);
    m_placed = placed;
}

// send_event is just an override
void FluxboxWindow::moveResize(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height,
                               bool send_event) {

    m_placed = true;
    send_event = send_event || frame().x() != new_x || frame().y() != new_y;

    if ((new_width != frame().width() || new_height != frame().height()) &&
        isResizable() && !isShaded()) {

        if ((((signed) frame().width()) + new_x) < 0)
            new_x = 0;
        if ((((signed) frame().height()) + new_y) < 0)
            new_y = 0;

        frame().moveResize(new_x, new_y, new_width, new_height);
        setFocusFlag(m_focused);

        send_event = true;
    } else if (send_event)
        frame().move(new_x, new_y);

    if (send_event && ! moving) {
        sendConfigureNotify();
    }


    if (!moving) {
        m_last_resize_x = new_x;
        m_last_resize_y = new_y;
    }

}

void FluxboxWindow::moveResizeForClient(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height, int gravity, unsigned int client_bw) {

    m_placed = true;
    frame().moveResizeForClient(new_x, new_y, new_width, new_height, gravity, client_bw);
    setFocusFlag(m_focused);
    shaded = false;
    sendConfigureNotify();

    if (!moving) {
        m_last_resize_x = new_x;
        m_last_resize_y = new_y;
    }

}

void FluxboxWindow::maxSize(unsigned int &max_width, unsigned int &max_height) {
    ClientList::const_iterator it = clientList().begin();
    ClientList::const_iterator it_end = clientList().end();
    max_width = (unsigned int) ~0; // unlimited
    max_height = (unsigned int) ~0; // unlimited
    for (; it != it_end; ++it) {
        // special case for max height/width == 0
        // 0 indicates unlimited size, so we skip them
        // and set max size to 0 if max size == ~0 after the loop
        if ((*it)->maxHeight() != 0)
            max_height = std::min( (*it)->maxHeight(), max_height );
        if ((*it)->maxWidth() != 0)
            max_width = std::min( (*it)->maxWidth(), max_width );
    }

    if (max_width == (unsigned int) ~0)
        max_width = 0;
    if (max_height == (unsigned int) ~0)
        max_height = 0;
}

// returns whether the focus was "set" to this window
// it doesn't guarantee that it has focus, but says that we have
// tried. A FocusIn event should eventually arrive for that
// window if it actually got the focus, then setFocusFlag is called,
// which updates all the graphics etc
bool FluxboxWindow::focus() {

    if (((signed) (frame().x() + frame().width())) < 0) {
        if (((signed) (frame().y() + frame().height())) < 0) {
            moveResize(frame().window().borderWidth(), frame().window().borderWidth(),
                       frame().width(), frame().height());
        } else if (frame().y() > (signed) screen().height()) {
            moveResize(frame().window().borderWidth(), screen().height() - frame().height(),
                       frame().width(), frame().height());
        } else {
            moveResize(frame().window().borderWidth(), frame().y() + frame().window().borderWidth(),
                       frame().width(), frame().height());
        }
    } else if (frame().x() > (signed) screen().width()) {
        if (((signed) (frame().y() + frame().height())) < 0) {
            moveResize(screen().width() - frame().width(), frame().window().borderWidth(),
                       frame().width(), frame().height());
        } else if (frame().y() > (signed) screen().height()) {
            moveResize(screen().width() - frame().width(),
                       screen().height() - frame().height(),
                       frame().width(), frame().height());
        } else {
            moveResize(screen().width() - frame().width(),
                       frame().y() + frame().window().borderWidth(),
                       frame().width(), frame().height());
        }
    }

    if (! m_client->validateClient())
        return false;

    if (screen().currentWorkspaceID() != workspaceNumber() && !isStuck()) {

        BScreen::FollowModel model = screen().getUserFollowModel();
        if (model == BScreen::IGNORE_OTHER_WORKSPACES)
            return false;

        // fetch the window to the current workspace
        if (model == BScreen::FETCH_ACTIVE_WINDOW ||
            (isIconic() && model == BScreen::SEMIFOLLOW_ACTIVE_WINDOW))
            screen().sendToWorkspace(screen().currentWorkspaceID(), this, false);
        // warp to the workspace of the window
        else
            screen().changeWorkspaceID(workspaceNumber(), false);
    }

    if (isIconic()) {
        deiconify();
        m_focused = true; // signal to mapNotifyEvent to set focus when mapped
        return true; // the window probably will get focused, just not yet
    }

    // this needs to be here rather than setFocusFlag because
    // FocusControl::revertFocus will return before FocusIn events arrive
    m_screen.focusControl().setScreenFocusedWindow(*m_client);

#ifdef DEBUG
    cerr<<"FluxboxWindow::"<<__FUNCTION__<<" isModal() = "<<m_client->isModal()<<endl;
    cerr<<"FluxboxWindow::"<<__FUNCTION__<<" transient size = "<<m_client->transients.size()<<endl;
#endif // DEBUG
    if (!m_client->transients.empty() && m_client->isModal()) {
#ifdef DEBUG
        cerr<<__FUNCTION__<<": isModal and have transients client = "<<
            hex<<m_client->window()<<dec<<endl;
        cerr<<__FUNCTION__<<": this = "<<this<<endl;
#endif // DEBUG

        WinClient::TransientList::iterator it = m_client->transients.begin();
        WinClient::TransientList::iterator it_end = m_client->transients.end();
        for (; it != it_end; ++it) {
#ifdef DEBUG
            cerr<<__FUNCTION__<<": transient 0x"<<(*it)<<endl;
#endif // DEBUG
            if ((*it)->isStateModal())
                return (*it)->focus();
        }
    }

    if (m_client->isModal())
        return false;

    return m_client->sendFocus();
}

// don't hide the frame directly, use this function
void FluxboxWindow::hide(bool interrupt_moving) {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<")["<<this<<"]"<<endl;
#endif // DEBUG
    // resizing always stops on hides
    if (resizing)
        stopResizing(true);

    if (interrupt_moving) {
        if (moving)
            stopMoving(true);
        if (m_attaching_tab)
            attachTo(0, 0, true);
    }

    setState(IconicState, false);

    menu().hide();
    frame().hide();

    if (FocusControl::focusedFbWindow() == this)
        FocusControl::setFocusedWindow(0);
}

void FluxboxWindow::show() {
    frame().show();
    setState(NormalState, false);
}

void FluxboxWindow::toggleIconic() {
    if (isIconic())
        deiconify();
    else
        iconify();
}

/**
   Unmaps the window and removes it from workspace list
*/
void FluxboxWindow::iconify() {
    if (isIconic()) // no need to iconify if we're already
        return;

    iconic = true;
    m_statesig.notify();

    hide(true);

    screen().focusControl().setFocusBack(*this);

    ClientList::iterator client_it = m_clientlist.begin();
    const ClientList::iterator client_it_end = m_clientlist.end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient &client = *(*client_it);
        if (client.transientFor() &&
            client.transientFor()->fbwindow()) {
            if (!client.transientFor()->fbwindow()->isIconic()) {
                client.transientFor()->fbwindow()->iconify();
            }
        }

        if (!client.transientList().empty()) {
            WinClient::TransientList::iterator it = client.transientList().begin();
            WinClient::TransientList::iterator it_end = client.transientList().end();
            for (; it != it_end; it++)
                if ((*it)->fbwindow())
                    (*it)->fbwindow()->iconify();
        }
    }

    // focus revert is done elsewhere (based on signal)
}

void FluxboxWindow::deiconify(bool reassoc, bool do_raise) {
    if (numClients() == 0)
        return;

    if (oplock) return;
    oplock = true;

    if (iconic || reassoc) {
        screen().reassociateWindow(this, screen().currentWorkspace()->workspaceID(), false);
    } else if (moving || m_workspace_number != screen().currentWorkspace()->workspaceID()) {
        oplock = false;
        return;
    }

    bool was_iconic = iconic;

    iconic = false;
    m_statesig.notify();

    if (reassoc && !m_client->transients.empty()) {
        // deiconify all transients
        ClientList::iterator client_it = clientList().begin();
        ClientList::iterator client_it_end = clientList().end();
        for (; client_it != client_it_end; ++client_it) {
            //TODO: Can this get stuck in a loop?
            WinClient::TransientList::iterator trans_it =
                (*client_it)->transientList().begin();
            WinClient::TransientList::iterator trans_it_end =
                (*client_it)->transientList().end();
            for (; trans_it != trans_it_end; ++trans_it) {
                if ((*trans_it)->fbwindow())
                    (*trans_it)->fbwindow()->deiconify(true, false);
            }
        }
    }

    show();

    // focus new, OR if it's the only window on the workspace
    // but not on startup: focus will be handled after creating everything
    // we use m_focused as a signal to focus the window when mapped
    if (was_iconic && (screen().currentWorkspace()->numberOfWindows() == 1 ||
        screen().focusControl().focusNew() || m_client->isTransient()))
        m_focused = true;

    oplock = false;

    if (do_raise)
        raise();
}

/** setFullscreen mode:

    - maximize as big as the screen is, dont care about slit / toolbar
    - raise to toplayer
*/
void FluxboxWindow::setFullscreen(bool flag) {

    if (!m_initialized) {
        // this will interfere with window placement, so we delay it
        fullscreen = flag;
        return;
    }

    const int head = screen().getHead(fbWindow());

    if (flag && !isFullscreen()) {

        if (isShaded())
            shade();

        frame().setUseShape(false);

        if (!m_toggled_decos)
            m_old_decoration_mask = decorationMask();

        m_old_layernum = layerNum();
        if (!maximized) {
            m_old_pos_x = frame().x();
            m_old_pos_y = frame().y();
            m_old_width = frame().width();
            m_old_height = frame().height();
        }

        // clear decorations
        setDecorationMask(0);

        // dont call Window::moveResize here, it might ignore the 
        // resize if win state is not resizable; 
        // instead we call frame resize directly
        // (see tests/fullscreentest.cc)

        // be xinerama aware
        frame().moveResize(screen().getHeadX(head), screen().getHeadY(head),
                           screen().getHeadWidth(head), screen().getHeadHeight(head));
        sendConfigureNotify();
        m_last_resize_x = frame().x();
        m_last_resize_y = frame().y();

        moveToLayer(::Layer::ABOVE_DOCK);

        fullscreen = true;

        stateSig().notify();

    } else if (!flag && isFullscreen()) {

        fullscreen = false;

        frame().setUseShape(true);
        if (m_toggled_decos) {
            if (m_old_decoration_mask & (DECORM_TITLEBAR | DECORM_TAB))
                setDecorationMask(DECOR_NONE);
            else
                setDecorationMask(DECOR_NORMAL);
        } else
            setDecorationMask(m_old_decoration_mask);

        // ensure we apply the sizehints here, otherwise some
        // apps (eg xterm) end up a little bit .. crappy (visually)
        m_last_resize_x = m_old_pos_x;
        m_last_resize_y = m_old_pos_y;
        m_last_resize_w = m_old_width;
        m_last_resize_h = m_old_height;
        m_resize_corner = NOCORNER;
        fixsize();

        moveResize(m_last_resize_x, m_last_resize_y, m_last_resize_w, m_last_resize_h);
        moveToLayer(m_old_layernum);

        m_old_layernum = ::Layer::NORMAL;

        if (maximized) {
            int tmp = maximized;
            maximized = MAX_NONE;
            setMaximizedState(tmp);
        } else
            stateSig().notify();
    }
}

/**
   Maximize window both horizontal and vertical
*/
void FluxboxWindow::maximize(int type) {

    // nothing to do
    if (type == MAX_NONE)
        return;

    int new_max = maximized;

    // toggle maximize vertically?
    // when _don't_ we want to toggle?
    // - type is horizontal maximise, or
    // - type is full and we are not maximised horz but already vertically
    if (type != MAX_HORZ && !(type == MAX_FULL && maximized == MAX_VERT))
        new_max ^= MAX_VERT;

    // maximize horizontally?
    if (type != MAX_VERT && !(type == MAX_FULL && maximized == MAX_HORZ))
        new_max ^= MAX_HORZ;

    setMaximizedState(new_max);
}

void FluxboxWindow::setMaximizedState(int type) {

    if (!m_initialized || isFullscreen() || type == maximized) {
        // this will interfere with window placement, so we delay it
        maximized = type;
        return;
    }

    if (isShaded())
        shade();

    if (isResizing())
        stopResizing();

    int head = screen().getHead(frame().window());
    int new_x = frame().x(),
        new_y = frame().y(),
        new_w = frame().width(),
        new_h = frame().height();

    // These evaluate whether we need to TOGGLE the value for that field
    // Why? If maximize is only set to zero outside this,
    // and we only EVER toggle them, then:
    // 1) We will never loose the old_ values
    // 2) It shouldn't get confused

    // Worst case being that some action will toggle the wrong way, but
    // we still won't lose the state in that case.

    // toggle maximize vertically?
    if ((maximized ^ type) & MAX_VERT) {
        // already maximized in that direction?
        if (maximized & MAX_VERT) {
            new_y = m_old_pos_y;
            new_h = m_old_height;
        } else {
            m_old_pos_y  = new_y;
            m_old_height = new_h;
            new_y = screen().maxTop(head);
            new_h = screen().maxBottom(head) - new_y - 2*frame().window().borderWidth();
            if (!screen().getMaxOverTabs()) {
                new_y += yOffset();
                new_h -= heightOffset();
            }
        }
        maximized ^= MAX_VERT;
    }

    // toggle maximize horizontally?
    if ((maximized ^ type) & MAX_HORZ) {
        // already maximized in that direction?
        if (maximized & MAX_HORZ) {
            new_x = m_old_pos_x;
            new_w = m_old_width;
        } else {
            // only save if we weren't already maximized
            m_old_pos_x = new_x;
            m_old_width = new_w;
            new_x = screen().maxLeft(head);
            new_w = screen().maxRight(head) - new_x - 2*frame().window().borderWidth();
            if (!screen().getMaxOverTabs()) {
                new_x += xOffset();
                new_w -= widthOffset();
            }
        }
        maximized ^= MAX_HORZ;
    }

    // ensure we apply the sizehints here, otherwise some
    // apps (eg xterm) end up a little bit .. crappy (visually)
    m_last_resize_x = new_x;
    m_last_resize_y = new_y;
    m_last_resize_w = new_w;
    m_last_resize_h = new_h;

    // frankly, that xterm bug was pretty obscure, and it's really annoying not
    // being able to maximize my terminals, so we make an option
    // but we do fix size hints when restoring the window to normal size
    if (!screen().getMaxIgnoreIncrement() || !maximized) {
        ResizeDirection old_resize_corner = m_resize_corner;
        m_resize_corner = NOCORNER;
        fixsize(0, 0, (maximized ? true : false));
        m_resize_corner = old_resize_corner;
    }

    moveResize(m_last_resize_x, m_last_resize_y, m_last_resize_w, m_last_resize_h);

    // notify listeners that we changed state
    stateSig().notify();
}

/**
 * Maximize window horizontal
 */
void FluxboxWindow::maximizeHorizontal() {
    maximize(MAX_HORZ);
}

/**
 * Maximize window vertical
 */
void FluxboxWindow::maximizeVertical() {
    maximize(MAX_VERT);
}

/**
 * Maximize window fully
 */
void FluxboxWindow::maximizeFull() {
    maximize(MAX_FULL);
}

void FluxboxWindow::setWorkspace(int n) {
    unsigned int old_wkspc = m_workspace_number;

    m_workspace_number = n;

    // notify workspace change
    if (m_initialized && !stuck && old_wkspc != m_workspace_number) {
#ifdef DEBUG
        cerr<<this<<" notify workspace signal"<<endl;
#endif // DEBUG
        m_workspacesig.notify();
    }
}

void FluxboxWindow::setLayerNum(int layernum) {
    m_layernum = layernum;

    m_blackbox_attrib.flags |= ATTRIB_STACK;
    m_blackbox_attrib.stack = layernum;

    if (m_initialized) {
        saveBlackboxAttribs();

#ifdef DEBUG
        cerr<<this<<" notify layer signal"<<endl;
#endif // DEBUG

        m_layersig.notify();
    }
}

void FluxboxWindow::shade() {
    // we can only shade if we have a titlebar
    if (!decorations.titlebar)
        return;

    // we're toggling, so if they're equal now, we need to change it
    if (m_initialized && m_frame.isShaded() == shaded)
        frame().shade();

    shaded = !shaded;

    // TODO: this should set IconicState, but then we can't focus the window
}

void FluxboxWindow::shadeOn() {

    if (!shaded)
        shade();

}

void FluxboxWindow::shadeOff() {

    if (shaded)
        shade();

}

void FluxboxWindow::stick() {

    stuck = !stuck;

    if (m_initialized) {
        stateSig().notify();
        // notify since some things consider "stuck" to be a pseudo-workspace
        m_workspacesig.notify();
    }

    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_end = clientList().end();
    for (; client_it != client_it_end; ++client_it) {

        WinClient::TransientList::const_iterator it = (*client_it)->transientList().begin();
        WinClient::TransientList::const_iterator it_end = (*client_it)->transientList().end();
        for (; it != it_end; ++it) {
            if ((*it)->fbwindow() && (*it)->fbwindow()->isStuck() != stuck)
                (*it)->fbwindow()->stick();
        }

    }

}


void FluxboxWindow::raise() {
    if (isIconic())
        deiconify();
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::raise()[layer="<<layerNum()<<"]"<<endl;
#endif // DEBUG
    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we have transient_for then we should put ourself last in
    // transients list so we get raised last and thus gets above the other transients
    if (m_client->transientFor() && m_client != m_client->transientFor()->transientList().back()) {
        // remove and push back so this window gets raised last
        m_client->transientFor()->transientList().remove(m_client);
        m_client->transientFor()->transientList().push_back(m_client);
    }
    // raise this window and every transient in it with this one last
    if (client->fbwindow()) {
        // doing this on startup messes up the focus order
        if (!Fluxbox::instance()->isStartup() && client->fbwindow() != this &&
                &client->fbwindow()->winClient() != client)
            // activate the client so the transient won't get pushed back down
            client->fbwindow()->setCurrentClient(*client, false);
        raiseFluxboxWindow(*client->fbwindow());
    }

}

void FluxboxWindow::lower() {
    if (isIconic())
        deiconify();
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::lower()"<<endl;
#endif // DEBUG
    // get root window
    WinClient *client = getRootTransientFor(m_client);

    if (client->fbwindow())
        lowerFluxboxWindow(*client->fbwindow());
}

void FluxboxWindow::tempRaise() {
    // Note: currently, this causes a problem with cycling through minimized
    // clients if this window has more than one tab, since the window will not
    // match isIconic() when the rest of the tabs get checked
    if (isIconic())
        deiconify();

    // the root transient will get raised when we stop cycling
    // raising it here causes problems when it isn't the active tab
    tempRaiseFluxboxWindow(*this);
}


void FluxboxWindow::raiseLayer() {
    moveToLayer(m_layernum-1);
}

void FluxboxWindow::lowerLayer() {
    moveToLayer(m_layernum+1);
}


void FluxboxWindow::moveToLayer(int layernum, bool force) {
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::moveToLayer("<<layernum<<")"<<endl;
#endif // DEBUG

    // don't let it set its layer into menu area
    if (layernum <= ::Layer::MENU)
        layernum = ::Layer::MENU + 1;
    else if (layernum >= ::Layer::NUM_LAYERS)
        layernum = ::Layer::NUM_LAYERS - 1;

    if (!m_initialized)
        m_layernum = layernum;

    if (m_layernum == layernum && !force || !m_client)
        return;

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    FluxboxWindow *win = client->fbwindow();
    if (!win) return;

    win->layerItem().moveToLayer(layernum);
    // remember number just in case a transient happens to revisit this window
    layernum = win->layerItem().getLayerNum();
    win->setLayerNum(layernum);

    // move all the transients, too
    ClientList::iterator client_it = win->clientList().begin();
    ClientList::iterator client_it_end = win->clientList().end();
    for (; client_it != client_it_end; ++client_it) {

        WinClient::TransientList::const_iterator it = (*client_it)->transientList().begin();
        WinClient::TransientList::const_iterator it_end = (*client_it)->transientList().end();
        for (; it != it_end; ++it) {
            FluxboxWindow *fbwin = (*it)->fbwindow();
            if (fbwin && !fbwin->isIconic()) {
                fbwin->layerItem().moveToLayer(layernum);
                fbwin->setLayerNum(layernum);
            }
        }

    }

}

void FluxboxWindow::setFocusHidden(bool value) {
    m_focus_hidden = value;
    if (m_initialized)
        m_statesig.notify();
}

void FluxboxWindow::setIconHidden(bool value) {
    m_icon_hidden= value;
    if (m_initialized)
        m_statesig.notify();
}


// window has actually RECEIVED focus (got a FocusIn event)
// so now we make it a focused frame etc
void FluxboxWindow::setFocusFlag(bool focus) {
    if (!m_client) return;

    bool was_focused = isFocused();
    m_focused = focus;
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::setFocusFlag("<<focus<<")"<<endl;
#endif // DEBUG

    installColormap(focus);

    if (fullscreen && !focus)
        moveToLayer(m_old_layernum);
    if (fullscreen && focus)
        moveToLayer(::Layer::ABOVE_DOCK);

    if (focus != frame().focused())
        frame().setFocus(focus);

    if (focus && screen().focusControl().isCycling())
        tempRaise();
    else if (screen().doAutoRaise()) {
        if (m_focused)
            m_timer.start();
        else
            m_timer.stop();
    }

    // did focus change? notify listeners
    if (was_focused != focus) {
        m_attention_state = false;
        m_focussig.notify();
        if (m_client)
            m_client->focusSig().notify();
        WindowCmd<void>::setClient(m_client);
        Fluxbox::instance()->keys()->doAction(focus ? FocusIn : FocusOut, 0, 0,
                                              Keys::ON_WINDOW);
    }
}


void FluxboxWindow::installColormap(bool install) {
    if (m_client == 0) return;

    Fluxbox *fluxbox = Fluxbox::instance();
    fluxbox->grab();
    if (! m_client->validateClient())
        return;

    int i = 0, ncmap = 0;
    Colormap *cmaps = XListInstalledColormaps(display, m_client->window(), &ncmap);
    XWindowAttributes wattrib;
    if (cmaps) { //!!
        if (m_client->getAttrib(wattrib)) {
            if (install) {
                // install the window's colormap
                for (i = 0; i < ncmap; i++) {
                    if (*(cmaps + i) == wattrib.colormap) {
                        // this window is using an installed color map... do not install
                        install = false;
                        break; //end for-loop (we dont need to check more)
                    }
                }
                // otherwise, install the window's colormap
                if (install)
                    XInstallColormap(display, wattrib.colormap);
            } else {
                for (i = 0; i < ncmap; i++) { // uninstall the window's colormap
                    if (*(cmaps + i) == wattrib.colormap)
                       XUninstallColormap(display, wattrib.colormap);
                }
            }
        }

        XFree(cmaps);
    }

    fluxbox->ungrab();
}

/**
 Saves blackbox attributes for every client in our list
 */
void FluxboxWindow::saveBlackboxAttribs() {
    for_each(m_clientlist.begin(), m_clientlist.end(),
             FbTk::ChangeProperty(
                 display,
                 FbAtoms::instance()->getFluxboxAttributesAtom(),
                 PropModeReplace,
                 (unsigned char *)&m_blackbox_attrib,
                 PropBlackboxAttributesElements
                 ));
}

/**
 Sets state on each client in our list
 Use setting_up for setting startup state - it may not be committed yet
 That'll happen when its mapped
 */
void FluxboxWindow::setState(unsigned long new_state, bool setting_up) {
    m_current_state = new_state;
    if (numClients() == 0 || setting_up)
        return;

    unsigned long state[2];
    state[0] = (unsigned long) m_current_state;
    state[1] = (unsigned long) None;

    for_each(m_clientlist.begin(), m_clientlist.end(),
             FbTk::ChangeProperty(display,
                                  FbAtoms::instance()->getWMStateAtom(),
                                  PropModeReplace,
                                  (unsigned char *)state, 2));

    ClientList::iterator it = clientList().begin();
    ClientList::iterator it_end = clientList().end();
    for (; it != it_end; ++it) {
        (*it)->setEventMask(NoEventMask);
        if (new_state == IconicState)
            (*it)->hide();
        else if (new_state == NormalState)
            (*it)->show();
        (*it)->setEventMask(PropertyChangeMask | StructureNotifyMask | FocusChangeMask | KeyPressMask);
    }
}

bool FluxboxWindow::getState() {

    Atom atom_return;
    bool ret = false;
    int foo;
    unsigned long *state, ulfoo, nitems;
    if (!m_client->property(FbAtoms::instance()->getWMStateAtom(),
                             0l, 2l, false, FbAtoms::instance()->getWMStateAtom(),
                             &atom_return, &foo, &nitems, &ulfoo,
                             (unsigned char **) &state) || !state)
        return false;

    if (nitems >= 1) {
        m_current_state = static_cast<unsigned long>(state[0]);
        ret = true;
    }

    XFree(static_cast<void *>(state));

    return ret;
}

/**
 * Sets the attributes to what they should be
 * but doesn't change the actual state
 * (so the caller can set defaults etc as well)
 */
void FluxboxWindow::restoreAttributes() {
    if (!getState()) {
        m_current_state = m_client->initial_state;
        if (m_current_state == IconicState)
            iconic = true;
    }

    Atom atom_return;
    int foo;
    unsigned long ulfoo, nitems;
    FbAtoms *fbatoms = FbAtoms::instance();

    BlackboxAttributes *net;
    if (m_client->property(fbatoms->getFluxboxAttributesAtom(), 0l,
                           PropBlackboxAttributesElements, false,
                           fbatoms->getFluxboxAttributesAtom(), &atom_return, &foo,
                           &nitems, &ulfoo, (unsigned char **) &net) &&
        net) {
        if (nitems != (unsigned)PropBlackboxAttributesElements) {
            XFree(net);
            return;
        }
        m_blackbox_attrib.flags = net->flags;
        m_blackbox_attrib.attrib = net->attrib;
        m_blackbox_attrib.workspace = net->workspace;
        m_blackbox_attrib.stack = net->stack;
        m_blackbox_attrib.premax_x = net->premax_x;
        m_blackbox_attrib.premax_y = net->premax_y;
        m_blackbox_attrib.premax_w = net->premax_w;
        m_blackbox_attrib.premax_h = net->premax_h;

        XFree(static_cast<void *>(net));
    } else
        return;

    if (m_blackbox_attrib.flags & ATTRIB_STACK) {
        //!! TODO check value?
        m_layernum = m_blackbox_attrib.stack;
    }

}

/**
   Show the window menu at pos mx, my
*/
void FluxboxWindow::showMenu(int menu_x, int menu_y) {
    // move menu directly under titlebar

    int head = screen().getHead(menu_x, menu_y);

    // but not off the screen
    if (menu_y < static_cast<signed>(screen().maxTop(head)))
        menu_y = screen().maxTop(head);
    else if (menu_y + menu().height() >= screen().maxBottom(head))
        menu_y = screen().maxBottom(head) - menu().height() - 1 - menu().fbwindow().borderWidth();

    if (menu_x < static_cast<signed>(screen().maxLeft(head)))
        menu_x = screen().maxLeft(head);
    else if (menu_x + static_cast<signed>(menu().width()) >= static_cast<signed>(screen().maxRight(head)))
        menu_x = screen().maxRight(head) - menu().width() - 1;

    WindowCmd<void>::setWindow(this);
    menu().move(menu_x, menu_y);
    menu().show();
    menu().raise();
    menu().grabInputFocus();
}

/**
   Moves the menu to last button press position and shows it,
   if it's already visible it'll be hidden
 */
void FluxboxWindow::popupMenu() {

    // hide menu if it was opened for this window before
    if (menu().isVisible() && WindowCmd<void>::window() == this) {
        menu().hide();
        return;
    }

    menu().disableTitle();
    int menu_y = frame().titlebar().height() + frame().titlebar().borderWidth();
    if (!decorations.titlebar) // if we don't have any titlebar
        menu_y = 0;
    if (m_last_button_x < x() || m_last_button_x > x() + static_cast<signed>(width()))
        m_last_button_x = x();
    showMenu(m_last_button_x, menu_y + frame().y());
}


/**
   Redirect any unhandled event to our handlers
*/
void FluxboxWindow::handleEvent(XEvent &event) {
    switch (event.type) {
    case ConfigureRequest:
#ifdef DEBUG
        cerr<<"ConfigureRequest("<<title()<<")"<<endl;
#endif // DEBUG

        configureRequestEvent(event.xconfigurerequest);
        break;
    case MapNotify:
        mapNotifyEvent(event.xmap);
        break;
        // This is already handled in Fluxbox::handleEvent
        // case MapRequest:
        //        mapRequestEvent(event.xmaprequest);
        //break;
    case PropertyNotify: {

#ifdef DEBUG
        char *atomname = XGetAtomName(display, event.xproperty.atom);
        cerr<<"PropertyNotify("<<title()<<"), property = "<<atomname<<endl;
        if (atomname)
            XFree(atomname);
#endif // DEBUG
        WinClient *client = findClient(event.xproperty.window);
        if (client)
            propertyNotifyEvent(*client, event.xproperty.atom);

    }
        break;

    default:
#ifdef SHAPE
        if (Fluxbox::instance()->haveShape() &&
            event.type == Fluxbox::instance()->shapeEventbase() + ShapeNotify) {
#ifdef DEBUG
            cerr<<"ShapeNotify("<<title()<<")"<<endl;
#endif // DEBUG
            XShapeEvent *shape_event = (XShapeEvent *)&event;

            if (shape_event->shaped)
                frame().setShapingClient(m_client, true);
            else
                frame().setShapingClient(0, true);

            FbTk::App::instance()->sync(false);
            break;
        }
#endif // SHAPE

        break;
    }
}

void FluxboxWindow::mapRequestEvent(XMapRequestEvent &re) {

    // we're only concerned about client window event
    WinClient *client = findClient(re.window);
    if (client == 0) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): Can't find client!"<<endl;
#endif // DEBUG
        return;
    }

    // Note: this function never gets called from WithdrawnState
    // initial state is handled in restoreAttributes() and init()

    // if the user doesn't want the window, then ignore request
    if (!allowsFocusFromClient())
        return;

    setCurrentClient(*client, false); // focus handled on MapNotify
    deiconify(false);

}

bool FluxboxWindow::allowsFocusFromClient() {

    // check what to do if window is on another workspace
    if (screen().currentWorkspaceID() != workspaceNumber() && !isStuck()) {
        BScreen::FollowModel model = screen().getFollowModel();
        if (model == BScreen::IGNORE_OTHER_WORKSPACES)
            return false;
    }

    FluxboxWindow *cur = FocusControl::focusedFbWindow();
    WinClient *client = FocusControl::focusedWindow();
    if (cur && client && cur->isTyping() &&
        getRootTransientFor(m_client) != getRootTransientFor(client))
        return false;

    return true;

}

void FluxboxWindow::mapNotifyEvent(XMapEvent &ne) {
    WinClient *client = findClient(ne.window);
    if (!client || client != m_client)
        return;

    if (ne.override_redirect || !isVisible() || !client->validateClient())
        return;

    iconic = false;

    // setting state will cause all tabs to be mapped, but we only want the
    // original tab to be focused
    if (m_current_state != NormalState)
        setState(NormalState, false);

    // we use m_focused as a signal that this should be focused when mapped
    if (m_focused) {
        m_focused = false;
        focus();
    }

}

/**
   Unmaps frame window and client window if
   event.window == m_client->window
*/
void FluxboxWindow::unmapNotifyEvent(XUnmapEvent &ue) {
    WinClient *client = findClient(ue.window);
    if (client == 0)
        return;

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): 0x"<<hex<<client->window()<<dec<<endl;
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): title="<<client->title()<<endl;
#endif // DEBUG

    restore(client, false);

}

/**
   Checks if event is for m_client->window.
   If it isn't, we leave it until the window is unmapped, if it is,
   we just hide it for now.
*/
void FluxboxWindow::destroyNotifyEvent(XDestroyWindowEvent &de) {
    if (de.window == m_client->window()) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): DestroyNotifyEvent this="<<this<<" title = "<<title()<<endl;
#endif // DEBUG
        if (numClients() == 1)
            hide();
    }

}


void FluxboxWindow::propertyNotifyEvent(WinClient &client, Atom atom) {
    switch(atom) {
    case XA_WM_CLASS:
    case XA_WM_CLIENT_MACHINE:
    case XA_WM_COMMAND:
        break;

    case XA_WM_TRANSIENT_FOR: {
        bool was_transient = client.isTransient();
        client.updateTransientInfo();
        // update our layer to be the same layer as our transient for
        if (client.isTransient() && !was_transient
            && client.transientFor()->fbwindow())
            layerItem().setLayer(client.transientFor()->fbwindow()->layerItem().getLayer());

    } break;

    case XA_WM_HINTS:
        client.updateWMHints();
        titleSig().notify();
        // nothing uses this yet
        // hintSig().notify(); // notify listeners
        break;

    case XA_WM_ICON_NAME:
        // we don't use icon title, since many apps don't update it,
        // and we don't show icons anyway
        break;
    case XA_WM_NAME:
        client.updateTitle();
        break;

    case XA_WM_NORMAL_HINTS: {
#ifdef DEBUG
        cerr<<"XA_WM_NORMAL_HINTS("<<title()<<")"<<endl;
#endif // DEBUG
        unsigned int old_max_width = client.max_width;
        unsigned int old_min_width = client.min_width;
        unsigned int old_min_height = client.min_height;
        unsigned int old_max_height = client.max_height;
        bool changed = false;
        client.updateWMNormalHints();

        if (client.min_width != old_min_width ||
            client.max_width != old_max_width ||
            client.min_height != old_min_height ||
            client.max_height != old_max_height) {
            if (client.max_width != 0 && client.max_width <= client.min_width &&
                client.max_height != 0 && client.max_height <= client.min_height) {
                if (decorations.maximize ||
                    decorations.handle ||
                    functions.resize ||
                    functions.maximize)
                    changed = true;
                decorations.maximize = false;
                decorations.handle = false;
                functions.resize=false;
                functions.maximize=false;
            } else {
                // TODO: is broken while handled by FbW, needs to be in WinClient
                if (!client.isTransient() || screen().decorateTransient()) {
                    if (!decorations.maximize ||
                        !decorations.handle ||
                        !functions.maximize)
                        changed = true;
                    decorations.maximize = true;
                    decorations.handle = true;
                    functions.maximize = true;
                }
                if (!functions.resize)
                    changed = true;
                functions.resize = true;
            }

            if (changed) {
                setupWindow();
                applyDecorations();
            }
       }

        moveResize(frame().x(), frame().y(),
                   frame().width(), frame().height());

        break;
    }

    default:
        FbAtoms *fbatoms = FbAtoms::instance();
        if (atom == fbatoms->getWMProtocolsAtom()) {
            client.updateWMProtocols();
        } else if (atom == fbatoms->getMWMHintsAtom()) {
            client.updateMWMHints();
            updateMWMHintsFromClient(client);
            updateRememberStateFromClient(client);
            applyDecorations(); // update decorations (if they changed)
        }
        break;
    }

}


void FluxboxWindow::exposeEvent(XExposeEvent &ee) {
    frame().exposeEvent(ee);
}

void FluxboxWindow::configureRequestEvent(XConfigureRequestEvent &cr) {

    WinClient *client = findClient(cr.window);
    if (client == 0 || isIconic())
        return;

    int old_x = frame().x(), old_y = frame().y();
    unsigned int old_w = frame().width();
    unsigned int old_h = frame().height() - frame().titlebarHeight()
                       + frame().handleHeight();
    int cx = old_x, cy = old_y, ignore = 0;
    unsigned int cw = old_w, ch = old_h;

    // make sure the new width/height would be ok with all clients, or else they
    // could try to resize the window back and forth
    if (cr.value_mask & CWWidth || cr.value_mask & CWHeight) {
        int new_w = (cr.value_mask & CWWidth) ? cr.width : cw;
        int new_h = (cr.value_mask & CWHeight) ? cr.height : ch;
        ClientList::iterator it = clientList().begin();
        ClientList::iterator it_end = clientList().end();
        for (; it != it_end; ++it) {
            if (*it != client && !(*it)->checkSizeHints(new_w, new_h)) {
                sendConfigureNotify();
                return;
            }
        }
    }

#ifdef REMEMBER
    // don't let misbehaving clients (e.g. MPlayer) move/resize their windows
    // just after creation if the user has a saved position/size
    if (m_creation_time) {
        struct timeval now;
        gettimeofday(&now, NULL);

        if (now.tv_sec > m_creation_time + 1)
            m_creation_time = 0;
        else if (Remember::instance().isRemembered(*client,
                         Remember::REM_MAXIMIZEDSTATE) ||
                 Remember::instance().isRemembered(*client,
                         Remember::REM_FULLSCREENSTATE)) {
            cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
            cr.value_mask = cr.value_mask & ~(CWX | CWY);
        } else {
            if (Remember::instance().isRemembered(*client,
                                                  Remember::REM_DIMENSIONS))
                cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);

            if (Remember::instance().isRemembered(*client,
                                                  Remember::REM_POSITION))
                cr.value_mask = cr.value_mask & ~(CWX | CWY);
        }
    }
#endif // REMEMBER

    if (cr.value_mask & CWBorderWidth)
        client->old_bw = cr.border_width;

    if ((cr.value_mask & CWX) &&
        (cr.value_mask & CWY)) {
        cx = cr.x;
        cy = cr.y;
        frame().gravityTranslate(cx, cy, client->gravity(), client->old_bw, false);
        frame().setActiveGravity(client->gravity(), client->old_bw);
    } else if (cr.value_mask & CWX) {
        cx = cr.x;
        frame().gravityTranslate(cx, ignore, client->gravity(), client->old_bw, false);
        frame().setActiveGravity(client->gravity(), client->old_bw);
    } else if (cr.value_mask & CWY) {
        cy = cr.y;
        frame().gravityTranslate(ignore, cy, client->gravity(), client->old_bw, false);
        frame().setActiveGravity(client->gravity(), client->old_bw);
    }

    if (cr.value_mask & CWWidth)
        cw = cr.width;

    if (cr.value_mask & CWHeight)
        ch = cr.height;

    // the request is for client window so we resize the frame to it first
    if (old_w != cw || old_h != ch) {
        if (old_x != cx || old_y != cy)
            frame().moveResizeForClient(cx, cy, cw, ch);
        else
            frame().resizeForClient(cw, ch);
    } else if (old_x != cx || old_y != cy) {
        frame().move(cx, cy);
    }

    if (cr.value_mask & CWStackMode) {
        switch (cr.detail) {
        case Above:
        case TopIf:
        default:
            setCurrentClient(*client, m_focused);
            raise();
            break;

        case Below:
        case BottomIf:
            lower();
            break;
        }
    }

    sendConfigureNotify();

}

// keep track of last keypress in window, so we can decide not to focusNew
void FluxboxWindow::keyPressEvent(XKeyEvent &ke) {
    // if there's a modifier key down, the user probably expects the new window
    if (FbTk::KeyUtil::instance().cleanMods(ke.state))
        return;

    // we need to ignore modifier keys themselves, too
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);
    if (IsModifierKey(ks))
        return;

    // if the key was return/enter, the user probably expects the window
    // e.g., typed the command in a terminal
    if (ks == XK_KP_Enter || ks == XK_Return) {
        // we'll actually reset the time for this one
        m_last_keypress_time.tv_sec = 0;
        return;
    }

    // otherwise, make a note that the user is typing
    gettimeofday(&m_last_keypress_time, 0);
}

bool FluxboxWindow::isTyping() {
    timeval now;
    if (gettimeofday(&now, NULL) == -1)
        return false;

    unsigned int diff = 1000*(now.tv_sec - m_last_keypress_time.tv_sec);
    diff += (now.tv_usec - m_last_keypress_time.tv_usec)/1000;

    return (diff < screen().noFocusWhileTypingDelay());
}

void FluxboxWindow::buttonPressEvent(XButtonEvent &be) {
    m_last_button_x = be.x_root;
    m_last_button_y = be.y_root;

    // check keys file first
    WindowCmd<void>::setWindow(this);
    if (Fluxbox::instance()->keys()->doAction(be.type, be.state, be.button,
                                              Keys::ON_WINDOW)) {
        return;
    }

    frame().tabcontainer().tryButtonPressEvent(be);
    if (be.button == 1) {
        if (!m_focused && acceptsFocus()) //check focus
            focus();

        // click on titlebar
        if (frame().gripLeft().window() != be.window &&
            frame().gripRight().window() != be.window &&
            frame().clientArea().window() != be.window &&
            frame().window() != be.window)
            raise();

        if (frame().window().window() == be.window ||
            frame().tabcontainer().window() == be.window) {
            if (screen().clickRaises())
                raise();
#ifdef DEBUG
            cerr<<"FluxboxWindow::buttonPressEvent: AllowEvent"<<endl;
#endif // DEBUG

            XAllowEvents(display, ReplayPointer, be.time);

            m_button_grab_x = be.x_root - frame().x() - frame().window().borderWidth();
            m_button_grab_y = be.y_root - frame().y() - frame().window().borderWidth();
        } else if (frame().handle() == be.window)
            raise();

        Fluxbox::instance()->hideExtraMenus(screen());
        screen().hideWindowMenus(this);
    }
}

void FluxboxWindow::buttonReleaseEvent(XButtonEvent &re) {

    if (isMoving())
        stopMoving();
    else if (isResizing())
        stopResizing();
    else if (m_attaching_tab)
        attachTo(re.x_root, re.y_root);
    else {
        frame().tabcontainer().tryButtonReleaseEvent(re);
        if (frame().gripLeft().window() == re.window ||
            frame().gripRight().window() == re.window ||
            frame().clientArea().window() == re.window ||
            frame().handle().window() == re.window ||
            frame().window() == re.window)
            return;

        static Time last_release_time = 0;
        bool double_click = (re.time - last_release_time <=
            Fluxbox::instance()->getDoubleClickInterval());
        last_release_time = re.time;

        if (re.button == 1 && double_click)
            shade();
        if (re.button == 3)
            popupMenu();
        if (re.button == 2)
            lower();

        unsigned int reverse = (screen().getScrollReverse() ? 1 : 0);
        if (re.button == 4 || re.button == 5) {
            if (StringUtil::toLower(screen().getScrollAction()) == "shade") {
                if (re.button == 5 - reverse)
                    shadeOn();
                else
                    shadeOff();
            }
            if (StringUtil::toLower(screen().getScrollAction()) == "nexttab") {
                if (re.button == 5 - reverse)
                    nextClient();
                else
                    prevClient();
            }
        }
    }

}


void FluxboxWindow::motionNotifyEvent(XMotionEvent &me) {
    if (isMoving() && me.window == parent()) {
        me.window = frame().window().window();
    }

    bool inside_titlebar = (frame().titlebar() == me.window
                            || frame().label() == me.window
                            || frame().tabcontainer() == me.window
                            || frame().handle() == me.window
                            || frame().window() == me.window);

    if (Fluxbox::instance()->getIgnoreBorder() && m_attaching_tab == 0
        && !(isMoving() || isResizing())) {
        int borderw = frame().window().borderWidth();
        //!! TODO(tabs): the below test ought to be in FbWinFrame
        // if mouse is currently on the window border, ignore it
        if ((me.x_root < (frame().x() + borderw) ||
            me.y_root < (frame().y() + borderw) ||
            me.x_root >= (frame().x() + (int)frame().width() + borderw) ||
            me.y_root >= (frame().y() + (int)frame().height() + borderw))
            && (!frame().externalTabMode() ||
                (me.x_root < (frame().tabcontainer().x() + borderw) ||
                 me.y_root < (frame().tabcontainer().y() + borderw) ||
                 me.x_root >= (frame().tabcontainer().x() +
                         (int)frame().tabcontainer().width() + borderw) ||
                 me.y_root >= (frame().tabcontainer().y() +
                         (int)frame().tabcontainer().height() + borderw)))
            // or if mouse was on border when it was last clicked
            || (m_last_button_x < (frame().x() + borderw) ||
                m_last_button_y < (frame().y() + borderw) ||
                m_last_button_x >= (frame().x() +
                         (int)frame().width() + borderw) ||
                m_last_button_y >= (frame().y() +
                         (int)frame().height() + borderw))
            && (!frame().externalTabMode() ||
                (m_last_button_x < (frame().tabcontainer().x() + borderw) ||
                 m_last_button_y < (frame().tabcontainer().y() + borderw) ||
                 m_last_button_x >= (frame().tabcontainer().x() +
                         (int)frame().tabcontainer().width() + borderw) ||
                 m_last_button_y >= (frame().tabcontainer().y() +
                         (int)frame().tabcontainer().height() + borderw))))
            return;
    }

    WinClient *client = 0;
    if (!inside_titlebar) {
        // determine if we're in titlebar
        Client2ButtonMap::iterator it =
            find_if(m_labelbuttons.begin(),
                    m_labelbuttons.end(),
                    Compose(bind2nd(equal_to<Window>(), me.window),
                            Compose(mem_fun(&TextButton::window),
                                    Select2nd<Client2ButtonMap::value_type>())));
        if (it != m_labelbuttons.end()) {
            inside_titlebar = true;
            client = (*it).first;
        }
    }

    if ((me.state & Button1Mask) && functions.move &&
        inside_titlebar &&
        !isResizing()) {

        if (! isMoving()) {
            startMoving(me.x_root, me.y_root);
        } else {
            // Warp to next or previous workspace?, must have moved sideways some
            int moved_x = me.x_root - m_last_resize_x;
            // save last event point
            m_last_resize_x = me.x_root;
            m_last_resize_y = me.y_root;

            if (moved_x && screen().isWorkspaceWarping()) {
                unsigned int cur_id = screen().currentWorkspaceID();
                unsigned int new_id = cur_id;
                const int warpPad = screen().getEdgeSnapThreshold();
                // 1) if we're inside the border threshold
                // 2) if we moved in the right direction
                if (me.x_root >= int(screen().width()) - warpPad - 1 &&
                    moved_x > 0) {
                    //warp right
                    new_id = (cur_id + 1) % screen().numberOfWorkspaces();
                    m_last_resize_x = 0; // move mouse back to x=0
                } else if (me.x_root <= warpPad &&
                           moved_x < 0) {
                    //warp left
                    new_id = (cur_id + screen().numberOfWorkspaces() - 1) % screen().numberOfWorkspaces();
                    m_last_resize_x = screen().width() - 1; // move mouse to screen width - 1
                }
                if (new_id != cur_id) {

                    // remove motion events from queue to avoid repeated warps
                    XEvent e;
                    while (XCheckTypedEvent(display, MotionNotify, &e)) {
                        // might as well update the y-coordinate
                        m_last_resize_y = e.xmotion.y_root;
                    }

                    // move the pointer to (m_last_resize_x,m_last_resize_y)
                    XWarpPointer(display, None, me.root, 0, 0, 0, 0,
                                 m_last_resize_x, m_last_resize_y);

                    screen().sendToWorkspace(new_id, this, true);
                }
            }

            int dx = m_last_resize_x - m_button_grab_x,
                dy = m_last_resize_y - m_button_grab_y;

            dx -= frame().window().borderWidth();
            dy -= frame().window().borderWidth();

            // dx = current left side, dy = current top
            doSnapping(dx, dy);

            if (! screen().doOpaqueMove()) {
                parent().drawRectangle(screen().rootTheme().opGC(),
                                       m_last_move_x, m_last_move_y,
                                       frame().width() + 2*frame().window().borderWidth()-1,
                                       frame().height() + 2*frame().window().borderWidth()-1);

                parent().drawRectangle(screen().rootTheme().opGC(),
                                       dx, dy,
                                       frame().width() + 2*frame().window().borderWidth()-1,
                                       frame().height() + 2*frame().window().borderWidth()-1);
                m_last_move_x = dx;
                m_last_move_y = dy;
            } else {
                //moveResize(dx, dy, frame().width(), frame().height());
                // need to move the base window without interfering with transparency
                frame().quietMoveResize(dx, dy, frame().width(), frame().height());
            }

            screen().showPosition(dx, dy);
        } // end if moving
    } else if (functions.resize &&
               (((me.state & Button1Mask) &&
                 (me.window == frame().gripRight() ||
                  me.window == frame().gripLeft())) ||
                me.window == frame().window())) {

        if (! resizing) {

          ResizeDirection resize_corner = RIGHTBOTTOM;
          if (me.window == frame().gripRight())
              resize_corner = RIGHTBOTTOM;
          else if (me.window == frame().gripLeft())
              resize_corner = LEFTBOTTOM;
          else // dragging border of window, so choose nearest corner
              resize_corner = getResizeDirection(me.x, me.y, QUADRANTRESIZE);

          // We are grabbing frame window in startResizing
          // we need to translate coordinates to it.
          int start_x = me.x, start_y = me.y;
          Window child;
          XTranslateCoordinates(display,
                                me.window, fbWindow().window(),
                                start_x, start_y,
                                &start_x, &start_y,
                                &child);

          startResizing(start_x, start_y, resize_corner);

        } else if (resizing) {

            int old_resize_x = m_last_resize_x;
            int old_resize_y = m_last_resize_y;
            unsigned int old_resize_w = m_last_resize_w;
            unsigned int old_resize_h = m_last_resize_h;

            // move rectangle
            int gx = 0, gy = 0;

            int dx = me.x - m_button_grab_x;
            int dy = me.y - m_button_grab_y;
            switch (m_resize_corner) {
            case LEFTTOP:
                 m_last_resize_w = frame().width() - dx;
                 m_last_resize_x = frame().x() + dx;
                 // no break, use code below too
            case RIGHTTOP:
                m_last_resize_h = frame().height() - dy;
                m_last_resize_y = frame().y() + dy;
                break;
            case LEFTBOTTOM:
                 m_last_resize_w = frame().width() - dx;
                 m_last_resize_x = frame().x() + dx;
                 break;
            case ALLCORNERS:
                // dx or dy must be at least 2
                if (abs(dx) >= 2 || abs(dy) >= 2) {
                    // take max and make it even
                    int diff = 2 * (max(dx, dy) / 2);

                    m_last_resize_h =  frame().height() + diff;

                    m_last_resize_w = frame().width() + diff;
                    m_last_resize_x = frame().x() - diff/2;
                    m_last_resize_y = frame().y() - diff/2;
                }
                break;
            default: // kill warning
                break;
            };

            // if not on top or all corner then move bottom

            if (!(m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP ||
                  m_resize_corner == ALLCORNERS))
                m_last_resize_h = frame().height() + dy;

            // if not top or left bottom or all corners then move right side
            if (!(m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM ||
                  m_resize_corner == ALLCORNERS))
                m_last_resize_w = frame().width() + dx;

            fixsize(&gx, &gy);

            if (old_resize_x != m_last_resize_x ||
                old_resize_y != m_last_resize_y ||
                old_resize_w != m_last_resize_w ||
                old_resize_h != m_last_resize_h ) {

                // draw over old rect
                parent().drawRectangle(screen().rootTheme().opGC(),
                                       old_resize_x, old_resize_y,
                                       old_resize_w - 1 + 2 * frame().window().borderWidth(),
                                       old_resize_h - 1 + 2 * frame().window().borderWidth());

                // draw resize rectangle
                parent().drawRectangle(screen().rootTheme().opGC(),
                                       m_last_resize_x, m_last_resize_y,
                                       m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                                       m_last_resize_h - 1 + 2 * frame().window().borderWidth());

                screen().showGeometry(gx, gy);
            }
        }
    } else if (functions.tabable &&
               (me.state & Button2Mask) && inside_titlebar && (client != 0 || m_attaching_tab != 0)) {
        //
        // drag'n'drop code for tabs
        //
        FbTk::TextButton &active_button = *m_labelbuttons[(m_attaching_tab==0)?client:m_attaching_tab];

        if (m_attaching_tab == 0) {
            if (s_num_grabs > 0)
                return;
            // start drag'n'drop for tab
            m_attaching_tab = client;
            grabPointer(me.window, False, ButtonMotionMask |
                        ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                        None, frame(). theme().moveCursor(), CurrentTime);
            // relative position on button
            m_button_grab_x = me.x;
            m_button_grab_y = me.y;
            // last known root mouse position
            m_last_move_x = me.x_root - me.x;
            m_last_move_y = me.y_root - me.y;
            // hijack extra vars for initial grab location
            m_last_resize_x = me.x_root;
            m_last_resize_y = me.y_root;

            Fluxbox::instance()->grab();

            parent().drawRectangle(screen().rootTheme().opGC(),
                                   m_last_move_x, m_last_move_y,
                                   active_button.width(),
                                   active_button.height());

            menu().hide();
        } else {
            // we already grabed and started to drag'n'drop tab
            // so we update drag'n'drop-rectangle
            int dx = me.x_root - m_button_grab_x, dy = me.y_root - m_button_grab_y;

            //erase rectangle
            parent().drawRectangle(screen().rootTheme().opGC(),
                                   m_last_move_x, m_last_move_y,
                                   active_button.width(),
                                   active_button.height());


            // redraw rectangle at new pos
            m_last_move_x = dx;
            m_last_move_y = dy;
            parent().drawRectangle(screen().rootTheme().opGC(),
                                   m_last_move_x, m_last_move_y,
                                   active_button.width(),
                                   active_button.height());


        }
    }

}

void FluxboxWindow::enterNotifyEvent(XCrossingEvent &ev) {

    // ignore grab activates, or if we're not visible
    if (ev.mode == NotifyGrab || ev.mode == NotifyUngrab ||
        !isVisible()) {
        return;
    }

    if (ev.window == frame().window()) {
        WindowCmd<void>::setWindow(this);
        Fluxbox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                              Keys::ON_WINDOW);
    }

    WinClient *client = 0;
    if (screen().focusControl().isMouseTabFocus()) {
        // determine if we're in a label button (tab)
        Client2ButtonMap::iterator it =
            find_if(m_labelbuttons.begin(),
                    m_labelbuttons.end(),
                    Compose(bind2nd(equal_to<Window>(), ev.window),
                            Compose(mem_fun(&TextButton::window),
                                    Select2nd<Client2ButtonMap::value_type>())));
        if (it != m_labelbuttons.end())
            client = (*it).first;

    }

    if (ev.window == frame().window() ||
        ev.window == m_client->window() ||
        client) {

        if (screen().focusControl().isMouseFocus() && !isFocused() &&
            acceptsFocus() && getWindowType() != Focusable::TYPE_DESKTOP) {

            // check that there aren't any subsequent leave notify events in the
            // X event queue
            XEvent dummy;
            scanargs sa;
            sa.w = ev.window;
            sa.enter = sa.leave = False;
            XCheckIfEvent(display, &dummy, queueScanner, (char *) &sa);

            if ((!sa.leave || sa.inferior) && !screen().focusControl().isCycling() ) {
                focus();
            }
        }
    }

    if (screen().focusControl().isMouseTabFocus() && client && client != m_client) {
        setCurrentClient(*client, isFocused());
    }

}

void FluxboxWindow::leaveNotifyEvent(XCrossingEvent &ev) {

    // ignore grab activates, or if we're not visible
    if (ev.mode == NotifyGrab || ev.mode == NotifyUngrab ||
        !isVisible()) {
        return;
    }

    // still inside?
    if (ev.x_root > frame().x() && ev.y_root > frame().y() &&
        ev.x_root <= (int)(frame().x() + frame().width()) &&
        ev.y_root <= (int)(frame().y() + frame().height()))
        return;

    WindowCmd<void>::setWindow(this);
    Fluxbox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                          Keys::ON_WINDOW);

    // I hope commenting this out is right - simon 21jul2003
    //if (ev.window == frame().window())
    //installColormap(false);
}

// commit current decoration values to actual displayed things
void FluxboxWindow::applyDecorations(bool initial) {
    frame().clientArea().setBorderWidth(0); // client area bordered by other things

    unsigned int border_width = 0;
    if (decorations.border)
        border_width = frame().theme().border().width();

    bool client_move = false;

    // borderWidth setting handles its own gravity
    if (initial || frame().window().borderWidth() != border_width) {
        client_move = true;
        frame().setBorderWidth(border_width);
    }

    int grav_x=0, grav_y=0;
    // negate gravity
    frame().gravityTranslate(grav_x, grav_y, -m_client->gravity(), m_client->old_bw, false);

    // tab deocration only affects if we're external
    // must do before the setTabMode in case it goes
    // to external and is meant to be hidden
    if (decorations.tab)
        client_move |= frame().showTabs();
    else
        client_move |= frame().hideTabs();

    // we rely on frame not doing anything if it is already shown/hidden
    if (decorations.titlebar) {
        bool change = frame().showTitlebar();
        client_move |= change;
        if (screen().getDefaultInternalTabs()) {
            client_move |= frame().setTabMode(FbWinFrame::INTERNAL);
        } else {
            client_move |= frame().setTabMode(FbWinFrame::EXTERNAL);
        }
    } else {
        client_move |= frame().hideTitlebar();
        if (decorations.tab)
            client_move |= frame().setTabMode(FbWinFrame::EXTERNAL);
    }

    if (decorations.handle) {
        client_move |= frame().showHandle();
    } else
        client_move |= frame().hideHandle();

    // apply gravity once more
    frame().gravityTranslate(grav_x, grav_y, m_client->gravity(), m_client->old_bw, false);

    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0) {
        move(grav_x + frame().x(), grav_y + frame().y());
        client_move = true;
    }

    frame().reconfigure();
    if (client_move)
        Fluxbox::instance()->updateFrameExtents(*this);

    if (!initial && client_move)
        sendConfigureNotify();

}

void FluxboxWindow::toggleDecoration() {
    //don't toggle decor if the window is shaded
    if (isShaded() || isFullscreen())
        return;

    m_toggled_decos = !m_toggled_decos;

    if (m_toggled_decos) {
        m_old_decoration_mask = decorationMask();
        if (decorations.titlebar | decorations.tab)
            setDecorationMask(DECOR_NONE);
        else
            setDecorationMask(DECOR_NORMAL);
    } else //revert back to old decoration
        setDecorationMask(m_old_decoration_mask);

}

unsigned int FluxboxWindow::decorationMask() const {
    unsigned int ret = 0;
    if (decorations.titlebar)
        ret |= DECORM_TITLEBAR;
    if (decorations.handle)
        ret |= DECORM_HANDLE;
    if (decorations.border)
        ret |= DECORM_BORDER;
    if (decorations.iconify)
        ret |= DECORM_ICONIFY;
    if (decorations.maximize)
        ret |= DECORM_MAXIMIZE;
    if (decorations.close)
        ret |= DECORM_CLOSE;
    if (decorations.menu)
        ret |= DECORM_MENU;
    if (decorations.sticky)
        ret |= DECORM_STICKY;
    if (decorations.shade)
        ret |= DECORM_SHADE;
    if (decorations.tab)
        ret |= DECORM_TAB;
    if (decorations.enabled)
        ret |= DECORM_ENABLED;
    return ret;
}

void FluxboxWindow::setDecorationMask(unsigned int mask, bool apply) {
    decorations.titlebar = mask & DECORM_TITLEBAR;
    decorations.handle   = mask & DECORM_HANDLE;
    decorations.border   = mask & DECORM_BORDER;
    decorations.iconify  = mask & DECORM_ICONIFY;
    decorations.maximize = mask & DECORM_MAXIMIZE;
    decorations.close    = mask & DECORM_CLOSE;
    decorations.menu     = mask & DECORM_MENU;
    decorations.sticky   = mask & DECORM_STICKY;
    decorations.shade    = mask & DECORM_SHADE;
    decorations.tab      = mask & DECORM_TAB;
    decorations.enabled  = mask & DECORM_ENABLED;
    // we don't want to do this during initialization
    if (apply)
        applyDecorations();
}

void FluxboxWindow::startMoving(int x, int y) {
    if (s_num_grabs > 0)
        return;

    if (isMaximized() && screen().getMaxDisableMove())
        return;

    // save first event point
    m_last_resize_x = x;
    m_last_resize_y = y;
    m_button_grab_x = x - frame().x() - frame().window().borderWidth();
    m_button_grab_y = y - frame().y() - frame().window().borderWidth();

    moving = true;

    Fluxbox *fluxbox = Fluxbox::instance();
    // grabbing (and masking) on the root window allows us to
    // freely map and unmap the window we're moving.
    grabPointer(screen().rootWindow().window(), False, Button1MotionMask |
                ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                screen().rootWindow().window(), frame().theme().moveCursor(), CurrentTime);

    if (menu().isVisible())
        menu().hide();

    fluxbox->maskWindowEvents(screen().rootWindow().window(), this);

    m_last_move_x = frame().x();
    m_last_move_y = frame().y();
    if (! screen().doOpaqueMove()) {
        fluxbox->grab();
        parent().drawRectangle(screen().rootTheme().opGC(),
                               frame().x(), frame().y(),
                               frame().width() + 2*frame().window().borderWidth()-1,
                               frame().height() + 2*frame().window().borderWidth()-1);
        screen().showPosition(frame().x(), frame().y());
    }
}

void FluxboxWindow::stopMoving(bool interrupted) {
    moving = false;
    Fluxbox *fluxbox = Fluxbox::instance();

    fluxbox->maskWindowEvents(0, 0);

    // if no real movement happend -> raise if clickrais is disabled
    if (m_last_move_x - frame().x() == 0 && 
            m_last_move_y - frame().y() == 0 &&
            !screen().clickRaises()) {

        raise();
    }


    if (! screen().doOpaqueMove()) {
        parent().drawRectangle(screen().rootTheme().opGC(),
                               m_last_move_x, m_last_move_y,
                               frame().width() + 2*frame().window().borderWidth()-1,
                               frame().height() + 2*frame().window().borderWidth()-1);
        if (!interrupted) {
            moveResize(m_last_move_x, m_last_move_y, frame().width(), frame().height());
            if (m_workspace_number != screen().currentWorkspaceID()) {
                screen().reassociateWindow(this, screen().currentWorkspaceID(), true);
                frame().show();
                focus();
            }
        }
        fluxbox->ungrab();
    } else if (!interrupted) {
        moveResize(frame().x(), frame().y(), frame().width(), frame().height(), true);
        frame().notifyMoved(true);
    }


    screen().hidePosition();
    ungrabPointer(CurrentTime);

    FbTk::App::instance()->sync(false); //make sure the redraw is made before we continue
}

void FluxboxWindow::pauseMoving() {
    if (screen().doOpaqueMove()) {
        return;
    }

    parent().drawRectangle(screen().rootTheme().opGC(),
                           m_last_move_x, m_last_move_y,
                           frame().width() + 2*frame().window().borderWidth()-1,
                           frame().height() + 2*frame().window().borderWidth()-1);

}


void FluxboxWindow::resumeMoving() {
    if (screen().doOpaqueMove()) {
        return;
    }

    if (m_workspace_number == screen().currentWorkspaceID()) {
        frame().show();
        focus();
    }

    FbTk::App::instance()->sync(false);

    parent().drawRectangle(screen().rootTheme().opGC(),
                           m_last_move_x, m_last_move_y,
                           frame().width() + 2*frame().window().borderWidth()-1,
                           frame().height() + 2*frame().window().borderWidth()-1);

}

/**
 * Helper function that snaps a window to another window
 * We snap if we're closer than the x/ylimits.
 */
inline void snapToWindow(int &xlimit, int &ylimit,
                         int left, int right, int top, int bottom,
                         int oleft, int oright, int otop, int obottom) {
    // Only snap if we're adjacent to the edge we're looking at

    // for left + right, need to be in the right y range
    if (top <= obottom && bottom >= otop) {
        // left
        if (abs(left-oleft)  < abs(xlimit)) xlimit = -(left-oleft);
        if (abs(right-oleft) < abs(xlimit)) xlimit = -(right-oleft);

        // right
        if (abs(left-oright)  < abs(xlimit)) xlimit = -(left-oright);
        if (abs(right-oright) < abs(xlimit)) xlimit = -(right-oright);
    }

    // for top + bottom, need to be in the right x range
    if (left <= oright && right >= oleft) {
        // top
        if (abs(top-otop)    < abs(ylimit)) ylimit = -(top-otop);
        if (abs(bottom-otop) < abs(ylimit)) ylimit = -(bottom-otop);

        // bottom
        if (abs(top-obottom)    < abs(ylimit)) ylimit = -(top-obottom);
        if (abs(bottom-obottom) < abs(ylimit)) ylimit = -(bottom-obottom);
    }

}

/*
 * Do Whatever snapping magic is necessary, and return using the orig_left
 * and orig_top variables to indicate the new x,y position
 */
void FluxboxWindow::doSnapping(int &orig_left, int &orig_top) {
    /*
     * Snap to screen/head edges
     * Snap to windows
     */

    if (screen().getEdgeSnapThreshold() == 0) return;

    // Keep track of our best offsets so far
    // We need to find things less than or equal to the threshold
    int dx = screen().getEdgeSnapThreshold() + 1;
    int dy = screen().getEdgeSnapThreshold() + 1;

    // we only care about the left/top etc that includes borders
    int borderW = 0;

    if (decorationMask() & (DECORM_BORDER|DECORM_HANDLE))
        borderW = frame().window().borderWidth();

    int top = orig_top; // orig include the borders
    int left = orig_left;

    int right = orig_left + width() + 2 * borderW;
    int bottom = orig_top + height() + 2 * borderW;

    // test against tabs too
    bool i_have_tabs = frame().externalTabMode();
    int xoff = 0, yoff = 0, woff = 0, hoff = 0;
    if (i_have_tabs) {
        xoff = xOffset();
        yoff = yOffset();
        woff = widthOffset();
        hoff = heightOffset();
    }

    /////////////////////////////////////
    // begin by checking the screen (or Xinerama head) edges

    int starth = 0;

    // head "0" == whole screen width + height, which we skip since the
    // sum of all the heads covers those edges, if >1 head
    if (screen().numHeads() > 0)
        starth=1;

    for (int h=starth; h <= screen().numHeads(); h++) {
        snapToWindow(dx, dy, left, right, top, bottom,
                     screen().maxLeft(h),
                     screen().maxRight(h),
                     screen().maxTop(h),
                     screen().maxBottom(h));

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         screen().maxLeft(h),
                         screen().maxRight(h),
                         screen().maxTop(h),
                         screen().maxBottom(h));
    }
    for (int h=starth; h <= screen().numHeads(); h++) {
        snapToWindow(dx, dy, left, right, top, bottom,
                     screen().getHeadX(h),
                     screen().getHeadX(h) + screen().getHeadWidth(h),
                     screen().getHeadY(h),
                     screen().getHeadY(h) + screen().getHeadHeight(h));

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         screen().getHeadX(h),
                         screen().getHeadX(h) + screen().getHeadWidth(h),
                         screen().getHeadY(h),
                         screen().getHeadY(h) + screen().getHeadHeight(h));
    }

    /////////////////////////////////////
    // now check window edges

    Workspace::Windows &wins =
        screen().currentWorkspace()->windowList();

    Workspace::Windows::iterator it = wins.begin();
    Workspace::Windows::iterator it_end = wins.end();

    unsigned int bw;
    for (; it != it_end; it++) {
        if ((*it) == this)
            continue; // skip myself

        bw = (*it)->decorationMask() & (DECORM_BORDER|DECORM_HANDLE) ?
                (*it)->frame().window().borderWidth() : 0;

        snapToWindow(dx, dy, left, right, top, bottom,
                     (*it)->x(),
                     (*it)->x() + (*it)->width() + 2 * bw,
                     (*it)->y(),
                     (*it)->y() + (*it)->height() + 2 * bw);

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         (*it)->x(),
                         (*it)->x() + (*it)->width() + 2 * bw,
                         (*it)->y(),
                         (*it)->y() + (*it)->height() + 2 * bw);

        // also snap to the box containing the tabs (don't bother with actual
        // tab edges, since they're dynamic
        if ((*it)->frame().externalTabMode())
            snapToWindow(dx, dy, left, right, top, bottom,
                         (*it)->x() - (*it)->xOffset(),
                         (*it)->x() - (*it)->xOffset() + (*it)->width() + 2 * bw + (*it)->widthOffset(),
                         (*it)->y() - (*it)->yOffset(),
                         (*it)->y() - (*it)->yOffset() + (*it)->height() + 2 * bw + (*it)->heightOffset());

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         (*it)->x() - (*it)->xOffset(),
                         (*it)->x() - (*it)->xOffset() + (*it)->width() + 2 * bw + (*it)->widthOffset(),
                         (*it)->y() - (*it)->yOffset(),
                         (*it)->y() - (*it)->yOffset() + (*it)->height() + 2 * bw + (*it)->heightOffset());

    }

    // commit
    if (dx <= screen().getEdgeSnapThreshold())
        orig_left += dx;
    if (dy <= screen().getEdgeSnapThreshold())
        orig_top  += dy;

}

FluxboxWindow::ResizeDirection FluxboxWindow::getResizeDirection(int x, int y,
                                   ResizeModel model) {
    int cx = frame().width() / 2;
    int cy = frame().height() / 2;
    if (model == CENTERRESIZE)
        return ALLCORNERS;
    if (model == NEARESTEDGERESIZE) {
        if (abs(cy - abs(y - cy)) > abs(cx - abs(x - cx))) // y is nearest
            return (y > cy) ? BOTTOM : TOP;
        return (x > cx) ? RIGHT : LEFT;
    }
    if (model == QUADRANTRESIZE) {
        if (x < cx)
            return (y < cy) ? LEFTTOP : LEFTBOTTOM;
        return (y < cy) ? RIGHTTOP : RIGHTBOTTOM;
    }
    if (model == TOPLEFTRESIZE) return LEFTTOP;
    if (model == TOPRESIZE) return TOP;
    if (model == TOPRIGHTRESIZE) return RIGHTTOP;
    if (model == LEFTRESIZE) return LEFT;
    if (model == RIGHTRESIZE) return RIGHT;
    if (model == BOTTOMLEFTRESIZE) return LEFTBOTTOM;
    if (model == BOTTOMRESIZE) return BOTTOM;
    return RIGHTBOTTOM;
}

void FluxboxWindow::startResizing(int x, int y, ResizeDirection dir) {

    if (s_num_grabs > 0 || isShaded() || isIconic() )
        return;

    if (isMaximized() && screen().getMaxDisableResize())
        return;

    m_resize_corner = dir;

    resizing = true;
    maximized = MAX_NONE;

    const Cursor& cursor = (m_resize_corner == LEFTTOP) ? frame().theme().upperLeftAngleCursor() :
                           (m_resize_corner == RIGHTTOP) ? frame().theme().upperRightAngleCursor() :
                           (m_resize_corner == RIGHTBOTTOM) ? frame().theme().lowerRightAngleCursor() :
                                                            frame().theme().lowerLeftAngleCursor();

    grabPointer(fbWindow().window(),
                false, ButtonMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);

    int gx = 0, gy = 0;
    m_button_grab_x = x;
    m_button_grab_y = y;
    m_last_resize_x = frame().x();
    m_last_resize_y = frame().y();
    m_last_resize_w = frame().width();
    m_last_resize_h = frame().height();

    fixsize(&gx, &gy);

    screen().showGeometry(gx, gy);

    parent().drawRectangle(screen().rootTheme().opGC(),
                       m_last_resize_x, m_last_resize_y,
                       m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                       m_last_resize_h - 1 + 2 * frame().window().borderWidth());
}

void FluxboxWindow::stopResizing(bool interrupted) {
    resizing = false;

    parent().drawRectangle(screen().rootTheme().opGC(),
                           m_last_resize_x, m_last_resize_y,
                           m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                           m_last_resize_h - 1 + 2 * frame().window().borderWidth());

    screen().hideGeometry();

    if (!interrupted) {
        fixsize();

        moveResize(m_last_resize_x, m_last_resize_y,
                   m_last_resize_w, m_last_resize_h);
    }

    ungrabPointer(CurrentTime);
}

void FluxboxWindow::attachTo(int x, int y, bool interrupted) {
    if (m_attaching_tab == 0)
        return;

    parent().drawRectangle(screen().rootTheme().opGC(),
                           m_last_move_x, m_last_move_y,
                           m_labelbuttons[m_attaching_tab]->width(),
                           m_labelbuttons[m_attaching_tab]->height());

    ungrabPointer(CurrentTime);

    Fluxbox::instance()->ungrab();

    // make sure we clean up here, since this object may be deleted inside attachClient
    WinClient *old_attached = m_attaching_tab;
    m_attaching_tab = 0;

    if (interrupted)
        return;

    int dest_x = 0, dest_y = 0;
    Window child = 0;
    if (XTranslateCoordinates(display, parent().window(),
                              parent().window(),
                              x, y, &dest_x, &dest_y, &child)) {

        bool inside_titlebar = false;
        // search for a fluxboxwindow
        WinClient *client = Fluxbox::instance()->searchWindow(child);
        FluxboxWindow *attach_to_win = 0;
        if (client) {

            inside_titlebar = client->fbwindow()->hasTitlebar() &&
                client->fbwindow()->y() + static_cast<signed>(client->fbwindow()->titlebarHeight()) > dest_y;

            Fluxbox::TabsAttachArea area= Fluxbox::instance()->getTabsAttachArea();
            if (area == Fluxbox::ATTACH_AREA_WINDOW)
                attach_to_win = client->fbwindow();
            else if (area == Fluxbox::ATTACH_AREA_TITLEBAR && inside_titlebar) {
                attach_to_win = client->fbwindow();
            }
        }

        if (attach_to_win != this &&
            attach_to_win != 0 && attach_to_win->isTabable()) {

            attach_to_win->attachClient(*old_attached,x,y );
            // we could be deleted here, DO NOT do anything else that alters this object
        } else if (attach_to_win != this || (attach_to_win == this && !inside_titlebar)) {
            // disconnect client if we didn't drop on a window
            WinClient &client = *old_attached;
            detachClient(*old_attached);
            // move window by relative amount of mouse movement
            // since just detached, move relative to old location
            if (client.fbwindow() != 0) {
                client.fbwindow()->move(frame().x() - m_last_resize_x + x, frame().y() - m_last_resize_y + y);
            }
        } else if( attach_to_win == this && attach_to_win->isTabable()) {
            //reording of tabs within a frame
            moveClientTo(*old_attached, x, y);
        }
    }
}

void FluxboxWindow::restore(WinClient *client, bool remap) {
    if (client->fbwindow() != this)
        return;

    XChangeSaveSet(display, client->window(), SetModeDelete);
    client->setEventMask(NoEventMask);

    int wx = frame().x(), wy = frame().y();
    // don't move the frame, in case there are other tabs in it
    // just set the new coordinates on the reparented window
    frame().gravityTranslate(wx, wy, -client->gravity(), client->old_bw, false); // negative to invert

    // Why was this hide done? It broke vncviewer (and mplayer?),
    // since it would reparent when going fullscreen.
    // is it needed for anything? Reparent should imply unmap
    // ok, it should hide sometimes, e.g. if the reparent was sent by a client
    //client->hide();

    // restore old border width
    client->setBorderWidth(client->old_bw);

    XEvent xev;
    if (! XCheckTypedWindowEvent(display, client->window(), ReparentNotify,
                                 &xev)) {
#ifdef DEBUG
        cerr<<"FluxboxWindow::restore: reparent 0x"<<hex<<client->window()<<dec<<" to root"<<endl;

#endif // DEBUG
        // reparent to root window
        client->reparent(screen().rootWindow(), wx, wy, false);

        if (!remap)
            client->hide();
    }

    if (remap)
        client->show();

    installColormap(false);

    delete client;


#ifdef DEBUG
    cerr<<"FluxboxWindow::restore: remap = "<<remap<<endl;
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): numClients() = "<<numClients()<<endl;
#endif // DEBUG
    if (numClients() == 0) {
        hide(true);
    }

}

void FluxboxWindow::restore(bool remap) {
    if (numClients() == 0)
        return;
#ifdef DEBUG
    cerr<<"restore("<<remap<<")"<<endl;
#endif // DEBUG
    while (!clientList().empty()) {
        restore(clientList().back(), remap);
        // deleting winClient removes it from the clientList
    }
}

bool FluxboxWindow::isVisible() const {
    return frame().isVisible();
}

FbTk::FbWindow &FluxboxWindow::fbWindow() {
    return frame().window();
}

const FbTk::FbWindow &FluxboxWindow::fbWindow() const {
    return frame().window();
}

FbTk::Menu &FluxboxWindow::menu() {
    return screen().windowMenu();
}

bool FluxboxWindow::acceptsFocus() const {
    return (m_client ? m_client->acceptsFocus() : false);
}

const FbTk::PixmapWithMask &FluxboxWindow::icon() const {
    return (m_client ? m_client->icon() : m_icon);
}

const FbTk::Menu &FluxboxWindow::menu() const {
    return screen().windowMenu();
}

unsigned int FluxboxWindow::titlebarHeight() const {
    return frame().titlebarHeight();
}

Window FluxboxWindow::clientWindow() const  {
    if (m_client == 0)
        return 0;
    return m_client->window();
}


const string &FluxboxWindow::title() const {
    return (m_client ? m_client->title() : m_title);
}

const std::string &FluxboxWindow::getWMClassName() const {
    return (m_client ? m_client->getWMClassName() : m_instance_name);
}

const std::string &FluxboxWindow::getWMClassClass() const {
    return (m_client ? m_client->getWMClassClass() : m_class_name);
}

std::string FluxboxWindow::getWMRole() const {
    return (m_client ? m_client->getWMRole() : "FluxboxWindow");
}

Focusable::WindowType FluxboxWindow::getWindowType() const {
    return (m_client ? m_client->getWindowType() : Focusable::TYPE_NORMAL);
}

bool FluxboxWindow::isTransient() const {
    return (m_client && m_client->isTransient());
}

int FluxboxWindow::normalX() const {
    if (maximized & MAX_HORZ)
        return m_old_pos_x;
    return x();
}

int FluxboxWindow::normalY() const {
    if (maximized & MAX_VERT)
        return m_old_pos_y;
    return y();
}

unsigned int FluxboxWindow::normalWidth() const {
    if (maximized & MAX_HORZ)
        return m_old_width;
    return width();
}

unsigned int FluxboxWindow::normalHeight() const {
    if (maximized & MAX_VERT)
        return m_old_height;
    if (shaded)
        return frame().normalHeight();
    return height();
}

int FluxboxWindow::initialState() const { return m_client->initial_state; }

void FluxboxWindow::fixsize(int *user_w, int *user_h, bool maximizing) {
    int titlebar_height = (decorations.titlebar ?
                           frame().titlebar().height()  +
                           frame().titlebar().borderWidth() : 0);
    int handle_height = (decorations.handle ?
                         frame().handle().height() +
                         frame().handle().borderWidth() : 0);
    int decoration_height = titlebar_height + handle_height;

    // dx is new width = current width + difference between new and old x values
    //int dx = frame().width() + frame().x() - m_last_resize_x;
    int dw = m_last_resize_w;

    // dy = new height (w/o decorations), similarly
    int dh = m_last_resize_h - decoration_height;

    m_client->applySizeHints(dw, dh, user_w, user_h, maximizing);

    // update last resize
    m_last_resize_w = dw;
    m_last_resize_h = dh + decoration_height;

    // move X if necessary
    if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM) {
        m_last_resize_x = frame().x() + frame().width() - m_last_resize_w;
    }

    if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP) {
        m_last_resize_y = frame().y() + frame().height() - m_last_resize_h;
    }

}

void FluxboxWindow::moveResizeClient(WinClient &client, int x, int y,
                                 unsigned int height, unsigned int width) {
    client.moveResize(x, y,
                      frame().clientArea().width(),
                      frame().clientArea().height());
    client.sendConfigureNotify(frame().x() + frame().clientArea().x(),
                      frame().y() + frame().clientArea().y(),
                      frame().clientArea().width(),
                      frame().clientArea().height());
}

void FluxboxWindow::sendConfigureNotify() {
    ClientList::iterator client_it = m_clientlist.begin();
    ClientList::iterator client_it_end = m_clientlist.end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient &client = *(*client_it);
        /*
          Send event telling where the root position
          of the client window is. (ie frame pos + client pos inside the frame = send pos)
        */
        //!!
        client.x = frame().x();
        client.y = frame().y();
        moveResizeClient(client,
                     frame().clientArea().x(),
                     frame().clientArea().y(),
                     frame().clientArea().width(),
                     frame().clientArea().height());

    } // end for

}


void FluxboxWindow::close() {
    if (WindowCmd<void>::window() == this && WindowCmd<void>::client())
        WindowCmd<void>::client()->sendClose(false);
    else if (m_client)
        m_client->sendClose(false);
}

void FluxboxWindow::kill() {
    if (WindowCmd<void>::window() == this && WindowCmd<void>::client())
        WindowCmd<void>::client()->sendClose(true);
    else if (m_client)
        m_client->sendClose(true);
}

void FluxboxWindow::setupWindow() {
    // sets up our window
    // we allow both to be done at once to share the commands

    using namespace FbTk;
    typedef FbTk::Resource<vector<WinButton::Type> > WinButtonsResource;

    string titlebar_name[2];
    string titlebar_alt_name[2];
    titlebar_name[0] = screen().name() + ".titlebar.left";
    titlebar_alt_name[0] = screen().altName() + ".Titlebar.Left";
    titlebar_name[1] = screen().name() + ".titlebar.right";
    titlebar_alt_name[1] = screen().altName() + ".Titlebar.Right";

    WinButtonsResource *titlebar_side[2];



    ResourceManager &rm = screen().resourceManager();

    // create resource for titlebar
    for (int i=0; i < 2; ++i) {
        titlebar_side[i] = dynamic_cast<WinButtonsResource *>(
                            rm.findResource( titlebar_name[i] ) );

        if (titlebar_side[i] != 0)
            continue; // find next resource too

        WinButton::Type titlebar_left[] =  {
            WinButton::STICK
        };

        WinButton::Type titlebar_right[] =  {
            WinButton::MINIMIZE,
            WinButton::MAXIMIZE,
            WinButton::CLOSE
        };

        WinButton::Type *begin = 0;
        WinButton::Type *end = 0;

        if (i == 0) {
            begin = titlebar_left;
            end = titlebar_left + 1;
        } else {
            begin = titlebar_right;
            end = titlebar_right + 3;
        }

        titlebar_side[i] =
            new WinButtonsResource(rm,
                                   WinButtonsResource::Type(begin, end),
                                   titlebar_name[i], titlebar_alt_name[i]);


        screen().addManagedResource(titlebar_side[i]);
    }

    updateButtons();

    // end setup frame

}

void FluxboxWindow::updateButtons() {
    string titlebar_name[2];
    titlebar_name[0] = screen().name() + ".titlebar.left";
    titlebar_name[1] = screen().name() + ".titlebar.right";

    typedef FbTk::Resource<vector<WinButton::Type> > WinButtonsResource;
    WinButtonsResource *titlebar_side[2];
    ResourceManager &rm = screen().resourceManager();

    bool need_update = false;
    // get resource for titlebar
    for (int i=0; i < 2; ++i) {
        titlebar_side[i] = dynamic_cast<WinButtonsResource *>(
                            rm.findResource( titlebar_name[i] ) );

        // check if we need to update our buttons
        size_t new_size = (*titlebar_side[i])->size();
        if (new_size != m_titlebar_buttons[i].size() || need_update)
            need_update = true;
        else {
            for (size_t j=0; j < new_size && !need_update; j++) {
                if ((*(*titlebar_side[i]))[j] != m_titlebar_buttons[i][j])
                    need_update = true;
            }
        }
                
    }

    if (!need_update)
        return;

    // clear old buttons from frame
    frame().removeAllButtons();

    using namespace FbTk;
    typedef RefCount<Command> CommandRef;
    typedef SimpleCommand<FluxboxWindow> WindowCmd;

    CommandRef iconify_cmd(new WindowCmd(*this, &FluxboxWindow::iconify));
    CommandRef maximize_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeFull));
    CommandRef maximize_vert_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeVertical));
    CommandRef maximize_horiz_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeHorizontal));
    CommandRef close_cmd(new WindowCmd(*this, &FluxboxWindow::close));
    CommandRef shade_cmd(new WindowCmd(*this, &FluxboxWindow::shade));
    CommandRef stick_cmd(new WindowCmd(*this, &FluxboxWindow::stick));
    CommandRef show_menu_cmd(new WindowCmd(*this, &FluxboxWindow::popupMenu));

    WinButtonTheme &winbutton_theme = screen().winButtonTheme();

    for (size_t c = 0; c < 2 ; c++) {
        // get titlebar configuration for current side
        const vector<WinButton::Type> &dir = *(*titlebar_side[c]);
        m_titlebar_buttons[c] = dir;

        for (size_t i=0; i < dir.size(); ++i) {
            //create new buttons
            WinButton *winbtn = 0;

            switch (dir[i]) {
            case WinButton::MINIMIZE:
                if (isIconifiable()) {
                    winbtn = new WinButton(*this, winbutton_theme,
                                           WinButton::MINIMIZE,
                                           frame().titlebar(),
                                           0, 0, 10, 10);
                    winbtn->setOnClick(iconify_cmd);
                }
                break;
            case WinButton::MAXIMIZE:
                if (isMaximizable()) {
                    winbtn = new WinButton(*this, winbutton_theme,
                                           dir[i],
                                           frame().titlebar(),
                                           0, 0, 10, 10);
                    winbtn->setOnClick(maximize_cmd, 1);
                    winbtn->setOnClick(maximize_horiz_cmd, 3);
                    winbtn->setOnClick(maximize_vert_cmd, 2);

                }
                break;
            case WinButton::CLOSE:
                if (m_client->isClosable()) {
                    winbtn = new WinButton(*this, winbutton_theme,
                                           dir[i],
                                           frame().titlebar(),
                                           0, 0, 10, 10);

                    winbtn->setOnClick(close_cmd);
                    stateSig().attach(winbtn);
                }
                break;
            case WinButton::STICK:
                winbtn =  new WinButton(*this, winbutton_theme,
                                        dir[i],
                                        frame().titlebar(),
                                        0, 0, 10, 10);

                stateSig().attach(winbtn);
                winbtn->setOnClick(stick_cmd);
                break;
            case WinButton::SHADE:
                winbtn = new WinButton(*this, winbutton_theme,
                                       dir[i],
                                       frame().titlebar(),
                                       0, 0, 10, 10);
                stateSig().attach(winbtn);
                winbtn->setOnClick(shade_cmd);
                break;
            case WinButton::MENUICON:
                winbtn = new WinButton(*this, winbutton_theme,
                                       dir[i],
                                       frame().titlebar(),
                                       0, 0, 10, 10);
                titleSig().attach(winbtn);
                winbtn->setOnClick(show_menu_cmd);
                break;
            }


            if (winbtn != 0) {
                winbtn->show();
                if (c == 0)
                    frame().addLeftButton(winbtn);
                else
                    frame().addRightButton(winbtn);
            }
        } //end for i


    } // end for c

    frame().reconfigure();
}

/**
 * reconfigTheme: must be called after frame is reconfigured
 * Client windows need to be made the same size and location as
 * the frame's client area.
 */
void FluxboxWindow::reconfigTheme() {

    m_frame.setBorderWidth(decorations.border ?
                           frame().theme().border().width() : 0);
    if (decorations.handle && frame().theme().handleWidth() != 0)
        frame().showHandle();
    else
        frame().hideHandle();

    ClientList::iterator it = clientList().begin();
    ClientList::iterator it_end = clientList().end();

    int x = m_frame.clientArea().x(),
        y = m_frame.clientArea().y();

    unsigned int width = m_frame.clientArea().width(),
        height = m_frame.clientArea().height();

    for (; it != it_end; ++it) {
        (*it)->moveResize(x, y, width, height);
    }

    sendConfigureNotify();
}

// grab pointer and increase counter.
// we need this to count grab pointers,
// especially at startup, where we can drag/resize while starting
// and causing it to send events to windows later on and make
// two different windows do grab pointer which only one window
// should do at the time
void FluxboxWindow::grabPointer(Window grab_window,
                                Bool owner_events,
                                unsigned int event_mask,
                                int pointer_mode, int keyboard_mode,
                                Window confine_to,
                                Cursor cursor,
                                Time time) {
    XGrabPointer(FbTk::App::instance()->display(),
                 grab_window,
                 owner_events,
                 event_mask,
                 pointer_mode, keyboard_mode,
                 confine_to,
                 cursor,
                 time);
    s_num_grabs++;
}

// ungrab and decrease counter
void FluxboxWindow::ungrabPointer(Time time) {
    XUngrabPointer(FbTk::App::instance()->display(), time);
    s_num_grabs--;
    if (s_num_grabs < 0)
        s_num_grabs = 0;
}

void FluxboxWindow::associateClient(WinClient &client) {

    IconButton *btn = frame().createTab(client);

    FbTk::RefCount<FbTk::Command> setcmd(new SetClientCmd(client));
    btn->setOnClick(setcmd, 1);
    btn->setTextPadding(Fluxbox::instance()->getTabsPadding());
    btn->setPixmap(screen().getTabsUsePixmap());

    m_labelbuttons[&client] = btn;

    FbTk::EventManager &evm = *FbTk::EventManager::instance();

    evm.add(*this, btn->window()); // we take care of button events for this
    evm.add(*this, client.window());
    client.setFluxboxWindow(this);
}

int FluxboxWindow::getDecoMaskFromString(const string &str_label) {
    if (strcasecmp(str_label.c_str(), "NONE") == 0)
        return DECOR_NONE;
    if (strcasecmp(str_label.c_str(), "NORMAL") == 0)
        return DECOR_NORMAL;
    if (strcasecmp(str_label.c_str(), "TINY") == 0)
        return DECOR_TINY;
    if (strcasecmp(str_label.c_str(), "TOOL") == 0)
        return DECOR_TOOL;
    if (strcasecmp(str_label.c_str(), "BORDER") == 0)
        return DECOR_BORDER;
    if (strcasecmp(str_label.c_str(), "TAB") == 0)
        return DECOR_TAB;
    int mask = -1;
    if (str_label.size() > 1 && str_label[0] == '0' && str_label[1] == 'x' ||
        str_label.size() > 0 && isdigit(str_label[0]))
        mask = strtol(str_label.c_str(), NULL, 0);
    return mask;
}

int FluxboxWindow::getOnHead() const {
    return screen().getHead(fbWindow());
}

void FluxboxWindow::setOnHead(int head) {
    if (head > 0 && head <= screen().numHeads()) {
        int cur = screen().getHead(fbWindow());
        bool placed = m_placed;
        move(screen().getHeadX(head) + frame().x() - screen().getHeadX(cur),
             screen().getHeadY(head) + frame().y() - screen().getHeadY(cur));
        m_placed = placed;
    }
}

void FluxboxWindow::placeWindow(int head) {
    int place_x, place_y;
    // we ignore the return value,
    // the screen placement strategy is guaranteed to succeed.
    screen().placementStrategy().placeWindow(*this, head, place_x, place_y);
    move(place_x, place_y);
}

void FluxboxWindow::setWindowType(Focusable::WindowType type) {
    switch (type) {
    case Focusable::TYPE_DOCK:
        /* From Extended Window Manager Hints, draft 1.3:
         *
         * _NET_WM_WINDOW_TYPE_DOCK indicates a dock or panel feature.
         * Typically a Window Manager would keep such windows on top
         * of all other windows.
         *
         */
        setFocusHidden(true);
        setIconHidden(true);
        setDecorationMask(DECOR_NONE);
        moveToLayer(::Layer::DOCK);
        break;
    case Focusable::TYPE_DESKTOP:
        /*
         * _NET_WM_WINDOW_TYPE_DESKTOP indicates a "false desktop" window
         * We let it be the size it wants, but it gets no decoration,
         * is hidden in the toolbar and window cycling list, plus
         * windows don't tab with it and is right on the bottom.
         */
        setFocusHidden(true);
        setIconHidden(true);
        moveToLayer(::Layer::DESKTOP);
        setDecorationMask(DECOR_NONE);
        setTabable(false);
        setMovable(false);
        setResizable(false);
        stick();
        break;
    case Focusable::TYPE_SPLASH:
        /*
         * _NET_WM_WINDOW_TYPE_SPLASH indicates that the
         * window is a splash screen displayed as an application
         * is starting up.
         */
        setDecorationMask(DECOR_NONE);
        setFocusHidden(true);
        setIconHidden(true);
        setMovable(false);
        break;
    case Focusable::TYPE_DIALOG:
        setTabable(false);
        break;
    case Focusable::TYPE_MENU:
    case Focusable::TYPE_TOOLBAR:
        /*
         * _NET_WM_WINDOW_TYPE_TOOLBAR and _NET_WM_WINDOW_TYPE_MENU
         * indicate toolbar and pinnable menu windows, respectively
         * (i.e. toolbars and menus "torn off" from the main
         * application). Windows of this type may set the
         * WM_TRANSIENT_FOR hint indicating the main application window.
         */
        setDecorationMask(DECOR_TOOL);
        setIconHidden(true);
        moveToLayer(::Layer::ABOVE_DOCK);
        break;
    case Focusable::TYPE_NORMAL:
    default:
        break;
    }

    /*
     * NOT YET IMPLEMENTED:
     *   _NET_WM_WINDOW_TYPE_UTILITY
     */
}
