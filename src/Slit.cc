// Slit.cc for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Slit.cc for Blackbox - an X11 Window manager
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

#include "Slit.hh"

#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Theme.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/MemFun.hh"

#include "FbCommands.hh"
#include "Keys.hh"
#include "Layer.hh"
#include "LayerMenu.hh"
#include "FbTk/Layer.hh"
#include "RootTheme.hh"
#include "FbMenu.hh"
#include "fluxbox.hh"

#include "SlitTheme.hh"
#include "SlitClient.hh"
#include "Xutil.hh"
#include "Debug.hh"

#include "FbTk/App.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/I18n.hh"
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/IntMenuItem.hh"
#include "FbTk/RadioMenuItem.hh"

#include <X11/Xatom.h>

#include <iostream>
#include <algorithm>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::pair;
using std::list;
using std::ifstream;
using std::ofstream;
using std::endl;
using std::hex;
using std::dec;

namespace FbTk {

template<>
string FbTk::Resource<Slit::Placement>::getString() const {
    switch (m_value) {
    case Slit::TOPLEFT:
        return string("TopLeft");
        break;
    case Slit::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Slit::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Slit::TOPCENTER:
        return string("TopCenter");
        break;
    case Slit::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Slit::TOPRIGHT:
        return string("TopRight");
        break;
    case Slit::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Slit::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case Slit::LEFTTOP:
        return string("LeftTop");
        break;
    case Slit::RIGHTTOP:
        return string("RightTop");
        break;
    case Slit::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case Slit::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("RightBottom");
}

template<>
void FbTk::Resource<Slit::Placement>::setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Slit::TOPLEFT;
    else if (strcasecmp(strval, "LeftCenter")==0)
        m_value = Slit::LEFTCENTER;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Slit::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Slit::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Slit::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Slit::TOPRIGHT;
    else if (strcasecmp(strval, "RightCenter")==0)
        m_value = Slit::RIGHTCENTER;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Slit::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop")==0)
        m_value = Slit::LEFTTOP;
    else if (strcasecmp(strval, "LeftBottom")==0)
        m_value = Slit::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop")==0)
        m_value = Slit::RIGHTTOP;
    else if (strcasecmp(strval, "RightBottom")==0)
        m_value = Slit::RIGHTBOTTOM;
    else
        setDefaultValue();
}

} // end namespace FbTk
namespace {

class SlitClientMenuItem: public FbTk::MenuItem{
public:
    explicit SlitClientMenuItem(Slit& slit, SlitClient &client, FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::MenuItem(client.matchName(), cmd), m_slit(slit), m_client(client) {
        setCommand(cmd);
        FbTk::MenuItem::setSelected(client.visible());
        setToggleItem(true);
        setCloseOnClick(false);
    }
    const FbTk::BiDiString &label() const {
        return m_client.matchName();
    }
    bool isSelected() const {
        return m_client.visible();
    }
    void click(int button, int time, unsigned int mods) {
        if (button == 4 || button == 2) { // wheel up
            m_slit.clientUp(&m_client);
        } else if (button == 5 || button == 3) { // wheel down
            m_slit.clientDown(&m_client);
        } else {
            m_client.setVisible(!m_client.visible());
            FbTk::MenuItem::setSelected(m_client.visible());
            FbTk::MenuItem::click(button, time, mods);
        }
    }
private:
    Slit& m_slit;
    SlitClient &m_client;
};

class PlaceSlitMenuItem: public FbTk::RadioMenuItem {
public:
    PlaceSlitMenuItem(const FbTk::FbString &label, Slit &slit, Slit::Placement place, FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd), m_slit(slit), m_place(place) {
        setCloseOnClick(false);
    }
    bool isSelected() const { return m_slit.placement() == m_place; }
    void click(int button, int time, unsigned int mods) {
        m_slit.setPlacement(m_place);
        FbTk::RadioMenuItem::click(button, time, mods);
    }
private:
    Slit &m_slit;
    Slit::Placement m_place;
};

} // End anonymous namespace

unsigned int Slit::s_eventmask = SubstructureRedirectMask |  ButtonPressMask |
                                 EnterWindowMask | LeaveWindowMask | ExposureMask;

Slit::Slit(BScreen &scr, FbTk::Layer &layer, const char *filename)
    : m_hidden(false), m_visible(false), m_pending_reconfigure(false),
      m_screen(scr),
      m_clientlist_menu(scr.menuTheme(),
                        scr.imageControl(),
                        *scr.layerManager().getLayer(ResourceLayer::MENU)),
      m_slitmenu(scr.menuTheme(),
                 scr.imageControl(),
                 *scr.layerManager().getLayer(ResourceLayer::MENU)),
#ifdef XINERAMA
      m_xineramaheadmenu(0),
#endif // XINERAMA
      frame(scr.rootWindow()),
       //For KDE dock applets
      m_kwm1_dockwindow(XInternAtom(FbTk::App::instance()->display(),
                                    "KWM_DOCKWINDOW", False)), //KDE v1.x
      m_kwm2_dockwindow(XInternAtom(FbTk::App::instance()->display(),
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False)), //KDE v2.x

