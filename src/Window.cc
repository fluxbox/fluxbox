// Window.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Window.cc,v 1.119 2003/02/17 22:42:52 fluxgen Exp $

#include "Window.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "StringUtil.hh"
#include "Netizen.hh"
#include "FbWinFrameTheme.hh"
#include "MenuTheme.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

namespace {

void grabButton(Display *display, unsigned int button, 
                               Window window, Cursor cursor) {

    //numlock
    XGrabButton(display, button, Mod1Mask|Mod2Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);
    //scrolllock
    XGrabButton(display, button, Mod1Mask|Mod5Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);
	
    //capslock
    XGrabButton(display, button, Mod1Mask|LockMask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock+numlock
    XGrabButton(display, Button1, Mod1Mask|LockMask|Mod2Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //capslock+scrolllock
    XGrabButton(display, button, Mod1Mask|LockMask|Mod5Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);
	
    //capslock+numlock+scrolllock
    XGrabButton(display, button, Mod1Mask|LockMask|Mod2Mask|Mod5Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);

    //numlock+scrollLock
    XGrabButton(display, button, Mod1Mask|Mod2Mask|Mod5Mask, window, True,
                ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                GrabModeAsync, None, cursor);
	
}

};

FluxboxWindow::FluxboxWindow(Window w, BScreen *s, int screen_num, 
                             FbTk::ImageControl &imgctrl, FbWinFrameTheme &tm,
                             FbTk::MenuTheme &menutheme, 
                             FbTk::XLayer &layer):
    m_hintsig(*this),
    m_statesig(*this),
    m_layersig(*this),
    m_workspacesig(*this),
    m_diesig(*this),
    moving(false), resizing(false), shaded(false), maximized(false),
    visible(false), iconic(false), transient(false), focused(false),
    stuck(false), modal(false), send_focus_message(false), m_managed(false),
    screen(s),
    timer(this),
    display(0),
    lastButtonPressTime(0),
    m_windowmenu(menutheme, screen_num, imgctrl),
    m_layermenu(menutheme, screen_num, imgctrl),
    old_decoration(DECOR_NORMAL),
    tab(0),
    m_frame(tm, imgctrl, screen_num, 0, 0, 100, 100),
    m_layeritem(getFrameWindow(), layer),
    m_layernum(layer.getLayerNum()) {



    // redirect events from frame to us
    m_frame.setEventHandler(*this); 

    lastFocusTime.tv_sec = lastFocusTime.tv_usec = 0;

    // display connection
    display = FbTk::App::instance()->display();

    blackbox_attrib.workspace = workspace_number = window_number = -1;

    blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack = 0;
    blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
    blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

    //use tab as default
    decorations.tab = true;
    // enable decorations
    decorations.enabled = true;
    // set client window
    client.window = w;

    // set default values for decoration
    decorations.menu = true;	//override menu option
    // all decorations on by default
    decorations.titlebar = decorations.border = decorations.handle = true;
    decorations.maximize = decorations.close = decorations.sticky = decorations.shade =
	decorations.tab = true;


    functions.resize = functions.move = functions.iconify = functions.maximize = true;
    functions.close = decorations.close = false;

    client.wm_hint_flags = client.normal_hint_flags = 0;
    client.transient_for = 0;
    client.mwm_hint = 0;
    client.blackbox_hint = 0;

    getBlackboxHints();
    if (! client.blackbox_hint) {
        getMWMHints();
    }
    
    // get size, aspect, minimum/maximum size and other hints set
    // by the client

    getWMProtocols();
    getWMHints(); 
    getWMNormalHints();

    // fetch client size and placement
    XWindowAttributes wattrib;
    if ((! XGetWindowAttributes(display, client.window, &wattrib)) ||
        !wattrib.screen // no screen?
        || wattrib.override_redirect) { // override redirect
        return;
    }

    // save old border width so we can restore it later
    client.old_bw = wattrib.border_width;
    client.x = wattrib.x; client.y = wattrib.y;
    client.width = wattrib.width;
    client.height = wattrib.height;

    m_frame.move(wattrib.x, wattrib.y);
    m_frame.resizeForClient(wattrib.width, wattrib.height);

    Fluxbox *fluxbox = Fluxbox::instance();

    timer.setTimeout(fluxbox->getAutoRaiseDelay());
    timer.fireOnce(true);

    if (client.initial_state == WithdrawnState) {
        return;
    }

    m_managed = true; //this window is managed
	
    // update transient infomation
    updateTransientInfo();
	
    // adjust the window decorations based on transience and window sizes
    if (transient) {
        decorations.maximize =  functions.maximize = false;
        decorations.handle = decorations.border = false;
    }	
	
    if ((client.normal_hint_flags & PMinSize) &&
        (client.normal_hint_flags & PMaxSize) &&
        client.max_width != 0 && client.max_width <= client.min_width &&
        client.max_height != 0 && client.max_height <= client.min_height) {
        decorations.maximize = decorations.handle =
            functions.resize = functions.maximize = false;
        decorations.tab = false; //no tab for this window
    }

    upsize();

    bool place_window = true;
    if (fluxbox->isStartup() || transient ||
        client.normal_hint_flags & (PPosition|USPosition)) {
        setGravityOffsets();

        if (! fluxbox->isStartup()) {

            int real_x = m_frame.x();
            int real_y = m_frame.y();

            if (decorations.tab) {
                if (screen->getTabPlacement() == Tab::PTOP) {
                    real_y -= screen->getTabHeight();
                } else if (screen->getTabPlacement() == Tab::PLEFT) {
                    real_x -= (screen->isTabRotateVertical())
                        ? screen->getTabHeight()
                        : screen->getTabWidth();
                }
            }

            if (real_x >= 0 && 
                real_y + m_frame.y() >= 0 &&
                real_x <= (signed) screen->getWidth() &&
                real_y <= (signed) screen->getHeight())
                place_window = false;

        } else
            place_window = false;

    }

    associateClientWindow();

    grabButtons();
		
    positionWindows();



    if (workspace_number < 0 || workspace_number >= screen->getCount())
        workspace_number = screen->getCurrentWorkspaceID();

    restoreAttributes();

    moveToLayer(m_layernum);
    screen->getWorkspace(workspace_number)->addWindow(this, place_window);

    moveResize(m_frame.x(), m_frame.y(), m_frame.width(), m_frame.height());

    if (shaded) { // start shaded
        shaded = false;
        shade();
    }

    if (maximized && functions.maximize) { // start maximized
        maximized = false;
        maximize();
    }	

    if (stuck) {
        stuck = false;
        stick();
        deiconify(); //we're omnipresent and visible
    }

    setState(current_state);

    // no focus default
    setFocusFlag(false);

}


FluxboxWindow::~FluxboxWindow() {
    // notify die
    m_diesig.notify();

    if (screen == 0) //the window wasn't created 
        return;

    timer.stop();

    Fluxbox *fluxbox = Fluxbox::instance();
	
    if (moving || resizing) {
        screen->hideGeometry();
        XUngrabPointer(display, CurrentTime);
    }
	
    if (!iconic) {
        Workspace *workspace = screen->getWorkspace(workspace_number);
        if (workspace)
            workspace->removeWindow(this);
    } else //it's iconic
        screen->removeIcon(this);
	
    if (tab != 0) {
        delete tab;	
        tab = 0;
    }
	
    if (client.mwm_hint != 0) {
        XFree(client.mwm_hint);
        client.mwm_hint = 0;
    }

    if (client.blackbox_hint != 0) {
        XFree(client.blackbox_hint);
        client.blackbox_hint = 0;
    }


    if (client.transient_for != 0) {
        if (client.transient_for == this) {
            client.transient_for = 0;
        }

        fluxbox->setFocusedWindow(client.transient_for);

        if (client.transient_for) {
            client.transient_for->client.transients.remove(this);			
            client.transient_for->setInputFocus();
            client.transient_for = 0;
        }
    }
	
    while (!client.transients.empty()) {
        client.transients.back()->client.transient_for = 0;
        client.transients.pop_back();
    }
	
    if (client.window_group) {
        fluxbox->removeGroupSearch(client.window_group);
        client.window_group = 0;
    }

    if (client.window)
        fluxbox->removeWindowSearch(client.window);

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ~FluxboxWindow("<<this<<")"<<endl;
#endif // DEBUG
}

bool FluxboxWindow::isGroupable() const {
    if (isResizable() && isMaximizable() && !isTransient())
        return true;
    return false;
}

void FluxboxWindow::associateClientWindow() {
    XSetWindowBorderWidth(display, client.window, 0);
    updateTitleFromClient();
    updateIconNameFromClient();

    m_frame.setClientWindow(client.window);

    // make sure the frame reconfigures
    m_frame.reconfigure();
}


void FluxboxWindow::grabButtons() {
    Fluxbox *fluxbox = Fluxbox::instance();

    XGrabButton(display, Button1, AnyModifier, 
		m_frame.clientArea().window(), True, ButtonPressMask,
		GrabModeSync, GrabModeSync, None, None);		
    XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask, m_frame.clientArea().window());


    XGrabButton(display, Button1, Mod1Mask, m_frame.window().window(), True,
		ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
		GrabModeAsync, None, fluxbox->getMoveCursor());

    //----grab with "all" modifiers
    grabButton(display, Button1, m_frame.window().window(), fluxbox->getMoveCursor());
	
    XGrabButton(display, Button2, Mod1Mask, m_frame.window().window(), True,
		ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
		
    XGrabButton(display, Button3, Mod1Mask, m_frame.window().window(), True,
		ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
		GrabModeAsync, None, fluxbox->getLowerRightAngleCursor());
	
    //---grab with "all" modifiers
    grabButton(display, Button3, m_frame.window().window(), fluxbox->getLowerRightAngleCursor());
}


void FluxboxWindow::reconfigure() {
    
    upsize();

    positionWindows();

    setFocusFlag(focused);

    moveResize(m_frame.x(), m_frame.y(), m_frame.width(), m_frame.height());
	
    grabButtons();

    m_frame.setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());

    m_windowmenu.reconfigure();
	
}


void FluxboxWindow::positionWindows() {

    m_frame.window().setBorderWidth(screen->getBorderWidth());
    m_frame.clientArea().setBorderWidth(screen->getFrameWidth());

    if (decorations.titlebar) {
        m_frame.titlebar().setBorderWidth(screen->getBorderWidth());
        m_frame.showTitlebar();
    } else
        m_frame.hideTitlebar();

    if (decorations.handle) {
        m_frame.handle().setBorderWidth(screen->getBorderWidth());
        m_frame.gripLeft().setBorderWidth(screen->getBorderWidth());
        m_frame.gripRight().setBorderWidth(screen->getBorderWidth());
        m_frame.showHandle();
    } else 
        m_frame.hideHandle();
	
    m_frame.reconfigure();
	
    if (tab)
        tab->setPosition();
}


void FluxboxWindow::updateTitleFromClient() {

    XTextProperty text_prop;
    char **list;
    int num;
    I18n *i18n = I18n::instance();

    if (XGetWMName(display, client.window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    client.title = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    client.title = (char *)text_prop.value;
					
            } else
                client.title = (char *)text_prop.value;
            XFree((char *) text_prop.value);
        } else { // ok, we don't have a name, set default name
            client.title = i18n->getMessage(
                                            FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                            "Unnamed");
        }
    } else {
        client.title = i18n->getMessage(
                                        FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                        "Unnamed");
    }

