// Slit.cc for fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Slit.cc,v 1.34 2003/02/17 12:53:21 fluxgen Exp $

#include "Slit.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "Screen.hh"
#include "ImageControl.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"
#include "BoolMenuItem.hh"
#include "EventManager.hh"
#include "MacroCommand.hh"

#include <algorithm>
#include <iostream>
#include <cassert>

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <X11/Xatom.h>

#include <iostream>
#include <algorithm>
using namespace std;
namespace {

void getWMName(BScreen *screen, Window window, std::string& name) {
    name = "";

    if (screen == 0 || window == None)
        return;

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    char **list;
    int num;
    I18n *i18n = I18n::instance();

    if (XGetWMName(display, window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    name = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    name = (char *)text_prop.value;
					
            } else				
                name = (char *)text_prop.value;
        } else { // default name
            name = i18n->getMessage(
                                    FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                    "Unnamed");
        }
    } else {
        // default name
        name = i18n->getMessage(
                                FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                "Unnamed");
    }

}

};
/// holds slit client info
class SlitClient {
public:
    /// For adding an actual window
    SlitClient(BScreen *screen, Window win) {
        initialize(screen, win);
    }
    /// For adding a placeholder
    explicit SlitClient(const char *name) { 
        initialize();
        match_name = (name == 0 ? "" : name);

    }

    // Now we pre-initialize a list of slit clients with names for
    // comparison with incoming client windows.  This allows the slit
    // to maintain a sorted order based on a saved window name list.
    // Incoming windows not found in the list are appended.  Matching
    // duplicates are inserted after the last found instance of the
    // matching name.
    std::string match_name;

    Window window, client_window, icon_window;

    int x, y;
    unsigned int width, height;
    bool visible; ///< wheter the client should be visible or not

    void initialize(BScreen *screen = 0, Window win= None) {
        client_window = win;
        window = icon_window = None;
        x = y = 0;
        width = height = 0;
        if (match_name.size() == 0)
            getWMName(screen, client_window, match_name);
        visible = true;        
    }
    void disableEvents() {
        Display *disp = FbTk::App::instance()->display();
        XSelectInput(disp, window, NoEventMask);
    }
    void enableEvents() {
        Display *disp = FbTk::App::instance()->display();
        XSelectInput(disp, window, StructureNotifyMask |
                 SubstructureNotifyMask | EnterWindowMask);
    }
};

namespace { 

class SlitClientMenuItem: public FbTk::MenuItem {
public:
    explicit SlitClientMenuItem(SlitClient &client, FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(client.match_name.c_str(), cmd), m_client(client) {
        FbTk::MenuItem::setSelected(client.visible);
    }
    const std::string &label() const {
        return m_client.match_name;
    }
    bool isSelected() const {
        return m_client.visible;
    }
    void click(int button, int time) { 
        m_client.visible = !m_client.visible;
        FbTk::MenuItem::click(button, time);
    }
private:
    SlitClient &m_client;
};

class SlitDirMenuItem: public FbTk::MenuItem {
public:
    SlitDirMenuItem(const char *label, Slit &slit):FbTk::MenuItem(label), 
                                                   m_slit(slit), 
                                                   m_label(label ? label : "") { 
        setLabel(m_label.c_str()); // update label
    }
    void click(int button, int time) {
        // toggle direction
        if (m_slit.direction() == Slit::HORIZONTAL)
            m_slit.setDirection(Slit::VERTICAL);
        else
            m_slit.setDirection(Slit::HORIZONTAL);
        setLabel(m_label.c_str());
    }

