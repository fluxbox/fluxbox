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

#include "Window.hh"

#include "CurrentWindowCmd.hh"
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
#ifdef REMEMBER
#include "Remember.hh"
#endif
#include "MenuCreator.hh"
#include "FocusControl.hh"
#include "IconButton.hh"
#include "ScreenPlacement.hh"
#include "RectangleUtil.hh"
#include "Debug.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/Compose.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/MemFun.hh"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <functional>
#include <algorithm>

using std::endl;
using std::string;
using std::vector;
using std::bind2nd;
using std::mem_fun;
using std::equal_to;
using std::max;
using std::swap;
using std::dec;
using std::hex;

using namespace FbTk;

namespace {

// X event scanner for enter/leave notifies - adapted from twm
typedef struct scanargs {
    Window w;
    Bool leave, inferior, enter;
} scanargs;

// look for valid enter or leave events (that may invalidate the earlier one we are interested in)
extern "C" int queueScanner(Display *, XEvent *e, char *args) {
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


void callForAllTransient(FluxboxWindow& win, void (*callFunc)(FluxboxWindow&)) {
    WinClient::TransientList::const_iterator it = win.winClient().transientList().begin();
    WinClient::TransientList::const_iterator it_end = win.winClient().transientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && !(*it)->fbwindow()->isIconic())
            // TODO: should we also check if it is the active client?
            callFunc(*(*it)->fbwindow());
    }
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

    callForAllTransient(win, raiseFluxboxWindow);

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
    const WinClient::TransientList& transients = win.winClient().transientList();
    WinClient::TransientList::const_reverse_iterator it =     transients.rbegin();
    WinClient::TransientList::const_reverse_iterator it_end = transients.rend();
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

    callForAllTransient(win, tempRaiseFluxboxWindow);

    win.oplock = false;
}

bool isWindowVisibleOnSomeHeadOrScreen(FluxboxWindow const& w) {
    int real_x = w.frame().x();
    int real_y = w.frame().y();

    if (w.screen().hasXinerama()) { // xinerama available => use head info
        return (0 != w.screen().getHead(real_x, real_y)); // if visible on some head
    }
    return RectangleUtil::insideRectangle(0, 0, w.screen().width(), w.screen().height(), real_x, real_y);
}

class SetClientCmd:public FbTk::Command<void> {
public:
    explicit SetClientCmd(WinClient &client):m_client(client) { }
    void execute() {
        m_client.focus();
    }
private:
    WinClient &m_client;
};


/// helper class for some STL routines
class ChangeProperty {
public:
    ChangeProperty(Display *disp, Atom prop, int mode,
                   unsigned char *state, int num):m_disp(disp),
                                                  m_prop(prop),
                                                  m_state(state),
                                                  m_num(num),
                                                  m_mode(mode){

    }
    void operator () (FbTk::FbWindow *win) {
        XChangeProperty(m_disp, win->window(), m_prop, m_prop, 32, m_mode,
                        m_state, m_num);
    }
private:
    Display *m_disp;
    Atom m_prop;
    unsigned char *m_state;
    int m_num;
    int m_mode;
};


// Helper class for getResizeDirection below
// Tests whether a point is on an edge or the corner.
struct TestCornerHelper {
    int corner_size_px, corner_size_pc;
    bool operator()(int xy, int wh)
    {
        /* The % checking must be right: 0% must fail, 100% must succeed. */
        return xy < corner_size_px  ||  100 * xy < corner_size_pc * wh;
    }
};

// used in FluxboxWindow::updateAll() and FluxboxWindow::Setu
typedef FbTk::Resource<vector<WinButton::Type> > WBR;

// create a button ready to be used in the frame-titlebar
WinButton* makeButton(FluxboxWindow& win, FocusableTheme<WinButtonTheme>& btheme, WinButton::Type btype) {

    FbTk::ThemeProxy<WinButtonTheme>& theme = win.screen().pressedWinButtonTheme();
    FbWinFrame& frame = win.frame();
    FbTk::FbWindow& parent = frame.titlebar();
    const unsigned int h = frame.buttonHeight();
    const unsigned int w = h;

    return new WinButton(win, btheme, theme, btype, parent, 0, 0, w, h);
}

}


int FluxboxWindow::s_num_grabs = 0;
static int s_original_workspace = 0;

FluxboxWindow::FluxboxWindow(WinClient &client):
    Focusable(client.screen(), this),
    oplock(false),
    m_creation_time(0),
    moving(false), resizing(false),
    m_initialized(false),
    m_attaching_tab(0),
    display(FbTk::App::instance()->display()),
    m_button_grab_x(0), m_button_grab_y(0),
    m_last_move_x(0), m_last_move_y(0),
    m_last_resize_h(1), m_last_resize_w(1),
    m_last_pressed_button(0),
    m_workspace_number(0),
    m_current_state(0),
    m_old_decoration_mask(0),
    m_client(&client),
    m_toggled_decos(false),
    m_focus_protection(Focus::NoProtection),
    m_mouse_focus(BoolAcc(screen().focusControl(), &FocusControl::isMouseFocus)),
    m_click_focus(true),
    m_last_button_x(0),  m_last_button_y(0),
    m_button_theme(*this, screen().focusedWinButtonTheme(),
                   screen().unfocusedWinButtonTheme()),
    m_theme(*this, screen().focusedWinFrameTheme(),
            screen().unfocusedWinFrameTheme()),
    m_frame(client.screen(), client.depth(), m_state, m_theme),
    m_placed(false),
    m_old_layernum(0),
    m_parent(client.screen().rootWindow()),
    m_resize_corner(RIGHTBOTTOM) {

    join(m_theme.reconfigSig(), FbTk::MemFun(*this, &FluxboxWindow::themeReconfigured));
    join(m_frame.frameExtentSig(), FbTk::MemFun(*this, &FluxboxWindow::frameExtentChanged));

    init();

    if (!isManaged())
        return;

    // add the window to the focus list
    // always add to front on startup to keep the focus order the same
    if (isFocused() || Fluxbox::instance()->isStartup())
        screen().focusControl().addFocusWinFront(*this);
    else
        screen().focusControl().addFocusWinBack(*this);

    Fluxbox::instance()->keys()->registerWindow(frame().window().window(),
                                                *this, Keys::ON_WINDOW);

}


FluxboxWindow::~FluxboxWindow() {
    if (WindowCmd<void>::window() == this)
        WindowCmd<void>::setWindow(0);
    if (FbMenu::window() == this)
        FbMenu::setWindow(0);
    if ( Fluxbox::instance()->keys() != 0 ) {
        Fluxbox::instance()->keys()->
            unregisterWindow(frame().window().window());
    }

    fbdbg << "starting ~FluxboxWindow(" << this << "," 
        << (m_client ? m_client->title().logical().c_str() : "") << ")" << endl
        << "num clients = " << numClients() << endl
        << "curr client = "<< m_client << endl
        << "m_labelbuttons.size = " << m_labelbuttons.size() << endl;

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
    m_tabActivationTimer.stop();

    // notify die
    dieSig().emit(*this);

    if (m_client != 0 && !m_screen.isShuttingdown())
        delete m_client; // this also removes client from our list
    m_client = 0;

    if (m_clientlist.size() > 1) {
        fbdbg<<"(~FluxboxWindow()) WARNING! clientlist > 1"<<endl;
        while (!m_clientlist.empty()) {
            detachClient(*m_clientlist.back());
        }
    }

    if (!screen().isShuttingdown())
        screen().focusControl().removeWindow(*this);


    fbdbg<<"~FluxboxWindow("<<this<<")"<<endl;
}