    m_frame.setTitle(client.title);	
}


void FluxboxWindow::updateIconNameFromClient() {

    XTextProperty text_prop;
    char **list;
    int num;

    if (XGetWMIconName(display, client.window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
                text_prop.nitems = strlen((char *) text_prop.value);

                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    client.icon_title = (char *)*list;
                    XFreeStringList(list);
                } else
                    client.icon_title = (char *)text_prop.value;
            } else
                client.icon_title = (char *)text_prop.value;

            XFree((char *) text_prop.value);
        } else
            client.icon_title = getTitle();
    } else
        client.icon_title = getTitle();
	
}


void FluxboxWindow::getWMProtocols() {
    Atom *proto = 0;
    int num_return = 0;
    Fluxbox *fluxbox = Fluxbox::instance();

    if (XGetWMProtocols(display, client.window, &proto, &num_return)) {

        for (int i = 0; i < num_return; ++i) {
            if (proto[i] == fluxbox->getWMDeleteAtom())
                functions.close = true;
            else if (proto[i] == fluxbox->getWMTakeFocusAtom())
                send_focus_message = true;
            else if (proto[i] == fluxbox->getFluxboxStructureMessagesAtom())
                screen->addNetizen(new Netizen(screen, client.window));
        }

        XFree(proto);
    } else {
        cerr<<"Warning: Failed to read WM Protocols"<<endl;
    }

}


void FluxboxWindow::getWMHints() {
    XWMHints *wmhint = XGetWMHints(display, client.window);
    if (! wmhint) {
        visible = true;
        iconic = false;
        focus_mode = F_PASSIVE;
        client.window_group = None;
        client.initial_state = NormalState;
    } else {
        client.wm_hint_flags = wmhint->flags;
        if (wmhint->flags & InputHint) {
            if (wmhint->input) {
                if (send_focus_message)
                    focus_mode = F_LOCALLYACTIVE;
                else
                    focus_mode = F_PASSIVE;
            } else {
                if (send_focus_message)
                    focus_mode = F_GLOBALLYACTIVE;
                else
                    focus_mode = F_NOINPUT;
            }
        } else
            focus_mode = F_PASSIVE;

        if (wmhint->flags & StateHint)
            client.initial_state = wmhint->initial_state;
        else
            client.initial_state = NormalState;

        if (wmhint->flags & WindowGroupHint) {
            if (! client.window_group) {
                client.window_group = wmhint->window_group;
                Fluxbox::instance()->saveGroupSearch(client.window_group, this);
            }
        } else
            client.window_group = None;

        XFree(wmhint);
    }
}


void FluxboxWindow::getWMNormalHints() {
    long icccm_mask;
    XSizeHints sizehint;
    if (! XGetWMNormalHints(display, client.window, &sizehint, &icccm_mask)) {
        client.min_width = client.min_height =
            client.base_width = client.base_height =
            client.width_inc = client.height_inc = 1;
        client.max_width = 0; // unbounded
        client.max_height = 0;
        client.min_aspect_x = client.min_aspect_y =
            client.max_aspect_x = client.max_aspect_y = 1;
        client.win_gravity = NorthWestGravity;
    } else {
        client.normal_hint_flags = sizehint.flags;

        if (sizehint.flags & PMinSize) {
            client.min_width = sizehint.min_width;
            client.min_height = sizehint.min_height;
        } else
            client.min_width = client.min_height = 1;

        if (sizehint.flags & PMaxSize) {
            client.max_width = sizehint.max_width;
            client.max_height = sizehint.max_height;
        } else {
            client.max_width = 0; // unbounded
            client.max_height = 0;
        }

        if (sizehint.flags & PResizeInc) {
            client.width_inc = sizehint.width_inc;
            client.height_inc = sizehint.height_inc;
        } else
            client.width_inc = client.height_inc = 1;

        if (sizehint.flags & PAspect) {
            client.min_aspect_x = sizehint.min_aspect.x;
            client.min_aspect_y = sizehint.min_aspect.y;
            client.max_aspect_x = sizehint.max_aspect.x;
            client.max_aspect_y = sizehint.max_aspect.y;
        } else
            client.min_aspect_x = client.min_aspect_y =
                client.max_aspect_x = client.max_aspect_y = 1;

        if (sizehint.flags & PBaseSize) {
            client.base_width = sizehint.base_width;
            client.base_height = sizehint.base_height;
        } else
            client.base_width = client.base_height = 0;

        if (sizehint.flags & PWinGravity)
            client.win_gravity = sizehint.win_gravity;
        else
            client.win_gravity = NorthWestGravity;
    }
}


void FluxboxWindow::getMWMHints() {
    int format;
    Atom atom_return;
    unsigned long num, len;
    Fluxbox *fluxbox = Fluxbox::instance();
    if (!XGetWindowProperty(display, client.window,
                            fluxbox->getMotifWMHintsAtom(), 0,
                            PropMwmHintsElements, false,
                            fluxbox->getMotifWMHintsAtom(), &atom_return,
                            &format, &num, &len,
                            (unsigned char **) &client.mwm_hint) == Success &&
        client.mwm_hint) {
        return;
    }
    if (num != PropMwmHintsElements)
        return;
	
    if (client.mwm_hint->flags & MwmHintsDecorations) {
        if (client.mwm_hint->decorations & MwmDecorAll) {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.close = decorations.menu = true;
        } else {
            decorations.titlebar = decorations.handle = decorations.border =
                decorations.iconify = decorations.maximize =
                decorations.close = decorations.tab = false;
            decorations.menu = true;
            if (client.mwm_hint->decorations & MwmDecorBorder)
                decorations.border = true;
            if (client.mwm_hint->decorations & MwmDecorHandle)
                decorations.handle = true;
            if (client.mwm_hint->decorations & MwmDecorTitle)
                decorations.titlebar = decorations.tab = true; //only tab on windows with titlebar
            if (client.mwm_hint->decorations & MwmDecorMenu)
                decorations.menu = true;
            if (client.mwm_hint->decorations & MwmDecorIconify)
                decorations.iconify = true;
            if (client.mwm_hint->decorations & MwmDecorMaximize)
                decorations.maximize = true;
        }
    }
	
    if (client.mwm_hint->flags & MwmHintsFunctions) {
        if (client.mwm_hint->functions & MwmFuncAll) {
            functions.resize = functions.move = functions.iconify =
                functions.maximize = functions.close = true;
        } else {
            functions.resize = functions.move = functions.iconify =
                functions.maximize = functions.close = false;

            if (client.mwm_hint->functions & MwmFuncResize)
                functions.resize = true;
            if (client.mwm_hint->functions & MwmFuncMove)
                functions.move = true;
            if (client.mwm_hint->functions & MwmFuncIconify)
                functions.iconify = true;
            if (client.mwm_hint->functions & MwmFuncMaximize)
                functions.maximize = true;
            if (client.mwm_hint->functions & MwmFuncClose)
                functions.close = true;
        }
    }
	
	
}