      m_slit_theme(new SlitTheme(scr.rootWindow().screenNumber())),
      m_strut(0),
      // resources
      // lock in first resource
      m_rc_kde_dockapp(scr.resourceManager(), true,
                       scr.name() + ".slit.acceptKdeDockapps", scr.altName() + ".Slit.AcceptKdeDockapps"),
      m_rc_auto_hide(scr.resourceManager().lock(), false,
                     scr.name() + ".slit.autoHide", scr.altName() + ".Slit.AutoHide"),
      m_rc_auto_raise(scr.resourceManager().lock(), false,
                     scr.name() + ".slit.autoRaise", scr.altName() + ".Slit.AutoRaise"),
      // TODO: this resource name must change
      m_rc_maximize_over(scr.resourceManager(), false,
                         scr.name() + ".slit.maxOver", scr.altName() + ".Slit.MaxOver"),
      m_rc_placement(scr.resourceManager(), RIGHTBOTTOM,
                     scr.name() + ".slit.placement", scr.altName() + ".Slit.Placement"),
      m_rc_alpha(scr.resourceManager(), 255,
                 scr.name() + ".slit.alpha", scr.altName() + ".Slit.Alpha"),
      m_rc_on_head(scr.resourceManager(), 0,
                   scr.name() + ".slit.onhead", scr.altName() + ".Slit.onHead"),
      m_rc_layernum(scr.resourceManager(), ResourceLayer(ResourceLayer::DOCK),
                    scr.name() + ".slit.layer", scr.altName() + ".Slit.Layer") {

    _FB_USES_NLS;

    frame.window.setWindowRole("fluxbox-slit");

    // attach to theme and root window change signal
    join(theme().reconfigSig(), FbTk::MemFun(*this, &Slit::reconfigure));

    join(scr.resizeSig(),
         FbTk::MemFun(*this, &Slit::screenSizeChanged));

    join(scr.bgChangeSig(),
         FbTk::MemFunIgnoreArgs(*this, &Slit::reconfigure));

    join(scr.reconfigureSig(), FbTk::MemFunIgnoreArgs(*this, &Slit::reconfigure));

    scr.addConfigMenu(_FB_XTEXT(Slit, Slit, "Slit", "The Slit"), m_slitmenu);

    frame.pixmap = None;
    // move the frame out of sight for a moment
    frame.window.move(-frame.window.width(), -frame.window.height());
    // setup timer
    m_timer.setTimeout(200L * FbTk::FbTime::IN_MILLISECONDS); // default timeout
    m_timer.fireOnce(true);
    FbTk::RefCount<FbTk::Command<void> > ucs(new FbTk::SimpleCommand<Slit>(*this, &Slit::updateCrossingState));
    m_timer.setCommand(ucs);


    FbTk::EventManager::instance()->add(*this, frame.window);
    FbTk::EventManager::instance()->addParent(*this, window());
    Fluxbox::instance()->keys()->registerWindow(window().window(), *this, Keys::ON_SLIT);

    if (FbTk::Transparent::haveComposite()) {
        frame.window.setOpaque(*m_rc_alpha);
    } else {
        frame.window.setAlpha(*m_rc_alpha);
    }

    m_layeritem.reset(new FbTk::LayerItem(frame.window, layer));

    m_layermenu.reset(new LayerMenu(scr.menuTheme(),
                                    scr.imageControl(),
                                    *scr.layerManager().
                                    getLayer(ResourceLayer::MENU),
                                    this,
                                    true));
    m_layermenu->setLabel(_FB_XTEXT(Slit, Layer, "Slit Layer", "Title of Slit Layer Menu"));

    moveToLayer((*m_rc_layernum).getNum());



    // Get client list for sorting purposes
    loadClientList(filename);

    setupMenu();

    scr.resourceManager().unlock();
}


Slit::~Slit() {
    clearStrut();
    if (frame.pixmap != 0)
        screen().imageControl().removeImage(frame.pixmap);

    // otherwise it will try to access it on deletion
    screen().removeConfigMenu(m_slitmenu);

    shutdown();
}

void Slit::clearStrut() {
    if (m_strut != 0) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Slit::updateStrut() {
    bool had_strut = m_strut ? true : false;
    clearStrut();
    // no need for area if we're autohiding or set maximize over
    // or if we dont have any clients
    if (doAutoHide() || *m_rc_maximize_over || !m_visible) {
        // update screen area if we had a strut before
        if (had_strut)
            screen().updateAvailableWorkspaceArea();
        return;
    }

    const unsigned int bw = m_slit_theme->borderWidth() * 2;
    int left = 0, right = 0, top = 0, bottom = 0;
    switch (placement()) {
    case TOPLEFT:
        top = height() + bw;
        break;
    case LEFTTOP:
        left = width() + bw;
        break;
    case TOPCENTER:
        top = height() + bw;
        break;
    case TOPRIGHT:
        top = height() + bw;
        break;
    case RIGHTTOP:
        right = width() + bw;
        break;
    case BOTTOMLEFT:
        bottom = height() + bw;
        break;
    case LEFTBOTTOM:
        left = width() + bw;
        break;
    case BOTTOMCENTER:
        bottom = height() + bw;
        break;
    case BOTTOMRIGHT:
        bottom = height() + bw;
        break;
    case RIGHTBOTTOM:
        right = width() + bw;
        break;
    case LEFTCENTER:
        left = width() + bw;
        break;
    case RIGHTCENTER:
        right = width() + bw;
        break;
    }

    m_strut = screen().requestStrut(getOnHead(), left, right, top, bottom);
    screen().updateAvailableWorkspaceArea();
}

void Slit::addClient(Window w) {

    fbdbg<<"addClient(w = 0x"<<hex<<w<<dec<<")"<<endl;

    // Can't add non existent window
    if (w == None)
        return;

    if (!acceptKdeDockapp() && screen().isKdeDockapp(w))
        return;

    // Look for slot in client list by name
    SlitClient *client = 0;
    FbTk::FbString match_name = Xutil::getWMClassName(w);
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    bool found_match = false;
    for (; it != it_end; ++it) {
        // If the name matches...
        if ((*it)->matchName().logical() == match_name) {
            // Use the slot if no window is assigned
            if ((*it)->window() == None) {
                client = (*it);
                client->initialize(&screen(), w);
                break;
            }
            // Otherwise keep looking for an unused match or a non-match
            found_match = true;		// Possibly redundant

        } else if (found_match) {
            // Insert before first non-match after a previously found match?
            client = new SlitClient(&screen(), w);
            m_client_list.insert(it, client);
            break;
        }
    }
    // Append to client list?
    if (client == 0) {
        client = new SlitClient(&screen(), w);
        m_client_list.push_back(client);
    }

    Display *disp = FbTk::App::instance()->display();
    XWMHints *wmhints = XGetWMHints(disp, w);

    if (wmhints != 0) {
        if ((wmhints->flags & IconWindowHint) &&
            (wmhints->icon_window != None)) {
            XMoveWindow(disp, client->clientWindow(), -100, -100);
            XMapWindow(disp, client->clientWindow());
            client->setIconWindow(wmhints->icon_window);
            client->setWindow(client->iconWindow());
        } else {
            client->setIconWindow(None);
            client->setWindow(client->clientWindow());
        }

        XFree((void *) wmhints);
    } else {
        client->setIconWindow(None);
        client->setWindow(client->clientWindow());
    }

    Atom *proto = 0;
    int num_return = 0;

    if (XGetWMProtocols(disp, w, &proto, &num_return)) {
        XFree((void *) proto);
    } else {
        fbdbg<<"Warning: Failed to read WM Protocols. "<<endl;
    }

    XWindowAttributes attrib;

    if (screen().isKdeDockapp(w))
        client->resize(24, 24);
    else if (XGetWindowAttributes(disp, client->window(), &attrib))
        client->resize(attrib.width, attrib.height);
    else // set default size if we failed to get window attributes
        client->resize(64, 64);

    // disable border for client window
    XSetWindowBorderWidth(disp, client->window(), 0);

    // disable events to frame.window
    frame.window.setEventMask(NoEventMask);
    client->disableEvents();


    XReparentWindow(disp, client->window(), frame.window.window(), 0, 0);
    XMapRaised(disp, client->window());
    XChangeSaveSet(disp, client->window(), SetModeInsert);
    // reactivate events for frame.window
    frame.window.setEventMask(s_eventmask);
    // setup event for slit client window
    client->enableEvents();

    // flush events
    //    XFlush(disp);

    // add slit client to eventmanager
    FbTk::EventManager::instance()->add(*this, client->clientWindow());
    FbTk::EventManager::instance()->add(*this, client->iconWindow());

    //    frame.window.show();
    clearWindow();
    reconfigure();

    updateClientmenu();

    saveClientList();

}

void Slit::setPlacement(Placement place) {
    *m_rc_placement = place;
    reconfigure();
}

void Slit::removeClient(SlitClient *client, bool remap, bool destroy) {
    if (client == 0)
        return;

    // remove from event manager
    if (client->clientWindow() != 0)
        FbTk::EventManager::instance()->remove(client->clientWindow());
    if (client->iconWindow() != 0)
        FbTk::EventManager::instance()->remove(client->iconWindow());

    // Destructive removal?
    if (destroy)
        m_client_list.remove(client);
    else // Clear the window info, but keep around to help future sorting?
        client->initialize();

    if (remap && client->window() != 0) {
        Display *disp = FbTk::App::instance()->display();

        if (!client->visible())
            client->show();

        client->disableEvents();
        // stop events to frame.window temporarly
        frame.window.setEventMask(NoEventMask);
        XReparentWindow(disp, client->window(), screen().rootWindow().window(),
			client->x(), client->y());
        XChangeSaveSet(disp, client->window(), SetModeDelete);
        // reactivate events to frame.window
        frame.window.setEventMask(s_eventmask);
        XFlush(disp);
    }

    // Destructive removal?
    if (destroy)
        delete client;

    updateClientmenu();
}


void Slit::removeClient(Window w, bool remap) {

    if (w == frame.window)
        return;

    bool reconf = false;

    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == w) {
            removeClient((*it), remap, false);
            reconf = true;

            break;
        }
    }
    if (reconf)
        reconfigure();

}


