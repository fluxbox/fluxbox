// SystemTray.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "SystemTray.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/TextUtils.hh"
#include "FbTk/MemFun.hh"

#include "AtomHandler.hh"
#include "fluxbox.hh"
#include "WinClient.hh"
#include "Screen.hh"
#include "ButtonTheme.hh"
#include "Debug.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>


using std::string;
using std::endl;
using std::hex;
using std::dec;


namespace {

void getScreenCoordinates(Window win, int x, int y, int &screen_x, int &screen_y) {

    XWindowAttributes attr;
    if (XGetWindowAttributes(FbTk::App::instance()->display(), win, &attr) == 0) {
        return;
    }

    Window unused_win;
    Window parent_win;
    Window root_win = 0;
    Window* unused_childs = 0;
    unsigned int unused_number;

    XQueryTree(FbTk::App::instance()->display(), win,
               &root_win,
               &parent_win,
               &unused_childs, &unused_number);

    if (unused_childs != 0) {
        XFree(unused_childs);
    }

    XTranslateCoordinates(FbTk::App::instance()->display(),
                          parent_win, root_win,
                          x, y,
                          &screen_x, &screen_y, &unused_win);
}

};

static SystemTray *s_theoneandonly = 0;

/// helper class for tray windows, so we dont call XDestroyWindow
class SystemTray::TrayWindow : public FbTk::FbWindow {
public:
    TrayWindow(Window win, bool using_xembed):FbTk::FbWindow(win), m_visible(false), m_xembedded(using_xembed) {
        setEventMask(PropertyChangeMask);
    }

    bool isVisible() { return m_visible; }
    bool isXEmbedded() { return m_xembedded; }
    void show() {
        if (!m_visible) {
            m_visible = true;
            FbTk::FbWindow::show();
        }
    }
    void hide() {
        if (m_visible) {
            m_visible = false;
            FbTk::FbWindow::hide();
        }
    }

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                   (1 << 0)

    bool getMappedDefault() const {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned long *prop;
        Atom embed_info = SystemTray::getXEmbedInfoAtom();
        if (property(embed_info, 0l, 2l, false, embed_info,
                     &actual_type, &actual_format, &nitems, &bytes_after,
                     (unsigned char **) &prop) && prop != 0) {

            XFree(static_cast<void *>(prop));
            fbdbg << "(SystemTray::TrayWindow::getMappedDefault(): XEMBED_MAPPED = "
                << (bool)(static_cast<unsigned long>(prop[1]) & XEMBED_MAPPED)
                << endl;
        }
        return true;
    }

private:
    bool m_visible;
    bool m_xembedded; // using xembed protocol? (i.e. unmap when done)
};

/// handles clientmessage event and notifies systemtray
class SystemTrayHandler: public AtomHandler {
public:
    SystemTrayHandler(SystemTray &tray):m_tray(tray) {
    }
    // client message is the only thing we care about
    bool checkClientMessage(const XClientMessageEvent &ce,
                            BScreen * screen, WinClient * const winclient) {
        // must be on the same screen
        if ((screen && screen->screenNumber() != m_tray.window().screenNumber()) ||
            (winclient && winclient->screenNumber() != m_tray.window().screenNumber()) )
            return false;
        return m_tray.clientMessage(ce);
    }

    void initForScreen(BScreen &screen) { };
    void setupFrame(FluxboxWindow &win) { };
    void setupClient(WinClient &winclient) {
        // must be on the same screen
        if (winclient.screenNumber() != m_tray.window().screenNumber())
            return;

        // we dont want a managed window
        if (winclient.fbwindow() != 0)
            return;
        // if not kde dockapp...
        if (!winclient.screen().isKdeDockapp(winclient.window()))
            return;
        // if not our screen...
        if (winclient.screenNumber() != m_tray.window().screenNumber())
            return;
        winclient.setEventMask(StructureNotifyMask |
                               SubstructureNotifyMask | EnterWindowMask);
        m_tray.addClient(winclient.window(), false);
    };

    void updateWorkarea(BScreen &) { }
    void updateFocusedWindow(BScreen &, Window) { }
    void updateClientList(BScreen &screen) { };
    void updateWorkspaceNames(BScreen &screen) { };
    void updateCurrentWorkspace(BScreen &screen) { };
    void updateWorkspaceCount(BScreen &screen) { };