void FluxboxWindow::getBlackboxHints() {
    int format;
    Atom atom_return;
    unsigned long num, len;
    FbAtoms *atoms = FbAtoms::instance();
	
    if (XGetWindowProperty(display, client.window,
                           atoms->getFluxboxHintsAtom(), 0,
                           PropBlackboxHintsElements, False,
                           atoms->getFluxboxHintsAtom(), &atom_return,
                           &format, &num, &len,
                           (unsigned char **) &client.blackbox_hint) == Success &&
        client.blackbox_hint) {

        if (num == PropBlackboxHintsElements) {
            if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_SHADED)
                shaded = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_SHADED);

            if ((client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
                (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT))
                maximized = ((client.blackbox_hint->attrib &
                              (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?	1 : 0);
            else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT)
                maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
            else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ)
                maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);

            if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_OMNIPRESENT)
                stuck = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_OMNIPRESENT);

            if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_WORKSPACE)
                workspace_number = client.blackbox_hint->workspace;

            if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_STACK)
                workspace_number = client.blackbox_hint->stack;

            if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_DECORATION) {
                old_decoration = static_cast<Decoration>(client.blackbox_hint->decoration);
                setDecoration(old_decoration);
            }
        }
    }
}

void FluxboxWindow::move(int x, int y) {
    moveResize(x, y, m_frame.width(), m_frame.height());
}

void FluxboxWindow::resize(unsigned int width, unsigned int height) {
    moveResize(m_frame.x(), m_frame.y(), width, height);
}

void FluxboxWindow::moveResize(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height) {

    bool send_event = (m_frame.x() != new_x || m_frame.y() != new_y);

    if (new_width != m_frame.width() || new_height != m_frame.height()) {
        if ((((signed) m_frame.width()) + new_x) < 0) 
            new_x = 0;
        if ((((signed) m_frame.height()) + new_y) < 0) 
            new_y = 0;

        downsize();

        m_frame.moveResize(new_x, new_y, new_width, new_height);
        if (tab)
            tab->resize();

        positionWindows();
        setFocusFlag(focused);
        shaded = false;
        send_event = true;
    } else {
        m_frame.move(new_x, new_y);
        //move the tab and the chain		
        if (tab)
            tab->setPosition();
		
        // if (! moving)
        send_event = true;
    }

    if (send_event && ! moving) {
        /*
          Send event telling where the root position 
          of the client window is. (ie frame pos + client pos inside the frame = send pos)
        */

        client.width = m_frame.clientArea().width();
        client.height = m_frame.clientArea().height();
        client.x = m_frame.x();
        client.y = m_frame.y();

        XEvent event;
        event.type = ConfigureNotify;

        event.xconfigure.display = display;
        event.xconfigure.event = client.window;
        event.xconfigure.window = client.window;
        event.xconfigure.x = m_frame.x() + m_frame.clientArea().x();
        event.xconfigure.y = m_frame.y() + m_frame.clientArea().y();
        event.xconfigure.width = client.width;
        event.xconfigure.height = client.height;
        event.xconfigure.border_width = client.old_bw;
        event.xconfigure.above = m_frame.window().window();
        event.xconfigure.override_redirect = false;

        XSendEvent(display, client.window, False, StructureNotifyMask, &event);

        screen->updateNetizenConfigNotify(&event);
    }
}	

bool FluxboxWindow::setInputFocus() {

    //TODO hint skip focus
    if (((signed) (m_frame.x() + m_frame.width())) < 0) {
        if (((signed) (m_frame.y() + m_frame.height())) < 0) {
            moveResize(screen->getBorderWidth(), screen->getBorderWidth(),
                       m_frame.width(), m_frame.height());
        } else if (m_frame.y() > (signed) screen->getHeight()) {
            moveResize(screen->getBorderWidth(), screen->getHeight() - m_frame.height(),
                       m_frame.width(), m_frame.height());
        } else {
            moveResize(screen->getBorderWidth(), m_frame.y() + screen->getBorderWidth(),
                       m_frame.width(), m_frame.height());
        }
    } else if (m_frame.x() > (signed) screen->getWidth()) {
        if (((signed) (m_frame.y() + m_frame.height())) < 0) {
            moveResize(screen->getWidth() - m_frame.width(), screen->getBorderWidth(),
                       m_frame.width(), m_frame.height());
        } else if (m_frame.y() > (signed) screen->getHeight()) {
            moveResize(screen->getWidth() - m_frame.width(),
                       screen->getHeight() - m_frame.height(), m_frame.width(), m_frame.height());
        } else {
            moveResize(screen->getWidth() - m_frame.width(),
                       m_frame.y() + screen->getBorderWidth(), m_frame.width(), m_frame.height());
        }
    }

    if (! validateClient())
        return false;

    bool ret = false;

    if (client.transients.size() && modal) {
        std::list<FluxboxWindow *>::iterator it = client.transients.begin();
        std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
        for (; it != it_end; ++it) {
            if ((*it)->modal)
                return (*it)->setInputFocus();
        }
    } else {
        if (focus_mode == F_LOCALLYACTIVE || focus_mode == F_PASSIVE) {
            XSetInputFocus(display, client.window,
                           RevertToPointerRoot, CurrentTime);
        } else {
            return false;
        }

	m_frame.setFocus(true);
	
        Fluxbox *fb = Fluxbox::instance();
        fb->setFocusedWindow(this);
			
        if (send_focus_message) {
            XEvent ce;
            ce.xclient.type = ClientMessage;
            ce.xclient.message_type = fb->getWMProtocolsAtom();
            ce.xclient.display = display;
            ce.xclient.window = client.window;
            ce.xclient.format = 32;
            ce.xclient.data.l[0] = fb->getWMTakeFocusAtom();
            ce.xclient.data.l[1] = fb->getLastTime();
            ce.xclient.data.l[2] = 0l;
            ce.xclient.data.l[3] = 0l;
            ce.xclient.data.l[4] = 0l;
            XSendEvent(display, client.window, false, NoEventMask, &ce);
        }

        if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus())
            && screen->doAutoRaise())
            timer.start();

        ret = true;
    }

    return ret;
}

/**
   Enables or disables the tab on the window
*/
void FluxboxWindow::setTab(bool flag) {
    /*    if (flag) {
          if (!tab && isGroupable())
          tab = new Tab(this, 0, 0);
		
          if (tab) {
          tab->focus(); // draws the tab with correct texture
          tab->setPosition(); // set tab windows position
          }

          } else if (tab) {
          delete tab;
          tab = 0;		
          }	
          decorations.tab = flag;
    */
}

void FluxboxWindow::hide() {
    m_windowmenu.hide();
    m_frame.hide();
}

void FluxboxWindow::show() {
    m_frame.show();
}