void Slit::reconfigure() {

    bool allow_autohide = true;
    if (m_hidden)
        m_pending_reconfigure = true;
    else if (m_pending_reconfigure)
        allow_autohide = false; // this is for a pending one, triggerd by unhide

    frame.width = 0;
    frame.height = 0;

    // Need to count windows because not all client list entries
    // actually correspond to mapped windows.
    int num_windows = 0;
    const int bevel_width = theme()->bevelWidth();
    // determine width or height increase
    bool height_inc = false;
    switch (placement()) {
    case LEFTTOP:
    case RIGHTTOP:
    case LEFTCENTER:
    case RIGHTCENTER:
    case LEFTBOTTOM:
    case RIGHTBOTTOM:
        height_inc = true;
    default:
        break;
    }

    SlitClients::iterator client_it = m_client_list.begin();
    SlitClients::iterator client_it_end = m_client_list.end();
    for (; client_it != client_it_end; ++client_it) {
        // client created window?
        if ((*client_it)->window() != None && (*client_it)->visible()) {
            num_windows++;

            // get the dockapps to update their backgrounds
            if (screen().isKdeDockapp((*client_it)->window())) {
                (*client_it)->hide();
                (*client_it)->show();
            }

            if (height_inc) {
                // increase height of slit for each client (VERTICAL mode)
                frame.height += (*client_it)->height() + bevel_width;
                // the slit should always have the width of the largest client
                if (frame.width < (*client_it)->width())
                    frame.width = (*client_it)->width();
            } else {
                // increase width of slit for each client (HORIZONTAL mode)
                frame.width += (*client_it)->width() + bevel_width;
                // the slit should always have the width of the largest client
                if (frame.height < (*client_it)->height())
                    frame.height = (*client_it)->height();
            }
        }
    }

    if (frame.width < 1)
        frame.width = 1;
    else
        frame.width += bevel_width;

    if (frame.height < 1)
        frame.height = 1;
    else
        frame.height += bevel_width*2;

    Display *disp = FbTk::App::instance()->display();

    frame.window.setBorderWidth(theme()->borderWidth());
    frame.window.setBorderColor(theme()->borderColor());

    Pixmap tmp = frame.pixmap;
    FbTk::ImageControl &image_ctrl = screen().imageControl();
    const FbTk::Texture &texture = m_slit_theme->texture();
    if (!texture.usePixmap()) {
        frame.pixmap = 0;
        frame.window.setBackgroundColor(texture.color());
    } else {
        frame.pixmap = image_ctrl.renderImage(frame.width, frame.height,
                                              texture);
        if (frame.pixmap == 0)
            frame.window.setBackgroundColor(texture.color());
        else
            frame.window.setBackgroundPixmap(frame.pixmap);
    }

    if (tmp)
        image_ctrl.removeImage(tmp);

    // could have changed types, so we must set both
    if (FbTk::Transparent::haveComposite()) {
        frame.window.setAlpha(255);
        frame.window.setOpaque(*m_rc_alpha);
    } else {
        frame.window.setAlpha(*m_rc_alpha);
        frame.window.setOpaque(255);
    }
    // reposition clears the bg
    reposition();

    // did we actually use slit slots
    if (num_windows == 0)
        hide();
    else
        show();

    int x = 0, y = 0;
    if (height_inc)
        y = bevel_width;
    else
        x = bevel_width;

    client_it = m_client_list.begin();
    for (; client_it != client_it_end; ++client_it) {
        if ((*client_it)->window() == None)
            continue;

        //client created window?
        if ((*client_it)->visible())
            (*client_it)->show();
        else {
            (*client_it)->disableEvents();
            (*client_it)->hide();
            (*client_it)->enableEvents();
            continue;
        }

        if (height_inc)
            x = (frame.width - (*client_it)->width()) / 2;
        else
            y = (frame.height - (*client_it)->height()) / 2;

        XMoveResizeWindow(disp, (*client_it)->window(), x, y,
                          (*client_it)->width(), (*client_it)->height());

        // for ICCCM compliance
        (*client_it)->move(x, y);

        XEvent event;
        event.type = ConfigureNotify;

        event.xconfigure.display = disp;
        event.xconfigure.event = (*client_it)->window();
        event.xconfigure.window = (*client_it)->window();
        event.xconfigure.x = (*client_it)->x();
        event.xconfigure.y = (*client_it)->y();
        event.xconfigure.width = (*client_it)->width();
        event.xconfigure.height = (*client_it)->height();
        event.xconfigure.border_width = 0;
        event.xconfigure.above = frame.window.window();
        event.xconfigure.override_redirect = False;

        XSendEvent(disp, (*client_it)->window(), False, StructureNotifyMask,
                   &event);

        if (height_inc)
            y += (*client_it)->height() + bevel_width;
        else
            x += (*client_it)->width() + bevel_width;
    } // end for

    if (allow_autohide && doAutoHide() && !isHidden() && !m_timer.isTiming())
        m_timer.start();
    else if (!doAutoHide() && isHidden())
        toggleHidden(); // restore visible

    m_slitmenu.reconfigure();
    updateClientmenu();
    updateStrut();

}


