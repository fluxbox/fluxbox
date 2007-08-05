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

#include "FbTk/TextButton.hh"
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
    while (client->transientFor()) {
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

    win.screen().updateNetizenWindowRaise(win.clientWindow());
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

    win.screen().updateNetizenWindowLower(win.clientWindow());
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
        // don't update netizen, as it is only temporary
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
    oplock(false),
    m_hintsig(*this),
    m_statesig(*this),
    m_layersig(*this),
    m_workspacesig(*this),
    m_diesig(*this),
    m_focussig(*this),
    m_titlesig(*this),
    m_attentionsig(*this),
    m_themelistener(*this),
    m_creation_time(0),
    moving(false), resizing(false), shaded(false),
    iconic(false), focused(false),
    stuck(false), m_initialized(false), fullscreen(false),
    maximized(MAX_NONE),
    m_attaching_tab(0),
    m_screen(client.screen()),
    display(FbTk::App::instance()->display()),
    m_button_grab_x(0), m_button_grab_y(0),
    m_last_move_x(0), m_last_move_y(0),
    m_last_resize_h(1), m_last_resize_w(1),
    m_workspace_number(0),
    m_current_state(0),
    m_old_decoration_mask(0),
    m_client(&client),
    m_toggled_decos(false),
    m_shaped(false),
    m_icon_hidden(false),
    m_focus_hidden(false),
    m_old_pos_x(0), m_old_pos_y(0),
    m_old_width(1),  m_old_height(1),
    m_last_button_x(0),  m_last_button_y(0),
    m_frame(client.screen(), tm, client.screen().imageControl(), layer, 0, 0, 100, 100),
    m_layernum(layer.getLayerNum()),
    m_old_layernum(0),
    m_parent(client.screen().rootWindow()),
    m_resize_corner(RIGHTBOTTOM) {

    tm.reconfigSig().attach(&m_themelistener);

    init();
}


FluxboxWindow::~FluxboxWindow() {
    if (WindowCmd<void>::window() == this)
        WindowCmd<void>::setWindow(0);

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

    // deal with extra menus
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ~FluxboxWindow("<<this<<")"<<endl;
#endif // DEBUG
}


void FluxboxWindow::init() {
    m_attaching_tab = 0;
    // magic to detect if moved by hints
    // don't use 0, since setting maximized or fullscreen on the window will set
    // this to 0
    m_old_pos_x = m_screen.width();

    assert(m_client);
    m_client->setFluxboxWindow(this);
    m_client->setGroupLeftWindow(None); // nothing to the left.

    // check for shape extension and whether the window is shaped
    m_shaped = false;

    if (Fluxbox::instance()->haveShape()) {
        Shape::setShapeNotify(winClient());
        m_shaped = Shape::isShaped(winClient());
    }

    frame().setUseShape(!m_shaped);

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

    if (m_client->getBlackboxHint() != 0)
        updateBlackboxHintsFromClient(*m_client);
    else
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

    Fluxbox::instance()->attachSignals(*this);

    // this window is managed, we are now allowed to modify actual state
    m_initialized = true;

    applyDecorations(true);

    grabButtons();

    restoreAttributes();

    if (m_workspace_number >= screen().numberOfWorkspaces())
        m_workspace_number = screen().currentWorkspaceID();

    bool place_window = (m_old_pos_x == static_cast<signed>(m_screen.width()));

    if (fluxbox.isStartup())
        place_window = false;
    else if (m_client->isTransient() ||
        m_client->normal_hint_flags & (PPosition|USPosition)) {

        int real_x = frame().x();
        int real_y = frame().y();

        if (real_x >= 0 &&
            real_y >= 0 &&
            real_x <= (signed) screen().width() &&
            real_y <= (signed) screen().height())
            place_window = false;

    }
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

    if (!place_window)
        moveResize(frame().x(), frame().y(), real_width, real_height);

    screen().getWorkspace(m_workspace_number)->addWindow(*this, place_window);

    if (maximized && functions.maximize) { // start maximized
        // This will set it to the appropriate style of maximisation
        int req_maximized = maximized;
        // NOTE: don't manually change maximized ANYWHERE else, it isn't safe
        maximized = MAX_NONE; // it is not maximized now
        maximize(req_maximized);
    }

    if (stuck) {
        stuck = false;
        stick();
        deiconify(); //we're omnipresent and visible
    }

    if (shaded) { // start shaded
        shaded = false;
        shade();
    }

    if (iconic) {
        iconic = false;
        iconify();
    } else
        deiconify(false);

    struct timeval now;
    gettimeofday(&now, NULL);
    m_creation_time = now.tv_sec;

    sendConfigureNotify();
    // no focus default
    setFocusFlag(false);

    if (m_shaped)
        shape();

    setupWindow();

    FbTk::App::instance()->sync(false);

}

/// apply shape to this window
void FluxboxWindow::shape() {
#ifdef SHAPE
    if (m_shaped) {
        XShapeCombineShape(display,
                           frame().window().window(), ShapeBounding,
                           0, frame().clientArea().y(), // xOff, yOff
                           m_client->window(),
                           ShapeBounding, ShapeSet);
        XFlush(display);
    }
#endif // SHAPE

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

        // figure out which client to raise at the end
        if (FocusControl::focusedFbWindow() == old_win) {
            was_focused = true;
        } else if (FocusControl::focusedFbWindow() != this) {
            FocusControl::FocusedWindows focus_list =
                    screen().focusControl().focusedOrderList();
            FocusControl::FocusedWindows::iterator it = focus_list.begin();
            for (; it != focus_list.end() && !focused_win; ++it) {
                if ((*it)->fbwindow() == this || (*it)->fbwindow() == old_win)
                    focused_win = *it;
            }
        }

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
        if (screen().focusControl().focusNew() ||
                Fluxbox::instance()->isStartup())
            focused_win = &client;

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
        setCurrentClient(client);
        FocusControl::setFocusedWindow(&client);
    } else if (focused_win) {
        setCurrentClient(*focused_win, false);
        if (isIconic() && screen().focusControl().focusNew() && !Fluxbox::instance()->isStartup())
            deiconify();
    } else
        // reparenting puts the new client on top, but the old client is keeping
        // the focus, so we raise it
        m_client->raise();

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

    FbTk::TextButton *label_btn = m_labelbuttons[&client];
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

    screen().focusControl().cycleFocus(m_clientlist, 0);
}

