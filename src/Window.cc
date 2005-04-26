// Window.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
#include "Remember.hh"
#include "MenuCreator.hh"

#include "FbTk/I18n.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/Compose.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/SimpleCommand.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
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

using namespace std;

namespace {

void grabButton(Display *display, unsigned int button,
                Window window, Cursor cursor) {

    const int numlock = FbTk::KeyUtil::instance().numlock();
    const int capslock = FbTk::KeyUtil::instance().capslock();
    const int scrolllock = FbTk::KeyUtil::instance().scrolllock();

    // Grab with Mod1 and with all lock modifiers
    // (num, scroll and caps)

    //numlock
    XGrabButton(display, button, Mod1Mask|numlock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);
    //scrolllock
    XGrabButton(display, button, Mod1Mask|scrolllock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock
    XGrabButton(display, button, Mod1Mask|capslock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock+numlock
    XGrabButton(display, Button1, Mod1Mask|capslock|numlock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock+scrolllock
    XGrabButton(display, button, Mod1Mask|capslock|scrolllock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock+numlock+scrolllock
    XGrabButton(display, button, Mod1Mask|capslock|numlock|scrolllock, window,
                True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //numlock+scrollLock
    XGrabButton(display, button, Mod1Mask|numlock|scrolllock, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

}

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
    if (win.oplock) return;
    win.oplock = true;
#ifdef DEBUG
    cerr<<"raiseFluxboxWindow("<<win.title()<<")"<<endl;
#endif // DEBUG
    // we need to lock actual restacking so that raising above active transient
    // won't do anything nasty
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().lock();

    if (!win.isIconic()) {
        win.screen().updateNetizenWindowRaise(win.clientWindow());
        win.layerItem().raise();
    }

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
#ifdef DEBUG
    cerr<<"window("<<win.title()<<") transient size: "<<win.winClient().transientList().size()<<endl;
#endif // DEBUG
}

/// lower window and do the same for each transient it holds
void lowerFluxboxWindow(FluxboxWindow &win) {
    if (win.oplock) return;
    win.oplock = true;

    // we need to lock actual restacking so that raising above active transient
    // won't do anything nasty
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().lock();

    if (!win.isIconic()) {
        win.screen().updateNetizenWindowLower(win.clientWindow());
        win.layerItem().lower();
    }

    WinClient::TransientList::const_iterator it = win.winClient().transientList().begin();
    WinClient::TransientList::const_iterator it_end = win.winClient().transientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && !(*it)->fbwindow()->isIconic())
            // TODO: should we also check if it is the active client?
            lowerFluxboxWindow(*(*it)->fbwindow());
    }
    win.oplock = false;
    if (!win.winClient().transientList().empty())
        win.screen().layerManager().unlock();
}

/// raise window and do the same for each transient it holds
void tempRaiseFluxboxWindow(FluxboxWindow &win) {
    if (win.oplock) return;
    win.oplock = true;

    if (!win.winClient().transientList().empty())
        win.screen().layerManager().lock();

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

    if (!win.winClient().transientList().empty())
        win.screen().layerManager().unlock();

}

class SetClientCmd:public FbTk::Command {
public:
    explicit SetClientCmd(WinClient &client):m_client(client) {
    }
    void execute() {
        if (m_client.m_win != 0)
            m_client.m_win->setCurrentClient(m_client);
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
    m_themelistener(*this),
    moving(false), resizing(false), shaded(false),
    iconic(false), focused(false),
    stuck(false), m_initialized(false), fullscreen(false),
    maximized(MAX_NONE),
    m_attaching_tab(0),
    m_screen(client.screen()),
    display(FbTk::App::instance()->display()),
    m_windowmenu(MenuCreator::createMenu("", client.screenNumber())),
    m_button_grab_x(0), m_button_grab_y(0),
    m_last_move_x(0), m_last_move_y(0),
    m_last_resize_h(1), m_last_resize_w(1),
    m_workspace_number(0),
    m_current_state(0),
    m_old_decoration(DECOR_NORMAL),
    m_old_decoration_mask(0),
    m_client(&client),
    m_toggled_decos(false),
    m_shaped(false),
    m_icon_hidden(false),
    m_old_pos_x(0), m_old_pos_y(0),
    m_old_width(1),  m_old_height(1),
    m_last_button_x(0),  m_last_button_y(0),
    m_frame(tm, client.screen().imageControl(), 0, 0, 100, 100),
    m_layeritem(m_frame.window(), layer),
    m_layernum(layer.getLayerNum()),
    m_old_layernum(0),
    m_parent(client.screen().rootWindow()),
    m_resize_corner(RIGHTBOTTOM) {

    tm.reconfigSig().attach(&m_themelistener);

    init();
}


FluxboxWindow::~FluxboxWindow() {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): starting ~FluxboxWindow("<<this<<", "<<title()<<")"<<endl;
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

    Client2ButtonMap::iterator it = m_labelbuttons.begin();
    Client2ButtonMap::iterator it_end = m_labelbuttons.end();
    for (; it != it_end; ++it) {
        frame().removeLabelButton(*(*it).second);
        delete (*it).second;
    }
    m_labelbuttons.clear();

    m_timer.stop();

    // notify die
    m_diesig.notify();

    if (m_client != 0)
        delete m_client; // this also removes client from our list
    m_client = 0;

    if (m_clientlist.size() > 1) {
        cerr<<__FILE__<<"(~FluxboxWindow()) WARNING! clientlist > 1"<<endl;
        while (!m_clientlist.empty()) {
            detachClient(*m_clientlist.back());
        }
    }

    // deal with extra menus
    ExtraMenus::iterator mit = m_extramenus.begin();
    ExtraMenus::iterator mit_end = m_extramenus.end();
    for (; mit != mit_end; ++mit) {
        // we set them to NOT internal so that they will be deleted when the
        // menu is cleaned up. We can't delete them here because they are
        // still in the menu
        // (They need to be internal for most of the time so that if we
        // rebuild the menu, then they won't be removed.
        if (mit->second->parent() == 0) {
            // not attached to our windowmenu
            // so we clean it up
            delete mit->second;
        } else {
            // let the parent clean it up
            mit->second->setInternalMenu(false);
        }
    }

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ~FluxboxWindow("<<this<<")"<<endl;
#endif // DEBUG
}


void FluxboxWindow::init() {
    m_attaching_tab = 0;
    // magic to detect if moved by hints
    m_old_pos_x = 0;

    assert(m_client);
    m_client->m_win = this;
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


    FbTk::TextButton *btn =  new FbTk::TextButton(frame().label(),
                                                  frame().theme().font(),
                                                  m_client->title());
    btn->setJustify(frame().theme().justify());
    m_labelbuttons[m_client] = btn;
    frame().addLabelButton(*btn);
    frame().setLabelButtonFocus(*btn);
    btn->show();
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // we need motion notify so we mask it
    btn->setEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | EnterWindowMask);

    FbTk::RefCount<FbTk::Command> set_client_cmd(new SetClientCmd(*m_client));
    btn->setOnClick(set_client_cmd);
    evm.add(*this, btn->window()); // we take care of button events for this
    evm.add(*this, m_client->window());

    // redirect events from frame to us

    frame().setEventHandler(*this);

    frame().resize(m_client->width(), m_client->height());

    m_last_focus_time.tv_sec = m_last_focus_time.tv_usec = 0;

    m_blackbox_attrib.workspace = m_workspace_number = ~0;

    m_blackbox_attrib.flags = m_blackbox_attrib.attrib = m_blackbox_attrib.stack = 0;
    m_blackbox_attrib.premax_x = m_blackbox_attrib.premax_y = 0;
    m_blackbox_attrib.premax_w = m_blackbox_attrib.premax_h = 0;

    //use tab as default
    decorations.tab = true;
    // enable decorations
    decorations.enabled = true;

    // set default values for decoration
    decorations.menu = true;	//override menu option
    // all decorations on by default
    decorations.titlebar = decorations.border = decorations.handle = true;
    decorations.maximize = decorations.close =
        decorations.sticky = decorations.shade = decorations.tab = true;


    functions.resize = functions.move = functions.iconify = functions.maximize = functions.tabable = true;
    decorations.close = false;

    if (m_client->getBlackboxHint() != 0)
        updateBlackboxHintsFromClient(*m_client);
    else
        updateMWMHintsFromClient(*m_client);

    //!!
    // fetch client size and placement
    XWindowAttributes wattrib;
    if (! m_client->getAttrib(wattrib) ||
        !wattrib.screen // no screen? ??
        || wattrib.override_redirect) { // override redirect
        return;
    }

    // save old border width so we can restore it later
    m_client->old_bw = wattrib.border_width;
    m_client->x = wattrib.x; m_client->y = wattrib.y;

    m_timer.setTimeout(fluxbox.getAutoRaiseDelay());
    FbTk::RefCount<FbTk::Command> raise_cmd(new FbTk::SimpleCommand<FluxboxWindow>(*this,
                                                                                   &FluxboxWindow::raise));
    m_timer.setCommand(raise_cmd);
    m_timer.fireOnce(true);

    // Slit client?
    if (m_client->initial_state == WithdrawnState) {
        return;
    }

    Fluxbox::instance()->saveWindowSearchGroup(frame().window().window(), this);

    /**************************************************/
    /* Read state above here, apply state below here. */
    /**************************************************/

    // update transient infomation
    m_client->updateTransientInfo();

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


    associateClientWindow(true, wattrib.x, wattrib.y, wattrib.width, wattrib.height);

    
    Fluxbox::instance()->attachSignals(*this);

    // this window is managed, we are now allowed to modify actual state
    m_initialized = true;

    applyDecorations(true);

    grabButtons();

    restoreAttributes();

    if (m_workspace_number < 0 || m_workspace_number >= screen().getCount())
        m_workspace_number = screen().currentWorkspaceID();

    bool place_window = (m_old_pos_x == 0);
    if (fluxbox.isStartup() || m_client->isTransient() ||
        m_client->normal_hint_flags & (PPosition|USPosition)) {

        frame().gravityTranslate(wattrib.x, wattrib.y, m_client->gravity(), false);

        if (! fluxbox.isStartup()) {

            int real_x = frame().x();
            int real_y = frame().y();

            if (real_x >= 0 &&
                real_y + frame().y() >= 0 &&
                real_x <= (signed) screen().width() &&
                real_y <= (signed) screen().height())
                place_window = false;

        } else
            place_window = false;

    }
    if (wattrib.width <= 0)
        wattrib.width = 1;
    if (wattrib.height <= 0)
        wattrib.height = 1;

    // if we're a transient then we should be on the same layer as our parent
    if (m_client->isTransient() &&
        m_client->transientFor()->fbwindow() &&
        m_client->transientFor()->fbwindow() != this)
        layerItem().setLayer(m_client->transientFor()->fbwindow()->layerItem().getLayer());
    else // if no parent then set default layer
        moveToLayer(m_layernum);
#ifdef DEBUG
    cerr<<"FluxboxWindow::init("<<title()<<") transientFor: "<<
        m_client->transientFor()<<endl;
    if (m_client->transientFor() && m_client->transientFor()->fbwindow()) {
        cerr<<"FluxboxWindow::init("<<title()<<") transientFor->title(): "<<
            m_client->transientFor()->fbwindow()->title()<<endl;
    }
#endif // DEBUG

    if (!place_window)
        moveResize(frame().x(), frame().y(), frame().width(), frame().height());

    screen().getWorkspace(m_workspace_number)->addWindow(*this, place_window);
    setWorkspace(m_workspace_number);

    if (shaded) { // start shaded
        shaded = false;
        shade();
    }

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
    if (client.m_win == this)
        return;

    menu().hide();

    // reparent client win to this frame
    frame().setClientWindow(client);
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    WinClient *was_focused = 0;
    WinClient *focused_win = Fluxbox::instance()->getFocusedWindow();

    // get the current window on the end of our client list
    Window leftwin = None;
    if (!clientList().empty())
        leftwin = clientList().back()->window();

    client.setGroupLeftWindow(leftwin);

    if (client.fbwindow() != 0) {
        FluxboxWindow *old_win = client.fbwindow(); // store old window

	ClientList::iterator client_insert_pos=getClientInsertPosition(x,y);
	FbTk::TextButton *button_insert_pos=NULL;
	if(client_insert_pos!=m_clientlist.end())
		button_insert_pos=m_labelbuttons[*client_insert_pos];
		
	
        // make sure we set new window search for each client
        ClientList::iterator client_it = old_win->clientList().begin();
        ClientList::iterator client_it_end = old_win->clientList().end();
	for (; client_it != client_it_end; ++client_it) {
            // setup eventhandlers for client
            evm.add(*this, (*client_it)->window());

            // reparent window to this
            frame().setClientWindow(**client_it);
            if ((*client_it) == focused_win)
                was_focused = focused_win;

            moveResizeClient(**client_it,
                             frame().clientArea().x(),
                             frame().clientArea().y(),
                             frame().clientArea().width(),
                             frame().clientArea().height());

            (*client_it)->m_win = this;
            // create a labelbutton for this client and
            // associate it with the pointer
            FbTk::TextButton *btn = new FbTk::TextButton(frame().label(),
                                             frame().theme().font(),
                                             (*client_it)->title());
            btn->setJustify(frame().theme().justify());
            m_labelbuttons[(*client_it)] = btn;
            frame().addLabelButton(*btn);
	    if(x >= 0) {
		    if(button_insert_pos){ //null if we want the new button at the end of the list
			    frame().moveLabelButtonLeftOf(*btn, *button_insert_pos);
		    }
	    }
            btn->show();
            // we need motion notify so we mask it
            btn->setEventMask(ExposureMask | ButtonPressMask |
                              ButtonReleaseMask | ButtonMotionMask |
                              EnterWindowMask);


            FbTk::RefCount<FbTk::Command>
                set_client_cmd(new SetClientCmd(*(*client_it)));
            btn->setOnClick(set_client_cmd);
            evm.add(*this, btn->window()); // we take care of button events for this

            (*client_it)->saveBlackboxAttribs(m_blackbox_attrib);
        }
	
        // add client and move over all attached clients
        // from the old window to this list
	m_clientlist.splice(client_insert_pos, old_win->m_clientlist);
	updateClientLeftWindow();
        old_win->m_client = 0;

        delete old_win;

    } else { // client.fbwindow() == 0
        // create a labelbutton for this client and associate it with the pointer
        FbTk::TextButton *btn = new FbTk::TextButton(frame().label(),
                                         frame().theme().font(),
                                         client.title());
        m_labelbuttons[&client] = btn;
        frame().addLabelButton(*btn);
        btn->show();
        FbTk::EventManager &evm = *FbTk::EventManager::instance();
        // we need motion notify so we mask it
        btn->setEventMask(ExposureMask | ButtonPressMask |
                          ButtonReleaseMask | ButtonMotionMask |
                          EnterWindowMask);


        FbTk::RefCount<FbTk::Command> set_client_cmd(new SetClientCmd(client));
        btn->setOnClick(set_client_cmd);
        evm.add(*this, btn->window()); // we take care of button events for this

        if (&client == focused_win)
            was_focused = focused_win;
        client.m_win = this;

        client.saveBlackboxAttribs(m_blackbox_attrib);
        m_clientlist.push_back(&client);
    }

    // make sure that the state etc etc is updated for the new client
    // TODO: one day these should probably be neatened to only act on the
    // affected clients if possible
    m_statesig.notify();
    m_workspacesig.notify();
    m_layersig.notify();

    if (was_focused != 0) {
        // already has focus, we're just assuming the state of the old window
        setCurrentClient(*was_focused, false);
        frame().setFocus(true);
    }

    frame().reconfigure();

    // keep the current window on top
    m_client->raise();
}


/// detach client from window and create a new window for it
bool FluxboxWindow::detachClient(WinClient &client) {
    if (client.m_win != this || numClients() <= 1)
        return false;

    // I'm not sure how to do this bit better
    // we need to find the window we've got, and update the
    // window to its right to have a left window set to the
    // window which is to the left of the current.
    // Think in terms of:
    // window1 <- my_window <- window2
    // we need to take out my_window, so update window2 leftwin to be window1

    Window leftwin = None;
    ClientList::iterator client_it_end = clientList().end();
    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_before = client_it_end;
    ClientList::iterator client_it_after = clientList().begin();
    if (!clientList().empty()) {
        ++client_it_after;
        if (clientList().front() == &client) {
            leftwin = None;
        } else {
            ++client_it;
            client_it_before = clientList().begin();
            ++client_it_after;

            while (client_it != client_it_end) {
                if (*client_it == &client) {
                    break;
                }
                ++client_it_before;
                ++client_it;
                ++client_it_after;
            }
        }
    }

    // update the leftwin of the window to the right
    if (client_it_before != client_it_end)
        leftwin = (*client_it_before)->window();

    if (client_it_after != client_it_end)
        (*client_it_after)->setGroupLeftWindow(leftwin);

    removeClient(client);

    // m_client must be valid as there should be at least one other window
    // otherwise this wouldn't be here (refer numClients() <= 1 return)
    client.m_win = screen().createWindow(client);
    m_client->raise();
    setInputFocus();
    return true;
}

void FluxboxWindow::detachCurrentClient() {
    // should only operate if we had more than one client
    if (numClients() <= 1)
        return;
    detachClient(*m_client);
}

/// removes client from client list, does not create new fluxboxwindow for it
bool FluxboxWindow::removeClient(WinClient &client) {
    if (client.m_win != this || numClients() == 0)
        return false;

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<")["<<this<<"]"<<endl;
#endif // DEBUG

    // if it is our active client, deal with it...
    if (m_client == &client) {
        WinClient *next_client = screen().getLastFocusedWindow(*this, m_client);
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
        frame().removeLabelButton(*label_btn);
        evm.remove(label_btn->window());
        delete label_btn;
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
                                      FbTk::Compose(bind2nd(equal_to<Window>(), win),
                                                    mem_fun(&WinClient::window)));
    return (it == clientList().end() ? 0 : *it);
}

/// raise and focus next client
void FluxboxWindow::nextClient() {
    if (numClients() <= 1)
        return;

    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(), m_client);
    WinClient *client = 0;
    if (it == m_clientlist.end()) {
        client = m_clientlist.front();
    } else {
        it++;
        if (it == m_clientlist.end())
            client = m_clientlist.front();
        else
            client = *it;
    }
    setCurrentClient(*client, true);
}

void FluxboxWindow::prevClient() {
    if (numClients() <= 1)
        return;

    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(), m_client);
    WinClient *client = 0;
    if (it == m_clientlist.end()) {
        client = m_clientlist.front();
    } else {
        if (it == m_clientlist.begin())
            client = m_clientlist.back();
        else
            client = *(--it);
    }

    setCurrentClient(*client, true);
}


void FluxboxWindow::moveClientLeft() {
    if (m_clientlist.size() == 1 ||
        *m_clientlist.begin() == &winClient())
        return;
    // move label button to the left
    frame().moveLabelButtonLeft(*m_labelbuttons[&winClient()]);
    // move client in clientlist to the left
    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(), &winClient());
    ClientList::iterator new_pos = it;
    new_pos--;
    m_clientlist.erase(it);
    m_clientlist.insert(new_pos, &winClient());

    updateClientLeftWindow();

}

void FluxboxWindow::moveClientRight() {
    if (m_clientlist.size() == 1 ||
            *m_clientlist.rbegin() == &winClient())
        return;
    // move label button to the right
    frame().moveLabelButtonRight(*m_labelbuttons[&winClient()]);
    // move client in clientlist to the right
    ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(), &winClient());
    ClientList::iterator new_pos = m_clientlist.erase(it);
    new_pos++;
    m_clientlist.insert(new_pos, &winClient());