void FluxboxWindow::init() {
    m_attaching_tab = 0;

    // fetch client size and placement
    XWindowAttributes wattrib;
    if (! m_client->getAttrib(wattrib) ||
        !wattrib.screen  || // no screen? ??
        wattrib.override_redirect || // override redirect
        m_client->initial_state == WithdrawnState ||
        m_client->getWMClassClass() == "DockApp") { // Slit client
        return;
    }

    if (m_client->initial_state == IconicState)
        m_state.iconic = true;

    m_client->setFluxboxWindow(this);
    m_client->setGroupLeftWindow(None); // nothing to the left.

    if (Fluxbox::instance()->haveShape())

        Shape::setShapeNotify(winClient());

    //!! TODO init of client should be better
    // we don't want to duplicate code here and in attachClient
    m_clientlist.push_back(m_client);

    fbdbg<<"FluxboxWindow::init(this="<<this<<", client="<<hex<<
        m_client->window()<<", frame = "<<frame().window().window()<<dec<<")"<<endl;

    Fluxbox &fluxbox = *Fluxbox::instance();

    associateClient(*m_client);

    frame().setFocusTitle(title());

    // redirect events from frame to us
    frame().setEventHandler(*this);
    fluxbox.saveWindowSearchGroup(frame().window().window(), this);
    fluxbox.saveWindowSearchGroup(frame().tabcontainer().window(), this);

    m_workspace_number = m_screen.currentWorkspaceID();

    // set default decorations but don't apply them
    setDecorationMask(WindowState::getDecoMaskFromString(screen().defaultDeco()),
                      false);

    functions.resize = functions.move = functions.iconify = functions.maximize
    = functions.close = functions.tabable = true;

    updateMWMHintsFromClient(*m_client);

    m_timer.setTimeout(fluxbox.getAutoRaiseDelay() * FbTk::FbTime::IN_MILLISECONDS);
    FbTk::RefCount<FbTk::Command<void> > raise_cmd(new FbTk::SimpleCommand<FluxboxWindow>(*this,
                                                                                   &FluxboxWindow::raise));
    m_timer.setCommand(raise_cmd);
    m_timer.fireOnce(true);

    m_tabActivationTimer.setTimeout(fluxbox.getAutoRaiseDelay() * FbTk::FbTime::IN_MILLISECONDS);
    FbTk::RefCount<ActivateTabCmd> activate_tab_cmd(new ActivateTabCmd());
    m_tabActivationTimer.setCommand(activate_tab_cmd);
    m_tabActivationTimer.fireOnce(true);

    m_reposLabels_timer.setTimeout(IconButton::updateLaziness());
    m_reposLabels_timer.fireOnce(true);
    FbTk::RefCount<FbTk::Command<void> > elrs(new FbTk::SimpleCommand<FluxboxWindow>(*this, &FluxboxWindow::emitLabelReposSig));
    m_reposLabels_timer.setCommand(elrs);

    /**************************************************/
    /* Read state above here, apply state below here. */
    /**************************************************/

    if (m_client->isTransient() && m_client->transientFor()->fbwindow())
        m_state.stuck = m_client->transientFor()->fbwindow()->isStuck();

    if (!m_client->sizeHints().isResizable()) {
        functions.resize = functions.maximize = false;
        decorations.tab = false; //no tab for this window
    }

    associateClientWindow();

    setWindowType(m_client->getWindowType());

    auto sanitizePosition = [&]() {
        int head = screen().getHead(fbWindow());
        if (head == 0 && screen().hasXinerama())
            head = screen().getCurrHead();
        int left = screen().maxLeft(head),   top  = screen().maxTop(head),
            btm  = screen().maxBottom(head), rght = screen().maxRight(head);
        const int margin = hasTitlebar() ? 32 : 8;
        // ensure the window intersects with the workspace x-axis
        if (int(frame().x() + frame().width()) < left) {
            left += margin - frame().width();
        } else if (frame().x() > rght) {
            left = rght - margin;
        } else {
            left = frame().x();
        }
        if (hasTitlebar()) {
            // ensure the titlebar is inside the workspace
            top  = std::max(top,  std::min(frame().y(), btm  - margin));
        } else {
            // ensure "something" is inside the workspace
            if (int(frame().y() + frame().height()) < top)
                top += margin - frame().height();
            else if (frame().y() > btm)
                top = btm - margin;
            else
                top = frame().y();
        }
        frame().move(left, top);
    };

    if (fluxbox.isStartup()) {
        m_placed = true;
    } else if (m_client->normal_hint_flags & (PPosition|USPosition)) {
        m_placed = true;
        sanitizePosition();
    } else {
        if (!isWindowVisibleOnSomeHeadOrScreen(*this)) {
            // this probably should never happen, but if a window
            // unexplicitly has its topleft corner outside any screen,
            // move it to the current screen and ensure it's just placed
            int cur = screen().getHead(fbWindow());
            move(screen().getHeadX(cur), screen().getHeadY(cur));
            m_placed = false; // allow placement strategy to fix position
        }
        setOnHead(screen().getCurrHead());
    }

    // we must do this now, or else resizing may not work properly
    applyDecorations();

    fluxbox.attachSignals(*this);

    if (!m_state.fullscreen) {
        unsigned int new_width = 0, new_height = 0;
        if (m_client->width() >= screen().width()) {
            m_state.maximized |= WindowState::MAX_HORZ;
            new_width = 2 * screen().width() / 3;
        }
        if (m_client->height() >= screen().height()) {
            m_state.maximized |= WindowState::MAX_VERT;
            new_height = 2 * screen().height() / 3;
        }
        if (new_width || new_height) {
            const int maximized = m_state.maximized;
            m_state.maximized = WindowState::MAX_NONE;
            resize(new_width ? new_width : width(), new_height ? new_height : height());
            m_placed = false;
            m_state.maximized = maximized;
        }
    }

    // this window is managed, we are now allowed to modify actual state
    m_initialized = true;

    if (m_workspace_number >= screen().numberOfWorkspaces())
        m_workspace_number = screen().currentWorkspaceID();

    unsigned int real_width = frame().width();
    unsigned int real_height = frame().height();
    frame().applySizeHints(real_width, real_height);

    // if we're a transient then we should be on the same layer and workspace
    FluxboxWindow* twin = m_client->transientFor() ? m_client->transientFor()->fbwindow() : 0;
    if (twin && twin != this) {
        if (twin->layerNum() < ResourceLayer::DESKTOP) { // don't confine layer for desktops
            layerItem().setLayer(twin->layerItem().getLayer());
            m_state.layernum = twin->layerNum();
        }
        m_workspace_number = twin->workspaceNumber();
        const int x = twin->frame().x() + int(twin->frame().width() - frame().width())/2;
        const int y = twin->frame().y() + int(twin->frame().height() - frame().height())/2;
        frame().move(x, y);
        sanitizePosition();
        m_placed = true;
    } else // if no parent then set default layer
        moveToLayer(m_state.layernum, m_state.layernum != ::ResourceLayer::NORMAL);

    fbdbg<<"FluxboxWindow::init("<<title().logical()<<") transientFor: "<<
       m_client->transientFor()<<endl;
    if (twin) {
        fbdbg<<"FluxboxWindow::init("<<title().logical()<<") transientFor->title(): "<<
           twin->title().logical()<<endl;
    }

    screen().getWorkspace(m_workspace_number)->addWindow(*this);
    if (m_placed)
        moveResize(frame().x(), frame().y(), real_width, real_height);
    else
        placeWindow(getOnHead());

    setFocusFlag(false); // update graphics before mapping

    if (m_state.stuck) {
        m_state.stuck = false;
        stick();
    }

    if (m_state.shaded) { // start shaded
        m_state.shaded = false;
        shade();
    }

    if (m_state.iconic) {
        m_state.iconic = false;
        iconify();
    } else if (m_workspace_number == screen().currentWorkspaceID()) {
        m_state.iconic = true;
        deiconify(false);
        // check if we should prevent this window from gaining focus
        m_focused = false; // deiconify sets this
        if (!Fluxbox::instance()->isStartup() && isFocusNew()) {
            Focus::Protection fp = m_focus_protection;
            m_focus_protection &= ~Focus::Deny; // new windows run as "Refuse"
            m_focused = focusRequestFromClient(*m_client);
            m_focus_protection = fp;
            if (!m_focused)
                lower();
        }
    }

    if (m_state.fullscreen) {
        m_state.fullscreen = false;
        setFullscreen(true);
    }

    if (m_state.maximized) {
        int tmp = m_state.maximized;
        m_state.maximized = WindowState::MAX_NONE;
        setMaximizedState(tmp);
    }

    m_workspacesig.emit(*this);
    m_creation_time = FbTk::FbTime::mono();
    frame().frameExtentSig().emit();
    setupWindow();
    fluxbox.sync(false);

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

            moveResizeClient(**client_it);

            // create a labelbutton for this client and
            // associate it with the pointer
            associateClient(*(*client_it));

            //null if we want the new button at the end of the list
            if (x >= 0 && button_insert_pos)
                frame().moveLabelButtonLeftOf(*m_labelbuttons[*client_it], *button_insert_pos);
        }

        // add client and move over all attached clients
        // from the old window to this list
        m_clientlist.splice(client_insert_pos, old_win->m_clientlist);
        updateClientLeftWindow();
        old_win->m_client = 0;

        delete old_win;

    } else { // client.fbwindow() == 0

        associateClient(client);
        moveResizeClient(client);

        // right now, this block only happens with new windows or on restart
        bool is_startup = Fluxbox::instance()->isStartup();

        // we use m_focused as a signal to focus the window when mapped
        if (isFocusNew() && !is_startup)
            m_focused = focusRequestFromClient(client);
        focused_win = (isFocusNew() || is_startup) ? &client : m_client;

        m_clientlist.push_back(&client);
    }

    // make sure that the state etc etc is updated for the new client
    // TODO: one day these should probably be neatened to only act on the
    // affected clients if possible
    m_statesig.emit(*this);
    m_workspacesig.emit(*this);
    m_layersig.emit(*this);

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


    fbdbg<<"("<<__FUNCTION__<<")["<<this<<"]"<<endl;


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

    fbdbg<<"("<<__FUNCTION__<<")["<<this<<"] numClients = "<<numClients()<<endl;

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

FluxboxWindow::ClientList::iterator FluxboxWindow::getClientInsertPosition(int x, int y) {

    int dest_x = 0, dest_y = 0;
    Window labelbutton = 0;
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               parent().window(), frame().tabcontainer().window(),
                               x, y, &dest_x, &dest_y,
                               &labelbutton))
        return m_clientlist.end();

    WinClient* c = winClientOfLabelButtonWindow(labelbutton);

    // label button not found
    if (!c)
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
                                       c);
    if (x > static_cast<signed>(m_labelbuttons[c]->width()) / 2)
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

    WinClient* client = winClientOfLabelButtonWindow(labelbutton);

    if (!client)
        return;

    Window child_return = 0;
    //make x and y relative to our labelbutton
    if (!XTranslateCoordinates(FbTk::App::instance()->display(),
                               frame().tabcontainer().window(), labelbutton,
                               dest_x, dest_y, &x, &y,
                               &child_return))
        return;
    if (x > static_cast<signed>(m_labelbuttons[client]->width()) / 2)
        moveClientRightOf(win, *client);
    else
        moveClientLeftOf(win, *client);

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
        if (it == m_clientlist.end() || new_pos==m_clientlist.end())
            return;
        //moving a button to the left of itself results in no change
        if (new_pos == it)
            return;
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

    if (!client.acceptsFocus())
        setinput = false; // don't try

    WinClient *old = m_client;
    m_client = &client;

    bool ret = setinput && focus();
    if (setinput && old->acceptsFocus()) {
        m_client = old;
        return ret;
    }

    m_client->raise();
    if (m_focused) {
        m_client->notifyFocusChanged();
        if (old)
            old->notifyFocusChanged();
    }

    fbdbg<<"FluxboxWindow::"<<__FUNCTION__<<": labelbutton[client] = "<<
        button<<endl;

    if (old != &client) {
        titleSig().emit(title().logical(), *this);
        frame().setFocusTitle(title());
        frame().setShapingClient(&client, false);
    }
    return ret;
}

bool FluxboxWindow::isGroupable() const {
    if (isResizable() && isMaximizable() && !winClient().isTransient())
        return true;
    return false;
}

bool FluxboxWindow::isFocusNew() const {
    if (m_focus_protection & Focus::Gain)
        return true;
    if (m_focus_protection & Focus::Refuse)
        return false;
    return screen().focusControl().focusNew();
}

void FluxboxWindow::associateClientWindow() {
    frame().setShapingClient(m_client, false);

    frame().moveResizeForClient(m_client->x(), m_client->y(),
                                m_client->width(), m_client->height(),
                                m_client->gravity(), m_client->old_bw);

    updateSizeHints();
    frame().setClientWindow(*m_client);
}

void FluxboxWindow::updateSizeHints() {
    m_size_hint = m_client->sizeHints();

    ClientList::const_iterator it = clientList().begin();
    ClientList::const_iterator it_end = clientList().end();
    for (; it != it_end; ++it) {
        if ((*it) == m_client)
            continue;

        const SizeHints &hint = (*it)->sizeHints();
        if (m_size_hint.min_width < hint.min_width)
            m_size_hint.min_width = hint.min_width;
        if (m_size_hint.max_width > hint.max_width)
            m_size_hint.max_width = hint.max_width;
        if (m_size_hint.min_height < hint.min_height)
            m_size_hint.min_height = hint.min_height;
        if (m_size_hint.max_height > hint.max_height)
            m_size_hint.max_height = hint.max_height;
        // lcm could end up a bit silly, and the situation is bad no matter what
        if (m_size_hint.width_inc < hint.width_inc)
            m_size_hint.width_inc = hint.width_inc;
        if (m_size_hint.height_inc < hint.height_inc)
            m_size_hint.height_inc = hint.height_inc;
        if (m_size_hint.base_width < hint.base_width)
            m_size_hint.base_width = hint.base_width;
        if (m_size_hint.base_height < hint.base_height)
            m_size_hint.base_height = hint.base_height;
        if (m_size_hint.min_aspect_x * hint.min_aspect_y >
            m_size_hint.min_aspect_y * hint.min_aspect_x) {
            m_size_hint.min_aspect_x = hint.min_aspect_x;
            m_size_hint.min_aspect_y = hint.min_aspect_y;
        }
        if (m_size_hint.max_aspect_x * hint.max_aspect_y >
            m_size_hint.max_aspect_y * hint.max_aspect_x) {
            m_size_hint.max_aspect_x = hint.max_aspect_x;
            m_size_hint.max_aspect_y = hint.max_aspect_y;
        }
    }
    frame().setSizeHints(m_size_hint);
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
    m_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay() * FbTk::FbTime::IN_MILLISECONDS);
    m_tabActivationTimer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay() * FbTk::FbTime::IN_MILLISECONDS);
    updateButtons();
    frame().reconfigure();
    menu().reconfigure();

    Client2ButtonMap::iterator it = m_labelbuttons.begin(),
                               it_end = m_labelbuttons.end();
    for (; it != it_end; ++it)
        it->second->setPixmap(screen().getTabsUsePixmap());

}