    void updateFrameClose(FluxboxWindow &win) { };
    void updateClientClose(WinClient &winclient) { };
    void updateWorkspace(FluxboxWindow &win) { };
    void updateState(FluxboxWindow &win) { };
    void updateHints(FluxboxWindow &win) { };
    void updateLayer(FluxboxWindow &win) { };

    virtual bool propertyNotify(WinClient &winclient, Atom the_property) { return false; }

private:
    SystemTray &m_tray;
};

SystemTray::SystemTray(const FbTk::FbWindow& parent,
        FbTk::ThemeProxy<ToolTheme> &theme, BScreen& screen):
    ToolbarItem(ToolbarItem::FIXED),
    m_window(parent, 0, 0, 1, 1, ExposureMask | ButtonPressMask | ButtonReleaseMask |
             SubstructureNotifyMask | SubstructureRedirectMask),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0), m_num_visible_clients(0),
    m_selection_owner(m_window, 0, 0, 1, 1, SubstructureNotifyMask, false, false, CopyFromParent, InputOnly),
    m_rc_systray_pinleft(screen.resourceManager(),
            "", screen.name() + ".systray.pinLeft",
            screen.altName() + ".Systray.PinLeft"),
    m_rc_systray_pinright(screen.resourceManager(),
            "", screen.name() + ".systray.pinRight",
            screen.altName() + ".Systray.PinRight") {

    FbTk::EventManager::instance()->add(*this, m_window);
    FbTk::EventManager::instance()->add(*this, m_selection_owner);
    // setup signals
    join(m_theme->reconfigSig(), FbTk::MemFun(*this, &SystemTray::update));

    join(screen.bgChangeSig(),
         FbTk::MemFunIgnoreArgs(*this, &SystemTray::update));


    Fluxbox* fluxbox = Fluxbox::instance();
    Display *disp = fluxbox->display();

    // get selection owner and see if it's free
    string atom_name = getNetSystemTrayAtom(m_window.screenNumber());
    Atom tray_atom = XInternAtom(disp, atom_name.c_str(), False);
    Window owner = XGetSelectionOwner(disp, tray_atom);
    if (owner != 0) {
        fbdbg<<"(SystemTray(const FbTk::FbWindow)): can't set owner!"<<endl;
        return;  // the're can't be more than one owner
    }

    // ok, it was free. Lets set owner

    fbdbg<<"(SystemTray(const FbTk::FbWindow)): SETTING OWNER!"<<endl;

    // set owner
    XSetSelectionOwner(disp, tray_atom, m_selection_owner.window(), CurrentTime);

    s_theoneandonly = this;

    m_handler.reset(new SystemTrayHandler(*this));

    m_handler.get()->setName(atom_name);
    fluxbox->addAtomHandler(m_handler.get());


    // send selection owner msg
    Window root_window = m_screen.rootWindow().window();
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = XInternAtom(disp, "MANAGER", False);
    ce.xclient.display = disp;
    ce.xclient.window = root_window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = CurrentTime; // timestamp
    ce.xclient.data.l[1] = tray_atom; // manager selection atom
    ce.xclient.data.l[2] = m_selection_owner.window(); // the window owning the selection
    ce.xclient.data.l[3] = 0l; // selection specific data
    ce.xclient.data.l[4] = 0l; // selection specific data

    XSendEvent(disp, root_window, false, StructureNotifyMask, &ce);

    update();
}

SystemTray::~SystemTray() {
    // remove us, else fluxbox might delete the memory too
    if (s_theoneandonly == this)
        s_theoneandonly = 0;
    Fluxbox* fluxbox = Fluxbox::instance();
    fluxbox->removeAtomHandler(m_handler.get());
    Display *disp = fluxbox->display();

    // get selection owner and see if it's free
    string atom_name = getNetSystemTrayAtom(m_window.screenNumber());
    Atom tray_atom = XInternAtom(disp, atom_name.c_str(), False);

    // Properly give up selection.
    XSetSelectionOwner(disp, tray_atom, None, CurrentTime);
    removeAllClients();

    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);

    // ~FbWindow cleans EventManager
}