    updateClientLeftWindow();
}

//std::list<*WinClient>::iterator FluxboxWindow::getClientInsertPosition(int x, int y) {
FluxboxWindow::ClientList::iterator FluxboxWindow::getClientInsertPosition(int x, int y) {

	int dest_x=0, dest_y=0;
	Window labelbutton=0;
	if(!XTranslateCoordinates(FbTk::App::instance()->display(),
				parent().window(), frame().label().window(),
				x,y, &dest_x, &dest_y,
				&labelbutton))
		return m_clientlist.end();
	Client2ButtonMap::iterator it = m_labelbuttons.begin();
	Client2ButtonMap::iterator it_end = m_labelbuttons.end();
	//find the label button to move next to
	for(; it!=it_end; it++) {
		if( (*it).second->window()==labelbutton)
			break;
	}
	//label button not found
	if(it==it_end)	{
		return m_clientlist.end();
	}
	Window child_return=0;
	//make x and y relative to our labelbutton
	if(!XTranslateCoordinates(FbTk::App::instance()->display(),
			        frame().label().window(),labelbutton,
				dest_x,dest_y, &x, &y,
				&child_return))
		return m_clientlist.end();
	ClientList::iterator client = find(m_clientlist.begin(),
				       m_clientlist.end(),
				       it->first);
	if(x>(*it).second->width()/2)
		client++;
	return client;
	

}
	