void FluxboxWindow::updateMWMHintsFromClient(WinClient &client) {
    const WinClient::MwmHints *hint = client.getMwmHint();

    if (hint && !m_toggled_decos && hint->flags & MwmHintsDecorations) {
        if (hint->decorations & MwmDecorAll) {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.menu = true;
        } else {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.tab = false;
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
    } else {
        decorations.titlebar = decorations.handle = decorations.border =
        decorations.iconify = decorations.maximize = decorations.menu = true;
    }

    unsigned int mask = decorationMask();
    mask &= WindowState::getDecoMaskFromString(screen().defaultDeco());
    setDecorationMask(mask, false);

    // functions.tabable is ours, not special one
    // note that it means this window is "tabbable"
    if (hint && hint->flags & MwmHintsFunctions) {
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
    } else {
        functions.resize = functions.move = functions.iconify =
        functions.maximize = functions.close = true;
    }
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

        /* Ignore all EnterNotify events until the pointer actually moves */
        screen().focusControl().ignoreAtPointer();
    }

}

void FluxboxWindow::moveResizeForClient(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height, int gravity, unsigned int client_bw) {

    m_placed = true;
    frame().moveResizeForClient(new_x, new_y, new_width, new_height, gravity, client_bw);
    setFocusFlag(m_focused);
    m_state.shaded = false;
    sendConfigureNotify();

    if (!moving) {
        m_last_resize_x = new_x;
        m_last_resize_y = new_y;
    }

}

void FluxboxWindow::getMaxSize(unsigned int* width, unsigned int* height) const {
    if (width)
        *width = m_size_hint.max_width;
    if (height)
        *height = m_size_hint.max_height;
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

        // fetch the window to the current workspace if minimized
        if (isIconic())
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


    fbdbg<<"FluxboxWindow::"<<__FUNCTION__<<" isModal() = "<<m_client->isModal()<<endl;
    fbdbg<<"FluxboxWindow::"<<__FUNCTION__<<" transient size = "<<m_client->transients.size()<<endl;

    if (!m_client->transients.empty() && m_client->isModal()) {
        fbdbg<<__FUNCTION__<<": isModal and have transients client = "<<
            hex<<m_client->window()<<dec<<endl;
        fbdbg<<__FUNCTION__<<": this = "<<this<<endl;

        WinClient::TransientList::iterator it = m_client->transients.begin();
        WinClient::TransientList::iterator it_end = m_client->transients.end();
        for (; it != it_end; ++it) {
            fbdbg<<__FUNCTION__<<": transient 0x"<<(*it)<<endl;
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
   fbdbg<<"("<<__FUNCTION__<<")["<<this<<"]"<<endl;

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

    m_state.iconic = true;
    m_statesig.emit(*this);

    hide(true);

    screen().focusControl().setFocusBack(*this);

    ClientList::iterator client_it = m_clientlist.begin();
    const ClientList::iterator client_it_end = m_clientlist.end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient &client = *(*client_it);
        WinClient::TransientList::iterator it = client.transientList().begin();
        WinClient::TransientList::iterator it_end = client.transientList().end();
        for (; it != it_end; ++it)
            if ((*it)->fbwindow())
                (*it)->fbwindow()->iconify();
    }

    // focus revert is done elsewhere (based on signal)
}

void FluxboxWindow::deiconify(bool do_raise) {
    if (numClients() == 0 || !m_state.iconic || oplock)
        return;

    oplock = true;

    // reassociate first, so it gets removed from screen's icon list
    screen().reassociateWindow(this, m_workspace_number, false);
    m_state.iconic = false;
    m_statesig.emit(*this);

    // deiconify all transients
    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_end = clientList().end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient::TransientList::iterator trans_it =
            (*client_it)->transientList().begin();
        WinClient::TransientList::iterator trans_it_end =
            (*client_it)->transientList().end();
        for (; trans_it != trans_it_end; ++trans_it) {
            if ((*trans_it)->fbwindow())
                (*trans_it)->fbwindow()->deiconify(false);
        }
    }

    if (m_workspace_number != screen().currentWorkspaceID()) {
        oplock = false;
        return;
    }

    show();

    // focus new, OR if it's the only window on the workspace
    // but not on startup: focus will be handled after creating everything
    // we use m_focused as a signal to focus the window when mapped
    if (screen().currentWorkspace()->numberOfWindows() == 1 ||
        isFocusNew() || m_client->isTransient())
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
        m_state.fullscreen = flag;
        return;
    }

    if (flag && !isFullscreen()) {

        m_old_layernum = layerNum();
        m_state.fullscreen = true;
        frame().applyState();

        setFullscreenLayer(); // calls stateSig().emit()
        if (!isFocused()) {
            join(screen().focusedWindowSig(),
                 FbTk::MemFun(*this, &FluxboxWindow::focusedWindowChanged));
        }

    } else if (!flag && isFullscreen()) {

        m_state.fullscreen = false;
        frame().applyState();

        moveToLayer(m_old_layernum);
        stateSig().emit(*this);
    }

    attachWorkAreaSig();
}

void FluxboxWindow::setFullscreenLayer() {

    FluxboxWindow *foc = FocusControl::focusedFbWindow();
    // if another window on the same head is focused, make sure we can see it
    if (isFocused() || !foc || &foc->screen() != &screen() ||
        getOnHead() != foc->getOnHead() ||
        (foc->winClient().isTransient() &&
         foc->winClient().transientFor()->fbwindow() == this)) {
        moveToLayer(::ResourceLayer::ABOVE_DOCK);
    } else {
        moveToLayer(foc->layerNum());
        foc->raise();
    }
    stateSig().emit(*this);

}

void FluxboxWindow::attachWorkAreaSig() {
    // notify when struts change, so we can resize accordingly
    // Subject checks for duplicates for us
    // XXX: this is no longer true with signals
    if (m_state.maximized || m_state.fullscreen)
        join(screen().workspaceAreaSig(),
             FbTk::MemFun(*this, &FluxboxWindow::workspaceAreaChanged));
    else
        leave(screen().workspaceAreaSig());
}

/**
   Maximize window both horizontal and vertical
*/
void FluxboxWindow::maximize(int type) {
    int new_max = m_state.queryToggleMaximized(type);
    setMaximizedState(new_max);
}

void FluxboxWindow::setMaximizedState(int type) {

    if (!m_initialized || type == m_state.maximized) {
        // this will interfere with window placement, so we delay it
        m_state.maximized = type;
        return;
    }

    if (isResizing())
        stopResizing();

    if (isShaded()) {
        // do not call ::shade() here to trigger frame().applyState() and
        // stateSig().emit() only once
        m_state.shaded = false;
    }

    m_state.maximized = type;
    frame().applyState();

    attachWorkAreaSig();

    // notify listeners that we changed state
    stateSig().emit(*this);
}

void FluxboxWindow::disableMaximization() {

    m_state.maximized = WindowState::MAX_NONE;
    // TODO: could be optional, if the window gets back to original size /
    // position after maximization is disabled
    m_state.saveGeometry(frame().x(), frame().y(),
                         frame().width(), frame().height());
    frame().applyState();
    stateSig().emit(*this);
}


/**
 * Maximize window horizontal
 */
void FluxboxWindow::maximizeHorizontal() {
    maximize(WindowState::MAX_HORZ);
}

/**
 * Maximize window vertical
 */
void FluxboxWindow::maximizeVertical() {
    maximize(WindowState::MAX_VERT);
}

/**
 * Maximize window fully
 */
void FluxboxWindow::maximizeFull() {
    maximize(WindowState::MAX_FULL);
}

void FluxboxWindow::setWorkspace(int n) {
    unsigned int old_wkspc = m_workspace_number;

    m_workspace_number = n;

    // notify workspace change
    if (m_initialized && old_wkspc != m_workspace_number) {
        fbdbg<<this<<" emit workspace signal"<<endl;
        m_workspacesig.emit(*this);
    }
}

void FluxboxWindow::setLayerNum(int layernum) {
    m_state.layernum = layernum;

    if (m_initialized) {
        fbdbg<<this<<" notify layer signal"<<endl;
        m_layersig.emit(*this);
    }
}

void FluxboxWindow::shade() {
    // we can only shade if we have a titlebar
    if (!decorations.titlebar)
        return;

    m_state.shaded = !m_state.shaded;
    if (!m_initialized)
        return;

    frame().applyState();
    stateSig().emit(*this);
    // TODO: this should set IconicState, but then we can't focus the window
}

void FluxboxWindow::shadeOn() {
    if (!m_state.shaded)
        shade();
}

void FluxboxWindow::shadeOff() {
    if (m_state.shaded)
        shade();
}

void FluxboxWindow::setShaded(bool val) {
    if (val != m_state.shaded)
        shade();
}

void FluxboxWindow::stick() {

    m_state.stuck = !m_state.stuck;

    if (m_initialized) {
        stateSig().emit(*this);
        // notify since some things consider "stuck" to be a pseudo-workspace
        m_workspacesig.emit(*this);
    }

    ClientList::iterator client_it = clientList().begin();
    ClientList::iterator client_it_end = clientList().end();
    for (; client_it != client_it_end; ++client_it) {

        WinClient::TransientList::const_iterator it = (*client_it)->transientList().begin();
        WinClient::TransientList::const_iterator it_end = (*client_it)->transientList().end();
        for (; it != it_end; ++it) {
            if ((*it)->fbwindow())
                (*it)->fbwindow()->setStuck(m_state.stuck);
        }

    }

}

void FluxboxWindow::setStuck(bool val) {
    if (val != m_state.stuck)
        stick();
}

void FluxboxWindow::setIconic(bool val) {
    if (!val && isIconic())
        deiconify();
    if (val && !isIconic())
        iconify();
}