void SystemTray::move(int x, int y) {
    m_window.move(x, y);
}

void SystemTray::resize(unsigned int width, unsigned int height) {
    if (width != m_window.width() ||
        height != m_window.height()) {
        m_window.resize(width, height);
        if (m_num_visible_clients)
            rearrangeClients();
        resizeSig().emit();
    }
}

void SystemTray::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {
    if (width != m_window.width() ||
        height != m_window.height()) {
        m_window.moveResize(x, y, width, height);
        if (m_num_visible_clients)
            rearrangeClients();
        resizeSig().emit();
    } else {
        move(x, y);
    }
}

void SystemTray::hide() {
    m_window.hide();
}

void SystemTray::show() {

    update();
    m_window.show();
}

unsigned int SystemTray::width() const {
    if (orientation() == FbTk::ROT90 || orientation() == FbTk::ROT270)
        return m_window.width();

    return m_num_visible_clients * (height() + 2 * m_theme->border().width());
}

unsigned int SystemTray::height() const {
    if (orientation() == FbTk::ROT0 || orientation() == FbTk::ROT180)
        return m_window.height();

    return m_num_visible_clients * (width() + 2 * m_theme->border().width());
}

unsigned int SystemTray::borderWidth() const {
    return m_window.borderWidth();
}

bool SystemTray::clientMessage(const XClientMessageEvent &event) {
    static const int SYSTEM_TRAY_REQUEST_DOCK  =  0;
    //    static const int SYSTEM_TRAY_BEGIN_MESSAGE =  1;
    //    static const int SYSTEM_TRAY_CANCEL_MESSAGE = 2;
    static Atom systray_opcode_atom = XInternAtom(FbTk::App::instance()->display(), "_NET_SYSTEM_TRAY_OPCODE", False);

    if (event.message_type == systray_opcode_atom) {

        int type = event.data.l[1];
        if (type == SYSTEM_TRAY_REQUEST_DOCK) {

            fbdbg<<"SystemTray::clientMessage(const XClientMessageEvent): SYSTEM_TRAY_REQUEST_DOCK"<<endl;
            fbdbg<<"window = event.data.l[2] = "<<event.data.l[2]<<endl;

            addClient(event.data.l[2], true);
        }
        /*
        else if (type == SYSTEM_TRAY_BEGIN_MESSAGE)
            fbdbg<<"BEGIN MESSAGE"<<endl;
        else if (type == SYSTEM_TRAY_CANCEL_MESSAGE)
            fbdbg<<"CANCEL MESSAGE"<<endl;
        */

        return true;
    }

    return false;
}

SystemTray::ClientList::iterator SystemTray::findClient(Window win) {

    ClientList::iterator it = m_clients.begin();
    ClientList::iterator it_end = m_clients.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == win)
            break;
    }

    return it;
}

void SystemTray::addClient(Window win, bool using_xembed) {
    if (win == 0)
        return;

    ClientList::iterator it = findClient(win);
    if (it != m_clients.end())
        return;

    Display *disp = Fluxbox::instance()->display();
    // make sure we have the same screen number
    XWindowAttributes attr;
    attr.screen = 0;
    if (XGetWindowAttributes(disp, win, &attr) != 0 &&
        attr.screen != 0 &&
        XScreenNumberOfScreen(attr.screen) != window().screenNumber()) {
        return;
    }

    TrayWindow *traywin = new TrayWindow(win, using_xembed);

    fbdbg<<"SystemTray::addClient(Window): 0x"<<hex<<win<<dec<<endl;

    m_clients.push_back(traywin);
    FbTk::EventManager::instance()->add(*this, win);
    traywin->reparent(m_window, 0, 0);
    traywin->addToSaveSet();

    if (using_xembed) {
        static Atom xembed_atom = XInternAtom(disp, "_XEMBED", False);

#define XEMBED_EMBEDDED_NOTIFY		0
        // send embedded message
        XEvent ce;
        ce.xclient.type = ClientMessage;
        ce.xclient.message_type = xembed_atom;
        ce.xclient.display = disp;
        ce.xclient.window = win;
        ce.xclient.format = 32;
        ce.xclient.data.l[0] = CurrentTime; // timestamp
        ce.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
        ce.xclient.data.l[2] = 0l; // The protocol version we support
        ce.xclient.data.l[3] = m_window.window(); // the window owning the selection
        ce.xclient.data.l[4] = 0l; // unused

        XSendEvent(disp, win, false, NoEventMask, &ce);
    }

    if (traywin->getMappedDefault())
        showClient(traywin);
}