void FluxboxWindow::moveClientTo(WinClient &win, int x, int y) {
	int dest_x=0, dest_y=0;
	Window labelbutton=0;
	if(!XTranslateCoordinates(FbTk::App::instance()->display(),
				parent().window(), frame().label().window(),
				x,y, &dest_x, &dest_y,
				&labelbutton))
		return;
	Client2ButtonMap::iterator it = m_labelbuttons.begin();
	Client2ButtonMap::iterator it_end = m_labelbuttons.end();
	//find the label button to move next to
	for(; it!=it_end; it++) {
		if( (*it).second->window()==labelbutton)
			break;
	}
	//label button not found
	if(it==it_end)	{
		return;
	}
	Window child_return=0;
	//make x and y relative to our labelbutton
	if(!XTranslateCoordinates(FbTk::App::instance()->display(),
			        frame().label().window(),labelbutton,
				dest_x,dest_y, &x, &y,
				&child_return))
		return;
	if(x>(*it).second->width()/2) {
		moveClientRightOf(win, *it->first);
	} else {
		moveClientLeftOf(win, *it->first);
	}

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
	if (it == m_clientlist.end() || new_pos==m_clientlist.end()) {
		return;
	}
	//moving a button to the right of itself results in no change
	if( new_pos == it) {
		return;
	}
	//remove from list
	m_clientlist.erase(it);
	//need to insert into the next position
	new_pos++;
	//insert on the new place
	if(new_pos == m_clientlist.end())
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
    if (client.m_win != this)
        return false;

    m_client = &client;
    m_client->raise();
    titleSig().notify();

#ifdef DEBUG
    cerr<<"FluxboxWindow::"<<__FUNCTION__<<": labelbutton[client] = "<<
        m_labelbuttons[m_client]<<endl;
#endif // DEBUG
    // frame focused doesn't necessarily mean input focused
    frame().setLabelButtonFocus(*m_labelbuttons[m_client]);

    if (setinput && setInputFocus()) {
        return true;
    }

    return false;
}

bool FluxboxWindow::isGroupable() const {
    if (isResizable() && isMaximizable() && !winClient().isTransient())
        return true;
    return false;
}

void FluxboxWindow::associateClientWindow(bool use_attrs,
                                          int x, int y,
                                          unsigned int width, unsigned int height) {
    m_client->setBorderWidth(0);
    updateTitleFromClient(*m_client);
    updateIconNameFromClient(*m_client);

    if (use_attrs)
        frame().moveResizeForClient(x, y,
                                    width, height);
    else
        frame().resizeForClient(m_client->width(), m_client->height());

    frame().setClientWindow(*m_client);
}