void FluxboxWindow::raise() {
    if (isIconic())
        return;

    fbdbg<<"FluxboxWindow("<<title().logical()<<")::raise()[layer="<<layerNum()<<"]"<<endl;

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
        return;

    fbdbg<<"FluxboxWindow("<<title().logical()<<")::lower()"<<endl;

    /* Ignore all EnterNotify events until the pointer actually moves */
    screen().focusControl().ignoreAtPointer();

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


void FluxboxWindow::changeLayer(int diff) {
    moveToLayer(m_state.layernum+diff);
}

void FluxboxWindow::moveToLayer(int layernum, bool force) {

    fbdbg<<"FluxboxWindow("<<title().logical()<<")::moveToLayer("<<layernum<<")"<<endl;

    // don't let it set its layer into menu area
    if (layernum <= ::ResourceLayer::MENU)
        layernum = ::ResourceLayer::MENU + 1;
    else if (layernum >= ::ResourceLayer::NUM_LAYERS)
        layernum = ::ResourceLayer::NUM_LAYERS - 1;

    if (!m_initialized)
        m_state.layernum = layernum;

    if ((m_state.layernum == layernum && !force) || !m_client)
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
    m_state.focus_hidden = value;
    if (m_initialized)
        m_statesig.emit(*this);
}

void FluxboxWindow::setIconHidden(bool value) {
    m_state.icon_hidden = value;
    if (m_initialized)
        m_statesig.emit(*this);
}


// window has actually RECEIVED focus (got a FocusIn event)
// so now we make it a focused frame etc
void FluxboxWindow::setFocusFlag(bool focus) {
    if (!m_client) return;

    bool was_focused = isFocused();
    m_focused = focus;

    fbdbg<<"FluxboxWindow("<<title().logical()<<")::setFocusFlag("<<focus<<")"<<endl;


    installColormap(focus);

    // if we're fullscreen and another window gains focus on the same head,
    // then we need to let the user see it
    if (m_state.fullscreen && !focus) {
        join(screen().focusedWindowSig(),
             FbTk::MemFun(*this, &FluxboxWindow::focusedWindowChanged));
    }

    if (m_state.fullscreen && focus) {
        moveToLayer(::ResourceLayer::ABOVE_DOCK);
        leave(screen().focusedWindowSig());
    }

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
        notifyFocusChanged();
        if (m_client)
            m_client->notifyFocusChanged();
        Fluxbox::instance()->keys()->doAction(focus ? FocusIn : FocusOut, 0, 0,
                                              Keys::ON_WINDOW, m_client);
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
             ChangeProperty(display,
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
   Show the window menu at pos x, y
*/
void FluxboxWindow::showMenu(int x, int y) {

    menu().reloadHelper()->checkReload();
    FbMenu::setWindow(this);
    screen().placementStrategy()
        .placeAndShowMenu(menu(), x, y, true);
}

void FluxboxWindow::popupMenu(int x, int y) {
    // hide menu if it was opened for this window before
    if (menu().isVisible() && FbMenu::window() == this) {
       menu().hide();
       return;
    }

    menu().disableTitle();

    showMenu(x, y);
}

/**
   Moves the menu to last button press position and shows it,
   if it's already visible it'll be hidden
 */
void FluxboxWindow::popupMenu() {

    if (m_last_button_x < x() || m_last_button_x > x() + static_cast<signed>(width()))
        m_last_button_x = x();

    popupMenu(m_last_button_x, frame().titlebarHeight() + frame().y());
}


/**
   Redirect any unhandled event to our handlers
*/
void FluxboxWindow::handleEvent(XEvent &event) {
    switch (event.type) {
    case ConfigureRequest:
       fbdbg<<"ConfigureRequest("<<title().logical()<<")"<<endl;

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
        fbdbg<<"PropertyNotify("<<title().logical()<<"), property = "<<atomname<<endl;
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

            fbdbg<<"ShapeNotify("<<title().logical()<<")"<<endl;

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
        fbdbg<<"("<<__FUNCTION__<<"): Can't find client!"<<endl;
        return;
    }

    // Note: this function never gets called from WithdrawnState
    // initial state is handled in init()

    setCurrentClient(*client, false); // focus handled on MapNotify
    deiconify();

    if (isFocusNew()) {
        m_focused = false; // deiconify sets this
        Focus::Protection fp = m_focus_protection;
        m_focus_protection &= ~Focus::Deny; // goes by "Refuse"
        m_focused = focusRequestFromClient(*client);
        m_focus_protection = fp;
        if (!m_focused)
            lower();
    }

}

bool FluxboxWindow::focusRequestFromClient(WinClient &from) {

    if (from.fbwindow() != this)
        return false;

    bool ret = true;

    FluxboxWindow *cur = FocusControl::focusedFbWindow();
    WinClient *client = FocusControl::focusedWindow();
    if ((from.fbwindow() && (from.fbwindow()->focusProtection() & Focus::Deny)) ||
        (cur && (cur->focusProtection() & Focus::Lock))) {
        ret = false;
    } else if (cur && getRootTransientFor(&from) != getRootTransientFor(client)) {
        ret = !(cur->isFullscreen() && getOnHead() == cur->getOnHead()) &&
              !cur->isTyping();
    }

    if (!ret)
        Fluxbox::instance()->attentionHandler().addAttention(from);
    return ret;

}

void FluxboxWindow::mapNotifyEvent(XMapEvent &ne) {
    WinClient *client = findClient(ne.window);
    if (!client || client != m_client)
        return;

    if (ne.override_redirect || !isVisible() || !client->validateClient())
        return;

    m_state.iconic = false;

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


    fbdbg<<"("<<__FUNCTION__<<"): 0x"<<hex<<client->window()<<dec<<endl;
    fbdbg<<"("<<__FUNCTION__<<"): title="<<client->title().logical()<<endl;

    if (numClients() == 1) // unmapping the last client
        frame().hide(); // hide this now, otherwise compositors will fade out the frame, bug #1110
    restore(client, false);

}

/**
   Checks if event is for m_client->window.
   If it isn't, we leave it until the window is unmapped, if it is,
   we just hide it for now.
*/
void FluxboxWindow::destroyNotifyEvent(XDestroyWindowEvent &de) {
    if (de.window == m_client->window()) {
        fbdbg<<"DestroyNotifyEvent this="<<this<<" title = "<<title().logical()<<endl;
        delete m_client;
        if (numClients() == 0)
            delete this;
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
        titleSig().emit(title().logical(), *this);
        // nothing uses this yet
        // hintSig().emit(*this);
        break;

    case XA_WM_ICON_NAME:
        // we don't use icon title, since many apps don't update it,
        // and we don't show icons anyway
        break;
    case XA_WM_NAME:
        client.updateTitle();
        break;

    case XA_WM_NORMAL_HINTS: {
        fbdbg<<"XA_WM_NORMAL_HINTS("<<title().logical()<<")"<<endl;
        unsigned int old_max_width = client.maxWidth();
        unsigned int old_min_width = client.minWidth();
        unsigned int old_min_height = client.minHeight();
        unsigned int old_max_height = client.maxHeight();
        bool changed = false;
        client.updateWMNormalHints();
        updateSizeHints();

        if (client.minWidth() != old_min_width ||
            client.maxWidth() != old_max_width ||
            client.minHeight() != old_min_height ||
            client.maxHeight() != old_max_height) {
            if (!client.sizeHints().isResizable()) {
                if (functions.resize ||
                    functions.maximize)
                    changed = true;
                functions.resize = functions.maximize = false;
            } else {
                // TODO: is broken while handled by FbW, needs to be in WinClient
                if (!functions.maximize || !functions.resize)
                    changed = true;
                functions.maximize = functions.resize = true;
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
#ifdef REMEMBER
            if (!m_toggled_decos) {
                Remember::instance().updateDecoStateFromClient(client);
            }
#endif
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
                       - frame().handleHeight();
    int cx = old_x, cy = old_y, ignore = 0;
    unsigned int cw = old_w, ch = old_h;

    // make sure the new width/height would be ok with all clients, or else they
    // could try to resize the window back and forth
    if (cr.value_mask & CWWidth || cr.value_mask & CWHeight) {
        unsigned int new_w = (cr.value_mask & CWWidth) ? cr.width : cw;
        unsigned int new_h = (cr.value_mask & CWHeight) ? cr.height : ch;
        ClientList::iterator it = clientList().begin();
        ClientList::iterator it_end = clientList().end();
        for (; it != it_end; ++it) {
            if (*it != client && !(*it)->sizeHints().valid(new_w, new_h))
                cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
        }
    }

    // don't allow moving/resizing fullscreen or maximized windows
    if (isFullscreen() || (isMaximizedHorz() && screen().getMaxIgnoreIncrement()))
        cr.value_mask = cr.value_mask & ~(CWWidth | CWX);
    if (isFullscreen() || (isMaximizedVert() && screen().getMaxIgnoreIncrement()))
        cr.value_mask = cr.value_mask & ~(CWHeight | CWY);

#ifdef REMEMBER
    // don't let misbehaving clients (e.g. MPlayer) move/resize their windows
    // just after creation if the user has a saved position/size
    if (m_creation_time) {

        uint64_t now = FbTk::FbTime::mono();

        Remember& rinst = Remember::instance();

        if (now > (m_creation_time + FbTk::FbTime::IN_SECONDS)) {
            m_creation_time = 0;
        } else if (rinst.isRemembered(*client, Remember::REM_MAXIMIZEDSTATE) ||
                 rinst.isRemembered(*client, Remember::REM_FULLSCREENSTATE)) {
            cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
            cr.value_mask = cr.value_mask & ~(CWX | CWY);
        } else {
            if (rinst.isRemembered(*client, Remember::REM_DIMENSIONS))
                cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);

            if (rinst.isRemembered(*client, Remember::REM_POSITION))
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
            if ((isFocused() && focusRequestFromClient(*client)) ||
                !FocusControl::focusedWindow()) {
                setCurrentClient(*client, true);
                raise();
            } else if (getRootTransientFor(client) ==
                         getRootTransientFor(FocusControl::focusedWindow())) {
                setCurrentClient(*client, false);
                raise();
            }
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
        m_last_keypress_time = 0;
        return;
    }

    // otherwise, make a note that the user is typing
    m_last_keypress_time = FbTk::FbTime::mono();
}

bool FluxboxWindow::isTyping() const {

    uint64_t diff = FbTk::FbTime::mono() - m_last_keypress_time;
    return ((diff / 1000) < screen().noFocusWhileTypingDelay());
}

void FluxboxWindow::buttonPressEvent(XButtonEvent &be) {
    m_last_button_x = be.x_root;
    m_last_button_y = be.y_root;
    m_last_pressed_button = be.button;

    FbTk::Menu::hideShownMenu();

    Keys *k = Fluxbox::instance()->keys();
    int context = 0;
    context = frame().getContext(be.subwindow ? be.subwindow : be.window, be.x_root, be.y_root);
    if (!context && be.subwindow)
        context = frame().getContext(be.window);

    if (k->doAction(be.type, be.state, be.button, context, &winClient(), be.time)) {
        XAllowEvents(display, SyncPointer, CurrentTime);
        return;
    }

    WinClient *client = 0;
    if (!screen().focusControl().isMouseTabFocus()) {
        // determine if we're in a label button (tab)
        client = winClientOfLabelButtonWindow(be.window);
    }


    // - refeed the event into the queue so the app or titlebar subwindow gets it
    if (be.subwindow)
        XAllowEvents(display, ReplayPointer, CurrentTime);
    else
        XAllowEvents(display, SyncPointer, CurrentTime);

    // if nothing was bound via keys-file then
    // - raise() if clickRaise is enabled
    // - hide open menues
    // - focus on clickFocus
    if (frame().window().window() == be.window) {
        if (screen().clickRaises())
            raise();

        m_button_grab_x = be.x_root - frame().x() - frame().window().borderWidth();
        m_button_grab_y = be.y_root - frame().y() - frame().window().borderWidth();
    }

    if (!m_focused && acceptsFocus() && m_click_focus)
        focus();

    if (!screen().focusControl().isMouseTabFocus() &&
        client && client != m_client &&
        !screen().focusControl().isIgnored(be.x_root, be.y_root) ) {
        setCurrentClient(*client, isFocused());
    }



}

const unsigned int DEADZONE = 4;

void FluxboxWindow::buttonReleaseEvent(XButtonEvent &re) {

    if (m_last_pressed_button == static_cast<int>(re.button)) {
        m_last_pressed_button = 0;
    }

    if (isMoving())
        stopMoving();
    else if (isResizing())
        stopResizing();
    else if (m_attaching_tab)
        attachTo(re.x_root, re.y_root);
    else if (std::abs(m_last_button_x - re.x_root) + std::abs(m_last_button_y - re.y_root) < DEADZONE) {
        int context = 0;
        context = frame().getContext(re.subwindow ? re.subwindow : re.window,
                                     re.x_root, re.y_root);
        if (!context && re.subwindow)
            context = frame().getContext(re.window);

        Fluxbox::instance()->keys()->doAction(re.type, re.state, re.button,
                                              context, &winClient(), re.time);
    }
}


void FluxboxWindow::motionNotifyEvent(XMotionEvent &me) {

    if (isMoving() && me.window == parent()) {
        me.window = frame().window().window();
    }

    int context = frame().getContext(me.window,  me.x_root, me.y_root, m_last_button_x, m_last_button_y, true);

    if (Fluxbox::instance()->getIgnoreBorder() && m_attaching_tab == 0
        && !(isMoving() || isResizing())) {

        if (context & Keys::ON_WINDOWBORDER) {
            return;
        }
    }

    // in case someone put  MoveX :StartMoving etc into keys, we have
    // to activate it before doing the actual motionNotify code
    if (std::abs(m_last_button_x - me.x_root) + std::abs(m_last_button_y - me.y_root) >= DEADZONE) {
        XEvent &e = const_cast<XEvent&>(Fluxbox::instance()->lastEvent()); // is copy of "me"
        e.xmotion.x_root = m_last_button_x;
        e.xmotion.y_root = m_last_button_y;
        Fluxbox::instance()->keys()->doAction(me.type, me.state, m_last_pressed_button, context, &winClient(), me.time);
        e.xmotion.x = me.x_root;
        e.xmotion.y = me.y_root;
    }

    if (moving || m_attaching_tab) {

        XEvent e;

        if (XCheckTypedEvent(display, MotionNotify, &e)) {
            XPutBackEvent(display, &e);
            return;
        }

        const bool xor_outline = m_attaching_tab || !screen().doOpaqueMove();

        // Warp to next or previous workspace?, must have moved sideways some
        int moved_x = me.x_root - m_last_resize_x;

        // Warp to a workspace offset (if treating workspaces like a grid)
        int moved_y = me.y_root - m_last_resize_y;

        // save last event point
        m_last_resize_x = me.x_root;
        m_last_resize_y = me.y_root;

        // undraw rectangle before warping workspaces
        if (xor_outline) {
            int bw = static_cast<int>(frame().window().borderWidth());
            int w = static_cast<int>(frame().width()) + 2*bw -1;
            int h = static_cast<int>(frame().height()) + 2*bw - 1;
            if (w > 0 && h > 0) {
                parent().drawRectangle(screen().rootTheme()->opGC(),
                    m_last_move_x, m_last_move_y, w, h);
            }
        }


        // check for warping
        //
        // +--monitor-1--+--monitor-2---+
        // |w            |             w|
        // |w            |             w|
        // +-------------+--------------+
        //
        // mouse-warping is enabled, the mouse needs to be in the "warp_pad"
        // zone.
        //
        const int  warp_pad            = screen().getEdgeSnapThreshold();
        const int  workspaces          = screen().numberOfWorkspaces();
        const bool is_warping          = screen().isWorkspaceWarping();
        const bool is_warping_vertical = screen().isWorkspaceWarpingVertical();

        if ((moved_x || moved_y) && is_warping) {
            unsigned int cur_id = screen().currentWorkspaceID();
            unsigned int new_id = cur_id;

            // border threshold
            int bt_right  = int(screen().width()) - warp_pad - 1;
            int bt_left   = warp_pad;
            int bt_top    = int(screen().height()) - warp_pad - 1;
            int bt_bottom = warp_pad;

            if (moved_x) {
                if (me.x_root >= bt_right && moved_x > 0) { //warp right
                    new_id          = (cur_id + 1) % workspaces;
                    m_last_resize_x = 0;
                } else if (me.x_root <= bt_left && moved_x < 0) { //warp left
                    new_id          = (cur_id +  -1) % workspaces;
                    m_last_resize_x = screen().width() - 1;
                }
            }

            if (moved_y && is_warping_vertical) {

                const int warp_offset = screen().getWorkspaceWarpingVerticalOffset();

                if (me.y_root >= bt_top && moved_y > 0) { // warp down
                    new_id          = (cur_id + warp_offset) % workspaces;
                    m_last_resize_y = 0;
                } else if (me.y_root <= bt_bottom && moved_y < 0) { // warp up
                    new_id          = (cur_id + workspaces - warp_offset) % workspaces;
                    m_last_resize_y = screen().height() - 1;
                }
            }

            // if we are warping
            if (new_id != cur_id) {
                // remove motion events from queue to avoid repeated warps
                while (XCheckTypedEvent(display, MotionNotify, &e)) {
                    // might as well update the y-coordinate
                    m_last_resize_y = e.xmotion.y_root;
                }

                // move the pointer to (m_last_resize_x,m_last_resize_y)
                XWarpPointer(display, None, me.root, 0, 0, 0, 0,
                        m_last_resize_x, m_last_resize_y);

                if (m_attaching_tab || // tabbing grabs the pointer, we must not hide the window!
                            screen().doOpaqueMove())
                    screen().sendToWorkspace(new_id, this, true);
                else
                    screen().changeWorkspaceID(new_id, false);
            }
        }

        int dx = m_last_resize_x - m_button_grab_x,
            dy = m_last_resize_y - m_button_grab_y;

        dx -= frame().window().borderWidth();
        dy -= frame().window().borderWidth();

        // dx = current left side, dy = current top
        if (moving)
            doSnapping(dx, dy);

        // do not update display if another motion event is already pending

        if (xor_outline) {
            int bw = frame().window().borderWidth();
            int w = static_cast<int>(frame().width()) + 2*bw - 1;
            int h = static_cast<int>(frame().height()) + 2*bw - 1;
            if (w > 0 && h > 0) {
                parent().drawRectangle(screen().rootTheme()->opGC(), dx, dy, w, h);
            }
            m_last_move_x = dx;
            m_last_move_y = dy;
        } else {
            //moveResize(dx, dy, frame().width(), frame().height());
            // need to move the base window without interfering with transparency
            frame().quietMoveResize(dx, dy, frame().width(), frame().height());
        }
        if (moving)
            screen().showPosition(dx, dy);
        // end if moving
    } else if (resizing) {

        int old_resize_x = m_last_resize_x;
        int old_resize_y = m_last_resize_y;
        int old_resize_w = m_last_resize_w;
        int old_resize_h = m_last_resize_h;

        int dx = me.x - m_button_grab_x;
        int dy = me.y - m_button_grab_y;

        if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM ||
                m_resize_corner == LEFT) {
            m_last_resize_w = frame().width() - dx;
            m_last_resize_x = frame().x() + dx;
        }
        if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP ||
                m_resize_corner == TOP) {
            m_last_resize_h = frame().height() - dy;
            m_last_resize_y = frame().y() + dy;
        }
        if (m_resize_corner == LEFTBOTTOM || m_resize_corner == BOTTOM ||
                m_resize_corner == RIGHTBOTTOM)
            m_last_resize_h = frame().height() + dy;
        if (m_resize_corner == RIGHTBOTTOM || m_resize_corner == RIGHTTOP ||
                m_resize_corner == RIGHT)
            m_last_resize_w = frame().width() + dx;
        if (m_resize_corner == CENTER) {
            // dx or dy must be at least 2
            if (abs(dx) >= 2 || abs(dy) >= 2) {
                // take max and make it even
                int diff = 2 * (max(dx, dy) / 2);

                m_last_resize_h =  frame().height() + diff;

                m_last_resize_w = frame().width() + diff;
                m_last_resize_x = frame().x() - diff/2;
                m_last_resize_y = frame().y() - diff/2;
            }
        }

        fixSize();
        frame().displaySize(m_last_resize_w, m_last_resize_h);

        if (old_resize_x != m_last_resize_x ||
                old_resize_y != m_last_resize_y ||
                old_resize_w != m_last_resize_w ||
                old_resize_h != m_last_resize_h ) {

                if (screen().getEdgeResizeSnapThreshold() != 0) {
                    int tx, ty;
                    int botright_x = m_last_resize_x + m_last_resize_w;
                    int botright_y = m_last_resize_y + m_last_resize_h;

                    switch (m_resize_corner) {
                    case LEFTTOP:
                        tx = m_last_resize_x;
                        ty = m_last_resize_y;

                        doSnapping(tx, ty, true);

                        m_last_resize_x = tx;
                        m_last_resize_y = ty;

                        m_last_resize_w = botright_x - m_last_resize_x;
                        m_last_resize_h = botright_y - m_last_resize_y;

                        break;

                    case LEFTBOTTOM:
                        tx = m_last_resize_x;
                        ty = m_last_resize_y + m_last_resize_h;

                        ty += frame().window().borderWidth() * 2;

                        doSnapping(tx, ty, true);

                        ty -= frame().window().borderWidth() * 2;

                        m_last_resize_x = tx;
                        m_last_resize_h = ty - m_last_resize_y;

                        m_last_resize_w = botright_x - m_last_resize_x;

                        break;

                    case RIGHTTOP:
                        tx = m_last_resize_x + m_last_resize_w;
                        ty = m_last_resize_y;

                        tx += frame().window().borderWidth() * 2;

                        doSnapping(tx, ty, true);

                        tx -= frame().window().borderWidth() * 2;

                        m_last_resize_w = tx - m_last_resize_x;
                        m_last_resize_y = ty;

                        m_last_resize_h = botright_y - m_last_resize_y;

                        break;

                    case RIGHTBOTTOM:
                        tx = m_last_resize_x + m_last_resize_w;
                        ty = m_last_resize_y + m_last_resize_h;

                        tx += frame().window().borderWidth() * 2;
                        ty += frame().window().borderWidth() * 2;

                        doSnapping(tx, ty, true);

                        tx -= frame().window().borderWidth() * 2;
                        ty -= frame().window().borderWidth() * 2;

                        m_last_resize_w = tx - m_last_resize_x;
                        m_last_resize_h = ty - m_last_resize_y;

                        break;

                    default:
                        break;
                    }
                }

            // draw over old rect
            parent().drawRectangle(screen().rootTheme()->opGC(),
                    old_resize_x, old_resize_y,
                    old_resize_w - 1 + 2 * frame().window().borderWidth(),
                    old_resize_h - 1 + 2 * frame().window().borderWidth());

            // draw resize rectangle
            parent().drawRectangle(screen().rootTheme()->opGC(),
                    m_last_resize_x, m_last_resize_y,
                    m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                    m_last_resize_h - 1 + 2 * frame().window().borderWidth());

        }
    }
}

void FluxboxWindow::enterNotifyEvent(XCrossingEvent &ev) {

    static FluxboxWindow *s_last_really_entered = 0;

    if (ev.mode == NotifyUngrab && s_last_really_entered == this) {
        // if this results from an ungrab, only act if the window really changed.
        // otherwise we might pollute the focus which could have been assigned
        // by alt+tab (bug #597)
        return;
    }

    // ignore grab activates, or if we're not visible
    if (ev.mode == NotifyGrab || !isVisible()) {
        return;
    }

    s_last_really_entered = this;
    if (ev.window == frame().window())
        Fluxbox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                              Keys::ON_WINDOW, m_client);

    // determine if we're in a label button (tab)
    WinClient *client = winClientOfLabelButtonWindow(ev.window);
    if (client) {
        if (IconButton *tab = m_labelbuttons[client]) {
            m_has_tooltip = true;
            tab->showTooltip();
        }
    }

    if (ev.window == frame().window() ||
        ev.window == m_client->window() ||
        client) {

        if (m_mouse_focus && !isFocused() && acceptsFocus()) {

            // check that there aren't any subsequent leave notify events in the
            // X event queue
            XEvent dummy;
            scanargs sa;
            sa.w = ev.window;
            sa.enter = sa.leave = False;
            XCheckIfEvent(display, &dummy, queueScanner, (char *) &sa);

            if ((!sa.leave || sa.inferior) &&
                !screen().focusControl().isCycling() &&
                !screen().focusControl().isIgnored(ev.x_root, ev.y_root) ) {
                focus();
            }
        }
    }

    if (screen().focusControl().isMouseTabFocus() &&
        client && client != m_client &&
        !screen().focusControl().isIgnored(ev.x_root, ev.y_root) ) {
        m_tabActivationTimer.start();
    }

}

void FluxboxWindow::leaveNotifyEvent(XCrossingEvent &ev) {

    // ignore grab activates, or if we're not visible
    if (ev.mode == NotifyGrab || ev.mode == NotifyUngrab ||
        !isVisible()) {
        return;
    }

    if (m_has_tooltip) {
        m_has_tooltip = false;
        screen().hideTooltip();
    }

    // still inside?
    if (ev.x_root > frame().x() && ev.y_root > frame().y() &&
        ev.x_root <= (int)(frame().x() + frame().width()) &&
        ev.y_root <= (int)(frame().y() + frame().height()))
        return;

    Fluxbox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                          Keys::ON_WINDOW, m_client);

    // I hope commenting this out is right - simon 21jul2003
    //if (ev.window == frame().window())
    //installColormap(false);
}

void FluxboxWindow::setTitle(const std::string& title, Focusable &client) {
    // only update focus title for current client
    if (&client != m_client) {
        return;
    }

    frame().setFocusTitle(title);
    // relay title to others that display the focus title
    titleSig().emit(title, *this);
    m_reposLabels_timer.start();
}

void FluxboxWindow::emitLabelReposSig() {
    frame().tabcontainer().repositionItems();
}

void FluxboxWindow::frameExtentChanged() {
    if (m_initialized) {
        Fluxbox::instance()->updateFrameExtents(*this);
        sendConfigureNotify();
    }
}

void FluxboxWindow::themeReconfigured() {
    frame().applyDecorations();
    sendConfigureNotify();
}

void FluxboxWindow::workspaceAreaChanged(BScreen &screen) {
    frame().applyState();
}

// commit current decoration values to actual displayed things
void FluxboxWindow::applyDecorations() {
    frame().setDecorationMask(decorationMask());
    frame().applyDecorations();
}

void FluxboxWindow::toggleDecoration() {
    //don't toggle decor if the window is shaded
    if (isShaded() || isFullscreen())
        return;

    m_toggled_decos = !m_toggled_decos;

    if (m_toggled_decos) {
        m_old_decoration_mask = decorationMask();
        if (decorations.titlebar | decorations.tab)
            setDecorationMask(WindowState::DECOR_NONE);
        else
            setDecorationMask(WindowState::DECOR_NORMAL);
    } else //revert back to old decoration
        setDecorationMask(m_old_decoration_mask);

}

unsigned int FluxboxWindow::decorationMask() const {
    unsigned int ret = 0;
    if (decorations.titlebar)
        ret |= WindowState::DECORM_TITLEBAR;
    if (decorations.handle)
        ret |= WindowState::DECORM_HANDLE;
    if (decorations.border)
        ret |= WindowState::DECORM_BORDER;
    if (decorations.iconify)
        ret |= WindowState::DECORM_ICONIFY;
    if (decorations.maximize)
        ret |= WindowState::DECORM_MAXIMIZE;
    if (decorations.close)
        ret |= WindowState::DECORM_CLOSE;
    if (decorations.menu)
        ret |= WindowState::DECORM_MENU;
    if (decorations.sticky)
        ret |= WindowState::DECORM_STICKY;
    if (decorations.shade)
        ret |= WindowState::DECORM_SHADE;
    if (decorations.tab)
        ret |= WindowState::DECORM_TAB;
    if (decorations.enabled)
        ret |= WindowState::DECORM_ENABLED;
    return ret;
}

void FluxboxWindow::setDecorationMask(unsigned int mask, bool apply) {
    decorations.titlebar = mask & WindowState::DECORM_TITLEBAR;
    decorations.handle   = mask & WindowState::DECORM_HANDLE;
    decorations.border   = mask & WindowState::DECORM_BORDER;
    decorations.iconify  = mask & WindowState::DECORM_ICONIFY;
    decorations.maximize = mask & WindowState::DECORM_MAXIMIZE;
    decorations.close    = mask & WindowState::DECORM_CLOSE;
    decorations.menu     = mask & WindowState::DECORM_MENU;
    decorations.sticky   = mask & WindowState::DECORM_STICKY;
    decorations.shade    = mask & WindowState::DECORM_SHADE;
    decorations.tab      = mask & WindowState::DECORM_TAB;
    decorations.enabled  = mask & WindowState::DECORM_ENABLED;

    // we don't want to do this during initialization
    if (apply)
        applyDecorations();
}

void FluxboxWindow::startMoving(int x, int y) {

    if (isMoving()) {
        return;
    }

    if (s_num_grabs > 0) {
        return;
    }

    if ((isMaximized() || isFullscreen()) && screen().getMaxDisableMove())
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
    grabPointer(screen().rootWindow().window(), False, ButtonMotionMask |
                ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                screen().rootWindow().window(), frame().theme()->moveCursor(), CurrentTime);

    if (menu().isVisible())
        menu().hide();

    fluxbox->maskWindowEvents(screen().rootWindow().window(), this);

    m_last_move_x = frame().x();
    m_last_move_y = frame().y();
    if (! screen().doOpaqueMove()) {
        fluxbox->grab();
        parent().drawRectangle(screen().rootTheme()->opGC(),
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
        parent().drawRectangle(screen().rootTheme()->opGC(),
                               m_last_move_x, m_last_move_y,
                               frame().width() + 2*frame().window().borderWidth()-1,
                               frame().height() + 2*frame().window().borderWidth()-1);
        if (!interrupted) {
            moveResize(m_last_move_x, m_last_move_y, frame().width(), frame().height());
            if (m_workspace_number != screen().currentWorkspaceID())
                screen().sendToWorkspace(screen().currentWorkspaceID(), this);
            focus();
        }
        fluxbox->ungrab();
    } else if (!interrupted) {
        moveResize(frame().x(), frame().y(), frame().width(), frame().height(), true);
        frame().notifyMoved(true);
    }


    screen().hidePosition();
    ungrabPointer(CurrentTime);

    FbTk::App::instance()->sync(false); //make sure the redraw is made before we continue

    // if Head has been changed we want it to redraw by current state
    if (m_state.maximized || m_state.fullscreen) {
        frame().applyState();
        attachWorkAreaSig();
        stateSig().emit(*this);
    }
}

/**
 * Helper function that snaps a window to another window
 * We snap if we're closer than the x/ylimits.
 */
inline void snapToWindow(int &xlimit, int &ylimit,
                         int left, int right, int top, int bottom,
                         int oleft, int oright, int otop, int obottom,
                         bool resize) {
    // Only snap if we're adjacent to the edge we're looking at

    // for left + right, need to be in the right y range
    if (top <= obottom && bottom >= otop) {
        // left
        if (abs(left-oleft)  < abs(xlimit)) xlimit = -(left-oleft);
        if (abs(right-oleft) < abs(xlimit)) xlimit = -(right-oleft);

        // right
        if (abs(left-oright)  < abs(xlimit)) xlimit = -(left-oright);
        if (!resize && abs(right-oright) < abs(xlimit)) xlimit = -(right-oright);
    }

    // for top + bottom, need to be in the right x range
    if (left <= oright && right >= oleft) {
        // top
        if (abs(top-otop)    < abs(ylimit)) ylimit = -(top-otop);
        if (abs(bottom-otop) < abs(ylimit)) ylimit = -(bottom-otop);

        // bottom
        if (abs(top-obottom)    < abs(ylimit)) ylimit = -(top-obottom);
        if (!resize && abs(bottom-obottom) < abs(ylimit)) ylimit = -(bottom-obottom);
    }

}

/*
 * Do Whatever snapping magic is necessary, and return using the orig_left
 * and orig_top variables to indicate the new x,y position
 */
void FluxboxWindow::doSnapping(int &orig_left, int &orig_top, bool resize) {
    /*
     * Snap to screen/head edges
     * Snap to windows
     */
    int threshold;

    if (resize) {
        threshold = screen().getEdgeResizeSnapThreshold();
    } else {
        threshold = screen().getEdgeSnapThreshold();
    }

    if (0 == threshold) return;

    // Keep track of our best offsets so far
    // We need to find things less than or equal to the threshold
    int dx = threshold + 1;
    int dy = threshold + 1;

    // we only care about the left/top etc that includes borders
    int borderW = 0;

    if (decorationMask() & (WindowState::DECORM_BORDER|WindowState::DECORM_HANDLE))
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
                     screen().maxBottom(h),
                     resize);

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         screen().maxLeft(h),
                         screen().maxRight(h),
                         screen().maxTop(h),
                         screen().maxBottom(h),
                         resize);
    }
    for (int h=starth; h <= screen().numHeads(); h++) {
        snapToWindow(dx, dy, left, right, top, bottom,
                     screen().getHeadX(h),
                     screen().getHeadX(h) + screen().getHeadWidth(h),
                     screen().getHeadY(h),
                     screen().getHeadY(h) + screen().getHeadHeight(h),
                     resize);

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         screen().getHeadX(h),
                         screen().getHeadX(h) + screen().getHeadWidth(h),
                         screen().getHeadY(h),
                         screen().getHeadY(h) + screen().getHeadHeight(h),
                         resize);
    }

    /////////////////////////////////////
    // now check window edges

    Workspace::Windows &wins =
        screen().currentWorkspace()->windowList();

    Workspace::Windows::iterator it = wins.begin();
    Workspace::Windows::iterator it_end = wins.end();

    unsigned int bw;
    for (; it != it_end; ++it) {
        if ((*it) == this)
            continue; // skip myself

        bw = (*it)->decorationMask() & (WindowState::DECORM_BORDER|WindowState::DECORM_HANDLE) ?
                (*it)->frame().window().borderWidth() : 0;

        snapToWindow(dx, dy, left, right, top, bottom,
                     (*it)->x(),
                     (*it)->x() + (*it)->width() + 2 * bw,
                     (*it)->y(),
                     (*it)->y() + (*it)->height() + 2 * bw,
                     resize);

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         (*it)->x(),
                         (*it)->x() + (*it)->width() + 2 * bw,
                         (*it)->y(),
                         (*it)->y() + (*it)->height() + 2 * bw,
                         resize);

        // also snap to the box containing the tabs (don't bother with actual
        // tab edges, since they're dynamic
        if ((*it)->frame().externalTabMode())
            snapToWindow(dx, dy, left, right, top, bottom,
                         (*it)->x() - (*it)->xOffset(),
                         (*it)->x() - (*it)->xOffset() + (*it)->width() + 2 * bw + (*it)->widthOffset(),
                         (*it)->y() - (*it)->yOffset(),
                         (*it)->y() - (*it)->yOffset() + (*it)->height() + 2 * bw + (*it)->heightOffset(),
                         resize);

        if (i_have_tabs)
            snapToWindow(dx, dy, left - xoff, right - xoff + woff, top - yoff, bottom - yoff + hoff,
                         (*it)->x() - (*it)->xOffset(),
                         (*it)->x() - (*it)->xOffset() + (*it)->width() + 2 * bw + (*it)->widthOffset(),
                         (*it)->y() - (*it)->yOffset(),
                         (*it)->y() - (*it)->yOffset() + (*it)->height() + 2 * bw + (*it)->heightOffset(),
                         resize);

    }

    // commit
    if (dx <= threshold)
        orig_left += dx;
    if (dy <= threshold)
        orig_top  += dy;

}