void SystemTray::removeClient(Window win, bool destroyed) {
    ClientList::iterator tray_it = findClient(win);
    if (tray_it == m_clients.end())
        return;

    fbdbg<<"(SystemTray::removeClient(Window)): 0x"<<hex<<win<<dec<<endl;

    TrayWindow *traywin = *tray_it;
    m_clients.erase(tray_it);
    if (!destroyed) {
        traywin->setEventMask(NoEventMask);
        traywin->removeFromSaveSet();
    }
    hideClient(traywin, destroyed);
    delete traywin;
}

void SystemTray::exposeEvent(XExposeEvent &event) {
    m_window.clear();
}

void SystemTray::handleEvent(XEvent &event) {
    if (event.type == DestroyNotify) {
        removeClient(event.xdestroywindow.window, true);
    } else if (event.type == ReparentNotify && event.xreparent.parent != m_window.window()) {
        removeClient(event.xreparent.window, false);
    } else if (event.type == UnmapNotify && event.xany.send_event) {
        // we ignore server-generated events, which can occur
        // on restart. The ICCCM says that a client must send
        // a synthetic event for the withdrawn state
        ClientList::iterator it = findClient(event.xunmap.window);
        if (it != m_clients.end())
            hideClient(*it);
    } else if (event.type == ConfigureNotify) {
        // we got configurenotify from an client
        // check and see if we need to update it's size
        // and we must reposition and resize them to fit
        // our toolbar

        sortClients();

        ClientList::iterator it = findClient(event.xconfigure.window);
        if (it != m_clients.end()) {
            if (static_cast<unsigned int>(event.xconfigure.width) != (*it)->width() ||
                static_cast<unsigned int>(event.xconfigure.height) != (*it)->height()) {
                // the position might differ so we update from our local
                // copy of position
                XMoveResizeWindow(FbTk::App::instance()->display(), (*it)->window(),
                                  (*it)->x(), (*it)->y(),
                                  (*it)->width(), (*it)->height());

                // this was why gaim wasn't centring the icon
                (*it)->sendConfigureNotify(0, 0, (*it)->width(), (*it)->height());
                // so toolbar know that we changed size
                // done inside this loop, because otherwise we can get into nasty looping
                resizeSig().emit();
            }
        }
    } else if (event.type == PropertyNotify) {
        ClientList::iterator it = findClient(event.xproperty.window);
        if (it != m_clients.end()) {
            if (event.xproperty.atom == getXEmbedInfoAtom()) {
                if ((*it)->getMappedDefault())
                    showClient(*it);
                else
                    hideClient(*it);
            }
        }
    }
}

void SystemTray::rearrangeClients() {
    unsigned int w_rot0 = width(), h_rot0 = height();
    const unsigned int bw = m_theme->border().width();
    FbTk::translateSize(orientation(), w_rot0, h_rot0);
    unsigned int trayw = m_num_visible_clients*h_rot0 + bw, trayh = h_rot0;
    FbTk::translateSize(orientation(), trayw, trayh);
    resize(trayw, trayh);
    update();

    // move and resize clients
    ClientList::iterator client_it = m_clients.begin();
    ClientList::iterator client_it_end = m_clients.end();
    int next_x = bw;
    for (; client_it != client_it_end; ++client_it) {
        if (!(*client_it)->isVisible())
            continue;
        int x = next_x, y = bw;
        next_x += h_rot0+bw;
        translateCoords(orientation(), x, y, w_rot0, h_rot0);
        translatePosition(orientation(), x, y, h_rot0, h_rot0, 0);
        int screen_x = 0, screen_y = 0;
        getScreenCoordinates((*client_it)->window(), (*client_it)->x(), (*client_it)->y(), screen_x, screen_y);

        (*client_it)->moveResize(x, y, h_rot0, h_rot0);
        (*client_it)->sendConfigureNotify(screen_x, screen_y, h_rot0, h_rot0);
    }
}