void FluxboxWindow::grabButtons() {

    // needed for click to focus
    XGrabButton(display, Button1, AnyModifier,
                frame().window().window(), True, ButtonPressMask,
                GrabModeSync, GrabModeSync, None, None);
    XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask, frame().window().window());

    if (Fluxbox::instance()->useMod1()) {
        XGrabButton(display, Button1, Mod1Mask, frame().window().window(), True,
                    ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                    GrabModeAsync, None, frame().theme().moveCursor());

        //----grab with "all" modifiers
        grabButton(display, Button1, frame().window().window(), frame().theme().moveCursor());

        XGrabButton(display, Button2, Mod1Mask, frame().window().window(), True,
                    ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

        XGrabButton(display, Button3, Mod1Mask, frame().window().window(), True,
                    ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                    GrabModeAsync, None, None);

        //---grab with "all" modifiers
        grabButton(display, Button3, frame().window().window(), None);
    }
}


void FluxboxWindow::reconfigure() {

    applyDecorations();

    setFocusFlag(focused);

    moveResize(frame().x(), frame().y(), frame().width(), frame().height());

    grabButtons();

    frame().setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());

    frame().reconfigure();

    menu().reconfigure();

}

/// update current client title and title in our frame
void FluxboxWindow::updateTitleFromClient(WinClient &client) {
    client.updateTitle();
    // compare old title with new and see if we need to update
    // graphics
    if (m_labelbuttons[&client]->text() != client.title())
        m_labelbuttons[&client]->setText(client.title());
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
        m_old_decoration = static_cast<Decoration>(hint->decoration);
        setDecoration(m_old_decoration, false);
    }
}

void FluxboxWindow::move(int x, int y, int gravity) {
    moveResize(x, y, frame().width(), frame().height(), gravity);
}

void FluxboxWindow::resize(unsigned int width, unsigned int height) {
    moveResize(frame().x(), frame().y(), width, height);
}

// send_event is just an override
void FluxboxWindow::moveResize(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height, int gravity, bool send_event) {

    // magic to detect if moved during initialisation
    if (!isInitialized())
        m_old_pos_x = 1;

    if (gravity != ForgetGravity) {
        frame().gravityTranslate(new_x, new_y, gravity, false);
    }

    send_event = send_event || (frame().x() != new_x || frame().y() != new_y);

    if (new_width != frame().width() || new_height != frame().height()) {
        if ((((signed) frame().width()) + new_x) < 0)
            new_x = 0;
        if ((((signed) frame().height()) + new_y) < 0)
            new_y = 0;

        if (!isResizable()) {
            new_width = width();
            new_height = height();
        }

        frame().moveResize(new_x, new_y, new_width, new_height);
        setFocusFlag(focused);

        shaded = false;
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
                               unsigned int new_width, unsigned int new_height, int gravity) {

    // magic to detect if moved during initialisation
    if (!isInitialized())
        m_old_pos_x = 1;
    frame().moveResizeForClient(new_x, new_y, new_width, new_height, true, true, gravity);
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
            if ((*it)->isModal())
                return (*it)->fbwindow()->setCurrentClient(**it, true);
        }
    }

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