FluxboxWindow::ReferenceCorner FluxboxWindow::getResizeDirection(int x, int y,
        ResizeModel model, int corner_size_px, int corner_size_pc) const
{
    if (model == TOPLEFTRESIZE)     return LEFTTOP;
    if (model == TOPRESIZE)         return TOP;
    if (model == TOPRIGHTRESIZE)    return RIGHTTOP;
    if (model == LEFTRESIZE)        return LEFT;
    if (model == RIGHTRESIZE)       return RIGHT;
    if (model == BOTTOMLEFTRESIZE)  return LEFTBOTTOM;
    if (model == BOTTOMRESIZE)      return BOTTOM;
    if (model == CENTERRESIZE)      return CENTER;

    if (model == EDGEORCORNERRESIZE)
    {
        int w = frame().width();
        int h = frame().height();
        int cx = w / 2;
        int cy = h / 2;
        TestCornerHelper test_corner = { corner_size_px, corner_size_pc };
        if (x < cx  &&  test_corner(x, cx)) {
            if (y < cy  &&  test_corner(y, cy))
                return LEFTTOP;
            else if (test_corner(h - y - 1, h - cy))
                return LEFTBOTTOM;
        } else if (test_corner(w - x - 1, w - cx)) {
            if (y < cy  &&  test_corner(y, cy))
                return RIGHTTOP;
            else if (test_corner(h - y - 1, h - cy))
                return RIGHTBOTTOM;
        }

        /* Nope, not a corner; find the nearest edge instead. */
        if (cy - abs(y - cy) < cx - abs(x - cx)) // y is nearest
            return (y > cy) ? BOTTOM : TOP;
        else
            return (x > cx) ? RIGHT : LEFT;
    }
    return RIGHTBOTTOM;
}