    void setLabel(const char *label) {
        I18n *i18n = I18n::instance();
        m_label = (label ? label : "");
        std::string reallabel = m_label + " " + 
            ( m_slit.direction() == Slit::HORIZONTAL ? 
              i18n->getMessage(
                               FBNLS::CommonSet, FBNLS::CommonDirectionHoriz,
                               "Horizontal") :
              i18n->getMessage(
                               FBNLS::CommonSet, FBNLS::CommonDirectionVert,
                               "Vertical") );
        FbTk::MenuItem::setLabel(reallabel.c_str());
    }
private:
    Slit &m_slit;
    std::string m_label;
};

class PlaceSlitMenuItem: public FbTk::MenuItem {
public:
    PlaceSlitMenuItem(const char *label, Slit &slit, Slit::Placement place):
        FbTk::MenuItem(label), m_slit(slit), m_place(place) {
     
    }
    bool isEnabled() const { return m_slit.placement() != m_place; }
    void click(int button, int time) {
        m_slit.setPlacement(m_place);
    }
private:
    Slit &m_slit;
    Slit::Placement m_place;
};


}; // End anonymous namespace

Slit::Slit(BScreen &scr, const char *filename):
    m_screen(&scr), timer(this), 
    slitmenu(*scr.menuTheme(), 
             scr.getScreenNumber(), 
             *scr.getImageControl()),
    placement_menu(*scr.menuTheme(),
                   scr.getScreenNumber(),
                   *scr.getImageControl()),
    clientlist_menu(*scr.menuTheme(),
                    scr.getScreenNumber(),
                    *scr.getImageControl()) {

    // default placement and direction
    m_direction = HORIZONTAL;
    m_placement = TOPLEFT;
    on_top = false;
    hidden = do_auto_hide = false;

    frame.pixmap = None;

    timer.setTimeout(200); // default timeout
    timer.fireOnce(true);

    XSetWindowAttributes attrib;
    unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
        CWColormap | CWOverrideRedirect | CWEventMask;
    attrib.background_pixmap = None;
    attrib.background_pixel = attrib.border_pixel =
        screen()->getBorderColor()->pixel();
    attrib.colormap = screen()->colormap();
    attrib.override_redirect = True;
    attrib.event_mask = SubstructureRedirectMask | ButtonPressMask |
        EnterWindowMask | LeaveWindowMask;

    frame.x = frame.y = 0;
    frame.width = frame.height = 1;
    Display *disp = FbTk::App::instance()->display();
    frame.window =
        XCreateWindow(disp, screen()->getRootWindow(), frame.x, frame.y,
                      frame.width, frame.height, screen()->getBorderWidth(),
                      screen()->getDepth(), InputOutput, screen()->getVisual(),
                      create_mask, &attrib);

    FbTk::EventManager::instance()->add(*this, frame.window);

    //For KDE dock applets
    kwm1_dockwindow = XInternAtom(disp, "KWM_DOCKWINDOW", False); //KDE v1.x
    kwm2_dockwindow = XInternAtom(disp, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False); //KDE v2.x
    
    // Get client list for sorting purposes
    loadClientList(filename);

    setupMenu();

    reconfigure();
}


Slit::~Slit() {
    if (frame.pixmap != 0)
        screen()->getImageControl()->removeImage(frame.pixmap);
}


void Slit::addClient(Window w) {
#ifdef DEBUG
    cerr<<"Slit::addClient()"<<endl;
#endif // DEBUG
    //Can't add non existent window
    if (w == None)
        return;

    // Look for slot in client list by name
    SlitClient *client = 0;
    std::string match_name;
    ::getWMName(screen(), w, match_name);
    SlitClients::iterator it = clientList.begin();
    SlitClients::iterator it_end = clientList.end();
    bool found_match = false;
    for (; it != it_end; ++it) {
        // If the name matches...
        if ((*it)->match_name == match_name) {
            // Use the slot if no window is assigned
            if ((*it)->window == None) {
                client = (*it);
                client->initialize(screen(), w);
                break;
            }
            // Otherwise keep looking for an unused match or a non-match
            found_match = true;		// Possibly redundant
			
        } else if (found_match) {
            // Insert before first non-match after a previously found match?
            client = new SlitClient(screen(), w);
            clientList.insert(it, client);
            break;
        }
    }
    // Append to client list?
    if (client == 0) {
        client = new SlitClient(screen(), w);
        clientList.push_back(client);
    }

    Display *disp = FbTk::App::instance()->display();
    XWMHints *wmhints = XGetWMHints(disp, w);

    if (wmhints != 0) {
        if ((wmhints->flags & IconWindowHint) &&
            (wmhints->icon_window != None)) {
            XMoveWindow(disp, client->client_window, screen()->getWidth() + 10,
                        screen()->getHeight() + 10);
            XMapWindow(disp, client->client_window);				
            client->icon_window = wmhints->icon_window;
            client->window = client->icon_window;
        } else {
            client->icon_window = None;
            client->window = client->client_window;
        }

        XFree(wmhints);
    } else {
        client->icon_window = None;
        client->window = client->client_window;
    }

    XWindowAttributes attrib;
    
#ifdef KDE

    //Check and see if new client is a KDE dock applet
    //If so force reasonable size
    bool iskdedockapp=false;
    Atom ajunk;
    int ijunk;
    unsigned long *data = (unsigned long *) 0, uljunk;

    // Check if KDE v2.x dock applet
    if (XGetWindowProperty(disp, w,
                           kwm2_dockwindow, 0l, 1l, False,
                           kwm2_dockwindow,
                           &ajunk, &ijunk, &uljunk, &uljunk,
                           (unsigned char **) &data) == Success) {
        iskdedockapp = (data && data[0] != 0);
        XFree((char *) data);
    }

    // Check if KDE v1.x dock applet
    if (!iskdedockapp) {
        if (XGetWindowProperty(disp, w,
                               kwm1_dockwindow, 0l, 1l, False,
                               kwm1_dockwindow,
                               &ajunk, &ijunk, &uljunk, &uljunk,
                               (unsigned char **) &data) == Success) {
            iskdedockapp = (data && data[0] != 0);
            XFree((char *) data);
        }
    }

    if (iskdedockapp)
        client->width = client->height = 24;
    else
#endif // KDE
    
        {
            if (XGetWindowAttributes(disp, client->window, &attrib)) {
                client->width = attrib.width;
                client->height = attrib.height;
            } else { // set default size if we failed to get window attributes
                client->width = client->height = 64;
            }
        }

    // disable border for client window
    XSetWindowBorderWidth(disp, client->window, 0);

    // disable events to frame.window
    frame.window.setEventMask(NoEventMask);
    client->disableEvents();

    XReparentWindow(disp, client->window, frame.window.window(), 0, 0);
    XMapRaised(disp, client->window);
    XChangeSaveSet(disp, client->window, SetModeInsert);
    // reactivate events for frame.window
    frame.window.setEventMask(SubstructureRedirectMask |
                              ButtonPressMask | EnterWindowMask | LeaveWindowMask);
    // setup event for slit client window
    client->enableEvents();
    // flush events
    XFlush(disp);
    // add slit client to eventmanager
    FbTk::EventManager::instance()->add(*this, client->client_window);
    FbTk::EventManager::instance()->add(*this, client->icon_window);

    frame.window.show();
    frame.window.clear();
    reconfigure();

    updateClientmenu();

    saveClientList();

}

void Slit::setDirection(Direction dir) {
    m_direction = dir;
    reconfigure();
}

void Slit::setPlacement(Placement place) {
    m_placement = place;
    reconfigure();
}

void Slit::removeClient(SlitClient *client, bool remap, bool destroy) {
#ifdef DEBUG
    cerr<<"Slit::removeClient()"<<endl;
#endif // DEBUG
    // remove from event manager
    FbTk::EventManager::instance()->remove(client->client_window);
    FbTk::EventManager::instance()->remove(client->icon_window);

    // Destructive removal?
    if (destroy)
        clientList.remove(client);
    else // Clear the window info, but keep around to help future sorting?
        client->initialize();

    screen()->removeNetizen(client->window);

    if (remap) {
        Display *disp = FbTk::App::instance()->display();
        if (!client->visible)
            XMapWindow(disp, client->window);
        client->disableEvents();
        // stop events to frame.window temporarly
        frame.window.setEventMask(NoEventMask);
        XReparentWindow(disp, client->window, screen()->getRootWindow(),
			client->x, client->y);
        XChangeSaveSet(disp, client->window, SetModeDelete);
        // reactivate events to frame.window
        frame.window.setEventMask(SubstructureRedirectMask | ButtonPressMask |
                                  EnterWindowMask | LeaveWindowMask);
        XFlush(disp);
    }

    // Destructive removal?
    if (destroy)
        delete client;

    updateClientmenu();
}


void Slit::removeClient(Window w, bool remap) {

    bool reconf = false;

    SlitClients::iterator it = clientList.begin();
    SlitClients::iterator it_end = clientList.end();
    for (; it != it_end; ++it) {
        if ((*it)->window == w) {
            removeClient((*it), remap, false);
            reconf = true;

            break;
        }
    }
    if (reconf)
        reconfigure();

}


void Slit::reconfigure() {
    frame.width = 0;
    frame.height = 0;

    // Need to count windows because not all client list entries
    // actually correspond to mapped windows.
    int num_windows = 0;

    switch (direction()) {
    case VERTICAL: 
        {
            SlitClients::iterator it = clientList.begin();
            SlitClients::iterator it_end = clientList.end();
            for (; it != it_end; ++it) {
                //client created window?
                if ((*it)->window != None && (*it)->visible) {
                    num_windows++;
                    frame.height += (*it)->height + screen()->getBevelWidth();
					
                    //frame width < client window?
                    if (frame.width < (*it)->width) 
                        frame.width = (*it)->width;
                }
            }
        }

        if (frame.width < 1)
            frame.width = 1;
        else
            frame.width += (screen()->getBevelWidth() * 2);

        if (frame.height < 1)
            frame.height = 1;
        else
            frame.height += screen()->getBevelWidth();

        break;

    case HORIZONTAL: 
        {
            SlitClients::iterator it = clientList.begin();
            SlitClients::iterator it_end = clientList.end();
            for (; it != it_end; ++it) {
                //client created window?
                if ((*it)->window != None && (*it)->visible) {
                    num_windows++;
                    frame.width += (*it)->width + screen()->getBevelWidth();
                    //frame height < client height?
                    if (frame.height < (*it)->height)
                        frame.height = (*it)->height;
                }
            }
        }

        if (frame.width < 1)
            frame.width = 1;
        else
            frame.width += screen()->getBevelWidth();

        if (frame.height < 1)
            frame.height = 1;
        else
            frame.height += (screen()->getBevelWidth() * 2);

        break;
    }

    reposition();
    Display *disp = FbTk::App::instance()->display();

    frame.window.setBorderWidth(screen()->getBorderWidth());
    frame.window.setBorderColor(*screen()->getBorderColor());
    //did we actually use slit slots
    if (num_windows == 0)
        frame.window.hide();
    else
        frame.window.show();

    Pixmap tmp = frame.pixmap;
    FbTk::ImageControl *image_ctrl = screen()->getImageControl();
    const FbTk::Texture &texture = screen()->getTheme()->getSlitTexture();
    if (texture.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.pixmap = None;
        frame.window.setBackgroundColor(texture.color());
    } else {
        frame.pixmap = image_ctrl->renderImage(frame.width, frame.height,
                                               texture);
        frame.window.setBackgroundPixmap(frame.pixmap);
    }

    if (tmp) 
        image_ctrl->removeImage(tmp);

    frame.window.clear();

    int x, y;

    switch (direction()) {
    case VERTICAL:
        x = 0;
        y = screen()->getBevelWidth();

        {
            SlitClients::iterator it = clientList.begin();
            SlitClients::iterator it_end = clientList.end();
            for (; it != it_end; ++it) {
                if ((*it)->window == None)
                    continue;

                //client created window?
                if ((*it)->visible) 
                    XMapWindow(disp, (*it)->window);
                else {
                    (*it)->disableEvents();
                    XUnmapWindow(disp, (*it)->window);
                    (*it)->enableEvents();
                    continue;
                }

                x = (frame.width - (*it)->width) / 2;                

                XMoveResizeWindow(disp, (*it)->window, x, y,
                                  (*it)->width, (*it)->height);

                // for ICCCM compliance
                (*it)->x = x;
                (*it)->y = y;

                XEvent event;
                event.type = ConfigureNotify;

                event.xconfigure.display = disp;
                event.xconfigure.event = (*it)->window;
                event.xconfigure.window = (*it)->window;
                event.xconfigure.x = x;
                event.xconfigure.y = y;
                event.xconfigure.width = (*it)->width;
                event.xconfigure.height = (*it)->height;
                event.xconfigure.border_width = 0;
                event.xconfigure.above = frame.window.window();
                event.xconfigure.override_redirect = False;

                XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
                           &event);

                y += (*it)->height + screen()->getBevelWidth();
            }
        }

        break;

    case HORIZONTAL:
        x = screen()->getBevelWidth();
        y = 0;

        {
            SlitClients::iterator it = clientList.begin();
            SlitClients::iterator it_end = clientList.end();
            for (; it != it_end; ++it) {
                //client created window?
                if ((*it)->window == None)
                    continue;

                if ((*it)->visible) { 
                    XMapWindow(disp, (*it)->window);
                } else {
                    (*it)->disableEvents();
                    XUnmapWindow(disp, (*it)->window);
                    (*it)->enableEvents();
                    continue;
                }

                y = (frame.height - (*it)->height) / 2;

                XMoveResizeWindow(disp, (*it)->window, x, y,
                                  (*it)->width, (*it)->height);





                // for ICCCM compliance
                (*it)->x = x;
                (*it)->y = y;

                XEvent event;
                event.type = ConfigureNotify;

                event.xconfigure.display = disp;
                event.xconfigure.event = (*it)->window;
                event.xconfigure.window = (*it)->window;
                event.xconfigure.x = frame.x + x + screen()->getBorderWidth();
                event.xconfigure.y = frame.y + y + screen()->getBorderWidth();
                event.xconfigure.width = (*it)->width;
                event.xconfigure.height = (*it)->height;
                event.xconfigure.border_width = 0;
                event.xconfigure.above = frame.window.window();
                event.xconfigure.override_redirect = False;

                XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
                           &event);

                x += (*it)->width + screen()->getBevelWidth();
            }
        }