/**
   Unmaps the window and removes it from workspace list
*/
void FluxboxWindow::iconify() {
    if (isIconic()) // no need to iconify if we're already
        return;

    iconic = true;

    setState(IconicState, false);

    hide(true);

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

    iconic = false;
    setState(NormalState, false);

    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_end = clientList().end();
    for (; client_it != client_it_end; ++client_it) {
        (*client_it)->setEventMask(NoEventMask);
        (*client_it)->show();
        (*client_it)->setEventMask(PropertyChangeMask | StructureNotifyMask | FocusChangeMask);
    }

    if (reassoc && !m_client->transients.empty()) {
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

    if (was_iconic && screen().doFocusNew())
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
    Fluxbox* fb = Fluxbox::instance();
    
    if (flag && !isFullscreen()) {

        if (isIconic())
            deiconify();

        if (isShaded())
            shade();

        frame().setUseShape(false);

        m_old_decoration_mask = decorationMask();
        m_old_layernum =layerNum();
        m_old_pos_x = frame().x();
        m_old_pos_y = frame().y();
        m_old_width = frame().width();
        m_old_height = frame().height();
        
        // clear decorations
        setDecorationMask(0);

        // be xinerama aware
        moveResize(screen().getHeadX(head), screen().getHeadY(head),
                   screen().getHeadWidth(head), screen().getHeadHeight(head));
        moveToLayer(Fluxbox::instance()->getAboveDockLayer());

        fullscreen = true;

        stateSig().notify();

    } else if (!flag && isFullscreen()) {

        fullscreen = false;
    
        setDecorationMask(m_old_decoration_mask);
        frame().setUseShape(!m_shaped);
        
        moveResize(m_old_pos_x, m_old_pos_y, m_old_width, m_old_height);
        moveToLayer(m_old_layernum);

        m_old_decoration_mask = 0;
        m_old_layernum = Fluxbox::instance()->getNormalLayer();
       
        stateSig().notify();
    }
}

/**
   Maximize window both horizontal and vertical
*/
void FluxboxWindow::maximize(int type) {

    if (isFullscreen())
        return;
    
    if (isIconic())
        deiconify();

    if (isShaded())
        shade();

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

    // NOTE: There is one option to the way this works - what it does when
    // fully maximised and maximise(vert, horz) is selected.
    // There are 2 options here - either:
    // 1) maximiseVertical results in a vertically (not horz) maximised window, or
    // 2) " toggles vertical maximisation, thus resulting in a horizontally
    //      maximised window.
    //
    // The current implementation uses style 1, to change this, removed the
    // item corresponding to the [[ ]] comment

    // toggle maximize vertically?
    // when _don't_ we want to toggle?
    // - type is horizontal maximise, [[and we aren't fully maximised]] or
    // - [[ type is vertical maximise and we are fully maximised ]]
    // - type is none and we are not vertically maximised, or
    // - type is full and we are not horizontally maximised, but already vertically
    if (!(type == MAX_HORZ && orig_max != MAX_FULL ||
          type == MAX_VERT && orig_max == MAX_FULL ||
          type == MAX_NONE && !(orig_max & MAX_VERT) ||
          type == MAX_FULL && orig_max == MAX_VERT)) {
        // already maximized in that direction?
        if (orig_max & MAX_VERT) {
            new_y = m_old_pos_y;
            new_h = m_old_height;
        } else {
            m_old_pos_y  = new_y;
            m_old_height = new_h;
            new_y = screen().maxTop(head);
            new_h = screen().maxBottom(head) - new_y - 2*frame().window().borderWidth();
        }
        maximized ^= MAX_VERT;
    }

    // maximize horizontally?
    if (!(type == MAX_VERT && orig_max != MAX_FULL ||
          type == MAX_HORZ && orig_max == MAX_FULL ||
          type == MAX_NONE && !(orig_max & MAX_HORZ) ||
          type == MAX_FULL && orig_max == MAX_HORZ)) {
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
        }
        maximized ^= MAX_HORZ;
    }

    moveResize(new_x, new_y, new_w, new_h);

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
    if (isInitialized() && !stuck && old_wkspc != m_workspace_number) {
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

    if (isInitialized()) {
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

    frame().shade();

    if (shaded) {
        shaded = false;
        m_blackbox_attrib.flags ^= ATTRIB_SHADED;
        m_blackbox_attrib.attrib ^= ATTRIB_SHADED;

        if (isInitialized())
            setState(NormalState, false);
    } else {
        shaded = true;
        m_blackbox_attrib.flags |= ATTRIB_SHADED;
        m_blackbox_attrib.attrib |= ATTRIB_SHADED;
        // shading is the same as iconic
        if (isInitialized())
            setState(IconicState, false);
    }

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

    if (isInitialized()) {
        setState(m_current_state, false);
        // notify since some things consider "stuck" to be a pseudo-workspace
        m_workspacesig.notify();
    }

}


void FluxboxWindow::raise() {
    if (isIconic())
        deiconify();
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::raise()[layer="<<layerNum()<<""<<endl;
#endif // DEBUG
    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    // raise this window and every transient in it
    if (client->fbwindow())
        raiseFluxboxWindow(*client->fbwindow());
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

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    if (client->fbwindow())
        tempRaiseFluxboxWindow(*client->fbwindow());
}


void FluxboxWindow::raiseLayer() {
    // don't let it up to menu layer
    if (layerNum() == (Fluxbox::instance()->getMenuLayer()+1))
        return;

    if (!isInitialized()) {
        m_layernum++;
        return;
    }

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    FluxboxWindow *win = client->fbwindow();
    if (!win) return;

    if (!win->isIconic())
        screen().updateNetizenWindowRaise(client->window());

    win->layerItem().raiseLayer();

    // remember number just in case a transient happens to revisit this window
    int layer_num = win->layerItem().getLayerNum();
    win->setLayerNum(layer_num);

    WinClient::TransientList::const_iterator it = client->transientList().begin();
    WinClient::TransientList::const_iterator it_end = client->transientList().end();
    for (; it != it_end; ++it) {
        win = (*it)->fbwindow();
        if (win && !win->isIconic()) {
            screen().updateNetizenWindowRaise((*it)->window());
            win->layerItem().moveToLayer(layer_num);
            win->setLayerNum(layer_num);
        }
    }
}

void FluxboxWindow::lowerLayer() {
    if (!isInitialized()) {
        if (m_layernum > 0)
            m_layernum--;
        return;
    }

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    FluxboxWindow *win = client->fbwindow();
    if (!win) return;

    if (!win->isIconic()) {
        screen().updateNetizenWindowLower(client->window());
    }
    win->layerItem().lowerLayer();
    // remember number just in case a transient happens to revisit this window
    int layer_num = win->layerItem().getLayerNum();
    win->setLayerNum(layer_num);

    WinClient::TransientList::const_iterator it = client->transientList().begin();
    WinClient::TransientList::const_iterator it_end = client->transientList().end();
    for (; it != it_end; ++it) {
        win = (*it)->fbwindow();
        if (win && !win->isIconic()) {
            screen().updateNetizenWindowLower((*it)->window());
            win->layerItem().moveToLayer(layer_num);
            win->setLayerNum(layer_num);
        }
    }
}


void FluxboxWindow::moveToLayer(int layernum) {
#ifdef DEBUG
    cerr<<"FluxboxWindow("<<title()<<")::moveToLayer("<<layernum<<")"<<endl;
#endif // DEBUG

    Fluxbox * fluxbox = Fluxbox::instance();

    // don't let it set its layer into menu area
    if (layernum <= fluxbox->getMenuLayer()) {
        layernum = fluxbox->getMenuLayer() + 1;
    }

    if (!isInitialized()) {
        m_layernum = layernum;
        return;
    }

    // get root window
    WinClient *client = getRootTransientFor(m_client);

    // if we don't have any root window use this as root
    if (client == 0)
        client = m_client;

    FluxboxWindow *win = client->fbwindow();
    if (!win) return;

    if (!win->isIconic()) {
        screen().updateNetizenWindowRaise(client->window());
    }
    win->layerItem().moveToLayer(layernum);
    // remember number just in case a transient happens to revisit this window
    layernum = win->layerItem().getLayerNum();
    win->setLayerNum(layernum);

    WinClient::TransientList::const_iterator it = client->transientList().begin();
    WinClient::TransientList::const_iterator it_end = client->transientList().end();
    for (; it != it_end; ++it) {
        win = (*it)->fbwindow();
        if (win && !win->isIconic()) {
            screen().updateNetizenWindowRaise((*it)->window());
            win->layerItem().moveToLayer(layernum);
            win->setLayerNum(layernum);
        }
    }
}

void FluxboxWindow::setFocusHidden(bool value) {
    if(value)
        m_blackbox_attrib.flags |= ATTRIB_HIDDEN;
    else
        m_blackbox_attrib.flags ^= ATTRIB_HIDDEN;

    if (isInitialized())
        m_statesig.notify();
}

void FluxboxWindow::setIconHidden(bool value) {
    m_icon_hidden= value;
    if (isInitialized())
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
    // Record focus timestamp for window cycling enhancements
    if (focused) {
        gettimeofday(&m_last_focus_time, 0);
        screen().setFocusedWindow(*m_client);
    }

    installColormap(focus);

    if (focus != frame().focused())
        frame().setFocus(focus);

    if ((screen().isSloppyFocus() || screen().isSemiSloppyFocus())
        && screen().doAutoRaise()) {
        if (focused)
            m_timer.start();
        else
            m_timer.stop();
    }

    // did focus change? notify listeners
    if (was_focused != focus)
        m_focussig.notify();
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
    if ((XGetWindowProperty(display, m_client->window(), FbAtoms::instance()->getWMStateAtom(),
                            0l, 2l, false, FbAtoms::instance()->getWMStateAtom(),
                            &atom_return, &foo, &nitems, &ulfoo,
                            (unsigned char **) &state) != Success) ||
        (! state)) {
        return false;
    }

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
    if (!getState())
        m_current_state = m_client->initial_state;

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

    if (( m_blackbox_attrib.workspace != screen().currentWorkspaceID()) &&
        ( m_blackbox_attrib.workspace < screen().getCount()))
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
void FluxboxWindow::showMenu(int menu_x, int menu_y) {
    // move menu directly under titlebar

    int head = screen().getHead(menu_x, menu_y);

    // but not under screen
    if (menu_y + menu().height() >= screen().maxBottom(head))
        menu_y = screen().maxBottom(head) - menu().height() - 1 - menu().fbwindow().borderWidth();

    if (menu_x < static_cast<signed>(screen().maxLeft(head)))
        menu_x = screen().maxLeft(head);
    else if (menu_x + static_cast<signed>(menu().width()) >= static_cast<signed>(screen().maxRight(head)))
        menu_x = screen().maxRight(head) - menu().width() - 1;


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
    if (menu().isVisible()) {
        menu().hide();
        return;
    }

    menu().disableTitle();
    int menu_y = frame().titlebar().height() + frame().titlebar().borderWidth();
    if (!decorations.titlebar) // if we don't have any titlebar
        menu_y = 0;
    if (m_last_button_x < x() || m_last_button_x > x() + width())
        m_last_button_x = x();
    showMenu(m_last_button_x, menu_y + frame().y());
}

/**
   Determine if this is the lowest tab of them all
*/
bool FluxboxWindow::isLowerTab() const {
    cerr<<__FILE__<<"(FluxboxWindow::isLowerTab()) TODO!"<<endl;
    return true;
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
        XFree(atomname);
#endif // DEBUG
        WinClient *client = findClient(event.xproperty.window);
        if (client) {
            propertyNotifyEvent(*client, event.xproperty.atom);
        }
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

    Fluxbox *fluxbox = Fluxbox::instance();

    bool get_state_ret = getState();
    if (!(get_state_ret && fluxbox->isStartup())) {
        if (m_client->wm_hint_flags & StateHint)
            m_current_state = m_client->initial_state;
    } else if (iconic)
        m_current_state = NormalState;

    setState(m_current_state, false);

    switch (m_current_state) {
    case IconicState:
        iconify();
	break;

    case WithdrawnState:
        withdraw(true);
	break;

    case NormalState: {
        // if this window was destroyed while autogrouping
        bool destroyed = false;

        // check WM_CLASS only when we changed state to NormalState from
        // WithdrawnState (ICCC 4.1.2.5)
        client->updateWMClassHint();

        Workspace *wsp = screen().getWorkspace(m_workspace_number);
        if (wsp != 0 && isGroupable())
            destroyed = wsp->checkGrouping(*this);

	// if we weren't grouped with another window we deiconify ourself
        if (!destroyed)
            deiconify(false);


    } break;
    case InactiveState:
    case ZoomState:
    default:
        deiconify(false);
        break;
    }

}


void FluxboxWindow::mapNotifyEvent(XMapEvent &ne) {
    WinClient *client = findClient(ne.window);
    if (client == 0)
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

        if (client->isTransient() || screen().doFocusNew())
            setCurrentClient(*client, true);
        else
            setFocusFlag(false);

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
   Returns true if *this should die
   else false
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
        cerr<<__FILE__<<"("<<__LINE__<<"): DestroyNotifyEvent this="<<this<<endl;
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
        int old_max_width = client.max_width;
        int old_min_width = client.min_width;
        int old_min_height = client.min_height;
        int old_max_height = client.max_height;
        bool changed = false;
        client.updateWMNormalHints();

        if ((client.normal_hint_flags & PMinSize) &&
            (client.normal_hint_flags & PMaxSize) &&
            (client.min_width != old_min_width ||
             client.max_width != old_max_width ||
             client.min_height != old_min_height ||
             client.max_height != old_max_height)) {
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
                if (! client.isTransient()) {
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

    int cx = frame().x(), cy = frame().y(), ignore = 0;
    unsigned int cw = frame().width(), ch = frame().height();

    if (cr.value_mask & CWBorderWidth)
        client->old_bw = cr.border_width;

    if ((cr.value_mask & CWX) &&
        (cr.value_mask & CWY)) {
        cx = cr.x;
        cy = cr.y;
        frame().gravityTranslate(cx, cy, client->gravity(), false);
    } else if (cr.value_mask & CWX) {
        cx = cr.x;
        frame().gravityTranslate(cx, ignore, client->gravity(), false);
    } else if (cr.value_mask & CWY) {
        cy = cr.y;
        frame().gravityTranslate(ignore, cy, client->gravity(), false);
    }

    if (cr.value_mask & CWWidth)
        cw = cr.width;

    if (cr.value_mask & CWHeight)
        ch = cr.height;

    // whether we should send ConfigureNotify to netizens
    // the request is for client window so we resize the frame to it first
    if (frame().width() != cw || frame().height() != ch) {
        if (frame().x() != cx || frame().y() != cy)
            frame().moveResizeForClient(cx, cy, cw, ch);
        else
            frame().resizeForClient(cw, ch);
    } else if (frame().x() != cx || frame().y() != cy) {
        frame().move(cx, cy);
    }

    if (cr.value_mask & CWStackMode) {
        switch (cr.detail) {
        case Above:
        case TopIf:
        default:
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

    if (be.button == 1 || (be.button == 3 && be.state == Mod1Mask)) {
        if ((! focused) && (! screen().isSloppyFocus())) { //check focus
            setInputFocus();
        }

        if (frame().window().window() == be.window) {
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
    else if (re.window == frame().window()) {
        if (re.button == 2 && re.state == Mod1Mask)
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
    bool inside_titlebar = (frame().titlebar() == me.window || frame().label() == me.window ||
                            frame().handle() == me.window || frame().window() == me.window);

    if (Fluxbox::instance()->getIgnoreBorder()
        && !(me.state & Mod1Mask) // really should check for exact matches
        && !(isMoving() || isResizing() || m_attaching_tab != 0)) {
        int borderw = frame().window().borderWidth();
        if (me.x_root < (frame().x() + borderw) ||
            me.y_root < (frame().y() + borderw) ||
            me.x_root > (frame().x() + (int)frame().width() + borderw) ||
            me.y_root > (frame().y() + (int)frame().height() + borderw))
            return;
    }

    WinClient *client = 0;
    if (!inside_titlebar) {
        // determine if we're in titlebar
        Client2ButtonMap::iterator it = m_labelbuttons.begin();
        Client2ButtonMap::iterator it_end = m_labelbuttons.end();
        for (; it != it_end; ++it) {
            if ((*it).second->window() == me.window) {
                inside_titlebar = true;
                client = (*it).first;
                break;
            }
        }
    }

    if ((me.state & Button1Mask) && functions.move &&
        inside_titlebar &&
        !isResizing()) {

        if (! isMoving()) {
            startMoving(me.window);
            // save first event point
            m_last_resize_x = me.x_root;
            m_last_resize_y = me.y_root;
            m_button_grab_x = me.x_root - frame().x() - frame().window().borderWidth();
            m_button_grab_y = me.y_root - frame().y() - frame().window().borderWidth();
        } else {
            int dx = me.x_root - m_button_grab_x,
                dy = me.y_root - m_button_grab_y;

            dx -= frame().window().borderWidth();
            dy -= frame().window().borderWidth();

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
                    new_id = (cur_id + 1) % screen().getCount();
                    dx = - me.x_root; // move mouse back to x=0
                } else if (me.x_root <= warpPad &&
                           moved_x < 0) {
                    //warp left
                    new_id = (cur_id + screen().getCount() - 1) % screen().getCount();
                    dx = screen().width() - me.x_root-1; // move mouse to screen width - 1
                }
                if (new_id != cur_id) {

                    XWarpPointer(display, None, None, 0, 0, 0, 0, dx, 0);
                    screen().changeWorkspaceID(new_id);

                    m_last_resize_x = me.x_root + dx;

                    // dx is the difference, so our new x is what it would  have been
                    // without the warp, plus the difference.
                    dx += me.x_root - m_button_grab_x;
                }
            }
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
                frame().window().moveResize(dx, dy, frame().width(), frame().height());
            }

            screen().showPosition(dx, dy);
        } // end if moving
    } else if (functions.resize &&
               (((me.state & Button1Mask) && (me.window == frame().gripRight() ||
                                              me.window == frame().gripLeft())) ||
                me.window == frame().window())) {

        if (! resizing) {

          int cx = frame().width() / 2;
          int cy = frame().height() / 2;

          if (me.window == frame().gripRight())
              m_resize_corner = RIGHTBOTTOM;
          else if (me.window == frame().gripLeft())
              m_resize_corner = LEFTBOTTOM;
          else if (screen().getResizeModel() != BScreen::QUADRANTRESIZE)
              m_resize_corner = RIGHTBOTTOM;
          else if (me.x < cx)
              m_resize_corner = (me.y < cy) ? LEFTTOP : LEFTBOTTOM;
          else
              m_resize_corner = (me.y < cy) ? RIGHTTOP : RIGHTBOTTOM;

          startResizing(me.window, me.x, me.y);
        } else if (resizing) {
            // draw over old rect
            parent().drawRectangle(screen().rootTheme().opGC(),
                                   m_last_resize_x, m_last_resize_y,
                                   m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                                   m_last_resize_h - 1 + 2 * frame().window().borderWidth());


            // move rectangle
            int gx = 0, gy = 0;

            int dx = me.x - m_button_grab_x;
            int dy = me.y - m_button_grab_y;

            if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP) {
                m_last_resize_h = frame().height() - dy;
                m_last_resize_y = frame().y() + dy;
            } else {
                m_last_resize_h = frame().height() + dy;
            }

            if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM) {
                 m_last_resize_w = frame().width() - dx;
                 m_last_resize_x = frame().x() + dx;
            } else {
                 m_last_resize_w = frame().width() + dx;
            }

            fixsize(&gx, &gy);

            // draw resize rectangle
            parent().drawRectangle(screen().rootTheme().opGC(),
                                   m_last_resize_x, m_last_resize_y,
                                   m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                                   m_last_resize_h - 1 + 2 * frame().window().borderWidth());

            screen().showGeometry(gx, gy);
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
    if (ev.mode == NotifyGrab ||
        !isVisible()) {
        return;
    }

    WinClient *client = 0;
    // don't waste our time scanning if we aren't real sloppy focus
    if (screen().isSloppyFocus()) {
        // determine if we're in a label button (tab)
        Client2ButtonMap::iterator it = m_labelbuttons.begin();
        Client2ButtonMap::iterator it_end = m_labelbuttons.end();
        for (; it != it_end; ++it) {
            if ((*it).second->window() == ev.window) {
                client = (*it).first;
                break;
            }
        }
    }
    if (ev.window == frame().window() ||
        ev.window == m_client->window() ||
        client) {
        if ((screen().isSloppyFocus() || screen().isSemiSloppyFocus())
            && !isFocused() ||
            // or, we are focused, but it isn't the one we want
            client && screen().isSloppyFocus() && (m_client != client)) {

            // check that there aren't any subsequent leave notify events in the
            // X event queue
            XEvent dummy;
            scanargs sa;
            sa.w = ev.window;
            sa.enter = sa.leave = False;
            XCheckIfEvent(display, &dummy, queueScanner, (char *) &sa);

            // if client is set, use setCurrent client, otherwise just setInputFocus
            if ((!sa.leave || sa.inferior)) {
                if (client)
                    setCurrentClient(*client, true);
                else
                    setInputFocus();
            }

        }
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
	//	functions.iconify = functions.maximize = true;
	//	functions.move = true;   // We need to move even without decor
	//	functions.resize = true; // We need to resize even without decor
	break;

    default:
    case DECOR_NORMAL:
        decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.menu = true;
        functions.resize = functions.move = functions.iconify =
            functions.maximize = true;
	break;

    case DECOR_TINY:
        decorations.titlebar = decorations.iconify = decorations.menu =
            functions.move = functions.iconify = true;
        decorations.border = decorations.handle = decorations.maximize =
            functions.resize = functions.maximize = false;
	break;

    case DECOR_TOOL:
        decorations.titlebar = decorations.menu = functions.move = true;
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

    int grav_x=0, grav_y=0;
    // negate gravity
    frame().gravityTranslate(grav_x, grav_y, -m_client->gravity(), false);

    unsigned int border_width = 0;
    if (decorations.border)
        border_width = frame().theme().border().width();

    bool client_move = false;

    if (initial || frame().window().borderWidth() != border_width) {
        client_move = true;
        frame().setBorderWidth(border_width);
    }

    // we rely on frame not doing anything if it is already shown/hidden
    if (decorations.titlebar)
        client_move |= frame().showTitlebar();
    else
        client_move |= frame().hideTitlebar();

    if (decorations.handle) {
        client_move |= frame().showHandle();
    } else
        client_move |= frame().hideHandle();

    // apply gravity once more
    frame().gravityTranslate(grav_x, grav_y, m_client->gravity(), false);

    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0) {
        move(grav_x + frame().x(), grav_y + frame().y());
        client_move = true;
    }

    frame().reconfigure();
    if (!initial && client_move)
        sendConfigureNotify();

}

void FluxboxWindow::toggleDecoration() {
    //don't toggle decor if the window is shaded
    if (isShaded())
        return;

    m_toggled_decos= true;

    if (decorations.enabled) { //remove decorations
        decorations.enabled = false;
        setDecoration(DECOR_NONE);
    } else { //revert back to old decoration
        decorations.enabled = true;
        if (m_old_decoration == DECOR_NONE) { // make sure something happens
            setDecoration(DECOR_NORMAL);
        } else {
            setDecoration(m_old_decoration);
        }
    }
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

void FluxboxWindow::setDecorationMask(unsigned int mask) {
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
    applyDecorations();
}

void FluxboxWindow::startMoving(Window win) {
    if (s_num_grabs > 0)
        return;

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
            }
        }
        fluxbox->ungrab();
    } else if (!interrupted) {
        moveResize(frame().x(), frame().y(), frame().width(), frame().height(), ForgetGravity, true);
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

    /////////////////////////////////////
    // begin by checking the screen (or Xinerama head) edges

    int h;
    if (screen().numHeads() > 0) {
        // head "0" == whole screen width + height, which we skip since the
        // sum of all the heads covers those edges
        for (h = 1; h <= screen().numHeads(); h++) {
            snapToWindow(dx, dy, left, right, top, bottom,
                         screen().maxLeft(h),
                         screen().maxRight(h),
                         screen().maxTop(h),
                         screen().maxBottom(h));
        }
        for (h = 1; h <= screen().numHeads(); h++) {
            snapToWindow(dx, dy, left, right, top, bottom,
                         screen().getHeadX(h),
                         screen().getHeadX(h) + screen().getHeadWidth(h),
                         screen().getHeadY(h),
                         screen().getHeadY(h) + screen().getHeadHeight(h));
        }
    } else {
        snapToWindow(dx, dy, left, right, top, bottom,
                     screen().maxLeft(0),
                     screen().maxRight(0),
                     screen().maxTop(0),
                     screen().maxBottom(0));

        snapToWindow(dx, dy, left, right, top, bottom,
                     screen().getHeadX(0),
                     screen().getHeadX(0) + screen().getHeadWidth(0),
                     screen().getHeadY(0),
                     screen().getHeadY(0) + screen().getHeadHeight(0));
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
    }

    // commit
    if (dx <= screen().getEdgeSnapThreshold())
        orig_left += dx;
    if (dy <= screen().getEdgeSnapThreshold())
        orig_top  += dy;

}


void FluxboxWindow::startResizing(Window win, int x, int y) {
    if (s_num_grabs > 0 || isShaded() || isIconic() )
        return;

    resizing = true;
    maximized = MAX_NONE;

    const Cursor& cursor = (m_resize_corner == LEFTTOP) ? frame().theme().upperLeftAngleCursor() :
                           (m_resize_corner == RIGHTTOP) ? frame().theme().upperRightAngleCursor() :
                           (m_resize_corner == RIGHTBOTTOM) ? frame().theme().lowerRightAngleCursor() :
                                                            frame().theme().lowerLeftAngleCursor();

    grabPointer(win, false, ButtonMotionMask | ButtonReleaseMask,
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
        // search for a fluxboxwindow
        WinClient *client = Fluxbox::instance()->searchWindow(child);
        FluxboxWindow *attach_to_win = 0;
        if (client) {
            Fluxbox::TabsAttachArea area= Fluxbox::instance()->getTabsAttachArea();
            if (area == Fluxbox::ATTACH_AREA_WINDOW)
                attach_to_win = client->fbwindow();
            else if (area == Fluxbox::ATTACH_AREA_TITLEBAR) {
                if(client->fbwindow()->hasTitlebar() &&
                   client->fbwindow()->y() + client->fbwindow()->titlebarHeight() > dest_y)
                    attach_to_win = client->fbwindow();
            }
        }

        if (attach_to_win != this &&
            attach_to_win != 0 && attach_to_win->isTabable()) {

            attach_to_win->attachClient(*old_attached,x,y );
            // we could be deleted here, DO NOT do anything else that alters this object
        } else if (attach_to_win != this) {
            // disconnect client if we didn't drop on a window
            WinClient &client = *old_attached;
            detachClient(*old_attached);
            // move window by relative amount of mouse movement
            // since just detached, move relative to old location
            if (client.m_win != 0) {
                client.m_win->move(frame().x() - m_last_resize_x + x, frame().y() - m_last_resize_y + y);
                client.m_win->show();
            }
        } else if(attach_to_win==this && attach_to_win->isTabable()) {
            //reording of tabs within a frame
            moveClientTo(*old_attached, x, y);
        }

    }
}

void FluxboxWindow::restore(WinClient *client, bool remap) {
    if (client->m_win != this)
        return;

    XChangeSaveSet(display, client->window(), SetModeDelete);
    client->setEventMask(NoEventMask);

    int wx = frame().x(), wy = frame().y(); // not actually used here
    frame().gravityTranslate(wx, wy, -client->gravity(), true); // negative to invert

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
        client->reparent(screen().rootWindow(), frame().x(), frame().y(), false);

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

unsigned int FluxboxWindow::titlebarHeight() const {
    return frame().titlebarHeight();
}

Window FluxboxWindow::clientWindow() const  {
    if (m_client == 0)
        return 0;
    return m_client->window();
}

const std::string &FluxboxWindow::title() const {
    static string empty_string("");
    if (m_client == 0)
        return empty_string;
    return m_client->title();
}

const std::string &FluxboxWindow::iconTitle() const {
    static string empty_string("");
    if (m_client == 0)
        return empty_string;
    return m_client->iconTitle();
}

int FluxboxWindow::initialState() const { return m_client->initial_state; }

void FluxboxWindow::changeBlackboxHints(const BlackboxHints &net) {
    if ((net.flags & ATTRIB_SHADED) &&
        ((m_blackbox_attrib.attrib & ATTRIB_SHADED) !=
         (net.attrib & ATTRIB_SHADED)))
        shade();

    if (net.flags & (ATTRIB_MAXVERT | ATTRIB_MAXHORIZ)) {
        // make maximise look like the net maximise flags
        int want_max = MAX_NONE;

        if (net.flags & ATTRIB_MAXVERT)
            want_max |= MAX_VERT;
        if (net.flags & ATTRIB_MAXHORIZ)
            want_max |= MAX_HORZ;

        if (want_max == MAX_NONE && maximized != MAX_NONE) {
            maximize(MAX_NONE);
        } else if (want_max == MAX_FULL && maximized != MAX_FULL) {
            maximize(MAX_FULL);
            // horz and vert are a little trickier to morph
        }
            // to toggle vert
            // either we want vert and aren't
            // or we want horizontal, and are vertically (or full) at present
        if (want_max == MAX_VERT && !(maximized & MAX_VERT) ||
            want_max == MAX_HORZ && (maximized & MAX_VERT)) {
            maximize(MAX_VERT);
        }
        // note that if we want horz, it WONT be vert any more from above
        if (want_max == MAX_HORZ && !(maximized & MAX_HORZ) ||
            want_max == MAX_VERT && (maximized & MAX_HORZ)) {
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
        m_old_decoration = static_cast<Decoration>(net.decoration);
        setDecoration(m_old_decoration);
    }

}


void FluxboxWindow::fixsize(int *user_w, int *user_h) {
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

    m_client->applySizeHints(dw, dh, user_w, user_h);

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

void FluxboxWindow::addExtraMenu(const char *label, FbTk::Menu *menu) {
    menu->setInternalMenu();
    menu->disableTitle();
    m_extramenus.push_back(std::make_pair(label, menu));

    setupMenu();
}

void FluxboxWindow::removeExtraMenu(FbTk::Menu *menu) {
    ExtraMenus::iterator it = m_extramenus.begin();
    ExtraMenus::iterator it_end = m_extramenus.end();
    for (; it != it_end; ++it) {
        if (it->second == menu) {
            m_extramenus.erase(it);
            break;
        }
    }
    setupMenu();
}

void FluxboxWindow::close() {
    if (m_client)
        m_client->sendClose(false);
}

void FluxboxWindow::kill() {
    if (m_client)
        m_client->sendClose(true);
}

void FluxboxWindow::setupWindow() {
    // sets up our window
    // we allow both to be done at once to share the commands

    WinButtonTheme &winbutton_theme = screen().winButtonTheme();

    using namespace FbTk;
    typedef RefCount<Command> CommandRef;
    typedef SimpleCommand<FluxboxWindow> WindowCmd;

    CommandRef iconify_cmd(new WindowCmd(*this, &FluxboxWindow::iconify));
    CommandRef maximize_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeFull));
    CommandRef maximize_vert_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeVertical));
    CommandRef maximize_horiz_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeHorizontal));
    CommandRef close_cmd(new WindowCmd(*this, &FluxboxWindow::close));
    CommandRef shade_cmd(new WindowCmd(*this, &FluxboxWindow::shade));
    CommandRef raise_cmd(new WindowCmd(*this, &FluxboxWindow::raise));
    CommandRef lower_cmd(new WindowCmd(*this, &FluxboxWindow::lower));
    CommandRef raise_and_focus_cmd(new WindowCmd(*this, &FluxboxWindow::raiseAndFocus));
    CommandRef stick_cmd(new WindowCmd(*this, &FluxboxWindow::stick));
    CommandRef show_menu_cmd(new WindowCmd(*this, &FluxboxWindow::popupMenu));

    // clear old buttons from frame
    frame().removeAllButtons();
    //!! TODO: fix this ugly hack
    // get titlebar configuration
    const vector<Fluxbox::Titlebar> *dir = &Fluxbox::instance()->getTitlebarLeft();
    for (char c=0; c<2; c++) {
        for (size_t i=0; i< dir->size(); ++i) {
            //create new buttons
            FbTk::Button *newbutton = 0;
            if (isIconifiable() && (*dir)[i] == Fluxbox::MINIMIZE) {
                newbutton = new WinButton(*this, winbutton_theme,
                                          WinButton::MINIMIZE,
                                          frame().titlebar(),
                                          0, 0, 10, 10);
                newbutton->setOnClick(iconify_cmd);

            } else if (isMaximizable() && (*dir)[i] == Fluxbox::MAXIMIZE) {
                newbutton = new WinButton(*this, winbutton_theme,
                                          WinButton::MAXIMIZE,
                                          frame().titlebar(),
                                          0, 0, 10, 10);

                newbutton->setOnClick(maximize_cmd, 1);
                newbutton->setOnClick(maximize_horiz_cmd, 3);
                newbutton->setOnClick(maximize_vert_cmd, 2);

            } else if (m_client->isClosable() && (*dir)[i] == Fluxbox::CLOSE) {
                newbutton = new WinButton(*this, winbutton_theme,
                                          WinButton::CLOSE,
                                          frame().titlebar(),
                                          0, 0, 10, 10);

                newbutton->setOnClick(close_cmd);

            } else if ((*dir)[i] == Fluxbox::STICK) {
                WinButton *winbtn = new WinButton(*this, winbutton_theme,
                                                  WinButton::STICK,
                                                  frame().titlebar(),
                                                  0, 0, 10, 10);
                stateSig().attach(winbtn);
                winbtn->setOnClick(stick_cmd);
                newbutton = winbtn;
            } else if ((*dir)[i] == Fluxbox::SHADE) {
                WinButton *winbtn = new WinButton(*this, winbutton_theme,
                                                  WinButton::SHADE,
                                                  frame().titlebar(),
                                                  0, 0, 10, 10);
                winbtn->setOnClick(shade_cmd);
            }

            if (newbutton != 0) {
                newbutton->show();
                if (c == 0)
                    frame().addLeftButton(newbutton);
                else
                    frame().addRightButton(newbutton);
            }
        } //end for i
        dir = &Fluxbox::instance()->getTitlebarRight();
    } // end for c

    frame().reconfigure();

    // setup titlebar
    frame().setOnClickTitlebar(raise_and_focus_cmd, 1, false, true); // on press with button 1
    frame().setOnClickTitlebar(shade_cmd, 1, true); // doubleclick with button 1
    frame().setOnClickTitlebar(show_menu_cmd, 3); // on release with button 3
    frame().setOnClickTitlebar(lower_cmd, 2); // on release with button 2
    frame().setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());

    // end setup frame

    setupMenu();
}

void FluxboxWindow::setupMenu() {
    // setup menu

    menu().removeAll(); // clear old items
    menu().disableTitle(); // not titlebar

    if (screen().windowMenuFilename().empty() ||
        ! MenuCreator::createFromFile(screen().windowMenuFilename(), menu(), *this, true))

    {
        MenuCreator::createWindowMenuItem("shade", "", menu(), *this);
        MenuCreator::createWindowMenuItem("stick", "", menu(), *this);
        MenuCreator::createWindowMenuItem("maximize", "", menu(), *this);
        MenuCreator::createWindowMenuItem("iconify", "", menu(), *this);
        MenuCreator::createWindowMenuItem("raise", "", menu(), *this);
        MenuCreator::createWindowMenuItem("lower", "", menu(), *this);
        MenuCreator::createWindowMenuItem("sendto", "", menu(), *this);
        MenuCreator::createWindowMenuItem("layer", "", menu(), *this);
        MenuCreator::createWindowMenuItem("extramenus", "", menu(), *this);
        MenuCreator::createWindowMenuItem("separator", "", menu(), *this);
        MenuCreator::createWindowMenuItem("close", "", menu(), *this);
    }

    menu().reconfigure(); // update graphics
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