void FluxboxWindow::prevClient() {
    if (numClients() <= 1)
        return;

    screen().focusControl().cycleFocus(m_clientlist, 0, true);
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

    FbTk::TextButton *button = m_labelbuttons[&client];
    // in case the window is being destroyed, but this should never happen
    if (!button)
        return false;

    if (&client != m_client)
        m_screen.focusControl().setScreenFocusedWindow(client);
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

    if (setinput && setInputFocus()) {
        return true;
    }

    return false;
}

void FluxboxWindow::setLabelButtonFocus(WinClient &client, bool value) {
    // make sure it's in our list
    if (client.fbwindow() != this)
        return;

    frame().setLabelButtonFocus(*m_labelbuttons[&client], value);
}

void FluxboxWindow::setAttentionState(bool value) {
    m_attention_state = value;
    m_attentionsig.notify();
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
    updateTitleFromClient(*m_client);
    updateIconNameFromClient(*m_client);

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

    unsigned int modkey = Fluxbox::instance()->getModKey();

    if (modkey) {
        //----grab with "all" modifiers
        FbTk::KeyUtil::grabButton(Button1, modkey, frame().window().window(),
            ButtonReleaseMask | ButtonMotionMask, frame().theme().moveCursor());

        XGrabButton(display, Button2, modkey, frame().window().window(), True,
                    ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

        //---grab with "all" modifiers
        FbTk::KeyUtil::grabButton(Button3, modkey, frame().window().window(),
            ButtonReleaseMask | ButtonMotionMask);
    }
}


void FluxboxWindow::reconfigure() {

    applyDecorations();

    setFocusFlag(focused);

    moveResize(frame().x(), frame().y(), frame().width(), frame().height());

    grabButtons();

    frame().setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());

    updateButtons();
    frame().reconfigure();

    menu().reconfigure();

    typedef FbTk::RefCount<FbTk::Command> CommandRef;
    typedef FbTk::SimpleCommand<FluxboxWindow> WindowCmd;
    CommandRef shade_on_cmd(new WindowCmd(*this, &FluxboxWindow::shadeOn));
    CommandRef shade_off_cmd(new WindowCmd(*this, &FluxboxWindow::shadeOff));
    CommandRef next_tab_cmd(new WindowCmd(*this, &FluxboxWindow::nextClient));
    CommandRef prev_tab_cmd(new WindowCmd(*this, &FluxboxWindow::prevClient));
    CommandRef null_cmd;

    int reverse = 0;
    if (screen().getScrollReverse())
        reverse = 1;

    if (StringUtil::toLower(screen().getScrollAction()) == string("shade")) {
        frame().setOnClickTitlebar(shade_on_cmd, 5 - reverse); // shade on mouse roll
        frame().setOnClickTitlebar(shade_off_cmd, 4 + reverse); // unshade if rolled oposite direction
    } else if (StringUtil::toLower(screen().getScrollAction()) == string("nexttab")) {
        frame().setOnClickTitlebar(next_tab_cmd, 5 - reverse); // next tab
        frame().setOnClickTitlebar(prev_tab_cmd, 4 + reverse); // previous tab
    } else {
        frame().setOnClickTitlebar(null_cmd, 4);
        frame().setOnClickTitlebar(null_cmd, 5);
    }

}

/// update current client title and title in our frame
void FluxboxWindow::updateTitleFromClient(WinClient &client) {
    client.updateTitle();
    // compare old title with new and see if we need to update
    // graphics
    if (m_labelbuttons[&client]->text() != client.title()) {
        m_labelbuttons[&client]->setText(client.title());
        if (&client == m_client)
            frame().setFocusTitle(client.title());
    }
}