void FluxboxWindow::startResizing(int x, int y, ReferenceCorner dir) {

    if (isResizing())
        return;

    if (s_num_grabs > 0 || isShaded() || isIconic() )
        return;

    if ((isMaximized() || isFullscreen()) && screen().getMaxDisableResize())
        return;

    m_resize_corner = dir;

    resizing = true;

    disableMaximization();

    const Cursor& cursor = (m_resize_corner == LEFTTOP) ? frame().theme()->upperLeftAngleCursor() :
                           (m_resize_corner == RIGHTTOP) ? frame().theme()->upperRightAngleCursor() :
                           (m_resize_corner == RIGHTBOTTOM) ? frame().theme()->lowerRightAngleCursor() :
                           (m_resize_corner == LEFT) ? frame().theme()->leftSideCursor() :
                           (m_resize_corner == RIGHT) ? frame().theme()->rightSideCursor() :
                           (m_resize_corner == TOP) ? frame().theme()->topSideCursor() :
                           (m_resize_corner == BOTTOM) ? frame().theme()->bottomSideCursor() :
                                                            frame().theme()->lowerLeftAngleCursor();

    grabPointer(fbWindow().window(),
                false, ButtonMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);

    m_button_grab_x = x;
    m_button_grab_y = y;
    m_last_resize_x = frame().x();
    m_last_resize_y = frame().y();
    m_last_resize_w = frame().width();
    m_last_resize_h = frame().height();

    fixSize();
    frame().displaySize(m_last_resize_w, m_last_resize_h);

    parent().drawRectangle(screen().rootTheme()->opGC(),
                       m_last_resize_x, m_last_resize_y,
                       m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                       m_last_resize_h - 1 + 2 * frame().window().borderWidth());
}