/**
   Unmaps the window and removes it from workspace list
*/
void FluxboxWindow::iconify() {

    if (iconic) // no need to iconify if we're already
        return;

    m_windowmenu.hide();

    setState(IconicState);


    XSelectInput(display, client.window, NoEventMask);
    XUnmapWindow(display, client.window);
    XSelectInput(display, client.window,
                 PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

    m_frame.hide();

    visible = false;
    iconic = true;
	
    screen->getWorkspace(workspace_number)->removeWindow(this);

    if (client.transient_for) {
        if (! client.transient_for->iconic)
            client.transient_for->iconify();
    }
    screen->addIcon(this);

    if (tab) //if this window got a tab then iconify it too
        tab->iconify();
		
    if (client.transients.size()) {
        std::list<FluxboxWindow *>::iterator it = client.transients.begin();
        std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
        for (; it != it_end; ++it) {
            if (! (*it)->iconic)
                (*it)->iconify();
        }
    }

}


void FluxboxWindow::deiconify(bool reassoc, bool do_raise) {
    if (iconic || reassoc) {
        screen->reassociateWindow(this, screen->getCurrentWorkspace()->workspaceID(), false);
    } else if (workspace_number != screen->getCurrentWorkspace()->workspaceID())
        return;

    setState(NormalState);

    XSelectInput(display, client.window, NoEventMask);
    XMapWindow(display, client.window);
    XSelectInput(display, client.window,
                 PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

    m_frame.show();

    if (iconic && screen->doFocusNew())
        setInputFocus();

    if (focused != m_frame.focused())
        m_frame.setFocus(focused);

    visible = true;
    iconic = false;

    if (reassoc && client.transients.size()) {
        // deiconify all transients
        std::list<FluxboxWindow *>::iterator it = client.transients.begin();
        std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
        for (; it != it_end; ++it) {
            (*it)->deiconify(true, false);
        }
    }
	
    if (tab)
        tab->deiconify();
			
    if (do_raise)
	raise();
}

/**
   Send close request to client window
*/
void FluxboxWindow::close() {
    // fill in XClientMessage structure for delete message
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
    ce.xclient.display = FbTk::App::instance()->display();
    ce.xclient.window = client.window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = FbAtoms::instance()->getWMDeleteAtom();
    ce.xclient.data.l[1] = CurrentTime;
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    // send event delete message to client window
    XSendEvent(display, client.window, false, NoEventMask, &ce);
}

/**
 Set window in withdrawn state
*/
void FluxboxWindow::withdraw() {
    visible = false;
    iconic = false;

    if (isMoving())
        stopMoving();

    if (isResizing())
        stopResizing();

    m_frame.hide();

    m_windowmenu.hide();
	
    if (tab)
        tab->withdraw();
}

/**
   Maximize window both horizontal and vertical
*/
void FluxboxWindow::maximize() {
    if (isIconic())
        deiconify();

    if (!maximized) {
        // save old values
        m_old_width = frame().width();
        m_old_height = frame().height();
        m_old_pos_x = frame().x();
        m_old_pos_y = frame().y();
        unsigned int left_x = screen->getMaxLeft();
        unsigned int max_width = screen->getMaxRight();
        unsigned int max_top = screen->getMaxTop();
        moveResize(left_x, max_top, 
                   max_width - left_x, screen->getMaxBottom() - max_top - m_frame.window().borderWidth());
    } else { // demaximize, restore to old values
        moveResize(m_old_pos_x, m_old_pos_y,
                   m_old_width, m_old_height);
    }
    // toggle maximize
    maximized = !maximized;
}

void FluxboxWindow::maximizeHorizontal() {
    unsigned int left_x = screen->getMaxLeft();
    unsigned int max_width = screen->getMaxRight();
    moveResize(left_x, m_frame.y(), 
               max_width - left_x, m_frame.height() - m_frame.window().borderWidth());

}

/**
 Maximize window horizontal
 */
void FluxboxWindow::maximizeVertical() {
    unsigned int max_top = screen->getMaxTop();
    moveResize(m_frame.x(), max_top,
               m_frame.width() - m_frame.window().borderWidth(), screen->getMaxBottom() - max_top);
}


void FluxboxWindow::setWorkspace(int n) {

    workspace_number = n;

    blackbox_attrib.flags |= BaseDisplay::ATTRIB_WORKSPACE;
    blackbox_attrib.workspace = workspace_number;

    // notify workspace change
#ifdef DEBUG
    cerr<<this<<" notify workspace signal"<<endl;
#endif // DEBUG
    m_workspacesig.notify();
}

void FluxboxWindow::setLayerNum(int layernum) {
    m_layernum = layernum;

    blackbox_attrib.flags |= BaseDisplay::ATTRIB_STACK;
    blackbox_attrib.stack = layernum;
    saveBlackboxHints();

#ifdef DEBUG
    cerr<<this<<" notify layer signal"<<endl;
#endif // DEBUG

    m_layersig.notify();
}

void FluxboxWindow::shade() {
    if (!decorations.titlebar)
        return;

    // toggle shade on tab and frame
    m_frame.shade();
    if (tab)
        tab->shade();

    if (shaded) {
        shaded = false;
        blackbox_attrib.flags ^= BaseDisplay::ATTRIB_SHADED;
        blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_SHADED;

        setState(NormalState);
    } else {
        shaded = true;
        blackbox_attrib.flags |= BaseDisplay::ATTRIB_SHADED;
        blackbox_attrib.attrib |= BaseDisplay::ATTRIB_SHADED;

        setState(IconicState);
    }

}


void FluxboxWindow::stick() {

    if (tab) //if it got a tab then do tab's stick on all of the objects in the list
        tab->stick(); //this window will stick too.
    else if (stuck) {
        blackbox_attrib.flags ^= BaseDisplay::ATTRIB_OMNIPRESENT;
        blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_OMNIPRESENT;

        stuck = false;

    } else {
        stuck = true;
        if (screen->getCurrentWorkspaceID() != workspace_number) {
            screen->reassociateWindow(this,screen->getCurrentWorkspaceID(), true);
        }
		
        blackbox_attrib.flags |= BaseDisplay::ATTRIB_OMNIPRESENT;
        blackbox_attrib.attrib |= BaseDisplay::ATTRIB_OMNIPRESENT;

    }
    //TODO: make sure any button that listens to this state gets updated

    setState(current_state);
}

void FluxboxWindow::raise() {
    if (isIconic())
        deiconify();

    FluxboxWindow *win = this;

    while (win->getTransientFor()) {
        win = win->getTransientFor();
        assert(win != win->getTransientFor());
    }
  
    if (win == 0) 
        win = this;

    if (!win->isIconic()) {
        screen->updateNetizenWindowRaise(win->getClientWindow());
        win->getLayerItem().raise();
    }

    std::list<FluxboxWindow *>::const_iterator it = win->getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win->getTransients().end();
    for (; it != it_end; ++it) {
        if (!(*it)->isIconic()) {
            screen->updateNetizenWindowRaise((*it)->getClientWindow());
            (*it)->getLayerItem().raise();
        }
    }
}

void FluxboxWindow::lower() {
    if (isIconic())
        deiconify();

    FluxboxWindow *win = (FluxboxWindow *) 0, *bottom = this;

    while (bottom->getTransientFor()) {
        bottom = bottom->getTransientFor();
        assert(bottom != bottom->getTransientFor());
    }

    win = bottom;

    if (!win->isIconic()) {
        screen->updateNetizenWindowLower(win->getClientWindow());
        win->getLayerItem().lower();
    }
    std::list<FluxboxWindow *>::const_iterator it = win->getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win->getTransients().end();
    for (; it != it_end; ++it) {
        if (!(*it)->isIconic()) {
            screen->updateNetizenWindowLower((*it)->getClientWindow());
            (*it)->getLayerItem().lower();
        }
    }
   
}

void FluxboxWindow::raiseLayer() {
    // don't let it up to menu layer
    if (getLayerNum() == (Fluxbox::instance()->getMenuLayer()+1))
        return;

    FluxboxWindow *win = this;
        
    while (win->getTransientFor()) {
        win = win->getTransientFor();
        assert(win != win->getTransientFor());
    }

    if (!win->isIconic()) {
        screen->updateNetizenWindowRaise(win->getClientWindow());
        win->getLayerItem().raiseLayer();
        win->setLayerNum(win->getLayerItem().getLayerNum());
    }

    std::list<FluxboxWindow *>::const_iterator it = win->getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win->getTransients().end();
    for (; it != it_end; ++it) {
        if (!(*it)->isIconic()) {
            screen->updateNetizenWindowRaise((*it)->getClientWindow());
            (*it)->getLayerItem().raiseLayer();
            (*it)->setLayerNum((*it)->getLayerItem().getLayerNum());
        }
    }
}

void FluxboxWindow::lowerLayer() {
    FluxboxWindow *win = (FluxboxWindow *) 0, *bottom = this;
    
    while (bottom->getTransientFor()) {
        bottom = bottom->getTransientFor();
        assert(bottom != bottom->getTransientFor());
    }
    
    win = bottom;
    
    if (!win->isIconic()) {
        screen->updateNetizenWindowLower(win->getClientWindow());
        win->getLayerItem().lowerLayer();
        win->setLayerNum(win->getLayerItem().getLayerNum());
    }
    std::list<FluxboxWindow *>::const_iterator it = win->getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win->getTransients().end();
    for (; it != it_end; ++it) {
        if (!(*it)->isIconic()) {
            screen->updateNetizenWindowLower((*it)->getClientWindow());
            (*it)->getLayerItem().lowerLayer();
            (*it)->setLayerNum((*it)->getLayerItem().getLayerNum());
        }
    }

}

void FluxboxWindow::moveToLayer(int layernum) {
    Fluxbox * fluxbox = Fluxbox::instance();

    FluxboxWindow *win = this;

    // don't let it set its layer into menu area
    if (layernum <= fluxbox->getMenuLayer()) {
        layernum = fluxbox->getMenuLayer() + 1;
    }

    while (win->getTransientFor()) {
        win = win->getTransientFor();
        assert(win != win->getTransientFor());
    }

    if (!win->isIconic()) {
        screen->updateNetizenWindowRaise(win->getClientWindow());
        win->getLayerItem().moveToLayer(layernum);
        win->setLayerNum(win->getLayerItem().getLayerNum());
    }
    std::list<FluxboxWindow *>::const_iterator it = win->getTransients().begin();
    std::list<FluxboxWindow *>::const_iterator it_end = win->getTransients().end();
    for (; it != it_end; ++it) {
        if (!(*it)->isIconic()) {
            screen->updateNetizenWindowRaise((*it)->getClientWindow());
            (*it)->getLayerItem().moveToLayer(layernum);
            (*it)->setLayerNum((*it)->getLayerItem().getLayerNum());

        }
    }
}



void FluxboxWindow::setFocusFlag(bool focus) {
    focused = focus;

    // Record focus timestamp for window cycling enhancements, such as skipping lower tabs
    if (focused)
        gettimeofday(&lastFocusTime, 0);

    m_frame.setFocus(focus);
	
    if (tab)
        tab->focus();

    if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus()) &&
        screen->doAutoRaise())
        timer.stop();
}