        break;
    }

    slitmenu.reconfigure();
    updateClientmenu();
}


void Slit::reposition() {
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

    head_w = screen()->getWidth();
    head_h = screen()->getHeight();

    // place the slit in the appropriate place
    switch (placement()) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        if (direction() == VERTICAL) {
            frame.x_hidden = screen()->getBevelWidth() -
                screen()->getBorderWidth() - frame.width;
            frame.y_hidden = head_y;
        } else {
            frame.x_hidden = head_x;
            frame.y_hidden = screen()->getBevelWidth() -
                screen()->getBorderWidth() - frame.height;
        }
        break;

    case CENTERLEFT:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height) / 2;
        frame.x_hidden = head_x + screen()->getBevelWidth() -
            screen()->getBorderWidth() - frame.width;
        frame.y_hidden = frame.y;
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_h - frame.height - screen()->getBorderWidth2x();
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + screen()->getBevelWidth() -
                screen()->getBorderWidth() - frame.width;
            frame.y_hidden = frame.y;
        } else {
            frame.x_hidden = head_x;
            frame.y_hidden = head_y + head_h -
                screen()->getBevelWidth() - screen()->getBorderWidth();
        }
        break;

    case TOPCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + screen()->getBevelWidth() -
            screen()->getBorderWidth() - frame.height;
        break;

    case BOTTOMCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h -
            screen()->getBevelWidth() - screen()->getBorderWidth();
        break;

    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
        frame.y = head_y;
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + head_w -
                screen()->getBevelWidth() - screen()->getBorderWidth();
            frame.y_hidden = head_y;
        } else {
            frame.x_hidden = frame.x;
            frame.y_hidden = head_y + screen()->getBevelWidth() -
                screen()->getBorderWidth() - frame.height;
        }
        break;

    case CENTERRIGHT:
    default:
        frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
        frame.y = head_y + ((head_h - frame.height) / 2);
        frame.x_hidden = head_x + head_w -
            screen()->getBevelWidth() - screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
        frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + head_w - 
                screen()->getBevelWidth() - screen()->getBorderWidth();
            frame.y_hidden = frame.y;
        } else {
            frame.x_hidden = frame.x;
            frame.y_hidden = head_y + head_h - 
                screen()->getBevelWidth() - screen()->getBorderWidth();
        }
        break;
    }

    if (hidden) {
        frame.window.moveResize(frame.x_hidden,
                                frame.y_hidden, frame.width, frame.height);
    } else {
        frame.window.moveResize(frame.x,
                                frame.y, frame.width, frame.height);
    }
}