/// update icon title from client
void FluxboxWindow::updateIconNameFromClient(WinClient &client) {
    client.updateIconTitle();
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

void FluxboxWindow::updateBlackboxHintsFromClient(const WinClient &client) {
    const FluxboxWindow::BlackboxHints *hint = client.getBlackboxHint();
    if (!hint) return;

    if (hint->flags & ATTRIB_SHADED)
        shaded = (hint->attrib & ATTRIB_SHADED);

    if (hint->flags & ATTRIB_HIDDEN)
        iconic = (hint->attrib & ATTRIB_HIDDEN);

    if ((hint->flags & ATTRIB_MAXHORIZ) &&
        (hint->flags & ATTRIB_MAXVERT))
        maximized = ((hint->attrib &
                      (ATTRIB_MAXHORIZ |
                       ATTRIB_MAXVERT)) ? MAX_FULL : MAX_NONE);
    else if (hint->flags & ATTRIB_MAXVERT)
        maximized = ((hint->attrib &
                      ATTRIB_MAXVERT) ? MAX_VERT : MAX_NONE);
    else if (hint->flags & ATTRIB_MAXHORIZ)
        maximized = ((hint->attrib &
                      ATTRIB_MAXHORIZ) ? MAX_HORZ : MAX_NONE);

    if (hint->flags & ATTRIB_OMNIPRESENT)
        stuck = (hint->attrib & ATTRIB_OMNIPRESENT);

    if (hint->flags & ATTRIB_WORKSPACE)
        m_workspace_number = hint->workspace;

    if (hint->flags & ATTRIB_STACK)
        m_workspace_number = hint->stack;

    if (hint->flags & ATTRIB_DECORATION) {
        setDecoration(static_cast<Decoration>(hint->decoration), false);
    }
}

void FluxboxWindow::move(int x, int y) {
    moveResize(x, y, frame().width(), frame().height());
}

void FluxboxWindow::resize(unsigned int width, unsigned int height) {
    int old_x = m_old_pos_x;

    moveResize(frame().x(), frame().y(), width, height);

    // magic to detect if moved during initialisation
    // we restore the old state, because we were a resize, not a moveResize!
    if (!m_initialized)
        m_old_pos_x = old_x;
}

// send_event is just an override
void FluxboxWindow::moveResize(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height,
                               bool send_event) {

    // magic to detect if moved during initialisation
    if (!m_initialized)
        m_old_pos_x = 1;

    send_event = send_event || frame().x() != new_x || frame().y() != new_y;

    if ((new_width != frame().width() || new_height != frame().height()) &&
        isResizable() && !isShaded()) {

        if ((((signed) frame().width()) + new_x) < 0)
            new_x = 0;
        if ((((signed) frame().height()) + new_y) < 0)
            new_y = 0;

        frame().moveResize(new_x, new_y, new_width, new_height);
        setFocusFlag(focused);

        send_event = true;
    } else if (send_event)
        frame().move(new_x, new_y);

    if (send_event && ! moving) {
        sendConfigureNotify();
    }

    shape();

    if (!moving) {
        m_last_resize_x = new_x;
        m_last_resize_y = new_y;
    }

}

void FluxboxWindow::moveResizeForClient(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height, int gravity, unsigned int client_bw) {

    // magic to detect if moved during initialisation
    if (!m_initialized)
        m_old_pos_x = 1;
    frame().moveResizeForClient(new_x, new_y, new_width, new_height, gravity, client_bw);
    setFocusFlag(focused);
    shaded = false;
    sendConfigureNotify();

    shape();

    if (!moving) {
        m_last_resize_x = new_x;
        m_last_resize_y = new_y;
    }

}




// returns whether the focus was "set" to this window
// it doesn't guarantee that it has focus, but says that we have
// tried. A FocusqIn event should eventually arrive for that
// window if it actually got the focus, then setFocusedFlag is called,
// which updates all the graphics etc
bool FluxboxWindow::setInputFocus() {

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

    bool ret = false;

    if (m_client->getFocusMode() == WinClient::F_LOCALLYACTIVE ||
        m_client->getFocusMode() == WinClient::F_PASSIVE) {

        m_client->setInputFocus(RevertToPointerRoot, CurrentTime);

        FbTk::App *app = FbTk::App::instance();

        XFlush(app->display());

        m_client->sendFocus();

        app->sync(false);

        ret = true;
    } else {
        ret = m_client->sendFocus();
    }

    return ret;
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

    menu().hide();
    frame().hide();
}

void FluxboxWindow::show() {
    frame().show();
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

    m_blackbox_attrib.flags |= ATTRIB_HIDDEN;
    m_blackbox_attrib.attrib |= ATTRIB_HIDDEN;

    iconic = true;

    setState(IconicState, false);

    hide(true);

    screen().focusControl().setFocusBack(this);

    ClientList::iterator client_it = m_clientlist.begin();
    const ClientList::iterator client_it_end = m_clientlist.end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient &client = *(*client_it);
        client.setEventMask(NoEventMask);
        client.hide();
        client.setEventMask(PropertyChangeMask | StructureNotifyMask | FocusChangeMask);
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

    m_blackbox_attrib.flags &= ~ATTRIB_HIDDEN;
    iconic = false;

    setState(NormalState, false);

    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_end = clientList().end();
    for (; client_it != client_it_end; ++client_it) {
        (*client_it)->setEventMask(NoEventMask);
        (*client_it)->show();
        (*client_it)->setEventMask(PropertyChangeMask | StructureNotifyMask | FocusChangeMask);
    }

    if (reassoc) {
        // deiconify all transients
        client_it = clientList().begin();
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
    if (was_iconic && (screen().focusControl().focusNew() || screen().currentWorkspace()->numberOfWindows() == 1))
        setInputFocus();


    oplock = false;

    if (do_raise)
        raise();
}

/**
 Set window in withdrawn state
*/
void FluxboxWindow::withdraw(bool interrupt_moving) {
#ifdef DEBUG
    cerr<<"FluxboxWindow::"<<__FUNCTION__<<": this = "<<this<<endl;
#endif // DEBUG
    iconic = false;

    hide(interrupt_moving);
}

/** setFullscreen mode:

    - maximize as big as the screen is, dont care about slit / toolbar
    - raise to toplayer
*/
void FluxboxWindow::setFullscreen(bool flag) {

    const int head = screen().getHead(fbWindow());

    if (flag && !isFullscreen()) {

        if (isIconic())
            deiconify();

        if (isShaded())
            shade();

        frame().setUseShape(false);

        if (!m_toggled_decos)
            m_old_decoration_mask = decorationMask();

        m_old_layernum = layerNum();
        m_old_pos_x = frame().x();
        m_old_pos_y = frame().y();
        m_old_width = frame().width();
        m_old_height = frame().height();

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

        frame().setUseShape(!m_shaped);
        if (m_toggled_decos) {
            if (m_old_decoration_mask & DECORM_TITLEBAR)
                setDecoration(DECOR_NONE);
            else
                setDecoration(DECOR_NORMAL);
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

        stateSig().notify();
    }
}

/**
   Maximize window both horizontal and vertical
*/
void FluxboxWindow::maximize(int type) {

    // doesn't make sense to maximize
    if (isFullscreen() || type == MAX_NONE)
        return;

    if (isIconic())
        deiconify();

    if (isShaded())
        shade();

    if (isResizing())
        stopResizing();

    int head = screen().getHead(frame().window());
    int new_x = frame().x(),
        new_y = frame().y(),
        new_w = frame().width(),
        new_h = frame().height();

    int orig_max = maximized;

    // These evaluate whether we need to TOGGLE the value for that field
    // Why? If maximize is only set to zero outside this,
    // and we only EVER toggle them, then:
    // 1) We will never loose the old_ values
    // 2) It shouldn't get confused

    // Worst case being that some action will toggle the wrong way, but
    // we still won't lose the state in that case.

    // toggle maximize vertically?
    // when _don't_ we want to toggle?
    // - type is horizontal maximise, or
    // - type is full and we are not maximised horz but already vertically
    if (type != MAX_HORZ && !(type == MAX_FULL && orig_max == MAX_VERT)) {
        // already maximized in that direction?
        if (orig_max & MAX_VERT) {
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

    // maximize horizontally?
    if (type != MAX_VERT && !(type == MAX_FULL && orig_max == MAX_HORZ)) {
        // already maximized in that direction?
        if (orig_max & MAX_HORZ) {
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

    ResizeDirection old_resize_corner = m_resize_corner;
    m_resize_corner = NOCORNER;
    fixsize(0, 0, true);
    m_resize_corner = old_resize_corner;

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

    m_blackbox_attrib.flags |= ATTRIB_WORKSPACE;
    m_blackbox_attrib.workspace = m_workspace_number;

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

    if (shaded) {
        shaded = false;
        m_blackbox_attrib.flags ^= ATTRIB_SHADED;
        m_blackbox_attrib.attrib ^= ATTRIB_SHADED;

        if (m_initialized)
            setState(NormalState, false);
    } else {
        shaded = true;
        m_blackbox_attrib.flags |= ATTRIB_SHADED;
        m_blackbox_attrib.attrib |= ATTRIB_SHADED;
        // shading is the same as iconic
        if (m_initialized)
            setState(IconicState, false);
    }

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

    if (stuck) {
        m_blackbox_attrib.flags ^= ATTRIB_OMNIPRESENT;
        m_blackbox_attrib.attrib ^= ATTRIB_OMNIPRESENT;

        stuck = false;

    } else {
        stuck = true;

        m_blackbox_attrib.flags |= ATTRIB_OMNIPRESENT;
        m_blackbox_attrib.attrib |= ATTRIB_OMNIPRESENT;

    }

    if (m_initialized) {
        setState(m_current_state, false);
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

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;
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

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    if (client->fbwindow())
        lowerFluxboxWindow(*client->fbwindow());
}

void FluxboxWindow::tempRaise() {
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

    if (m_layernum == layernum && !force)
        return;

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    FluxboxWindow *win = client->fbwindow();
    if (!win) return;

    if (!win->isIconic()) {
        if (layernum > m_layernum)
            screen().updateNetizenWindowLower(client->window());
        else
            screen().updateNetizenWindowRaise(client->window());
    }
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
                screen().updateNetizenWindowRaise((*it)->window());
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
    bool was_focused = isFocused();
    focused = focus;
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::setFocusFlag("<<focus<<")"<<endl;
#endif // DEBUG

    installColormap(focus);

    if (focus != frame().focused())
        frame().setFocus(focus);

    if (screen().doAutoRaise() && !screen().focusControl().isCycling()) {
        if (focused)
            m_timer.start();
        else
            m_timer.stop();
    }

    // did focus change? notify listeners
    if (was_focused != focus) {
        m_focussig.notify();
        if (m_client)
            m_client->focusSig().notify();
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
    if (numClients() == 0)
        return;

    m_current_state = new_state;
    if (!setting_up) {
        unsigned long state[2];
        state[0] = (unsigned long) m_current_state;
        state[1] = (unsigned long) None;

        for_each(m_clientlist.begin(), m_clientlist.end(),
                 FbTk::ChangeProperty(display, FbAtoms::instance()->getWMStateAtom(),
                                      PropModeReplace,
                                      (unsigned char *)state, 2));

        saveBlackboxAttribs();
        //notify state changed
        m_statesig.notify();
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

    if (m_blackbox_attrib.flags & ATTRIB_SHADED &&
        m_blackbox_attrib.attrib & ATTRIB_SHADED)
        shaded = true;

    if (m_blackbox_attrib.flags & ATTRIB_HIDDEN &&
        m_blackbox_attrib.attrib & ATTRIB_HIDDEN) {
        iconic = true;
    }

    if (( m_blackbox_attrib.workspace != screen().currentWorkspaceID()) &&
        ( m_blackbox_attrib.workspace < screen().numberOfWorkspaces()))
        m_workspace_number = m_blackbox_attrib.workspace;

    if (m_blackbox_attrib.flags & ATTRIB_OMNIPRESENT &&
        m_blackbox_attrib.attrib & ATTRIB_OMNIPRESENT)
        stuck = true;

    if (m_blackbox_attrib.flags & ATTRIB_STACK) {
        //!! TODO check value?
        m_layernum = m_blackbox_attrib.stack;
    }

    if ((m_blackbox_attrib.flags & ATTRIB_MAXHORIZ) ||
        (m_blackbox_attrib.flags & ATTRIB_MAXVERT)) {
        int x = m_blackbox_attrib.premax_x, y = m_blackbox_attrib.premax_y;
        unsigned int w = m_blackbox_attrib.premax_w, h = m_blackbox_attrib.premax_h;
        maximized = MAX_NONE;
        if ((m_blackbox_attrib.flags & ATTRIB_MAXHORIZ) &&
            (m_blackbox_attrib.flags & ATTRIB_MAXVERT))
            maximized = MAX_FULL;
        else if (m_blackbox_attrib.flags & ATTRIB_MAXVERT)
            maximized = MAX_VERT;
        else if (m_blackbox_attrib.flags & ATTRIB_MAXHORIZ)
            maximized = MAX_HORZ;

        m_blackbox_attrib.premax_x = x;
        m_blackbox_attrib.premax_y = y;
        m_blackbox_attrib.premax_w = w;
        m_blackbox_attrib.premax_h = h;
    }

}

/**
   Show the window menu at pos mx, my
*/
void FluxboxWindow::showMenu(int menu_x, int menu_y, WinClient *client) {
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

    if (client && (client->fbwindow() == this))
        WindowCmd<void>::setClient(client);
    else
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

    /* Check if we're on a tab, we should make the menu for that tab */
    WinClient *client = 0;
    Window labelbutton = 0;
    int dest_x = 0, dest_y = 0;
    if (XTranslateCoordinates(FbTk::App::instance()->display(),
                              parent().window(), frame().tabcontainer().window(),
                              m_last_button_x, m_last_button_y, &dest_x, &dest_y,
                              &labelbutton)) {

        Client2ButtonMap::iterator it =
            find_if(m_labelbuttons.begin(),
                    m_labelbuttons.end(),
                    Compose(bind2nd(equal_to<Window>(), labelbutton),
                            Compose(mem_fun(&TextButton::window),
                                    Select2nd<Client2ButtonMap::value_type>())));
    
        // label button not found
        if (it != m_labelbuttons.end())
            client = it->first;
    }

    menu().disableTitle();
    int menu_y = frame().titlebar().height() + frame().titlebar().borderWidth();
    if (!decorations.titlebar) // if we don't have any titlebar
        menu_y = 0;
    if (m_last_button_x < x() || m_last_button_x > x() + static_cast<signed>(width()))
        m_last_button_x = x();
    showMenu(m_last_button_x, menu_y + frame().y(), client);
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

            if (shape_event->kind != ShapeBounding)
                break;

            if (shape_event->shaped) {
                m_shaped = true;
                shape();
            } else {
                m_shaped = false;
                // set no shape
                XShapeCombineMask(display,
                                  frame().window().window(), ShapeBounding,
                                  0, 0,
                                  None, ShapeSet);
            }

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
    setCurrentClient(*client, false); // focus handled on MapNotify
    deiconify(false);

}


void FluxboxWindow::mapNotifyEvent(XMapEvent &ne) {
    WinClient *client = findClient(ne.window);
    if (client == 0 || client != m_client)
        return;
#ifdef DEBUG
    cerr<<"FluxboxWindow::mapNotifyEvent: "
        <<"ne.override_redirect = "<<ne.override_redirect
        <<" isVisible() = "<<isVisible()<<endl;
#endif // DEBUG

    if (!ne.override_redirect && isVisible()) {
#ifdef DEBUG
        cerr<<"FluxboxWindow::mapNotify: not override redirect ans visible!"<<endl;
#endif // DEBUG
        Fluxbox *fluxbox = Fluxbox::instance();
        fluxbox->grab();
        if (! client->validateClient())
            return;

        setState(NormalState, false);

        FluxboxWindow *cur = FocusControl::focusedFbWindow();
        if (client->isTransient() ||
            m_screen.currentWorkspace()->numberOfWindows() == 1 ||
            m_screen.focusControl().focusNew() && !(cur && cur->isFullscreen()))
            setCurrentClient(*client, true);
        else if (m_screen.focusControl().focusNew())
            Fluxbox::instance()->attentionHandler().addAttention(*client);


        iconic = false;

        // Auto-group from tab?
        if (!client->isTransient()) {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__FUNCTION__<<") TODO check grouping here"<<endl;
#endif // DEBUG
        }

        fluxbox->ungrab();
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
        hintSig().notify(); // notify listeners
        break;

    case XA_WM_ICON_NAME:
        // update icon title and then do normal XA_WM_NAME stuff
        client.updateIconTitle();
    case XA_WM_NAME:
        updateTitleFromClient(client);
        titleSig().notify();
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

            if (changed)
                setupWindow();
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
        } else if (atom == fbatoms->getFluxboxHintsAtom()) {
            client.updateBlackboxHints();
            updateBlackboxHintsFromClient(client);
            if (client.getBlackboxHint() != 0 &&
                (client.getBlackboxHint()->flags & ATTRIB_DECORATION)) {
                updateRememberStateFromClient(client);
                applyDecorations(); // update decoration
            }
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
        else {
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

    // whether we should send ConfigureNotify to netizens
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
            setCurrentClient(*client, focused);
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


void FluxboxWindow::buttonPressEvent(XButtonEvent &be) {
    m_last_button_x = be.x_root;
    m_last_button_y = be.y_root;

    // check frame events first
    frame().buttonPressEvent(be);

    if (be.button == 1 || (be.button == 3 &&
                           be.state == Fluxbox::instance()->getModKey())) {
        if ( (! focused) ) { //check focus
            setInputFocus();
        }

        if (frame().window().window() == be.window || frame().tabcontainer().window() == be.window) {
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

    if ((re.button == 1) && (re.state & Mod1Mask) && !screen().clickRaises())
        if (!isMoving())
            raise();

    if (isMoving())
        stopMoving();
    else if (isResizing())
        stopResizing();
    else if (m_attaching_tab)
        attachTo(re.x_root, re.y_root);
    else if (re.window == frame().window()) {
        if (re.button == 2 && re.state == Fluxbox::instance()->getModKey())
            ungrabPointer(CurrentTime);
        else
            frame().buttonReleaseEvent(re);
    } else {
        frame().buttonReleaseEvent(re);
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

    if (Fluxbox::instance()->getIgnoreBorder()
        && !(me.state & Fluxbox::instance()->getModKey()) // really should check for exact matches
        && !(isMoving() || isResizing() || m_attaching_tab != 0)) {
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

                    screen().changeWorkspaceID(new_id);
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

          int cx = frame().width() / 2;
          int cy = frame().height() / 2;
          ResizeDirection resize_corner = RIGHTBOTTOM;
          if (me.window == frame().gripRight())
              resize_corner = RIGHTBOTTOM;
          else if (me.window == frame().gripLeft())
              resize_corner = LEFTBOTTOM;
          else if (screen().getResizeModel() != BScreen::QUADRANTRESIZE) {
              if (screen().getResizeModel() == BScreen::CENTERRESIZE)
                  resize_corner = ALLCORNERS;
              else
                  resize_corner = RIGHTBOTTOM;
          } else if (me.x < cx)
              resize_corner = (me.y < cy) ? LEFTTOP : LEFTBOTTOM;
          else
              resize_corner = (me.y < cy) ? RIGHTTOP : RIGHTBOTTOM;


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

        if (screen().focusControl().isMouseFocus() && !isFocused()) {

            // check that there aren't any subsequent leave notify events in the
            // X event queue
            XEvent dummy;
            scanargs sa;
            sa.w = ev.window;
            sa.enter = sa.leave = False;
            XCheckIfEvent(display, &dummy, queueScanner, (char *) &sa);

            if ((!sa.leave || sa.inferior) && !screen().focusControl().isCycling() ) {
                setInputFocus();
            }
        }
    }

    if (screen().focusControl().isMouseTabFocus() && client && client != m_client) {
        setCurrentClient(*client, isFocused());
    }

}

void FluxboxWindow::leaveNotifyEvent(XCrossingEvent &ev) {
    // I hope commenting this out is right - simon 21jul2003
    //if (ev.window == frame().window())
    //installColormap(false);
}

// TODO: functions should not be affected by decoration
void FluxboxWindow::setDecoration(Decoration decoration, bool apply) {
    switch (decoration) {
    case DECOR_NONE:
        decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.tab = false; //tab is also a decor
        decorations.menu = true; // menu is present
    //  functions.iconify = functions.maximize = true;
    //  functions.move = true;   // We need to move even without decor
    //  functions.resize = true; // We need to resize even without decor
    break;

    default:
    case DECOR_NORMAL:
        decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.menu = decorations.tab = true;
        functions.resize = functions.move = functions.iconify =
            functions.maximize = true;
    break;

    case DECOR_TAB:
        decorations.border = decorations.iconify = decorations.maximize =
            decorations.menu = decorations.tab = true;
        decorations.titlebar = decorations.handle = false;
        functions.resize = functions.move = functions.iconify =
            functions.maximize = true;
    break;

    case DECOR_TINY:
        decorations.titlebar = decorations.iconify = decorations.menu =
            functions.move = functions.iconify = decorations.tab = true;
        decorations.border = decorations.handle = decorations.maximize =
            functions.resize = functions.maximize = false;
    break;

    case DECOR_TOOL:
        decorations.titlebar = decorations.tab = decorations.menu = functions.move = true;
        decorations.iconify = decorations.border = decorations.handle =
            decorations.maximize = functions.resize = functions.maximize =
            functions.iconify = false;
    break;
    }

    // we might want to wait with apply decorations
    if (apply)
        applyDecorations();

    //!! TODO: make sure this is correct
    // is this reconfigure necessary???
    //    reconfigure();

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
    if (!initial && client_move) {
        Fluxbox::instance()->updateFrameExtents(*this);
        sendConfigureNotify();
    }

}

void FluxboxWindow::toggleDecoration() {
    //don't toggle decor if the window is shaded
    if (isShaded() || isFullscreen())
        return;

    m_toggled_decos = !m_toggled_decos;

    if (m_toggled_decos) {
        m_old_decoration_mask = decorationMask();
        if (decorations.titlebar)
            setDecoration(DECOR_NONE);
        else
            setDecoration(DECOR_NORMAL);
    } else
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

    // save first event point
    m_last_resize_x = x;
    m_last_resize_y = y;
    m_button_grab_x = x - frame().x() - frame().window().borderWidth();
    m_button_grab_y = y - frame().y() - frame().window().borderWidth();

    moving = true;
    maximized = MAX_NONE;

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
                setInputFocus();
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
        setInputFocus();
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

    if (decorationMask() & (DECORM_ENABLED|DECORM_BORDER|DECORM_HANDLE))
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

        bw = (*it)->decorationMask() & (DECORM_ENABLED|DECORM_BORDER|DECORM_HANDLE) ?
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


void FluxboxWindow::startResizing(int x, int y, ResizeDirection dir) {

    if (s_num_grabs > 0 || isShaded() || isIconic() )
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

const FbTk::FbPixmap &FluxboxWindow::iconPixmap() const { return m_client->iconPixmap(); }
const FbTk::FbPixmap &FluxboxWindow::iconMask() const { return m_client->iconMask(); }

const bool FluxboxWindow::usePixmap() const { 
    return m_client ? m_client->usePixmap() : false; 
}

const bool FluxboxWindow::useMask() const { return m_client->useMask(); }

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
    static string empty_string;
    if (m_client == 0)
        return empty_string;
    return m_client->title();
}

const string &FluxboxWindow::iconTitle() const {
    static string empty_string;
    if (m_client == 0)
        return empty_string;
    return m_client->iconTitle();
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

void FluxboxWindow::changeBlackboxHints(const BlackboxHints &net) {
    if ((net.flags & ATTRIB_SHADED) &&
        ((m_blackbox_attrib.attrib & ATTRIB_SHADED) !=
         (net.attrib & ATTRIB_SHADED)))
        shade();

    if ((net.flags & ATTRIB_HIDDEN) &&
        ((m_blackbox_attrib.attrib & ATTRIB_HIDDEN) !=
         (net.attrib & ATTRIB_HIDDEN))) {
        bool want_iconic = net.attrib & ATTRIB_HIDDEN;
        if (!iconic && want_iconic)
            iconify();
        else if (iconic && !want_iconic)
            deiconify();
    }

    if (net.flags & (ATTRIB_MAXVERT | ATTRIB_MAXHORIZ)) {
        // make maximise look like the net maximise flags
        int want_max = MAX_NONE;

        if (net.flags & ATTRIB_MAXVERT)
            want_max |= MAX_VERT;
        if (net.flags & ATTRIB_MAXHORIZ)
            want_max |= MAX_HORZ;

        if (want_max == MAX_NONE && maximized != MAX_NONE) {
            maximize(maximized);
        } else if (want_max == MAX_FULL && maximized != MAX_FULL) {
            maximize(MAX_FULL);
        } else {
            // either we want vert and aren't
            // or we want horizontal and aren't
            if (want_max == MAX_VERT ^ (bool)(maximized & MAX_VERT))
                maximize(MAX_VERT);
            if (want_max == MAX_HORZ ^ (bool)(maximized & MAX_HORZ))
                maximize(MAX_HORZ);
        }
    }

    if ((net.flags & ATTRIB_OMNIPRESENT) &&
        ((m_blackbox_attrib.attrib & ATTRIB_OMNIPRESENT) !=
         (net.attrib & ATTRIB_OMNIPRESENT)))
        stick();

    if ((net.flags & ATTRIB_WORKSPACE) &&
        (m_workspace_number !=  net.workspace)) {

        screen().reassociateWindow(this, net.workspace, true);

        if (screen().currentWorkspaceID() != net.workspace)
            withdraw(true);
        else
            deiconify();
    }

    if (net.flags & ATTRIB_STACK) {
        if ((unsigned int) m_layernum != net.stack) {
            moveToLayer(net.stack);
        }
    }

    if (net.flags & ATTRIB_DECORATION) {
        setDecoration(static_cast<Decoration>(net.decoration));
    }

}


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

void FluxboxWindow::sendConfigureNotify(bool send_to_netizens) {
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

        if (send_to_netizens) {
            XEvent event;
            event.type = ConfigureNotify;

            event.xconfigure.display = display;
            event.xconfigure.event = client.window();
            event.xconfigure.window = client.window();
            event.xconfigure.x = frame().x() + frame().clientArea().x();
            event.xconfigure.y = frame().y() + frame().clientArea().y();
            event.xconfigure.width = client.width();
            event.xconfigure.height = client.height();
            event.xconfigure.border_width = client.old_bw;
            event.xconfigure.above = frame().window().window();
            event.xconfigure.override_redirect = false;

            screen().updateNetizenConfigNotify(event);
        }
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
    typedef RefCount<Command> CommandRef;
    typedef SimpleCommand<FluxboxWindow> WindowCmd;

    CommandRef shade_cmd(new WindowCmd(*this, &FluxboxWindow::shade));
    CommandRef shade_on_cmd(new WindowCmd(*this, &FluxboxWindow::shadeOn));
    CommandRef shade_off_cmd(new WindowCmd(*this, &FluxboxWindow::shadeOff));
    CommandRef next_tab_cmd(new WindowCmd(*this, &FluxboxWindow::nextClient));
    CommandRef prev_tab_cmd(new WindowCmd(*this, &FluxboxWindow::prevClient));
    CommandRef lower_cmd(new WindowCmd(*this, &FluxboxWindow::lower));
    CommandRef raise_and_focus_cmd(new WindowCmd(*this, &FluxboxWindow::raiseAndFocus));
    CommandRef stick_cmd(new WindowCmd(*this, &FluxboxWindow::stick));
    CommandRef show_menu_cmd(new WindowCmd(*this, &FluxboxWindow::popupMenu));

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

    // setup titlebar
    frame().setOnClickTitlebar(raise_and_focus_cmd, 1, false, true); // on press with button 1
    frame().setOnClickTitlebar(shade_cmd, 1, true); // doubleclick with button 1
    frame().setOnClickTitlebar(show_menu_cmd, 3); // on release with button 3
    frame().setOnClickTitlebar(lower_cmd, 2); // on release with button 2

    int reverse = 0;
    if (screen().getScrollReverse())
        reverse = 1;

    if (StringUtil::toLower(screen().getScrollAction()) == string("shade")) {
        frame().setOnClickTitlebar(shade_on_cmd, 5 - reverse); // shade on mouse roll
        frame().setOnClickTitlebar(shade_off_cmd, 4 + reverse); // unshade if rolled oposite direction
    } else if (StringUtil::toLower(screen().getScrollAction()) == string("nexttab")) {
        frame().setOnClickTitlebar(next_tab_cmd, 5 - reverse); // next tab
        frame().setOnClickTitlebar(prev_tab_cmd, 4 + reverse); // previous tab
    }

    frame().setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());

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
                hintSig().attach(winbtn);
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

    FbWinFrame::ButtonId btn = frame().createTab(client.title(),
                                                 new SetClientCmd(client),
                                                 Fluxbox::instance()->getTabsPadding());

    m_labelbuttons[&client] = btn;



    FbTk::EventManager &evm = *FbTk::EventManager::instance();

    evm.add(*this, btn->window()); // we take care of button events for this
    evm.add(*this, client.window());
    client.setFluxboxWindow(this);
}

int FluxboxWindow::getDecoMaskFromString(const string &str_label) {
    if (strcasecmp(str_label.c_str(), "NONE") == 0)
        return 0;
    if (strcasecmp(str_label.c_str(), "NORMAL") == 0)
        return FluxboxWindow::DECORM_LAST - 1;
    if (strcasecmp(str_label.c_str(), "TINY") == 0)
        return FluxboxWindow::DECORM_TITLEBAR
               | FluxboxWindow::DECORM_ICONIFY
               | FluxboxWindow::DECORM_MENU
               | FluxboxWindow::DECORM_TAB;
    if (strcasecmp(str_label.c_str(), "TOOL") == 0)
        return FluxboxWindow::DECORM_TITLEBAR
               | FluxboxWindow::DECORM_MENU;
    if (strcasecmp(str_label.c_str(), "BORDER") == 0)
        return FluxboxWindow::DECORM_BORDER
               | FluxboxWindow::DECORM_MENU;
    if (strcasecmp(str_label.c_str(), "TAB") == 0)
        return FluxboxWindow::DECORM_BORDER
               | FluxboxWindow::DECORM_MENU
               | FluxboxWindow::DECORM_TAB;
    unsigned int mask = atoi(str_label.c_str());
    if (mask)
        return mask;
    return -1;
}