void Slit::reposition() {
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

    if (screen().hasXinerama()) {
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    } else {
        head_w = screen().width();
        head_h = screen().height();
    }

    int border_width = theme()->borderWidth();
    int pixel = (border_width == 0 ? 1 : 0);
    // place the slit in the appropriate place
    switch (placement()) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + pixel - border_width - frame.height;
        break;

    case LEFTTOP:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x + pixel - border_width - frame.width;
        frame.y_hidden = head_y;
        break;

    case LEFTCENTER:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height) / 2;
        frame.x_hidden = head_x + pixel - border_width - frame.width;
        frame.y_hidden = frame.y;
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - pixel - border_width;
        break;

    case LEFTBOTTOM:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x + pixel - border_width - frame.width;
        frame.y_hidden = frame.y;
        break;

    case TOPCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + pixel - border_width - frame.height;
        break;

    case BOTTOMCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - pixel - border_width;
        break;

    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + pixel - border_width - frame.height;
        break;

    case RIGHTTOP:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = head_x + head_w - pixel - border_width;
        frame.y_hidden = head_y;
        break;

    case RIGHTCENTER:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + ((head_h - frame.height) / 2);
        frame.x_hidden = head_x + head_w - pixel - border_width;
        frame.y_hidden = frame.y;
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - pixel - border_width;
        break;

    case RIGHTBOTTOM:
    default:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x + head_w - pixel - border_width;
        frame.y_hidden = frame.y;
        break;
    }

    if (isHidden()) {
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    } else {
        frame.window.moveResize(frame.x,  frame.y,
                                frame.width, frame.height);
    }
    frame.window.updateBackground(true);
    if (*m_rc_alpha != 255)
        clearWindow();
}