void Slit::shutdown() {
    saveClientList();
    while (!clientList.empty())
        removeClient(clientList.front(), true, true);
}

void Slit::cycleClientsUp() {
    if (clientList.size() < 2)
        return;

    // rotate client list up, ie the first goes last
    SlitClients::iterator it = clientList.begin();
    SlitClient *client = *it;
    clientList.erase(it);
    clientList.push_back(client);
    reconfigure();
}

void Slit::cycleClientsDown() {
    if (clientList.size() < 2)
        return;

    // rotate client list down, ie the last goes first
    SlitClient *client = clientList.back();
    clientList.remove(client);
    clientList.push_front(client);
    reconfigure();
}

void Slit::handleEvent(XEvent &event) {
    if (event.type == DestroyNotify) {
        removeClient(event.xdestroywindow.window, false);
    } else if (event.type == UnmapNotify) {        
        removeClient(event.xany.window);
    } else if (event.type == MapRequest) {
#ifdef KDE
        //Check and see if client is KDE dock applet.
        //If so add to Slit
        bool iskdedockapp = false;
        Atom ajunk;
        int ijunk;
        unsigned long *data = (unsigned long *) 0, uljunk;
        Display *disp = FbTk::App::instance()->display();
        // Check if KDE v2.x dock applet
        if (XGetWindowProperty(disp, event.xmaprequest.window,
                               kwm2_dockwindow, 0l, 1l, False,
                               XA_WINDOW, &ajunk, &ijunk, &uljunk,
                               &uljunk, (unsigned char **) &data) == Success) {
					
            if (data)
                iskdedockapp = True;
            XFree((char *) data);
	
        }

        // Check if KDE v1.x dock applet
        if (!iskdedockapp) {
            if (XGetWindowProperty(disp, event.xmaprequest.window,
                                   kwm1_dockwindow, 0l, 1l, False,
                                   kwm1_dockwindow, &ajunk, &ijunk, &uljunk,
                                   &uljunk, (unsigned char **) &data) == Success) {
                iskdedockapp = (data && data[0] != 0);
                XFree((char *) data);
            }
        }

        if (iskdedockapp) {
            XSelectInput(disp, event.xmaprequest.window, StructureNotifyMask);
            addClient(event.xmaprequest.window);
        }
#endif //KDE
        
    }
}