void SystemTray::removeAllClients() {
    BScreen *screen = Fluxbox::instance()->findScreen(window().screenNumber());
    while (!m_clients.empty()) {
        TrayWindow * traywin = m_clients.back();
        traywin->setEventMask(NoEventMask);

        if (traywin->isXEmbedded())
            traywin->hide();

        if (screen)
            traywin->reparent(screen->rootWindow(), 0, 0, false);
        traywin->removeFromSaveSet();
        delete traywin;
        m_clients.pop_back();
    }
    m_num_visible_clients = 0;
}

void SystemTray::hideClient(TrayWindow *traywin, bool destroyed) {
    if (!traywin || !traywin->isVisible())
        return;

    if (!destroyed)
        traywin->hide();
    m_num_visible_clients--;
    rearrangeClients();
}

void SystemTray::showClient(TrayWindow *traywin) {
    if (!traywin || traywin->isVisible())
        return;

    if (!m_num_visible_clients)
        show();

    traywin->show();
    m_num_visible_clients++;
    rearrangeClients();
}

static std::string trim(const std::string& str)
{
    const std::string whitespace(" \t");
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

static void parse_order(const std::string s, std::vector<std::string> &out) {
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, ','))
        out.push_back(trim(item));
}

static int client_to_ordinal(const std::vector<std::string> left,
        const std::vector<std::string> right,
        TrayWindow *i) {

    auto Xdeleter = [](XClassHint *x){XFree(x);};

    std::unique_ptr<XClassHint, decltype(Xdeleter)>
            xclasshint (XAllocClassHint(), Xdeleter);

    if(XGetClassHint(Fluxbox::instance()->display(),
                i->window(), xclasshint.get()) != BadWindow)
    {
        std::string classname(xclasshint.get()->res_class);

        auto ix = std::find(left.begin(), left.end(), classname);
        if (ix != left.end())
            return -(left.end()-ix); // the more left, the negative (<0)
        else {
            ix = std::find(right.begin(), right.end(), classname);
            if (ix != right.end())
                // the more right, the positive (>0)
                return ix-right.begin()+1;
        }
    }

    // in neither list or invalid window (=0)
    return 0;
}

static bool client_comperator(const std::vector<std::string> left,
        const std::vector<std::string> right,
        TrayWindow *item1, TrayWindow *item2) {
    const int a = client_to_ordinal(left, right, item1);
    const int b = client_to_ordinal(left, right, item2);
    return a<b;
}


void SystemTray::sortClients() {
    std::vector<std::string> pinleft, pinright;

    parse_order(m_rc_systray_pinleft, pinleft);
    parse_order(m_rc_systray_pinright, pinright);

    m_clients.sort(std::bind(client_comperator,
                pinleft, pinright,
                std::placeholders::_1, std::placeholders::_2));

    rearrangeClients();
}

void SystemTray::update() {

    if (!m_theme->texture().usePixmap()) {
        m_window.setBackgroundColor(m_theme->texture().color());
    }
    else {
        if(m_pixmap)
            m_screen.imageControl().removeImage(m_pixmap);
        m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                       m_theme->texture(), orientation());
        m_window.setBackgroundPixmap(m_pixmap);
    }

    ClientList::iterator client_it = m_clients.begin();
    ClientList::iterator client_it_end = m_clients.end();
    for (; client_it != client_it_end; ++client_it) {

        // maybe not the best solution (yet), force a refresh of the
        // background of the client
        if (!(*client_it)->isVisible())
            continue;
        (*client_it)->hide();
        (*client_it)->show();
    }

}

Atom SystemTray::getXEmbedInfoAtom() {
    static Atom theatom = XInternAtom(Fluxbox::instance()->display(), "_XEMBED_INFO", False);
    return theatom;
}

string SystemTray::getNetSystemTrayAtom(int screen_nr) {

    string atom_name("_NET_SYSTEM_TRAY_S");
    atom_name += FbTk::StringUtil::number2String(screen_nr);

    return atom_name;
}

bool SystemTray::doesControl(Window win) {
    if (win == None || !s_theoneandonly)
        return false;
    return win == s_theoneandonly->window().window() ||
           s_theoneandonly->findClient(win) != s_theoneandonly->m_clients.end();
}