void Slit::shutdown() {
    saveClientList();
    while (!m_client_list.empty())
        removeClient(m_client_list.front(), true, true);
}

void Slit::clientUp(SlitClient* client) {
    if (!client || m_client_list.size() < 2)
        return;

    if (client == m_client_list.front()) {
        cycleClientsUp();
        return;
    }

    SlitClients::iterator it = m_client_list.begin();
    for(++it; it != m_client_list.end(); ++it) {
        if ((*it) == client) {
            SlitClients::iterator prev = it;
            prev--;
            iter_swap(it, prev);
            reconfigure();
            break;
        }
    }
}

void Slit::clientDown(SlitClient* client) {
    if (!client || m_client_list.size() < 2)
        return;

    if (client == m_client_list.back()) {
        cycleClientsDown();
        return;
    }

    SlitClients::reverse_iterator it = m_client_list.rbegin();
    for(++it; it != m_client_list.rend(); ++it) {
        if ((*it) == client) {
            SlitClients::reverse_iterator next = it;
            next--;
            iter_swap(it, next);
            reconfigure();
            break;
        }
    }
}

void Slit::cycleClientsUp() {
    if (m_client_list.size() < 2)
        return;

    // rotate client list up, ie the first goes last
    SlitClients::iterator it = m_client_list.begin();
    SlitClient *client = *it;
    m_client_list.erase(it);
    m_client_list.push_back(client);
    reconfigure();
}