void FluxboxWindow::installColormap(bool install) {
    Fluxbox *fluxbox = Fluxbox::instance();
    fluxbox->grab();
    if (! validateClient()) return;

    int i = 0, ncmap = 0;
    Colormap *cmaps = XListInstalledColormaps(display, client.window, &ncmap);
    XWindowAttributes wattrib;
    if (cmaps) {
        if (XGetWindowAttributes(display, client.window, &wattrib)) {
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
                for (i = 0; i < ncmap; i++) // uninstall the window's colormap
                    if (*(cmaps + i) == wattrib.colormap)						
                        XUninstallColormap(display, wattrib.colormap); // we found the colormap to uninstall
            }
        }

        XFree(cmaps);
    }

    fluxbox->ungrab();
}

void FluxboxWindow::saveBlackboxHints() {
    Fluxbox *fluxbox = Fluxbox::instance();
    XChangeProperty(display, client.window, fluxbox->getFluxboxAttributesAtom(),
                    fluxbox->getFluxboxAttributesAtom(), 32, PropModeReplace,
                    (unsigned char *) &blackbox_attrib, PropBlackboxAttributesElements);
}


void FluxboxWindow::setState(unsigned long new_state) {
    current_state = new_state;
    Fluxbox *fluxbox = Fluxbox::instance();
    unsigned long state[2];
    state[0] = (unsigned long) current_state;
    state[1] = (unsigned long) None;
    XChangeProperty(display, client.window, fluxbox->getWMStateAtom(),
                    fluxbox->getWMStateAtom(), 32, PropModeReplace,
                    (unsigned char *) state, 2);

    saveBlackboxHints();
    //notify state changed
    m_statesig.notify();
}

bool FluxboxWindow::getState() {
    current_state = 0;

    Atom atom_return;
    bool ret = false;
    int foo;
    unsigned long *state, ulfoo, nitems;
    Fluxbox *fluxbox = Fluxbox::instance();
    if ((XGetWindowProperty(display, client.window, fluxbox->getWMStateAtom(),
                            0l, 2l, false, fluxbox->getWMStateAtom(),
                            &atom_return, &foo, &nitems, &ulfoo,
                            (unsigned char **) &state) != Success) ||
        (! state)) {
        return false;
    }

    if (nitems >= 1) {
        current_state = static_cast<unsigned long>(state[0]);
        ret = true;
    }

    XFree(static_cast<void *>(state));

    return ret;
}

//TODO: this functions looks odd
void FluxboxWindow::setGravityOffsets() {
    int newx = m_frame.x();
    int newy = m_frame.y();
    // translate x coordinate
    switch (client.win_gravity) {
        // handle Westward gravity
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
    default:
#ifdef DEBUG
        cerr<<__FILE__<<": Default gravity: SouthWest, NorthWest, West"<<endl;
#endif // DEBUG

        newx = m_frame.x();
        break;

        // handle Eastward gravity
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
#ifdef DEBUG
        cerr<<__FILE__<<": Gravity: SouthEast, NorthEast, East"<<endl;
#endif // DEBUG

        newx = m_frame.x() + m_frame.clientArea().width() - m_frame.width();
        break;

        // no x translation desired - default
    case StaticGravity:
    case ForgetGravity:
    case CenterGravity:
#ifdef DEBUG
        cerr<<__FILE__<<": Gravity: Center, Forget, Static"<<endl;
#endif // DEBUG

        newx = m_frame.x();
    }

    // translate y coordinate
    switch (client.win_gravity) {
        // handle Northbound gravity
    case NorthWestGravity:
    case NorthGravity:
    case NorthEastGravity:
    default:
        newy = m_frame.y();
        break;

        // handle Southbound gravity
    case SouthWestGravity:
    case SouthGravity:
    case SouthEastGravity:
        newy = m_frame.y() + m_frame.clientArea().height() - m_frame.height();
        break;

        // no y translation desired - default
    case StaticGravity:
    case ForgetGravity:
    case CenterGravity:
        newy = m_frame.y();
        break;
    }
    // finaly move the frame
    if (m_frame.x() != newx || m_frame.y() != newy)
        m_frame.move(newx, newy);
}

/* 
 * restoreAttributes sets the attributes to what they should be
 * but doesn't change the actual state
 * (so the caller can set defaults etc as well)
 */
void FluxboxWindow::restoreAttributes() {
    if (!getState())
        current_state = NormalState;

    Atom atom_return;
    int foo;
    unsigned long ulfoo, nitems;
    Fluxbox *fluxbox = Fluxbox::instance();
	
    BaseDisplay::BlackboxAttributes *net;
    if (XGetWindowProperty(display, client.window,
                           fluxbox->getFluxboxAttributesAtom(), 0l,
                           PropBlackboxAttributesElements, false,
                           fluxbox->getFluxboxAttributesAtom(), &atom_return, &foo,
                           &nitems, &ulfoo, (unsigned char **) &net) ==
        Success && net && nitems == PropBlackboxAttributesElements) {
        blackbox_attrib.flags = net->flags;
        blackbox_attrib.attrib = net->attrib;
        blackbox_attrib.workspace = net->workspace;
        blackbox_attrib.stack = net->stack;
        blackbox_attrib.premax_x = net->premax_x;
        blackbox_attrib.premax_y = net->premax_y;
        blackbox_attrib.premax_w = net->premax_w;
        blackbox_attrib.premax_h = net->premax_h;

        XFree(static_cast<void *>(net));
    } else
        return;

    if (blackbox_attrib.flags & BaseDisplay::ATTRIB_SHADED &&
        blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) {
        int save_state =
            ((current_state == IconicState) ? NormalState : current_state);

        shaded = true;
			
        current_state = save_state;
    }

    if (( blackbox_attrib.workspace != screen->getCurrentWorkspaceID()) &&
        ( blackbox_attrib.workspace < screen->getCount())) {
        workspace_number = blackbox_attrib.workspace;

        if (current_state == NormalState) current_state = WithdrawnState;
    } else if (current_state == WithdrawnState)
        current_state = NormalState;

    if (blackbox_attrib.flags & BaseDisplay::ATTRIB_OMNIPRESENT &&
        blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) {
        stuck = true;

        current_state = NormalState;
    }

    if (blackbox_attrib.flags & BaseDisplay::ATTRIB_STACK) {
        //TODO check value?
        m_layernum = blackbox_attrib.stack;
    }

    if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) ||
        (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)) {
        int x = blackbox_attrib.premax_x, y = blackbox_attrib.premax_y;
        unsigned int w = blackbox_attrib.premax_w, h = blackbox_attrib.premax_h;
        maximized = false;
        if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
            (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT))
            maximized = true;
        else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)
            maximizeVertical();
        else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ)
            maximizeHorizontal();

        blackbox_attrib.premax_x = x;
        blackbox_attrib.premax_y = y;
        blackbox_attrib.premax_w = w;
        blackbox_attrib.premax_h = h;
    }

    setState(current_state);
}

/**
   Show the window menu at pos mx, my
*/
void FluxboxWindow::showMenu(int mx, int my) {
    m_windowmenu.move(mx, my);
    m_windowmenu.show();		
    m_windowmenu.raise();
    //    m_windowmenu.getSendToMenu().raise();
    //    m_windowmenu.getSendGroupToMenu().raise();
}

/**
   Moves the menu to last button press position and shows it,
   if it's already visible it'll be hidden
 */