void Slit::buttonPressEvent(XButtonEvent &e) {
    if (e.window != frame.window.window()) 
        return;

    if (e.button == Button1 && (! on_top)) {
        Workspace::Stack st;
        st.push_back(frame.window.window());
        screen()->raiseWindows(st);
    } else if (e.button == Button2 && (! on_top)) {
        frame.window.lower();
    } else if (e.button == Button3) {
        if (! slitmenu.isVisible()) {
            int x = e.x_root - (slitmenu.width() / 2),
                y = e.y_root - (slitmenu.height() / 2); 

            if (x < 0)
                x = 0;
            else if (x + slitmenu.width() > screen()->getWidth())
                x = screen()->getWidth() - slitmenu.width();

            if (y < 0)
                y = 0;
            else if (y + slitmenu.height() > screen()->getHeight())
                y = screen()->getHeight() - slitmenu.height();

            slitmenu.move(x, y);
            slitmenu.show();
        } else
            slitmenu.hide();
    } else if (e.button == 4) {
        cycleClientsUp();
    } else if (e.button == 5) {
        cycleClientsDown();
    }
}


void Slit::enterNotifyEvent(XCrossingEvent &) {
    if (! do_auto_hide)
        return;

    if (hidden) {
        if (! timer.isTiming()) 
            timer.start();
    } else {
        if (timer.isTiming()) 
            timer.stop();
    }
}