void Slit::cycleClientsDown() {
    if (m_client_list.size() < 2)
        return;

    // rotate client list down, ie the last goes first
    SlitClient *client = m_client_list.back();
    m_client_list.remove(client);
    m_client_list.push_front(client);
    reconfigure();
}

void Slit::handleEvent(XEvent &event) {
    if (event.type == ConfigureRequest) {
        configureRequestEvent(event.xconfigurerequest);
    } else if (event.type == DestroyNotify) {
        removeClient(event.xdestroywindow.window, false);
    } else if (event.type == UnmapNotify && event.xany.send_event) {
        // we ignore server-generated events, which can occur
        // on restart. The ICCCM says that a client must send
        // a synthetic event for the withdrawn state
        removeClient(event.xunmap.window);
    }
}

void Slit::buttonPressEvent(XButtonEvent &be) {
    Display *dpy = Fluxbox::instance()->display();
    const bool myMenuWasVisible = m_slitmenu.isVisible();

    FbTk::Menu::hideShownMenu();

    if (Fluxbox::instance()->keys()->doAction(be.type, be.state, be.button,
                                              Keys::ON_SLIT, 0, be.time)) {
        XAllowEvents(dpy, SyncPointer, CurrentTime);
        return;
    }

    if (be.button == 1)
        frame.window.raise();

    if (be.button != Button3) {
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        return;
    }

    XAllowEvents(dpy, SyncPointer, CurrentTime);
    if (!myMenuWasVisible)
        screen().placementStrategy().placeAndShowMenu(m_slitmenu, be.x_root, be.y_root, false);
}


void Slit::updateCrossingState() {
    Window wr, wc;
    int rx, ry, x, y;
    unsigned int mask;
    const int bw = -theme()->borderWidth();
    bool hovered = false;
    if (XQueryPointer(Fluxbox::instance()->display(), window().window(), &wr, &wc, &rx, &ry, &x, &y, &mask))
        hovered = x >= bw && y >= bw && x < int(width()) && y < int(height());

    if (hovered) {
        if (m_rc_auto_raise)
            m_layeritem->moveToLayer(ResourceLayer::ABOVE_DOCK);
        if (m_rc_auto_hide && isHidden())
            toggleHidden();
    } else {
        if (m_rc_auto_hide && !isHidden())
            toggleHidden();
        if (m_rc_auto_raise)
            m_layeritem->moveToLayer(m_rc_layernum->getNum());
    }
}

void Slit::enterNotifyEvent(XCrossingEvent &ce) {
    Fluxbox::instance()->keys()->doAction(ce.type, ce.state, 0, Keys::ON_SLIT);

    if (!m_rc_auto_hide && isHidden()) {
        toggleHidden();
    }

    if (!m_timer.isTiming() && (m_rc_auto_hide && isHidden()) ||
       (m_rc_auto_raise && m_layeritem->getLayerNum() != ResourceLayer::ABOVE_DOCK))
        m_timer.start();
}

void Slit::leaveNotifyEvent(XCrossingEvent &event) {
    if (m_slitmenu.isVisible())
        return;

    if (!m_timer.isTiming() && (m_rc_auto_hide && !isHidden()) ||
       (m_rc_auto_raise && m_layeritem->getLayerNum() != m_rc_layernum->getNum()))
        m_timer.start();

    if (!isHidden())
        Fluxbox::instance()->keys()->doAction(event.type, event.state, 0, Keys::ON_SLIT);
}

void Slit::configureRequestEvent(XConfigureRequestEvent &event) {
    bool reconf = false;
    XWindowChanges xwc;

    xwc.x = event.x;
    xwc.y = event.y;
    xwc.width = event.width;
    xwc.height = event.height;
    xwc.border_width = 0;
    xwc.sibling = event.above;
    xwc.stack_mode = event.detail;

    XConfigureWindow(FbTk::App::instance()->display(),
                     event.window, event.value_mask, &xwc);

    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == event.window) {
            if ((*it)->width() != ((unsigned) event.width) ||
                (*it)->height() != ((unsigned) event.height)) {
                (*it)->resize(event.width, event.height);

                reconf = true; //requires reconfiguration

                break;
            }
        }
    }

    if (reconf)
        reconfigure();
}

void Slit::exposeEvent(XExposeEvent &ev) {
    // we don't need to clear the entire window
    // just the are that gets exposed
    frame.window.clearArea(ev.x, ev.y, ev.width, ev.height);
}

void Slit::screenSizeChanged(BScreen &screen) {
    reconfigure();
#ifdef XINERAMA
    if (m_xineramaheadmenu)
        m_xineramaheadmenu->reloadHeads();
#endif // XINERAMA
}

void Slit::clearWindow() {
    frame.window.clear();
}