void FluxboxWindow::popupMenu() {
    if (m_windowmenu.isVisible()) {
        m_windowmenu.hide(); 
        return;
    }

    m_windowmenu.move(m_last_button_x, m_frame.y() + m_frame.titlebar().height() + m_frame.titlebar().borderWidth()*2);
    m_windowmenu.show();
    m_windowmenu.raise();
}

void FluxboxWindow::restoreGravity() {
    // restore x coordinate
    switch (client.win_gravity) {
        // handle Westward gravity
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
    default:
        client.x = m_frame.x();
        break;

        // handle Eastward gravity
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
        client.x = (m_frame.x() + m_frame.width()) - client.width;
        break;
    }

    // restore y coordinate
    switch (client.win_gravity) {
        // handle Northbound gravity
    case NorthWestGravity:
    case NorthGravity:
    case NorthEastGravity:
    default:
        client.y = m_frame.y();
        break;

        // handle Southbound gravity
    case SouthWestGravity:
    case SouthGravity:
    case SouthEastGravity:
        client.y = (m_frame.y() + m_frame.height()) - client.height;
        break;
    }
}

/**
   Determine if this is the lowest tab of them all
*/
bool FluxboxWindow::isLowerTab() const {
    Tab* chkTab = (tab ? tab->first() : 0);
    while (chkTab) {
        const FluxboxWindow* chkWin = chkTab->getWindow();
        if (chkWin && chkWin != this &&
            timercmp(&chkWin->lastFocusTime, &lastFocusTime, >))
            return true;
        chkTab = chkTab->next();
    }
    return false;
}

/**
   Redirect any unhandled event to our handlers
*/
void FluxboxWindow::handleEvent(XEvent &event) {
    switch (event.type) {
    case ConfigureRequest:
        configureRequestEvent(event.xconfigurerequest);
    case MapNotify:
        mapNotifyEvent(event.xmap);
    case MapRequest:
        mapRequestEvent(event.xmaprequest);
        break;
    default:
        break;
    }
}

void FluxboxWindow::mapRequestEvent(XMapRequestEvent &re) {
    // we're only conserned about client window event
    if (re.window != client.window)
        return;

    Fluxbox *fluxbox = Fluxbox::instance();
	
    bool get_state_ret = getState();
    if (! (get_state_ret && fluxbox->isStartup())) {
        if ((client.wm_hint_flags & StateHint) &&
            (! (current_state == NormalState || current_state == IconicState))) {
            current_state = client.initial_state;
        } else
            current_state = NormalState;
    } else if (iconic)
        current_state = NormalState;
		
    switch (current_state) {
    case IconicState:
        iconify();
	break;

    case WithdrawnState:
        withdraw();
	break;

    case NormalState:
        // check WM_CLASS only when we changed state to NormalState from 
        // WithdrawnState (ICCC 4.1.2.5)
				
        XClassHint ch;

        if (XGetClassHint(display, getClientWindow(), &ch) == 0) {
            cerr<<"Failed to read class hint!"<<endl;
        } else {
            if (ch.res_name != 0) {
                m_instance_name = const_cast<char *>(ch.res_name);
                XFree(ch.res_name);
            } else
                m_instance_name = "";

            if (ch.res_class != 0) {
                m_class_name = const_cast<char *>(ch.res_class);
                XFree(ch.res_class);
            } else 
                m_class_name = "";

            Workspace *wsp = screen->getWorkspace(workspace_number);
            // we must be resizable AND maximizable to be autogrouped
            // TODO: there should be an isGroupable() function
            if (wsp != 0 && isResizable() && isMaximizable()) {
                wsp->checkGrouping(*this);
            }
        }		
		
        deiconify(false);
			
	break;
    case InactiveState:
    case ZoomState:
    default:
        deiconify(false);
        break;
    }

}


void FluxboxWindow::mapNotifyEvent(XMapEvent &ne) {
    if (ne.window == client.window && !ne.override_redirect && visible) {
        Fluxbox *fluxbox = Fluxbox::instance();
        fluxbox->grab();
        if (! validateClient())
            return;

        setState(NormalState);		
			
        if (transient || screen->doFocusNew())
            setInputFocus();
        else
            setFocusFlag(false);			

	if (focused)
            m_frame.setFocus(true);

        visible = true;
        iconic = false;

        // Auto-group from tab?
        if (!transient) {
            // Grab and clear the auto-group window
            FluxboxWindow* autoGroupWindow = screen->useAutoGroupWindow();
            if (autoGroupWindow) {
                Tab *groupTab = autoGroupWindow->getTab();
                if (groupTab)
                    groupTab->addWindowToGroup(this);
            }
        }

        fluxbox->ungrab();
    }
}

/**
   Unmaps frame window and client window if 
   event.window == client.window
   Returns true if *this should die
   else false
*/
void FluxboxWindow::unmapNotifyEvent(XUnmapEvent &ue) {
    if (ue.window != client.window)
        return;
	
#ifdef DEBUG
    cerr<<__FILE__<<": unmapNotifyEvent() 0x"<<hex<<client.window<<dec<<endl;
#endif // DEBUG

    restore(false);


}

/**
   Checks if event is for client.window.
*/
void FluxboxWindow::destroyNotifyEvent(XDestroyWindowEvent &de) {
    if (de.window == client.window) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): DestroyNotifyEvent this="<<this<<endl;
#endif // DEBUG
        m_frame.hide();
    }

}


void FluxboxWindow::propertyNotifyEvent(Atom atom) {

    switch(atom) {
    case XA_WM_CLASS:
    case XA_WM_CLIENT_MACHINE:
    case XA_WM_COMMAND:
        break;

    case XA_WM_TRANSIENT_FOR:
        updateTransientInfo();
        reconfigure();

        break;

    case XA_WM_HINTS:
        getWMHints();
        break;

    case XA_WM_ICON_NAME:
        updateIconNameFromClient();
        if (iconic)
            screen->iconUpdate();
        updateIcon();
        break;

    case XA_WM_NAME:
        updateTitleFromClient();
 
        if (hasTab()) // update tab
            getTab()->draw(false);

        if (! iconic)
            screen->getWorkspace(workspace_number)->update();
        else
            updateIcon();
		 

        break;

    case XA_WM_NORMAL_HINTS: {
        getWMNormalHints();

        if ((client.normal_hint_flags & PMinSize) &&
            (client.normal_hint_flags & PMaxSize)) {
 
            if (client.max_width != 0 && client.max_width <= client.min_width &&
                client.max_height != 0 && client.max_height <= client.min_height) {
                decorations.maximize = false;
                decorations.handle = false;
                functions.resize=false;
                functions.maximize=false;
            } else {
                if (! isTransient()) {
                    decorations.maximize = true;
                    decorations.handle = true;
                    functions.maximize = true;	        
                }
                functions.resize = true;
            }
 
    	}

        // save old values
        int x = m_frame.x(), y = m_frame.y();
        unsigned int w = m_frame.width(), h = m_frame.height();

        upsize();

        // reconfigure if the old values changed
        if ((x != m_frame.x()) || (y != m_frame.y()) ||
            (w != m_frame.width()) || (h != m_frame.height()))
            reconfigure();

        break; 
    }

    default:
        if (atom == FbAtoms::instance()->getWMProtocolsAtom()) {
            getWMProtocols();
            // reset window actions
            screen->setupWindowActions(*this);
            //!!TODO  check this area
        } 
        break;
    }

}


void FluxboxWindow::exposeEvent(XExposeEvent &ee) {
    m_frame.exposeEvent(ee);
}