void FluxboxWindow::stopResizing(bool interrupted) {
    resizing = false;

    parent().drawRectangle(screen().rootTheme()->opGC(),
                           m_last_resize_x, m_last_resize_y,
                           m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                           m_last_resize_h - 1 + 2 * frame().window().borderWidth());

    screen().hideGeometry();

    if (!interrupted) {
        fixSize();

        moveResize(m_last_resize_x, m_last_resize_y,
                   m_last_resize_w, m_last_resize_h);
    }

    ungrabPointer(CurrentTime);
}

WinClient* FluxboxWindow::winClientOfLabelButtonWindow(Window window) {
    WinClient* result = 0;
    Client2ButtonMap::iterator it =
        find_if(m_labelbuttons.begin(),
                m_labelbuttons.end(),
                Compose(bind2nd(equal_to<Window>(), window),
                        Compose(mem_fun(&FbTk::Button::window),
                                Select2nd<Client2ButtonMap::value_type>())));
    if (it != m_labelbuttons.end())
        result = it->first;

    return result;
}

void FluxboxWindow::startTabbing(const XButtonEvent &be) {

    if (s_num_grabs > 0)
        return;

    s_original_workspace = workspaceNumber();
    m_attaching_tab = winClientOfLabelButtonWindow(be.window);

    // start drag'n'drop for tab
    grabPointer(be.window, False, ButtonMotionMask |
                ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                None, frame().theme()->moveCursor(), CurrentTime);

    // relative position on the button
    m_button_grab_x = be.x;
    m_button_grab_y = be.y;
    // position of the button
    m_last_move_x = be.x_root - be.x;
    m_last_move_y = be.y_root - be.y;
    // hijack extra vars for initial grab location
    m_last_resize_x = be.x_root;
    m_last_resize_y = be.y_root;

    Fluxbox::instance()->grab();

    if (m_attaching_tab) {
        FbTk::TextButton &active_button = *m_labelbuttons[m_attaching_tab];
        m_last_resize_w = active_button.width();
        m_last_resize_h = active_button.height();
    } else {
        m_attaching_tab = m_client;
        unsigned int bw = 2*frame().window().borderWidth()-1;
        m_last_resize_w = frame().width() + bw;
        m_last_resize_h = frame().height() + bw;
    }

    parent().drawRectangle(screen().rootTheme()->opGC(),
                           m_last_move_x, m_last_move_y,
                           m_last_resize_w, m_last_resize_h);

    menu().hide();
}

void FluxboxWindow::attachTo(int x, int y, bool interrupted) {
    if (m_attaching_tab == 0)
        return;

    parent().drawRectangle(screen().rootTheme()->opGC(),
                           m_last_move_x, m_last_move_y,
                           m_last_resize_w, m_last_resize_h);

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
            screen().sendToWorkspace(s_original_workspace, this, false);
            if (FluxboxWindow *fbwin = client.fbwindow())
                fbwin->move(m_last_move_x, m_last_move_y);
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

        fbdbg<<"FluxboxWindow::restore: reparent 0x"<<hex<<client->window()<<dec<<" to root"<<endl;

        // reparent to root window
        client->reparent(screen().rootWindow(), wx, wy, false);

        if (!remap)
            client->hide();
    }

    if (remap)
        client->show();

    installColormap(false);

    delete client;


    fbdbg<<"FluxboxWindow::restore: remap = "<<remap<<endl;
    fbdbg<<"("<<__FUNCTION__<<"): numClients() = "<<numClients()<<endl;

    if (numClients() == 0)
        delete this;

}

void FluxboxWindow::restore(bool remap) {
    if (numClients() == 0)
        return;

    fbdbg<<"restore("<<remap<<")"<<endl;

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

FbMenu &FluxboxWindow::menu() {
    return screen().windowMenu();
}

bool FluxboxWindow::acceptsFocus() const {
    return (m_client ? m_client->acceptsFocus() : false);
}

bool FluxboxWindow::isModal() const {
    return (m_client ? m_client->isModal() : true);
}

const FbTk::PixmapWithMask &FluxboxWindow::icon() const {
    return (m_client ? m_client->icon() : m_icon);
}

const FbMenu &FluxboxWindow::menu() const {
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


const FbTk::BiDiString& FluxboxWindow::title() const {
    return (m_client ? m_client->title() : m_title);
}

const FbTk::FbString& FluxboxWindow::getWMClassName() const {
    return (m_client ? m_client->getWMClassName() : getWMClassName());
}

const FbTk::FbString& FluxboxWindow::getWMClassClass() const {
    return (m_client ? m_client->getWMClassClass() : getWMClassClass());
}

FbTk::FbString FluxboxWindow::getWMRole() const {
    return (m_client ? m_client->getWMRole() : "FluxboxWindow");
}

long FluxboxWindow::getCardinalProperty(Atom prop,bool*exists) const {
    return (m_client ? m_client->getCardinalProperty(prop,exists) : Focusable::getCardinalProperty(prop,exists));
}

FbTk::FbString FluxboxWindow::getTextProperty(Atom prop,bool*exists) const {
    return (m_client ? m_client->getTextProperty(prop,exists) : Focusable::getTextProperty(prop,exists));
}

bool FluxboxWindow::isTransient() const {
    return (m_client && m_client->isTransient());
}

int FluxboxWindow::initialState() const { return m_client->initial_state; }

void FluxboxWindow::fixSize() {

    // m_last_resize_w / m_last_resize_h could be negative
    // due to user interactions. check here and limit
    unsigned int w = 1;
    unsigned int h = 1;
    if (m_last_resize_w > 0)
        w = m_last_resize_w;
    if (m_last_resize_h > 0)
        h = m_last_resize_h;

    frame().applySizeHints(w, h);

    m_last_resize_w = w;
    m_last_resize_h = h;

    // move X if necessary
    if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM ||
        m_resize_corner == LEFT) {
        m_last_resize_x = frame().x() + frame().width() - m_last_resize_w;
    }

    if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP ||
        m_resize_corner == TOP) {
        m_last_resize_y = frame().y() + frame().height() - m_last_resize_h;
    }
}