void Slit::toggleHidden() {
    m_hidden = ! m_hidden; // toggle hidden state
    if (isHidden())
        frame.window.move(frame.x_hidden, frame.y_hidden);
    else {
        frame.window.move(frame.x, frame.y);
        if (m_pending_reconfigure) {
            reconfigure();
            m_pending_reconfigure = false;
        }
    }
}

void Slit::toggleAboveDock() {
    if (m_layeritem->getLayerNum() == m_rc_layernum->getNum())
        m_layeritem->moveToLayer(ResourceLayer::ABOVE_DOCK);
    else
        m_layeritem->moveToLayer(m_rc_layernum->getNum());
}

void Slit::loadClientList(const char *filename) {
    if (filename == 0 || filename[0] == '\0')
        return;

    // save filename so we can save client list later
    m_filename = filename;
    string real_filename= FbTk::StringUtil::expandFilename(filename);

    if (!FbTk::FileUtil::isRegularFile(real_filename.c_str())) {
        return;
    }

    ifstream file(real_filename.c_str());
    string name;
    while (! file.eof()) {
        name.clear();
        getline(file, name); // get the entire line
        if (name.empty())
            continue;

        // remove whitespaces from start and end
        FbTk::StringUtil::removeFirstWhitespace(name);

        // the cleaned string could still be a comment, or blank
        if ( name.empty() || name[0] == '#' || name[0] == '!' )
            continue;

        // trailing whitespace won't affect the above test
        FbTk::StringUtil::removeTrailingWhitespace(name);

        SlitClient *client = new SlitClient(name.c_str());
        m_client_list.push_back(client);
    }
}

void Slit::updateClientmenu() {
    if (screen().isShuttingdown())
        return;
    _FB_USES_NLS;

    // clear old items
    m_clientlist_menu.removeAll();
    m_clientlist_menu.setLabel(_FB_XTEXT(Slit, ClientsMenu, "Clients", "Slit client menu"));

    FbTk::RefCount<FbTk::Command<void> > cycle_up(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsUp));
    FbTk::RefCount<FbTk::Command<void> > cycle_down(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsDown));
    m_clientlist_menu.insertCommand(_FB_XTEXT(Slit, CycleUp, "Cycle Up", "Cycle clients upwards"), cycle_up);
    m_clientlist_menu.insertCommand(_FB_XTEXT(Slit, CycleDown, "Cycle Down", "Cycle clients downwards"), cycle_down);

    m_clientlist_menu.insertItem(new FbTk::MenuSeparator());

    FbTk::RefCount<FbTk::Command<void> > reconfig(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));
    SlitClients::iterator it = m_client_list.begin();
    for (; it != m_client_list.end(); ++it) {
        if ((*it) != 0 && (*it)->window() != 0)
            m_clientlist_menu.insertItem(new SlitClientMenuItem(*this, *(*it), reconfig));
    }

    m_clientlist_menu.insertItem(new FbTk::MenuSeparator());
    FbTk::RefCount<FbTk::Command<void> > savecmd(new FbTk::SimpleCommand<Slit>(*this, &Slit::saveClientList));
    m_clientlist_menu.insertCommand(_FB_XTEXT(Slit,
                                     SaveSlitList,
                                     "Save SlitList", "Saves the current order in the slit"),
                             savecmd);

    m_clientlist_menu.updateMenu();
}

void Slit::saveClientList() {

    ofstream file(FbTk::StringUtil::expandFilename(m_filename).c_str());
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    string prevName;
    string name;
    for (; it != it_end; ++it) {
        name = (*it)->matchName().logical();
        if (name != prevName)
            file << name.c_str() << endl;

        prevName = name;
    }
}

void Slit::setupMenu() {
    _FB_USES_NLS;
    using namespace FbTk;

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconf_slit_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command<void> > saverc_cmd(new FbCommands::SaveResources());
    FbTk::RefCount<FbTk::Command<void> > reconf_cmd(new FbCommands::ReconfigureFluxboxCmd());
    FbTk::RefCount<FbTk::Command<void> > reconf_slit_cmd(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));

    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);

    s_a_reconf_slit_macro->add(saverc_cmd);
    s_a_reconf_slit_macro->add(reconf_slit_cmd);

    FbTk::RefCount<FbTk::Command<void> > save_and_reconfigure(s_a_reconf_macro);
    FbTk::RefCount<FbTk::Command<void> > save_and_reconfigure_slit(s_a_reconf_slit_macro);


    // it'll be freed by the slitmenu (since not marked internal)
    FbMenu *placement_menu = new FbMenu(m_screen.menuTheme(),
                                        m_screen.imageControl(),
                                        *m_screen.layerManager().getLayer(::ResourceLayer::MENU));


    // setup base menu
    m_slitmenu.setLabel(_FB_XTEXT(Slit, Slit, "Slit", "The Slit"));
    m_slitmenu.insertSubmenu(
        _FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu"),
        placement_menu);

    m_slitmenu.insertSubmenu(
        _FB_XTEXT(Menu, Layer, "Layer...", "Title of Layer menu"),
        m_layermenu.get());