void FluxboxWindow::configureRequestEvent(XConfigureRequestEvent &cr) {
    if (cr.window != client.window)
        return;

    if (! validateClient())
        return;

    int cx = m_frame.x(), cy = m_frame.y();
    unsigned int cw = m_frame.width(), ch = m_frame.height();

    if (cr.value_mask & CWBorderWidth)
        client.old_bw = cr.border_width;

    if (cr.value_mask & CWX)
        cx = cr.x;// - frame_mwm_border_w - screen->getBorderWidth();

    if (cr.value_mask & CWY)
        cy = cr.y - m_frame.titlebar().height(); // - frame_mwm_border_w - screen->getBorderWidth();

    if (cr.value_mask & CWWidth)
        cw = cr.width;// + (frame_mwm_border_w * 2);

    if (cr.value_mask & CWHeight)
        ch = cr.height;

    if (m_frame.x() != cx || m_frame.y() != cy ||
        m_frame.width() != cw || m_frame.height() != ch) {
        // the request is for client window so we resize the frame to it first
        frame().resizeForClient(cw, ch);
        move(cx, cy);

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

}


void FluxboxWindow::buttonPressEvent(XButtonEvent &be) {
    m_last_button_x = be.x_root;
    m_last_button_y = be.y_root;

    // check frame events first
    m_frame.buttonPressEvent(be);

    if (be.button == 1 || (be.button == 3 && be.state == Mod1Mask)) {
        if ((! focused) && (! screen->isSloppyFocus())) { //check focus 
            setInputFocus(); 
        }

        if (m_frame.clientArea() == be.window) {
            raise();
            XAllowEvents(display, ReplayPointer, be.time);			
        } else {            
            button_grab_x = be.x_root - m_frame.x() - screen->getBorderWidth();
            button_grab_y = be.y_root - m_frame.y() - screen->getBorderWidth();      
        }
        
        if (m_windowmenu.isVisible())
                m_windowmenu.hide();
    }

}

void FluxboxWindow::shapeEvent(XShapeEvent *) { }

void FluxboxWindow::buttonReleaseEvent(XButtonEvent &re) {
    m_frame.buttonReleaseEvent(re); // let the frame handle the event first

    if (isMoving())
        stopMoving();		
    else if (isResizing())
        stopResizing();
    else if (re.window == m_frame.window()) {
        if (re.button == 2 && re.state == Mod1Mask)
            XUngrabPointer(display, CurrentTime);
    }


}


void FluxboxWindow::motionNotifyEvent(XMotionEvent &me) {
    if ((me.state & Button1Mask) && functions.move &&
        (m_frame.titlebar() == me.window || m_frame.label() == me.window ||
         m_frame.handle() == me.window || m_frame.window() == me.window) && !isResizing()) {
			 
        if (! isMoving()) {
            startMoving(me.window);
        } else {			
            int dx = me.x_root - button_grab_x, 
                dy = me.y_root - button_grab_y;

            dx -= screen->getBorderWidth();
            dy -= screen->getBorderWidth();

            // Warp to next or previous workspace?, must have moved sideways some
            int moved_x = me.x_root - last_resize_x;
            // save last event point
            last_resize_x = me.x_root;
            last_resize_y = me.y_root;
            if (moved_x && screen->isWorkspaceWarping()) {
                int cur_id = screen->getCurrentWorkspaceID();
                int new_id = cur_id;
                const int warpPad = screen->getEdgeSnapThreshold();
                if (me.x_root >= int(screen->getWidth()) - warpPad - 1 &&
                    m_frame.x() < int(me.x_root - button_grab_x - screen->getBorderWidth())) {
                    //warp right
                    new_id = (cur_id + 1) % screen->getCount();
                    dx = - me.x_root;
                } else if (me.x_root <= warpPad &&
                           m_frame.x() > int(me.x_root - button_grab_x - screen->getBorderWidth())) {
                    //warp left
                    new_id = (cur_id - 1 + screen->getCount()) % screen->getCount();
                    dx = screen->getWidth() - me.x_root-1;
                }

                if (new_id != cur_id) {
                    XWarpPointer(display, None, None, 0, 0, 0, 0, dx, 0);

                    screen->changeWorkspaceID(new_id);

                    last_resize_x = me.x_root + dx;
                    
                    dx += m_frame.x(); // for window in correct position
                
                }
            }

            
            if (! screen->doOpaqueMove()) {
                XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                               last_move_x, last_move_y, 
                               m_frame.width(), m_frame.height());

                XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                               dx, dy, 
                               m_frame.width(), m_frame.height());
                last_move_x = dx;
                last_move_y = dy;
            } else {
            
                moveResize(dx, dy, m_frame.width(), m_frame.height());
            }

            if (screen->doShowWindowPos())
                screen->showPosition(dx, dy);
        } // end if moving
    } else if (functions.resize &&
               (((me.state & Button1Mask) && (me.window == m_frame.gripRight() ||
                                              me.window == m_frame.gripLeft())) ||
                me.window == m_frame.window())) {
		
        bool left = (me.window == m_frame.gripLeft());

        if (! resizing) {			
            startResizing(me.window, me.x, me.y, left); 
        } else if (resizing) {
            // draw over old rect
            XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                           last_resize_x, last_resize_y,
                           last_resize_w - 1, last_resize_h-1);


            // move rectangle
            int gx = 0, gy = 0;

            last_resize_h = m_frame.height() + (me.y - button_grab_y);
            if (last_resize_h < 1)
                last_resize_h = 1;

            if (left) {
                last_resize_x = me.x_root - button_grab_x;
                if (last_resize_x > (signed) (m_frame.x() + m_frame.width()))
                    last_resize_x = last_resize_x + m_frame.width() - 1;

                left_fixsize(&gx, &gy);
            } else {
                last_resize_w = m_frame.width() + (me.x - button_grab_x);
                if (last_resize_w < 1) // clamp to 1
                    last_resize_w = 1;

                right_fixsize(&gx, &gy);
            }

            // draw resize rectangle
            XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                           last_resize_x, last_resize_y,
                           last_resize_w - 1, last_resize_h - 1);

            if (screen->doShowWindowPos())
                screen->showGeometry(gx, gy);
        }
    }

}

// TODO: functions should not be affected by decoration
void FluxboxWindow::setDecoration(Decoration decoration) {
    switch (decoration) {
    case DECOR_NONE:
        decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.tab = false; //tab is also a decor
        decorations.menu = true; // menu is present
	//	functions.iconify = functions.maximize = true;
	//	functions.move = true;   // We need to move even without decor
	//	functions.resize = true; // We need to resize even without decor
        frame().hideAllDecorations();
	break;

    default:
    case DECOR_NORMAL:
        decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.menu = true;
        functions.resize = functions.move = functions.iconify =
            functions.maximize = true;
        m_frame.showAllDecorations();
        m_frame.show();
        
	break;

    case DECOR_TINY:
        decorations.titlebar = decorations.iconify = decorations.menu =
            functions.move = functions.iconify = true;
        decorations.border = decorations.handle = decorations.maximize =
            functions.resize = functions.maximize = false;
        m_frame.show();
	break;

    case DECOR_TOOL:
        decorations.titlebar = decorations.menu = functions.move = true;
        decorations.iconify = decorations.border = decorations.handle =
            decorations.maximize = functions.resize = functions.maximize =
            functions.iconify = false;
	break;
    }

    reconfigure();

}

void FluxboxWindow::toggleDecoration() {
    //don't toggle decor if the window is shaded
    if (isShaded())
        return;
	
    if (decorations.enabled) { //remove decorations
        setDecoration(DECOR_NONE); 
        decorations.enabled = false;
    } else { //revert back to old decoration
        if (old_decoration == DECOR_NONE) { // make sure something happens
            setDecoration(DECOR_NORMAL);
        } else {
            setDecoration(old_decoration);
        }
        decorations.enabled = true;
    }
}

bool FluxboxWindow::validateClient() {
    XSync(display, false);

    XEvent e;
    if (XCheckTypedWindowEvent(display, client.window, DestroyNotify, &e) ||
        XCheckTypedWindowEvent(display, client.window, UnmapNotify, &e)) {
        XPutBackEvent(display, &e);
        Fluxbox::instance()->ungrab();

        return false;
    }

    return true;
}

void FluxboxWindow::startMoving(Window win) {
    moving = true;
    Fluxbox *fluxbox = Fluxbox::instance();
    XGrabPointer(display, win, False, Button1MotionMask |
                 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                 None, fluxbox->getMoveCursor(), CurrentTime);

    if (m_windowmenu.isVisible())
        m_windowmenu.hide();

    fluxbox->maskWindowEvents(client.window, this);
    last_move_x = frame().x();
    last_move_y = frame().y();
    if (! screen->doOpaqueMove()) {
        XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                       frame().x(), frame().y(),
                       frame().width(), frame().height());
        screen->showPosition(frame().x(), frame().y());
    }
}

void FluxboxWindow::stopMoving() {
    moving = false;
    Fluxbox *fluxbox = Fluxbox::instance();

    fluxbox->maskWindowEvents(0, 0);

   
    if (! screen->doOpaqueMove()) {
        XDrawRectangle(FbTk::App::instance()->display(), screen->getRootWindow(), screen->getOpGC(),
                       last_move_x, last_move_y, 
                       frame().width(), frame().height());
        moveResize(last_move_x, last_move_y, m_frame.width(), m_frame.height());
    } else
        moveResize(m_frame.x(), m_frame.y(), m_frame.width(), m_frame.height());

    screen->hideGeometry();
    XUngrabPointer(display, CurrentTime);
	
    XSync(display, False); //make sure the redraw is made before we continue
}

void FluxboxWindow::pauseMoving() {

}


void FluxboxWindow::resumeMoving() {

}