void FluxboxWindow::moveResizeClient(WinClient &client) {
    client.moveResize(frame().clientArea().x(), frame().clientArea().y(),
                      frame().clientArea().width(),
                      frame().clientArea().height());
    client.sendConfigureNotify(frame().x() + frame().clientArea().x() +
                                             frame().window().borderWidth(),
                               frame().y() + frame().clientArea().y() +
                                             frame().window().borderWidth(),
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
        moveResizeClient(client);

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
    FbTk::ResourceManager &rm = screen().resourceManager();

    struct {
        std::string name;
        std::string alt_name;
        size_t n_buttons;
        WinButton::Type buttons[3];
    } side[2] = {
        {
            screen().name() + ".titlebar.left",
            screen().name() + ".Titlebar.Left",
            1, { WinButton::STICK },
        },
        {
            screen().name() + ".titlebar.right",
            screen().name() + ".Titlebar.Right",
            3, { WinButton::MINIMIZE, WinButton::MAXIMIZE, WinButton::CLOSE },
        }
    };


    // create resource for titlebar
    for (size_t i = 0; i < sizeof(side)/sizeof(side[0]); ++i) {

        WBR* res = dynamic_cast<WBR*>(rm.findResource(side[i].name));
        if (res != 0)
            continue; // find next resource too

        WinButton::Type* s = &side[i].buttons[0];
        WinButton::Type* e = s + side[i].n_buttons;
        res = new WBR(rm, WBR::Type(s, e), side[i].name, side[i].alt_name);
        screen().addManagedResource(res);
    }

    updateButtons();
}


void FluxboxWindow::updateButtons() {

    ResourceManager &rm = screen().resourceManager();
    size_t i;
    size_t j;
    struct {
        std::string name;
        WBR* res;
    } sides[2] = {
        { screen().name() + ".titlebar.left", 0 },
        { screen().name() + ".titlebar.right", 0 },
    };
    const size_t n_sides = sizeof(sides)/sizeof(sides[0]);
    bool need_update = false;


    // get button resources for each titlebar and check if they differ
    for (i = 0; i < n_sides; ++i) {

        sides[i].res = dynamic_cast<WBR*>(rm.findResource(sides[i].name));

        if (sides[i].res == 0) {
            if (!m_titlebar_buttons[i].empty()) {
                need_update = true;
            }
            continue;
        }

        // check if we need to update our buttons
        const vector<WinButton::Type>& buttons = *(*sides[i].res);
        size_t s = buttons.size();

        if (s != m_titlebar_buttons[i].size()) {
            need_update = true;
            continue;
        }

        for (j = 0; !need_update && j < s; j++) {
            if (buttons[j] != m_titlebar_buttons[i][j]) {
                need_update = true;
                break;
            }
        }
    }

    if (!need_update)
        return;

    frame().removeAllButtons();

    using namespace FbTk;
    typedef RefCount<Command<void> > CommandRef;
    typedef SimpleCommand<FluxboxWindow> WindowCmd;

    CommandRef iconify_cmd(new WindowCmd(*this, &FluxboxWindow::iconify));
    CommandRef maximize_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeFull));
    CommandRef maximize_vert_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeVertical));
    CommandRef maximize_horiz_cmd(new WindowCmd(*this, &FluxboxWindow::maximizeHorizontal));
    CommandRef close_cmd(new WindowCmd(*this, &FluxboxWindow::close));
    CommandRef shade_cmd(new WindowCmd(*this, &FluxboxWindow::shade));
    CommandRef stick_cmd(new WindowCmd(*this, &FluxboxWindow::stick));
    CommandRef show_menu_cmd(new WindowCmd(*this, &FluxboxWindow::popupMenu));

    for (i = 0; i < n_sides; i++) {

        if (sides[i].res == 0) {
            continue;
        }

        const vector<WinButton::Type>& buttons = *(*sides[i].res);
        m_titlebar_buttons[i] = buttons;

        for (j = 0; j < buttons.size(); ++j) {

            WinButton* btn = 0;

            switch (buttons[j]) {
            case WinButton::MINIMIZE:
                if (isIconifiable() && (m_state.deco_mask & WindowState::DECORM_ICONIFY)) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->setOnClick(iconify_cmd);
                }
                break;
            case WinButton::MAXIMIZE:
                if (isMaximizable() && (m_state.deco_mask & WindowState::DECORM_MAXIMIZE) ) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->setOnClick(maximize_cmd, 1);
                    btn->setOnClick(maximize_horiz_cmd, 3);
                    btn->setOnClick(maximize_vert_cmd, 2);
                }
                break;
            case WinButton::CLOSE:
                if (m_client->isClosable() && (m_state.deco_mask & WindowState::DECORM_CLOSE)) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->setOnClick(close_cmd);
                    btn->join(stateSig(), FbTk::MemFunIgnoreArgs(*btn, &WinButton::updateAll));
                }
                break;
            case WinButton::STICK:
                if (m_state.deco_mask & WindowState::DECORM_STICKY) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->join(stateSig(), FbTk::MemFunIgnoreArgs(*btn, &WinButton::updateAll));
                    btn->setOnClick(stick_cmd);
                }
                break;
            case WinButton::SHADE:
                if (m_state.deco_mask & WindowState::DECORM_SHADE) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->join(stateSig(), FbTk::MemFunIgnoreArgs(*btn, &WinButton::updateAll));
                    btn->setOnClick(shade_cmd);
                }
                break;
            case WinButton::MENUICON:
                if (m_state.deco_mask & WindowState::DECORM_MENU) {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    btn->join(titleSig(), FbTk::MemFunIgnoreArgs(*btn, &WinButton::updateAll));
                    btn->setOnClick(show_menu_cmd);
                }
                break;

            case WinButton::LEFT_HALF:
                {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    CommandRef lhalf_cmd(FbTk::CommandParser<void>::instance().parse("MacroCmd {MoveTo 0 0} {ResizeTo 50% 100%}"));
                    btn->setOnClick(lhalf_cmd);
                }
                break;

            case WinButton::RIGHT_HALF:
                {
                    btn = makeButton(*this, m_button_theme, buttons[j]);
                    CommandRef rhalf_cmd(FbTk::CommandParser<void>::instance().parse("MacroCmd {MoveTo 50% 0} {ResizeTo 50% 100%}"));
                    btn->setOnClick(rhalf_cmd);
                }
                break;

            }


            if (btn != 0) {
                btn->show();
                if (i == 0)
                    frame().addLeftButton(btn);
                else
                    frame().addRightButton(btn);
            }
        }
    }

    frame().reconfigure();
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
    IconButton *btn = new IconButton(frame().tabcontainer(),
            frame().theme().focusedTheme()->iconbarTheme(),
            frame().theme().unfocusedTheme()->iconbarTheme(), client);
    frame().createTab(*btn);

    btn->setTextPadding(Fluxbox::instance()->getTabsPadding());
    btn->setPixmap(screen().getTabsUsePixmap());

    m_labelbuttons[&client] = btn;

    FbTk::EventManager &evm = *FbTk::EventManager::instance();

    evm.add(*this, btn->window()); // we take care of button events for this
    evm.add(*this, client.window());

    client.setFluxboxWindow(this);
    join(client.titleSig(),
         FbTk::MemFun(*this, &FluxboxWindow::setTitle));
}

FluxboxWindow::ReferenceCorner FluxboxWindow::getCorner(string str) {
    str = FbTk::StringUtil::toLower(str);
    if (str == "lefttop" || str == "topleft" || str == "upperleft" || str == "")
        return LEFTTOP;
    if (str == "top" || str == "upper" || str == "topcenter")
        return TOP;
    if (str == "righttop" || str == "topright" || str == "upperright")
        return RIGHTTOP;
    if (str == "left" || str == "leftcenter")
        return LEFT;
    if (str == "center" || str == "wincenter")
        return CENTER;
    if (str == "right" || str == "rightcenter")
        return RIGHT;
    if (str == "leftbottom" || str == "bottomleft" || str == "lowerleft")
        return LEFTBOTTOM;
    if (str == "bottom" || str == "lower" || str == "bottomcenter")
        return BOTTOM;
    if (str == "rightbottom" || str == "bottomright" || str == "lowerright")
        return RIGHTBOTTOM;
    return ERROR;
}

void FluxboxWindow::translateXCoords(int &x, ReferenceCorner dir) const {
    int head = getOnHead(), bw = 2 * frame().window().borderWidth(),
        left = screen().maxLeft(head), right = screen().maxRight(head);
    int w = width();

    if (dir == LEFTTOP || dir == LEFT || dir == LEFTBOTTOM)
        x += left;
    if (dir == RIGHTTOP || dir == RIGHT || dir == RIGHTBOTTOM)
        x = right - w - bw - x;
    if (dir == TOP || dir == CENTER || dir == BOTTOM)
        x += (left + right - w - bw)/2;
}

void FluxboxWindow::translateYCoords(int &y, ReferenceCorner dir) const {
    int head = getOnHead(), bw = 2 * frame().window().borderWidth(),
        top = screen().maxTop(head), bottom = screen().maxBottom(head);
    int h = height();

    if (dir == LEFTTOP || dir == TOP || dir == RIGHTTOP)
        y += top;
    if (dir == LEFTBOTTOM || dir == BOTTOM || dir == RIGHTBOTTOM)
        y = bottom - h - bw - y;
    if (dir == LEFT || dir == CENTER || dir == RIGHT)
        y += (top + bottom - h - bw)/2;
}

void FluxboxWindow::translateCoords(int &x, int &y, ReferenceCorner dir) const {
  translateXCoords(x, dir);
  translateYCoords(y, dir);
}

int FluxboxWindow::getOnHead() const {
    return screen().getHead(fbWindow());
}

void FluxboxWindow::setOnHead(int head) {
    if (head > 0 && head <= screen().numHeads()) {
        int cur = screen().getHead(fbWindow());
        bool placed = m_placed;
        int x = frame().x(), y = frame().y();
        const int w = frame().width(), h = frame().height(), bw = frame().window().borderWidth();
        const int sx = screen().getHeadX(cur), sw = screen().getHeadWidth(cur),
                  sy = screen().getHeadY(cur), sh = screen().getHeadHeight(cur);
        int d = sx + sw - (x + bw + w);
        if (std::abs(sx - x) > bw && std::abs(d) <= bw) // right aligned
            x = screen().getHeadX(head) + screen().getHeadWidth(head) - (w + bw + d);
        else // calc top-left relative position
            x = screen().getHeadWidth(head) * (x - sx) / sw + screen().getHeadX(head);
        d = sy + sh - (y + bw + h);
        if (std::abs(sy - y) > bw && std::abs(d) <= bw) // bottom aligned
            y = screen().getHeadY(head) + screen().getHeadHeight(head) - (h + bw + d);
        else // calc top-left relative position
            y = screen().getHeadHeight(head) * (y - sy) / sh + screen().getHeadY(head);
        move(x, y);
        m_placed = placed;
    }

    // if Head has been changed we want it to redraw by current state
    if (m_state.maximized || m_state.fullscreen) {
        frame().applyState();
        attachWorkAreaSig();
        stateSig().emit(*this);
    }
}

void FluxboxWindow::placeWindow(int head) {
    int new_x, new_y;
    // we ignore the return value,
    // the screen placement strategy is guaranteed to succeed.
    screen().placementStrategy().placeWindow(*this, head, new_x, new_y);
    m_state.saveGeometry(new_x, new_y, frame().width(), frame().height(), true);
    move(new_x, new_y);
}

void FluxboxWindow::setWindowType(WindowState::WindowType type) {
    m_state.type = type;
    switch (type) {
    case WindowState::TYPE_DOCK:
        /* From Extended Window Manager Hints, draft 1.3:
         *
         * _NET_WM_WINDOW_TYPE_DOCK indicates a dock or panel feature.
         * Typically a Window Manager would keep such windows on top
         * of all other windows.
         *
         */
        setFocusHidden(true);
        setIconHidden(true);
        setFocusNew(false);
        setMouseFocus(false);
        setClickFocus(false);
        setDecorationMask(WindowState::DECOR_NONE);
        moveToLayer(::ResourceLayer::DOCK);
        break;
    case WindowState::TYPE_DESKTOP:
        /*
         * _NET_WM_WINDOW_TYPE_DESKTOP indicates a "false desktop" window
         * We let it be the size it wants, but it gets no decoration,
         * is hidden in the toolbar and window cycling list, plus
         * windows don't tab with it and is right on the bottom.
         */
        setFocusHidden(true);
        setIconHidden(true);
        setFocusNew(false);
        setMouseFocus(false);
        moveToLayer(::ResourceLayer::DESKTOP);
        setDecorationMask(WindowState::DECOR_NONE);
        setTabable(false);
        setMovable(false);
        setResizable(false);
        setStuck(true);
        break;
    case WindowState::TYPE_SPLASH:
        /*
         * _NET_WM_WINDOW_TYPE_SPLASH indicates that the
         * window is a splash screen displayed as an application
         * is starting up.
         */
        setDecorationMask(WindowState::DECOR_NONE);
        setFocusHidden(true);
        setIconHidden(true);
        setFocusNew(false);
        setMouseFocus(false);
        setClickFocus(false);
        setMovable(false);
        break;
    case WindowState::TYPE_DIALOG:
        setTabable(false);
        break;
    case WindowState::TYPE_MENU:
    case WindowState::TYPE_TOOLBAR:
        /*
         * _NET_WM_WINDOW_TYPE_TOOLBAR and _NET_WM_WINDOW_TYPE_MENU
         * indicate toolbar and pinnable menu windows, respectively
         * (i.e. toolbars and menus "torn off" from the main
         * application). Windows of this type may set the
         * WM_TRANSIENT_FOR hint indicating the main application window.
         */
        setDecorationMask(WindowState::DECOR_TOOL);
        setIconHidden(true);
        moveToLayer(::ResourceLayer::ABOVE_DOCK);
        break;
    case WindowState::TYPE_NORMAL:
    default:
        break;
    }

    /*
     * NOT YET IMPLEMENTED:
     *   _NET_WM_WINDOW_TYPE_UTILITY
     */
}

void FluxboxWindow::focusedWindowChanged(BScreen &screen,
                                         FluxboxWindow *focused_win, WinClient* client) {
    if (focused_win) {
        setFullscreenLayer();
    }
}