#ifdef XINERAMA
    if (screen().hasXinerama()) {
        m_xineramaheadmenu = new XineramaHeadMenu<Slit>(
            screen().menuTheme(),
            screen(),
            screen().imageControl(),
            *screen().layerManager().getLayer(::ResourceLayer::MENU),
            *this,
            _FB_XTEXT(Slit, OnHead, "Slit on Head", "Title of Slits On Head menu"));
        m_slitmenu.insertSubmenu(_FB_XTEXT(Menu, OnHead, "On Head...", "Title of On Head menu"), m_xineramaheadmenu);
    }
#endif //XINERAMA

    m_slitmenu.insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, AutoHide, "Auto hide", "This thing automatically hides when not close by"),
                                       m_rc_auto_hide,
                                       save_and_reconfigure_slit));
    m_slitmenu.insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, AutoRaise, "Auto raise", "This thing automatically raises when entered"),
                                       m_rc_auto_raise,
                                       save_and_reconfigure_slit));

    m_slitmenu.insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,"Maximize Over", "Maximize over this thing when maximizing"),
                                       m_rc_maximize_over,
                                       save_and_reconfigure_slit));

    // this saves resources and clears the slit window to update alpha value
    FbTk::MenuItem *alpha_menuitem =
        new FbTk::IntMenuItem(_FB_XTEXT(Common, Alpha, "Alpha", "Transparency level"),
                           m_rc_alpha,
                           0, 255, m_slitmenu);
    // setup command for alpha value
    MacroCommand *alpha_macrocmd = new MacroCommand();
    RefCount<Command<void> > alpha_cmd(new SimpleCommand<Slit>(*this, &Slit::updateAlpha));
    alpha_macrocmd->add(saverc_cmd);
    alpha_macrocmd->add(alpha_cmd);
    RefCount<Command<void> > set_alpha_cmd(alpha_macrocmd);
    alpha_menuitem->setCommand(set_alpha_cmd);

    m_slitmenu.insertItem(alpha_menuitem);

    m_slitmenu.insertSubmenu(_FB_XTEXT(Slit, ClientsMenu, "Clients", "Slit client menu"), &m_clientlist_menu);
    m_slitmenu.updateMenu();

    // setup sub menu
    placement_menu->setLabel(_FB_XTEXT(Slit, Placement, "Slit Placement", "Slit Placement"));
    placement_menu->setMinimumColumns(3);
    m_layermenu->setInternalMenu();
    m_clientlist_menu.setInternalMenu();
    m_slitmenu.setInternalMenu();

    typedef pair<FbTk::FbString, Slit::Placement> PlacementP;
    typedef list<PlacementP> Placements;
    Placements place_menu;

    // menu is 3 wide, 5 down
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), Slit::TOPLEFT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), Slit::LEFTTOP));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), Slit::LEFTCENTER));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), Slit::LEFTBOTTOM));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), Slit::BOTTOMLEFT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), Slit::TOPCENTER));
    place_menu.push_back(PlacementP("", Slit::TOPLEFT));
    place_menu.push_back(PlacementP("", Slit::TOPLEFT));
    place_menu.push_back(PlacementP("", Slit::TOPLEFT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), Slit::BOTTOMCENTER));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), Slit::TOPRIGHT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), Slit::RIGHTTOP));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), Slit::RIGHTCENTER));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), Slit::RIGHTBOTTOM));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), Slit::BOTTOMRIGHT));


    // create items in sub menu
    for (size_t i=0; i<15; ++i) {
        const FbTk::FbString &str = place_menu.front().first;
        Slit::Placement placement = place_menu.front().second;

        if (str == "") {
            placement_menu->insert("");
            placement_menu->setItemEnabled(i, false);
        } else {
            FbTk::MenuItem* item = new PlaceSlitMenuItem(str, *this, placement, save_and_reconfigure);
            placement_menu->insertItem(item);
        }
        place_menu.pop_front();
    }

    // finaly update sub menu
    placement_menu->updateMenu();
}

void Slit::moveToLayer(int layernum) {
    m_layeritem->moveToLayer(layernum);
    *m_rc_layernum = layernum;
}

void Slit::saveOnHead(int head) {
    m_rc_on_head = head;
    reconfigure();
}

void Slit::updateAlpha() {
    // called when the alpha resource is changed
    if (FbTk::Transparent::haveComposite()) {
        frame.window.setOpaque(*m_rc_alpha);
    } else {
        frame.window.setAlpha(*m_rc_alpha);
        frame.window.updateBackground(true);
        clearWindow();
    }
}