void FluxboxWindow::startResizing(Window win, int x, int y, bool left) {
    resizing = true;
    Fluxbox *fluxbox = Fluxbox::instance();
    XGrabPointer(display, win, false, ButtonMotionMask | ButtonReleaseMask, 
                 GrabModeAsync, GrabModeAsync, None,
                 (left ? fluxbox->getLowerLeftAngleCursor() : fluxbox->getLowerRightAngleCursor()),
                 CurrentTime);

    int gx = 0, gy = 0;
    button_grab_x = x - screen->getBorderWidth();
    button_grab_y = y - screen->getBorderWidth2x();
    last_resize_x = m_frame.x();
    last_resize_y = m_frame.y();
    last_resize_w = m_frame.width() + screen->getBorderWidth2x();
    last_resize_h = m_frame.height() + screen->getBorderWidth2x();

    if (left)
        left_fixsize(&gx, &gy);
    else
        right_fixsize(&gx, &gy);

    if (screen->doShowWindowPos())
        screen->showGeometry(gx, gy);

    XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                   last_resize_x, last_resize_y,
                   last_resize_w - 1, last_resize_h - 1);
}

void FluxboxWindow::stopResizing(Window win) {
    resizing = false;
	
    XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
                   last_resize_x, last_resize_y,
                   last_resize_w - 1, last_resize_h - 1);

    screen->hideGeometry();

    if (win && win == m_frame.gripLeft())
        left_fixsize();
    else
        right_fixsize();

	
    moveResize(last_resize_x, last_resize_y,
               last_resize_w - screen->getBorderWidth2x(),
               last_resize_h - screen->getBorderWidth2x());
	
    XUngrabPointer(display, CurrentTime);
}

//finds and redraw the icon label
void FluxboxWindow::updateIcon() {
    if (Fluxbox::instance()->useIconBar()) {
        const IconBar *iconbar = 0;
        const IconBarObj *icon = 0;
        if ((iconbar = screen->getToolbar()->iconBar()) != 0) {
            if ((icon = iconbar->findIcon(this)) != 0)
                iconbar->draw(icon, icon->width());
        }
    }
}

void FluxboxWindow::updateTransientInfo() {
    // remove us from parent
    if (client.transient_for != 0) {
        client.transient_for->client.transients.remove(this);
    }
    client.transient_for = 0;
	
    // determine if this is a transient window
    Window win;
    if (!XGetTransientForHint(display, client.window, &win))
        return;
	
    if (win == client.window)
        return;
	
    if (win == screen->getRootWindow() && win != 0) {
        modal = true;
        return;
    }
	
    client.transient_for = Fluxbox::instance()->searchWindow(win);
    if (client.transient_for != 0 &&
        client.window_group != None && win == client.window_group) {		
        client.transient_for = Fluxbox::instance()->searchGroup(win, this);
    }
	
    // make sure we don't have deadlock loop in transient chain
    for (FluxboxWindow *w = this; w != 0; w = w->client.transient_for) {
        if (w == w->client.transient_for) {
            w->client.transient_for = 0;
#ifdef DEBUG	
            cerr<<"w = client.transient_for";
#endif // DEBUG
            break;
        }
    }
	
    if (client.transient_for != 0) {
        client.transient_for->client.transients.push_back(this);
        // make sure we only have on instance of this
        client.transient_for->client.transients.unique(); 
        stuck = client.transient_for->stuck;
    }
	
}

void FluxboxWindow::restore(bool remap) {
    XChangeSaveSet(display, client.window, SetModeDelete);
    XSelectInput(display, client.window, NoEventMask);

    restoreGravity();

    m_frame.hide();
    // make sure the frame doesn't change client window anymore
    m_frame.removeClient();

    XUnmapWindow(display, client.window);

    // restore old border width
    XSetWindowBorderWidth(display, client.window, client.old_bw);

    XEvent dummy;
    if (! XCheckTypedWindowEvent(display, client.window, ReparentNotify,
                                 &dummy)) {
#ifdef DEBUG
        cerr<<"FluxboxWindow::restore: reparent 0x"<<hex<<client.window<<dec<<" to root"<<endl;
#endif // DEBUG

        // reparent to screen window
        XReparentWindow(display, client.window, screen->getRootWindow(),
			m_frame.x(), m_frame.y());

    }

    if (remap)
        XMapWindow(display, client.window);

}


void FluxboxWindow::timeout() {
    raise();
}


void FluxboxWindow::changeBlackboxHints(const BaseDisplay::BlackboxHints &net) {
    if ((net.flags & BaseDisplay::ATTRIB_SHADED) &&
        ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) !=
         (net.attrib & BaseDisplay::ATTRIB_SHADED)))
        shade();

    if ((net.flags & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) &&
        ((blackbox_attrib.attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) !=
         (net.attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)))) {
        if (maximized) {
            maximize();
        } else {
            if ((net.flags & BaseDisplay::ATTRIB_MAXHORIZ) && (net.flags & BaseDisplay::ATTRIB_MAXVERT))
            	maximize();
            else if (net.flags & BaseDisplay::ATTRIB_MAXVERT)
                maximizeVertical();
            else if (net.flags & BaseDisplay::ATTRIB_MAXHORIZ)
                maximizeHorizontal();

        }
    }

    if ((net.flags & BaseDisplay::ATTRIB_OMNIPRESENT) &&
        ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) !=
         (net.attrib & BaseDisplay::ATTRIB_OMNIPRESENT)))
        stick();

    if ((net.flags & BaseDisplay::ATTRIB_WORKSPACE) &&
        (workspace_number !=  net.workspace)) {

        if (getTab()) // disconnect from tab chain before sending it to another workspace
            getTab()->disconnect();

        screen->reassociateWindow(this, net.workspace, true);

        if (screen->getCurrentWorkspaceID() != net.workspace)
            withdraw();
        else 
            deiconify();
    }

    if (net.flags & BaseDisplay::ATTRIB_STACK) {
        if ((unsigned int) m_layernum != net.stack) {
            moveToLayer(net.stack);
        }
    }

    if (net.flags & BaseDisplay::ATTRIB_DECORATION) {
        old_decoration = static_cast<Decoration>(net.decoration);
        setDecoration(old_decoration);
    }

}

void FluxboxWindow::upsize() {
    m_frame.setBevel(screen->getBevelWidth());
    m_frame.handle().resize(m_frame.handle().width(), screen->getHandleWidth());
    m_frame.gripLeft().resize(m_frame.buttonHeight(), screen->getHandleWidth());
    m_frame.gripRight().resize(m_frame.gripLeft().width(), m_frame.gripLeft().height());
    // convert client.width/height into frame sizes
    m_frame.resizeForClient(client.width, client.height);
 
}


///TODO
void FluxboxWindow::downsize() {
	
}


void FluxboxWindow::right_fixsize(int *gx, int *gy) {
    // calculate the size of the client window and conform it to the
    // size specified by the size hints of the client window...
    int dx = last_resize_w - client.base_width;
    int dy = last_resize_h - client.base_height;

    if (dx < (signed) client.min_width)
        dx = client.min_width;
    if (dy < (signed) client.min_height)
        dy = client.min_height;
    if (client.max_width > 0 && (unsigned) dx > client.max_width)
        dx = client.max_width;
    if (client.max_height > 0 && (unsigned) dy > client.max_height)
        dy = client.max_height;

    // make it snap
    dx /= client.width_inc;
    dy /= client.height_inc;

    if (gx) *gx = dx;
    if (gy) *gy = dy;

    dx = (dx * client.width_inc) + client.base_width;
    dy = (dy * client.height_inc) + client.base_height;

    last_resize_w = dx;
    last_resize_h = dy;
}

void FluxboxWindow::left_fixsize(int *gx, int *gy) {
    int dx = m_frame.width() + m_frame.x() - last_resize_x;
    int dy = last_resize_h - client.base_height;

    // check minimum size
    if (dx < static_cast<signed int>(client.min_width))
        dx = client.min_width;
    if (dy < static_cast<signed int>(client.min_height))
        dy = client.min_height;

    // check maximum size
    if (client.max_width > 0 && dx > static_cast<signed int>(client.max_width))
        dx = client.max_width;
    if (client.max_height > 0 && dy > static_cast<signed int>(client.max_height))
        dy = client.max_height;

    // make sure we have valid increment
    if (client.width_inc == 0)
        client.width_inc = 1;
    if (client.height_inc == 0)
        client.height_inc = 1;

    // set snaping
    dx /= client.width_inc;
    dy /= client.height_inc;

    // set return values
    if (gx != 0)
        *gx = dx;
    if (gy != 0)
        *gy = dy;

    // snapping
    dx = dx * client.width_inc + client.base_width;
    dy = dy * client.height_inc + client.base_height;

    // update last resize 
    last_resize_w = dx;
    last_resize_h = dy;
    last_resize_x = m_frame.x() + m_frame.width() - last_resize_w;	
}