void Slit::leaveNotifyEvent(XCrossingEvent &ev) {
    if (! do_auto_hide)
        return;

    if (hidden) {
        if (timer.isTiming()) 
            timer.stop();
    } else if (! slitmenu.isVisible()) {
        if (! timer.isTiming()) 
            timer.start();
    }

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

    SlitClients::iterator it = clientList.begin();
    SlitClients::iterator it_end = clientList.end();
    for (; it != it_end; ++it) {
        if ((*it)->window == event.window) {
            if ((*it)->width != ((unsigned) event.width) ||
                (*it)->height != ((unsigned) event.height)) {
                (*it)->width = (unsigned) event.width;
                (*it)->height = (unsigned) event.height;

                reconf = true; //requires reconfiguration

                break;
            }
        }
    }

    if (reconf) 
        reconfigure();
}


void Slit::timeout() {
    hidden = ! hidden; // toggle hidden state
    if (hidden)
        frame.window.move(frame.x_hidden, frame.y_hidden);
    else
        frame.window.move(frame.x, frame.y);
}

void Slit::loadClientList(const char *filename) {
    if (filename == 0)
        return;

    m_filename = filename; // save filename so we can save client list later

    struct stat buf;
    if (!stat(filename, &buf)) {
        std::ifstream file(filename);
        std::string name;
        while (! file.eof()) {
            name = "";
            std::getline(file, name); // get the entire line
            if (name.size() > 0) { // don't add client unless we have a valid line
                SlitClient *client = new SlitClient(name.c_str());
                clientList.push_back(client);
            }
        }
    }
}

void Slit::updateClientmenu() {
    // clear old items
    clientlist_menu.removeAll();
    clientlist_menu.setLabel("Clients");
    FbTk::RefCount<FbTk::Command> cycle_up(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsUp));
    FbTk::RefCount<FbTk::Command> cycle_down(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsDown));
    clientlist_menu.insert("Cycle Up", cycle_up);
    clientlist_menu.insert("Cycle Down", cycle_down);
    FbTk::MenuItem *separator = new FbTk::MenuItem("-------");
    separator->setEnabled(false);
    clientlist_menu.insert(separator);
    FbTk::RefCount<FbTk::Command> reconfig(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));
    SlitClients::iterator it = clientList.begin();
    for (; it != clientList.end(); ++it) {
        if ((*it) != 0)
            clientlist_menu.insert(new SlitClientMenuItem(*(*it), reconfig));
    }

    clientlist_menu.update();
}

void Slit::saveClientList() {

    std::ofstream file(m_filename.c_str());
    SlitClients::iterator it = clientList.begin();
    SlitClients::iterator it_end = clientList.end();
    std::string prevName;
    std::string name;
    for (; it != it_end; ++it) {
        name = (*it)->match_name;
        if (name != prevName)
            file << name.c_str() << std::endl;

        prevName = name;
    }
}

void Slit::setOnTop(bool val) {
    if (isOnTop())
        screen()->raiseWindows(Workspace::Stack());

}


void Slit::setAutoHide(bool val) {
    do_auto_hide = val;
}

void Slit::setupMenu() {
    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    using namespace FbTk;

    RefCount<Command> menu_cmd(new SimpleCommand<Slit>(*this, &Slit::reconfigure));
    // setup base menu
    slitmenu.setLabel("Slit");
    slitmenu.insert(i18n->getMessage(
                                     CommonSet, CommonPlacementTitle,
                                     "Placement"),
                    &placement_menu);
    slitmenu.insert(new BoolMenuItem(i18n->getMessage(
                                                      CommonSet, CommonAlwaysOnTop,
                                                      "Always on top"),
                                     on_top,
                                     menu_cmd));

    slitmenu.insert(new BoolMenuItem(i18n->getMessage(
                                                      CommonSet, CommonAutoHide,
                                                      "Auto hide"),
                                     do_auto_hide,
                                     menu_cmd));

    slitmenu.insert(new SlitDirMenuItem(i18n->getMessage(
                                                         SlitSet, SlitSlitDirection,
                                                         "Slit Direction"), *this));
    slitmenu.insert("Clients", &clientlist_menu);
    slitmenu.update();

    // setup sub menu

    
    placement_menu.setLabel(i18n->getMessage(
                                             SlitSet, SlitSlitPlacement,
                                             "Slit Placement"));
    placement_menu.setMinimumSublevels(3);
    placement_menu.setInternalMenu();
   

    // setup items in sub menu
    struct {
        int set;
        int base;
        const char *default_str;
        Placement slit_placement;
    } place_menu[]  = {
        {CommonSet, CommonPlacementTopLeft, "Top Left", Slit::TOPLEFT},
        {CommonSet, CommonPlacementCenterLeft, "Center Left", Slit::CENTERLEFT},
        {CommonSet, CommonPlacementBottomLeft, "Bottom Left", Slit::BOTTOMLEFT},
        {CommonSet, CommonPlacementTopCenter, "Top Center", Slit::TOPCENTER},
        {0, 0, 0, Slit::TOPLEFT}, // middle item, empty
        {CommonSet, CommonPlacementBottomCenter, "Bottom Center", Slit::BOTTOMCENTER},
        {CommonSet, CommonPlacementTopRight, "Top Right", Slit::TOPRIGHT},
        {CommonSet, CommonPlacementCenterRight, "Center Right", Slit::CENTERRIGHT},
        {CommonSet, CommonPlacementBottomRight, "Bottom Right", Slit::BOTTOMRIGHT}
    };
    // create items in sub menu
    for (size_t i=0; i<9; ++i) {
        if (place_menu[i].default_str == 0) {
            placement_menu.insert("");
        } else {
            const char *i18n_str = i18n->getMessage(place_menu[i].set, 
                                                    place_menu[i].base,
                                                    place_menu[i].default_str);
            placement_menu.insert(new PlaceSlitMenuItem(i18n_str, *this,
                                                        place_menu[i].slit_placement));
        }
    }
    // finaly update sub menu
    placement_menu.update();
}
