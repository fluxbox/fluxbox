// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Screen.cc for Blackbox - an X11 Window manager
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


#include "Screen.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Netizen.hh"

// themes
#include "FbWinFrameTheme.hh"
#include "MenuTheme.hh"
#include "RootTheme.hh"
#include "WinButtonTheme.hh"
#include "SlitTheme.hh"

// menu items
#include "BoolMenuItem.hh"
#include "IntResMenuItem.hh"
#include "FocusModelMenuItem.hh"

// menus
#include "FbMenu.hh"
#include "LayerMenu.hh"

#include "MenuCreator.hh"

#include "WinClient.hh"
#include "FbWinFrame.hh"
#include "Strut.hh"
#include "CommandParser.hh"
#include "AtomHandler.hh"
#include "HeadArea.hh"


#include "FbTk/I18n.hh"
#include "FbTk/Subject.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/MultLayers.hh"
#include "FbTk/XLayerItem.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/Compose.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#include "SlitClient.hh"
#else
// fill it in
class Slit {};
#endif // SLIT

#ifdef STDC_HEADERS
#include <sys/types.h>
#endif // STDC_HEADERS

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif // HAVE_STDARG_H

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef	HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef XINERAMA
extern  "C" {
#include <X11/extensions/Xinerama.h>
}
#endif // XINERAMA

#include <iostream>
#include <memory>
#include <algorithm>
#include <functional>
#include <stack>

using namespace std;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
    _FB_USES_NLS;
    cerr<<_FBTEXT(Screen, AnotherWMRunning, 
                  "BScreen::BScreen: an error occured while querying the X server.\n"
                  "	another window manager already running on display ",
                  "Message when another WM is found already active on all screens")
        <<DisplayString(display)<<endl;

    running = false;

    return -1;
}



} // end anonymous namespace


BScreen::ScreenResource::ScreenResource(FbTk::ResourceManager &rm, 
                                        const std::string &scrname, 
                                        const std::string &altscrname):
    image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
    opaque_move(rm, false, scrname + ".opaqueMove", altscrname+".OpaqueMove"),
    full_max(rm, true, scrname+".fullMaximization", altscrname+".FullMaximization"),
    sloppy_window_grouping(rm, true, 
                           scrname+".sloppywindowgrouping", altscrname+".SloppyWindowGrouping"),
    workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
    desktop_wheeling(rm, true, scrname+".desktopwheeling", altscrname+".DesktopWheeling"),
    show_window_pos(rm, true, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
    focus_last(rm, true, scrname+".focusLastWindow", altscrname+".FocusLastWindow"),
    focus_new(rm, true, scrname+".focusNewWindows", altscrname+".FocusNewWindows"),
    antialias(rm, false, scrname+".antialias", altscrname+".Antialias"),
    auto_raise(rm, false, scrname+".autoRaise", altscrname+".AutoRaise"),
    click_raises(rm, true, scrname+".clickRaises", altscrname+".ClickRaises"),
    decorate_transient(rm, false, scrname+".decorateTransient", altscrname+".DecorateTransient"),
    rootcommand(rm, "", scrname+".rootCommand", altscrname+".RootCommand"),
    resize_model(rm, BOTTOMRESIZE, scrname+".resizeMode", altscrname+".ResizeMode"),
    windowmenufile(rm, "", scrname+".windowMenu", altscrname+".WindowMenu"),
    focus_model(rm, CLICKTOFOCUS, scrname+".focusModel", altscrname+".FocusModel"),
    follow_model(rm, IGNORE_OTHER_WORKSPACES, scrname+".followModel", altscrname+".followModel"),
    workspaces(rm, 1, scrname+".workspaces", altscrname+".Workspaces"),
    edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    focused_alpha(rm, 255, scrname+".window.focus.alpha", altscrname+".Window.Focus.Alpha"),
    unfocused_alpha(rm, 255, scrname+".window.unfocus.alpha", altscrname+".Window.Unfocus.Alpha"),
    menu_alpha(rm, 255, scrname+".menu.alpha", altscrname+".Menu.Alpha"),
    menu_delay(rm, 0, scrname + ".menuDelay", altscrname+".MenuDelay"),
    menu_delay_close(rm, 0, scrname + ".menuDelayClose", altscrname+".MenuDelayClose"),
    menu_mode(rm, FbTk::MenuTheme::DELAY_OPEN, scrname+".menuMode", altscrname+".MenuMode"),
    placement_policy(rm, ROWSMARTPLACEMENT, scrname+".windowPlacement", altscrname+".WindowPlacement"),
    row_direction(rm, LEFTRIGHT, scrname+".rowPlacementDirection", altscrname+".RowPlacementDirection"),
    col_direction(rm, TOPBOTTOM, scrname+".colPlacementDirection", altscrname+".ColPlacementDirection"),
    gc_line_width(rm, 1, scrname+".overlay.lineWidth", altscrname+".Overlay.LineWidth"),
    gc_line_style(rm, 
                  FbTk::GContext::LINESOLID, 
                  scrname+".overlay.lineStyle", 
                  altscrname+".Overlay.LineStyle"),
    gc_join_style(rm,
                  FbTk::GContext::JOINMITER,
                  scrname+".overlay.joinStyle",
                  altscrname+".Overlay.JoinStyle"),
    gc_cap_style(rm,
                 FbTk::GContext::CAPNOTLAST,
                 scrname+".overlay.capStyle",
                 altscrname+".overlay.CapStyle") {

}

BScreen::BScreen(FbTk::ResourceManager &rm,
                 const string &screenname, const string &altscreenname,
                 int scrn, int num_layers) : 
    m_clientlist_sig(*this),  // client signal
    m_iconlist_sig(*this), // icon list signal
    m_workspacecount_sig(*this), // workspace count signal
    m_workspacenames_sig(*this), // workspace names signal 
    m_workspace_area_sig(*this), // workspace area signal
    m_currentworkspace_sig(*this), // current workspace signal
    m_reconfigure_sig(*this), // reconfigure signal
    m_resize_sig(*this),
    m_layermanager(num_layers),
    cycling_focus(false),
    cycling_last(0),
    m_windowtheme(new FbWinFrameTheme(scrn)), 
    // the order of windowtheme and winbutton theme is important
    // because winbutton need to rescale the pixmaps in winbutton theme
    // after fbwinframe have resized them
    m_winbutton_theme(new WinButtonTheme(scrn, *m_windowtheme)),
    m_menutheme(new MenuTheme(scrn)),
    m_root_theme(new 
                 RootTheme(scrn, 
                           *resource.rootcommand)),
    m_root_window(scrn),
    m_geom_window(m_root_window,
                  0, 0, 10, 10, 
                  false,  // override redirect
                  true), // save under
    m_pos_window(m_root_window,
                 0, 0, 10, 10,
                 false,  // override redirect
                 true), // save under
    resource(rm, screenname, altscreenname),
    m_name(screenname),
    m_altname(altscreenname),
    m_resource_manager(rm),
    m_xinerama_headinfo(0),
    m_shutdown(false) {


    Fluxbox *fluxbox = Fluxbox::instance();
    Display *disp = fluxbox->display();

    initXinerama();

    // setup error handler to catch "screen already managed by other wm"
    XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);

    rootWindow().setEventMask(ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
                              SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
                              ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask);

    fluxbox->sync(false);

    XSetErrorHandler((XErrorHandler) old);

    managed = running;
    if (! managed)
        return;

    // TODO fluxgen: check if this is the right place
    m_head_areas = new HeadArea[numHeads() ? numHeads() : 1];
	
    _FB_USES_NLS;
	
    fprintf(stderr, _FBTEXT(Screen, ManagingScreen,
                            "BScreen::BScreen: managing screen %d "
                            "using visual 0x%lx, depth %d\n", 
                            "informational message saying screen number (%d), visual (%lx), and colour depth (%d)"),
            screenNumber(), XVisualIDFromVisual(rootWindow().visual()),
            rootWindow().depth());

    cycling_window = focused_list.end();
    
    rootWindow().setCursor(XCreateFontCursor(disp, XC_left_ptr));

    // load this screens resources
    fluxbox->load_rc(*this);

    // setup image cache engine
    m_image_control.reset(new FbTk::ImageControl(scrn, true, fluxbox->colorsPerChannel(),
                                                 fluxbox->getCacheLife(), fluxbox->getCacheMax()));
    imageControl().installRootColormap();
    root_colormap_installed = true;

    m_windowtheme->setFocusedAlpha(*resource.focused_alpha);
    m_windowtheme->setUnfocusedAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);
    m_menutheme->setMenuMode(*resource.menu_mode);
    // clamp values
    if (*resource.menu_delay > 5000)
        *resource.menu_delay = 5000;
    if (*resource.menu_delay < 0)
        *resource.menu_delay = 0;

    if (*resource.menu_delay_close > 5000)
        *resource.menu_delay_close = 5000;
    if (*resource.menu_delay_close < 0)
        *resource.menu_delay_close = 0;

    m_menutheme->setDelayOpen(*resource.menu_delay);
    m_menutheme->setDelayClose(*resource.menu_delay_close);

    imageControl().setDither(*resource.image_dither);

    winFrameTheme().reconfigSig().attach(this);// for geom window


    geom_visible = false;
    geom_pixmap = 0;
    pos_visible = false;
    pos_pixmap = 0;

    renderGeomWindow();
    renderPosWindow();

    // setup workspaces and workspace menu
    int nr_ws = *resource.workspaces;
    addWorkspace(); // at least one
    for (int i = 1; i < nr_ws; ++i) {
        addWorkspace();
    }

    m_current_workspace = m_workspaces_list.front();


    //!! TODO: we shouldn't do this more than once, but since slit handles their
    // own resources we must do this.
    fluxbox->load_rc(*this);

    m_configmenu.reset(createMenu(_FBTEXT(Menu, Configuration, 
                                  "Configuration", "Title of configuration menu")));
    setupConfigmenu(*m_configmenu.get());
    m_configmenu->setInternalMenu();

    // start with workspace 0
    changeWorkspaceID(0);
    updateNetizenWorkspaceCount();

    // we need to load win frame theme before we create any fluxbox window
    // and after we've load the resources
    // else we get some bad handle/grip height/width
    //    FbTk::ThemeManager::instance().loadTheme(*m_windowtheme.get());
    //!! TODO: For some strange reason we must load everything,
    // else the focus label doesn't get updated
    // So we lock root theme temporary so it doesn't uses RootTheme::reconfigTheme
    // This must be fixed in the future.
    m_root_theme->lock(true);
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename(),
                                        m_root_theme->screenNum());
    m_root_theme->lock(false);
    m_root_theme->setLineAttributes(*resource.gc_line_width,
                                    *resource.gc_line_style,
                                    *resource.gc_cap_style,
                                    *resource.gc_join_style);

#ifdef SLIT
    m_slit.reset(new Slit(*this, *layerManager().getLayer(fluxbox->getDesktopLayer()),
                 fluxbox->getSlitlistFilename().c_str()));
#endif // SLIT

    rm.unlock();

    XFlush(disp);
}

template <typename A>
void destroyAndClearList(A &a) {
    typedef typename A::iterator iterator;
    iterator it = a.begin();
    iterator it_end = a.end();
    for (; it != it_end; ++it)
        delete (*it);

    a.clear();
}

BScreen::~BScreen() {
    
    if (! managed)
        return;
    
    if (m_rootmenu.get() != 0)
        m_rootmenu->removeAll();
    
    // Since workspacemenu holds client list menus (from workspace)
    // we need to destroy it before we destroy workspaces
    m_workspacemenu.reset(0);

    if (geom_pixmap != None)
        imageControl().removeImage(geom_pixmap);

    if (pos_pixmap != None)
        imageControl().removeImage(pos_pixmap);

    removeWorkspaceNames();

    destroyAndClearList(m_workspaces_list);
    destroyAndClearList(m_netizen_list);

    //why not destroyAndClearList(m_icon_list); ? 
    //problem with that: a delete FluxboxWindow* calls m_diesig.notify()
    //which leads to screen.removeWindow() which leads to removeIcon(win)
    //which would modify the m_icon_list anyways...
    Icons tmp;
    tmp = m_icon_list;
    while(!tmp.empty()) {
        removeWindow(tmp.back());
        tmp.back()->restore(true);
        delete (tmp.back());
        tmp.pop_back();
    }
    
    if (hasXinerama() && m_xinerama_headinfo) {
        delete [] m_xinerama_headinfo;
    }

    // slit must be destroyed before headAreas (Struts)
    m_slit.reset(0);

    // TODO fluxgen: check if this is the right place
    delete [] m_head_areas;
}

void BScreen::initWindows() {
    unsigned int nchild;
    Window r, p, *children;
    Display *disp = FbTk::App::instance()->display();
    XQueryTree(disp, rootWindow().window(), &r, &p, &children, &nchild);

    Fluxbox *fluxbox = Fluxbox::instance();

    // preen the window list of all icon windows... for better dockapp support
    for (unsigned int i = 0; i < nchild; i++) {

        if (children[i] == None)
            continue;

        XWMHints *wmhints = XGetWMHints(disp, children[i]);

        if (wmhints) {
            if ((wmhints->flags & IconWindowHint) &&
                (wmhints->icon_window != children[i]))
                for (unsigned int j = 0; j < nchild; j++) {
                    if (children[j] == wmhints->icon_window) {
#ifdef DEBUG
                        cerr<<"BScreen::initWindows(): children[j] = 0x"<<hex<<children[j]<<dec<<endl;
                        cerr<<"BScreen::initWindows(): = icon_window"<<endl;
#endif // DEBUG
                        children[j] = None;
                        break;
                    }
                }
            XFree(wmhints);
        }

    }

    // manage shown windows
    // complexity: O(n^2) if we have lots of transients to transient_for
    // but usually O(n)
    Window transient_for = 0;
    for (unsigned int i = 0; i < nchild; ++i) {
        if (children[i] == None)
            continue;
        else if (!fluxbox->validateWindow(children[i])) {
#ifdef DEBUG
            cerr<<"BScreen::initWindows(): not valid window = "<<hex<<children[i]<<dec<<endl;
#endif // DEBUG
            children[i] = None;
            continue;
        }

        // if we have a transient_for window and it isn't created yet...
        // postpone creation of this window and find transient_for window
        // in the list and swap place with it so we can create transient_for window
        // first
        if (XGetTransientForHint(disp, children[i], &transient_for) &&
            fluxbox->searchWindow(transient_for) == 0) {
            // search forward for transient_for
            // and swap place with it so it gets created first
            unsigned int j = i + 1;
            for (; j < nchild; ++j) {
                if (children[j] == transient_for) {
                    swap(children[i], children[j]);
                    break;
                }
            }
            // reevaluate window
            if (!fluxbox->validateWindow(children[i]))
                continue;

#ifdef DEBUG
            cerr<<"BScreen::initWindows(): j = "<<j<<" i = "<<i<<" nchild = "<<nchild<<endl;
#endif // DEBUG

#ifdef DEBUG
            if (j < nchild)
                cerr<<"BScreen::initWindows(): postpone creation of 0x"<<hex<<children[j]<<dec<<endl;
            else
                cerr<<"BScreen::initWindows(): postpone creation of 0x"<<hex<<children[i]<<dec<<endl;
            cerr<<"BScreen::initWindows(): transient_for = 0x"<<hex<<transient_for<<dec<<endl;
#endif // DEBUG
        }


        XWindowAttributes attrib;
        if (XGetWindowAttributes(disp, children[i],
                                 &attrib)) {
            if (attrib.override_redirect) {
                children[i] = None; // we dont need this anymore, since we already created a window for it
                continue;
            }

            if (attrib.map_state != IsUnmapped) {
                FluxboxWindow *win = createWindow(children[i]);

                if (win) {
                    XMapRequestEvent mre;
                    mre.window = children[i];
                    win->mapRequestEvent(mre);
                }
            }
        }
        children[i] = None; // we dont need this anymore, since we already created a window for it
    }


    XFree(children);

}

unsigned int BScreen::currentWorkspaceID() const { 
    return m_current_workspace->workspaceID(); 
}

const Strut* BScreen::availableWorkspaceArea(int head) const {
    return m_head_areas[head ? head-1 : 0].availableWorkspaceArea();
}

unsigned int BScreen::maxLeft(int head) const {
	
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) : 
            getHeadX(head) + availableWorkspaceArea(head)->left();
    else
        return doFullMax() ? 0 : availableWorkspaceArea(head)->left();
}

unsigned int BScreen::maxRight(int head) const {
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) + getHeadWidth(head) : 
            getHeadX(head) + getHeadWidth(head) - availableWorkspaceArea(head)->right();
    else
        return doFullMax() ? width() : width() - availableWorkspaceArea(head)->right();
}

unsigned int BScreen::maxTop(int head) const {
    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) : getHeadY(head) + availableWorkspaceArea(head)->top();
    else
        return doFullMax() ? 0 : availableWorkspaceArea(head)->top();
}

unsigned int BScreen::maxBottom(int head) const {
    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) + getHeadHeight(head) :
            getHeadY(head) + getHeadHeight(head) - availableWorkspaceArea(head)->bottom();
    else
        return doFullMax() ? height() : height() - availableWorkspaceArea(head)->bottom();
}

void BScreen::update(FbTk::Subject *subj) {
    // for now we're only listening to the theme sig, so no object check
    // if another signal is added later, will need to differentiate here

    renderGeomWindow();
    renderPosWindow();
}

FbTk::Menu *BScreen::createMenu(const std::string &label) {
    FbTk::Menu *menu = new FbMenu(menuTheme(), 
                                  imageControl(), 
                                  *layerManager().getLayer(Fluxbox::instance()->getMenuLayer()));
    if (!label.empty())
        menu->setLabel(label.c_str());

    return menu;
}

void BScreen::hideMenus() {
    // hide extra menus
    Fluxbox::instance()->hideExtraMenus(*this);

#ifdef SLIT
    // hide slit menu
    if (slit())
        slit()->menu().hide();
#endif // SLIT

    // hide icon menus
    if (getIconList().size()) {
        Icons::iterator it = getIconList().begin();
        const Icons::iterator it_end = getIconList().end();
        for (; it != it_end; ++it)
            (*it)->menu().hide();
    }
    // hide all client menus
    hideWindowMenus(); 

}

void BScreen::hideWindowMenus(const FluxboxWindow* except) {
    Workspaces::iterator w_it = getWorkspacesList().begin();
    const Workspaces::iterator w_it_end = getWorkspacesList().end();
    for (; w_it != w_it_end; ++w_it) {
        if ((*w_it)->windowList().size()) {
            Workspace::Windows::iterator win_it = (*w_it)->windowList().begin();
            const Workspace::Windows::iterator win_it_end = (*w_it)->windowList().end();
            for (; win_it != win_it_end; ++win_it) {
                if (*win_it != except)
                    (*win_it)->menu().hide();
            }
        }
    }
}



void BScreen::reconfigure() {

    m_windowtheme->setFocusedAlpha(*resource.focused_alpha);
    m_windowtheme->setUnfocusedAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);
    m_menutheme->setMenuMode(*resource.menu_mode);

    // clamp values
    if (*resource.menu_delay > 5000)
        *resource.menu_delay = 5000;
    if (*resource.menu_delay < 0)
        *resource.menu_delay = 0;

    if (*resource.menu_delay_close > 5000)
        *resource.menu_delay_close = 5000;
    if (*resource.menu_delay_close < 0)
        *resource.menu_delay_close = 0;

    m_root_theme->setLineAttributes(*resource.gc_line_width,
                                    *resource.gc_line_style,
                                    *resource.gc_cap_style,
                                    *resource.gc_join_style);

    m_menutheme->setDelayOpen(*resource.menu_delay);
    m_menutheme->setDelayClose(*resource.menu_delay_close);

    renderGeomWindow();
    renderPosWindow();

    // realize the number of workspaces from the init-file
    const int nr_ws = *resource.workspaces;
    if (nr_ws > m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            addWorkspace();
        }
    } else if (nr_ws < m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            removeLastWorkspace();
        }
    }

    //reconfigure menus
    m_workspacemenu->reconfigure();
    m_configmenu->reconfigure();

    // We need to check to see if the timestamps
    // changed before we actually can restore the menus 
    // in the same way, since we can't really say if
    // any submenu is in the same place as before if the
    // menu changed.

    // if timestamp changed then no restoring
    bool restore_menus = ! Fluxbox::instance()->menuTimestampsChanged();

    // destroy old timestamps
    Fluxbox::instance()->clearMenuFilenames();

    // save submenu index so we can restore them afterwards
    vector<int> remember_sub;
    if (restore_menus) {
        FbTk::Menu *menu = m_rootmenu.get();
        while (menu) {
            int r = menu->currentSubmenu();
            if (r < 0) break;
            remember_sub.push_back(r);
            menu = menu->find(r)->submenu();
        }
    }

    rereadMenu();

    if (restore_menus) {
        // restore submenus, no timestamp changed
        FbTk::Menu *menu = m_rootmenu.get();
        for (int i = 0; i < (int)remember_sub.size(); i++ ) {
            int sub = remember_sub[i];
            if (!menu || sub < 0) 
                break;
            FbTk::MenuItem *item = menu->find(sub);
            if (item != 0) {
                menu->drawSubmenu(sub);
                menu = item->submenu();
            } else
                menu = 0;

        }
    }

    // reconfigure workspaces
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::reconfigure));

    // reconfigure Icons
    for_each(m_icon_list.begin(),
             m_icon_list.end(),
             mem_fun(&FluxboxWindow::reconfigure));

    imageControl().cleanCache();
    // notify objects that the screen is reconfigured
    m_reconfigure_sig.notify();

}


void BScreen::rereadMenu() {
    initMenu();
    m_rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames() {
    m_workspace_names.clear();
}

void BScreen::updateWorkspaceNamesAtom() {
    m_workspacenames_sig.notify();
}

void BScreen::addIcon(FluxboxWindow *w) {
    if (w == 0) 
        return;

    // make sure we have a unique list
    if (find(getIconList().begin(), getIconList().end(), w) != getIconList().end())
        return;

    m_icon_list.push_back(w);

    // notify listeners
    m_iconlist_sig.notify();
}


void BScreen::removeIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    Icons::iterator erase_it = remove_if(getIconList().begin(),
                                         getIconList().end(),
                                         bind2nd(equal_to<FluxboxWindow *>(), w));
    // no need to send iconlist signal if we didn't 
    // change the iconlist
    if (erase_it != m_icon_list.end()) {
        getIconList().erase(erase_it);
        m_iconlist_sig.notify();
    }
}

void BScreen::removeWindow(FluxboxWindow *win) {
#ifdef DEBUG
    cerr<<"BScreen::removeWindow("<<win<<")"<<endl;
#endif // DEBUG
    // extra precaution, if for some reason, the 
    // icon list should be out of sync
    removeIcon(win);
    // remove from workspace
    Workspace *space = getWorkspace(win->workspaceNumber());
    if (space != 0)
        space->removeWindow(win, false);
}


void BScreen::removeClient(WinClient &client) {

    WinClient *cyc = 0;
    if (cycling_window != focused_list.end())
        cyc = *cycling_window;

    focused_list.remove(&client);
    if (cyc == &client) {
        cycling_window = focused_list.end();
    }

    if (cycling_last == &client)
        cycling_last = 0;

    for_each(getWorkspacesList().begin(), getWorkspacesList().end(),
             mem_fun(&Workspace::updateClientmenu));

    using namespace FbTk;

    // remove any grouping this is expecting
    Groupables::iterator erase_it = find_if(m_expecting_groups.begin(),
                                            m_expecting_groups.end(),
                                            Compose(bind2nd(equal_to<WinClient *>(), &client),
                                                    Select2nd<Groupables::value_type>()));

    if (erase_it != m_expecting_groups.end())
        m_expecting_groups.erase(erase_it);

    // the client could be on icon menu so we update it
    //!! TODO: check this with the new icon menu
    //    updateIconMenu();

}

int BScreen::addWorkspace() {

    bool save_name = getNameOfWorkspace(m_workspaces_list.size()) != "" ? false : true;
    Workspace *wkspc = new Workspace(*this, m_layermanager, 
                                     getNameOfWorkspace(m_workspaces_list.size()),
                                     m_workspaces_list.size());
    m_workspaces_list.push_back(wkspc);
    
    if (save_name)
        addWorkspaceName(wkspc->name().c_str()); //update names

    saveWorkspaces(m_workspaces_list.size());
    
    updateNetizenWorkspaceCount();	
	
    return m_workspaces_list.size();
	
}

/// removes last workspace
/// @return number of desktops left
int BScreen::removeLastWorkspace() {
    if (m_workspaces_list.size() <= 1)
        return 0;
    Workspace *wkspc = m_workspaces_list.back();

    if (m_current_workspace->workspaceID() == wkspc->workspaceID())
        changeWorkspaceID(m_current_workspace->workspaceID() - 1);

    wkspc->removeAll();

    //remove last workspace
    m_workspaces_list.pop_back();		

    updateNetizenWorkspaceCount();
    saveWorkspaces(m_workspaces_list.size());
    // must be deleted after we send notify!!
    // so we dont get bad pointers somewhere
    // while processing the notify signal
    delete wkspc;

    return m_workspaces_list.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {

    if (! m_current_workspace || id >= m_workspaces_list.size() ||
        id == m_current_workspace->workspaceID())
        return;

    FbTk::App::instance()->sync(false);

    WinClient *focused_client = Fluxbox::instance()->getFocusedWindow();
    FluxboxWindow *focused = 0;
    if (focused_client)
        focused = focused_client->fbwindow();
        
    if (focused && focused->isMoving()) {
        if (doOpaqueMove())
            reassociateWindow(focused, id, true);
        // don't reassociate if not opaque moving
        focused->pauseMoving();
    }

    // reassociate all windows that are stuck to the new workspace
    Workspace *wksp = currentWorkspace();
    Workspace::Windows wins = wksp->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it)->isStuck()) {
            reassociateWindow(*it, id, true);
        }
    }

    currentWorkspace()->hideAll(false);

    // set new workspace
    m_current_workspace = getWorkspace(id);

    // This is a little tricks to reduce flicker 
    // this way we can set focus pixmap on frame before we show it
    // and using ExposeEvent to redraw without flicker
    /*    
    WinClient *win = getLastFocusedWindow(currentWorkspaceID());
    if (win && win->fbwindow())
        win->fbwindow()->setFocusFlag(true);
    */

    currentWorkspace()->showAll();

    if (focused && (focused->isStuck() || focused->isMoving()))
        focused->setInputFocus();
    else
        Fluxbox::instance()->revertFocus(*this);

    if (focused && focused->isMoving())
        focused->resumeMoving();

    updateNetizenCurrentWorkspace();
    FbTk::App::instance()->sync(false);

}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
    if (! m_current_workspace || id >= m_workspaces_list.size())
        return;

    if (!win) {
        WinClient *client = Fluxbox::instance()->getFocusedWindow();
        if (client) 
            win = client->fbwindow();
    }


    FbTk::App::instance()->sync(false);

    if (win && &win->screen() == this &&
        (! win->isStuck())) {

        // if iconified, deiconify it before we send it somewhere
        if (win->isIconic())
            win->deiconify();

        // if the window isn't on current workspace, hide it
        if (id != currentWorkspace()->workspaceID())
            win->withdraw(true);

        reassociateWindow(win, id, true);

        // if the window is on current workspace, show it.
        if (id == currentWorkspace()->workspaceID())
            win->deiconify(false, false);

        // change workspace ?
        if (changeWS && id != currentWorkspace()->workspaceID()) {
            changeWorkspaceID(id);
            win->setInputFocus();
        }

    }

}


void BScreen::addNetizen(Window win) {
    Netizen *net = new Netizen(*this, win);
    m_netizen_list.push_back(net);

    net->sendWorkspaceCount();
    net->sendCurrentWorkspace();

    // send all windows to netizen
    Workspaces::iterator it = m_workspaces_list.begin();
    Workspaces::iterator it_end = m_workspaces_list.end();
    for (; it != it_end; ++it) {
        Workspace::Windows::iterator win_it = (*it)->windowList().begin();
        Workspace::Windows::iterator win_it_end = (*it)->windowList().end();
        for (; win_it != win_it_end; ++win_it) {
            net->sendWindowAdd((*win_it)->clientWindow(), 
                               (*it)->workspaceID());
        }
    }

    Window f = ((Fluxbox::instance()->getFocusedWindow()) ?
		Fluxbox::instance()->getFocusedWindow()->window() : None);
    net->sendWindowFocus(f);
}

void BScreen::removeNetizen(Window w) {
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == w) {
            Netizen *n = *it;
            delete n;
            m_netizen_list.erase(it);			
            break;
        }
    }
}


void BScreen::updateNetizenCurrentWorkspace() {
    m_currentworkspace_sig.notify();
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             mem_fun(&Netizen::sendCurrentWorkspace));
}


void BScreen::updateNetizenWorkspaceCount() {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             mem_fun(&Netizen::sendWorkspaceCount));
    m_workspacecount_sig.notify();	
}


void BScreen::updateNetizenWindowFocus() {
    Window f = ((Fluxbox::instance()->getFocusedWindow()) ?
                Fluxbox::instance()->getFocusedWindow()->window() : None);
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowFocus), f));
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {

    // update the list of clients
    m_clientlist_sig.notify();
    
    // and then send the signal to listeners
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowAdd(w, p);
    }
	
}


void BScreen::updateNetizenWindowDel(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowDel), w));
	
    m_clientlist_sig.notify();
}


void BScreen::updateNetizenWindowRaise(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowRaise), w));
}


void BScreen::updateNetizenWindowLower(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowLower), w));
}

void BScreen::updateNetizenConfigNotify(XEvent &e) {
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it)
        (*it)->sendConfigNotify(e);
}

bool BScreen::isKdeDockapp(Window client) const {
    //Check and see if client is KDE dock applet.
    //If so add to Slit
    bool iskdedockapp = false;
    Atom ajunk;
    int ijunk;
    unsigned long *data = 0, uljunk;
    Display *disp = FbTk::App::instance()->display();
    // Check if KDE v2.x dock applet
    if (XGetWindowProperty(disp, client,
                           XInternAtom(FbTk::App::instance()->display(), 
                                       "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False),
                           0l, 1l, False,
                           XA_WINDOW, &ajunk, &ijunk, &uljunk,
                           &uljunk, (unsigned char **) &data) == Success) {
					
        if (data)
            iskdedockapp = true;
        XFree((void *) data);
        data = 0;
    }

    // Check if KDE v1.x dock applet
    if (!iskdedockapp) {
        Atom kwm1 = XInternAtom(FbTk::App::instance()->display(), 
                                "KWM_DOCKWINDOW", False);
        if (XGetWindowProperty(disp, client,
                               kwm1, 0l, 1l, False,
                               kwm1, &ajunk, &ijunk, &uljunk,
                               &uljunk, (unsigned char **) &data) == Success && data) {
            iskdedockapp = (data && data[0] != 0);
            XFree((void *) data);
            data = 0;
        }
    } 

    return iskdedockapp;
}

bool BScreen::addKdeDockapp(Window client) {

    XSelectInput(FbTk::App::instance()->display(), client, StructureNotifyMask);
    char intbuff[16];    
    sprintf(intbuff, "%d", screenNumber());
    std::string atom_name("_NET_SYSTEM_TRAY_S");
    atom_name += intbuff; // append number
    // find the right atomhandler that has the name: _NET_SYSTEM_TRAY_S<num>
    AtomHandler *handler = Fluxbox::instance()->getAtomHandler(atom_name);
    FbTk::EventHandler *evh  = 0;
    FbTk::EventManager *evm = FbTk::EventManager::instance();
    if (handler == 0) {
#ifdef SLIT
        if (slit() != 0)
            slit()->addClient(client);
        else
#endif // SLIT
            return false;
    } else {
        // this handler is a special case 
        // so we call setupClient in it
        WinClient winclient(client, *this);
        handler->setupClient(winclient);
        // we need to save old handler and re-add it later
        evh = evm->find(client);
    }

    if (evh != 0) // re-add handler
        evm->add(*evh, client);

    return true;
}

FluxboxWindow *BScreen::createWindow(Window client) {
    FbTk::App::instance()->sync(false);


    if (isKdeDockapp(client) && addKdeDockapp(client)) {
        return 0; // dont create a FluxboxWindow for this one
    }

    WinClient *winclient = new WinClient(client, *this);

    if (winclient->initial_state == WithdrawnState) {
        delete winclient;
#ifdef SLIT
        if (slit())
            slit()->addClient(client);
#endif // SLIT
        return 0;
    }

    bool new_win = false;

    // check if it should be grouped with something else
    FluxboxWindow *win;
    if ((win = findGroupLeft(*winclient)) != 0) {
        win->attachClient(*winclient);
        Fluxbox::instance()->attachSignals(*winclient);
    } else {

        Fluxbox::instance()->attachSignals(*winclient);
        if (winclient->fbwindow()) // may have been set in an atomhandler
            win = winclient->fbwindow();
        else {
            win = new FluxboxWindow(*winclient,
                                    winFrameTheme(),
                                    *layerManager().getLayer(Fluxbox::instance()->getNormalLayer()));
            
            new_win = true;

            if (!win->isManaged()) {
                delete win;
                return 0;
            } 
        }
    }
                
    // always put on end of focused list, if it gets focused it'll get pushed up
    // there is only the one win client at this stage
    if (doFocusNew())
        focused_list.push_front(&win->winClient());
    else
        focused_list.push_back(&win->winClient());
 
    // we also need to check if another window expects this window to the left
    // and if so, then join it.
    FluxboxWindow *otherwin = 0;
    // TODO: does this do the right stuff focus-wise?
    if ((otherwin = findGroupRight(*winclient)) && otherwin != win) {
        win->attachClient(otherwin->winClient());
    }

    m_clientlist_sig.notify();

    FbTk::App::instance()->sync(false);
    return win;
}


FluxboxWindow *BScreen::createWindow(WinClient &client) {

    if (isKdeDockapp(client.window()) && addKdeDockapp(client.window())) {
        return 0;
    }

    FluxboxWindow *win = new FluxboxWindow(client,
                                           winFrameTheme(),
                                           *layerManager().getLayer(Fluxbox::instance()->getNormalLayer()));

#ifdef SLIT
    if (win->initialState() == WithdrawnState && slit() != 0) {
        slit()->addClient(win->clientWindow());
    }
#endif // SLIT
                 

    if (!win->isManaged()) {
        delete win;
        return 0;
    }

    m_clientlist_sig.notify();

    return win;
}

Strut *BScreen::requestStrut(int head, int left, int right, int top, int bottom) {
    if (head > numHeads() && head != 1) {
        // head does not exist (if head == 1, then numHeads() == 0, 
        // which means no xinerama, but there's a head after all
        head = numHeads();
    }

    int begin = head-1;
    int end   = head;

    if (head == 0) { // all heads (or no xinerama)
        begin = 0;
        end = (numHeads() ? numHeads() : 1);
    }

    Strut* next = 0;
    for (int i = begin; i != end; i++) {
        next = m_head_areas[i].requestStrut(i+1, left, right, top, bottom, next);
    }

    return next;
}

void BScreen::clearStrut(Strut *str) {
    if (str->next()) 
        clearStrut(str->next());
    int head = str->head() ? str->head() - 1 : 0;
    m_head_areas[head].clearStrut(str);
    // str is invalid now
}

void BScreen::updateAvailableWorkspaceArea() {
    size_t n = (numHeads() ? numHeads() : 1);
    bool updated = false;

    for (size_t i = 0; i < n; i++) {
        updated = m_head_areas[i].updateAvailableWorkspaceArea() || updated;
    }

    if (updated)
        m_workspace_area_sig.notify();
}

void BScreen::addWorkspaceName(const char *name) {
    m_workspace_names.push_back(name);
}


string BScreen::getNameOfWorkspace(unsigned int workspace) const {
    if (workspace < m_workspace_names.size())
        return m_workspace_names[workspace];
    else
        return "";
}

void BScreen::reassociateWindow(FluxboxWindow *w, unsigned int wkspc_id, 
                                bool ignore_sticky) {
    if (w == 0)
        return;

    if (wkspc_id >= getCount())
        wkspc_id = currentWorkspace()->workspaceID();

    if (!w->isIconic() && w->workspaceNumber() == wkspc_id)
        return;


    if (w->isIconic()) {
        removeIcon(w);
        getWorkspace(wkspc_id)->addWindow(*w);
        // client list need to notify now even though
        // we didn't remove/add any window,
        // so listeners that uses the client list to 
        // show whats on current/other workspace
        // gets updated
        m_clientlist_sig.notify(); 
    } else if (ignore_sticky || ! w->isStuck()) {
        // fresh windows have workspaceNumber == -1, which leads to
        // an invalid workspace (unsigned int)
        if (getWorkspace(w->workspaceNumber()))
            getWorkspace(w->workspaceNumber())->removeWindow(w, true);
        getWorkspace(wkspc_id)->addWindow(*w);
        // see comment above
        m_clientlist_sig.notify();
    }
}


void BScreen::nextFocus(int opts) {
    const int num_windows = currentWorkspace()->numberOfWindows();

    if (num_windows < 1)
        return;

    if (!(opts & CYCLELINEAR)) {
        if (!cycling_focus) {
            cycling_focus = True;
            cycling_window = focused_list.begin();
            cycling_last = 0;
        } else {
            // already cycling, so restack to put windows back in their proper order
            m_layermanager.restack();
        }
        // if it is stacked, we want the highest window in the focused list
        // that is on the same workspace
        FocusedWindows::iterator it = cycling_window;
        const FocusedWindows::iterator it_end = focused_list.end();

        while (true) {
            ++it;
            if (it == it_end) {
                it = focused_list.begin();
            }
            // give up [do nothing] if we reach the current focused again
            if ((*it) == (*cycling_window)) {
                break;
            }

            FluxboxWindow *fbwin = (*it)->fbwindow();
            if (fbwin && !fbwin->isIconic() &&
                (fbwin->isStuck() 
                 || fbwin->workspaceNumber() == currentWorkspaceID())) {
                // either on this workspace, or stuck

                // keep track of the originally selected window in a set
                WinClient &last_client = fbwin->winClient();

                if (! (doSkipWindow(**it, opts) || !fbwin->setCurrentClient(**it)) ) {
                    // moved onto a new fbwin
                    if (!cycling_last || cycling_last->fbwindow() != fbwin) {
                        if (cycling_last)
                            // set back to orig current Client in that fbwin
                            cycling_last->fbwindow()->setCurrentClient(*cycling_last, false);
                        cycling_last = &last_client;
                    }
                    fbwin->tempRaise();
                    break;
                }
            }
        }
        cycling_window = it;
    } else { // not stacked cycling
        // I really don't like this, but evidently some people use it(!)
        Workspace *wksp = currentWorkspace();
        Workspace::Windows &wins = wksp->windowList();
        Workspace::Windows::iterator it = wins.begin();

        FluxboxWindow *focused_group = 0;
        // start from the focused window
        bool have_focused = false;
        WinClient *focused = Fluxbox::instance()->getFocusedWindow();
        if (focused != 0) {
            if (focused->screen().screenNumber() == screenNumber()) {
                have_focused = true;
                focused_group = focused->fbwindow();
            }
        }

        if (!have_focused) {
            focused_group = (*it);
        } else {
            // get focused window iterator
            for (; it != wins.end() && (*it) != focused_group; ++it) 
                continue;
        }

        do {
            ++it;
            if (it == wins.end())
                it = wins.begin();
            // see if the window should be skipped
            if (! (doSkipWindow((*it)->winClient(), opts) || !(*it)->setInputFocus()) )
                break;
        } while ((*it) != focused_group);

        if ((*it) != focused_group && it != wins.end())
            (*it)->raise();
    }

}


void BScreen::prevFocus(int opts) {
    int num_windows = currentWorkspace()->numberOfWindows();
	
    if (num_windows < 1)
        return;

    if (!(opts & CYCLELINEAR)) {
        if (!cycling_focus) {
            cycling_focus = true;
            cycling_window = focused_list.end();
            cycling_last = 0;
        } else {
            // already cycling, so restack to put windows back in their proper order
            m_layermanager.restack();
        }
        // if it is stacked, we want the highest window in the focused list
        // that is on the same workspace
        FocusedWindows::iterator it = cycling_window;
        FocusedWindows::iterator it_end = focused_list.end();

        while (true) {
            --it;
            if (it == it_end) {
                it = focused_list.end();
                --it;
            }
            // give up [do nothing] if we reach the current focused again
            if ((*it) == (*cycling_window)) {
                break;
            }

            FluxboxWindow *fbwin = (*it)->fbwindow();
            if (fbwin && !fbwin->isIconic() &&
                (fbwin->isStuck() 
                 || fbwin->workspaceNumber() == currentWorkspaceID())) {
                // either on this workspace, or stuck

                // keep track of the originally selected window in a set
                WinClient &last_client = fbwin->winClient();


                if (! (doSkipWindow(**it, opts) || !fbwin->setCurrentClient(**it)) ) {
                    // moved onto a new fbwin
                    if (!cycling_last || cycling_last->fbwindow() != fbwin) {
                        if (cycling_last)
                            // set back to orig current Client in that fbwin
                            cycling_last->fbwindow()->setCurrentClient(*cycling_last, false);
                        cycling_last = &last_client;
                    }
                    fbwin->tempRaise();
                    break;
                }
            }
        }
        cycling_window = it;
    } else { // not stacked cycling
            
        Workspace *wksp = currentWorkspace();
        Workspace::Windows &wins = wksp->windowList();
        Workspace::Windows::iterator it = wins.begin();
            
        FluxboxWindow *focused_group = 0;
        // start from the focused window
        bool have_focused = false;
        WinClient *focused = Fluxbox::instance()->getFocusedWindow();
        if (focused != 0) {
            if (focused->screen().screenNumber() == screenNumber()) {
                have_focused = true;
                focused_group = focused->fbwindow();
            }
        }

        if (!have_focused) {
            focused_group = (*it);
        } else {
            //get focused window iterator
            for (; it != wins.end() && (*it) != focused_group; ++it) 
                continue;
        }

        do {
            if (it == wins.begin())
                it = wins.end();
            --it;
            // see if the window should be skipped
            if (! (doSkipWindow((*it)->winClient(), opts) || !(*it)->setInputFocus()) )
                break;
        } while ((*it) != focused_group);
            
        if ((*it) != focused_group && it != wins.end())
            (*it)->raise();
    }
}


void BScreen::raiseFocus() {
    bool have_focused = false;
    Fluxbox &fb = *Fluxbox::instance();
    // set have_focused if the currently focused window 
    // is on this screen
    if (fb.getFocusedWindow()) {
        if (fb.getFocusedWindow()->screen().screenNumber() == screenNumber()) {
            have_focused = true;
        }
    }

    // if we have a focused window on this screen and
    // number of windows is greater than one raise the focused window
    if (currentWorkspace()->numberOfWindows() > 1 && have_focused)
        fb.getFocusedWindow()->raise();
}

void BScreen::setFocusedWindow(WinClient &winclient) {
    // raise newly focused window to the top of the focused list
    if (!cycling_focus) { // don't change the order if we're cycling
        focused_list.remove(&winclient);
        focused_list.push_front(&winclient);
        cycling_window = focused_list.begin();
    }
}

void BScreen::dirFocus(FluxboxWindow &win, const FocusDir dir) {
    // change focus to the window in direction dir from the given window

    // we scan through the list looking for the window that is "closest"
    // in the given direction

    FluxboxWindow *foundwin = 0;
    int weight = 999999, exposure = 0; // extreme values
    int borderW = winFrameTheme().border().width(),
        top = win.y(), 
        bottom = win.y() + win.height() + 2*borderW,
        left = win.x(),
        right = win.x() + win.width() + 2*borderW;

    Workspace::Windows &wins = currentWorkspace()->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it) == &win 
            || (*it)->isIconic() 
            || (*it)->isFocusHidden() 
            || !(*it)->winClient().acceptsFocus()) 
            continue; // skip self
        
        // we check things against an edge, and within the bounds (draw a picture)
        int edge=0, upper=0, lower=0, oedge=0, oupper=0, olower=0;

        int otop = (*it)->y(), 
            obottom = (*it)->y() + (*it)->height() + 2*borderW,
            oleft = (*it)->x(),
            oright = (*it)->x() + (*it)->width() + 2*borderW;
        // check if they intersect
        switch (dir) {
        case FOCUSUP:
            edge = obottom;
            oedge = bottom;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSDOWN:
            edge = top;
            oedge = otop;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSLEFT:
            edge = oright;
            oedge = right;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        case FOCUSRIGHT:
            edge = left;
            oedge = oleft;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        }

        if (oedge < edge) continue; // not in the right direction
        if (olower <= upper || oupper >= lower) {
            // outside our horz bounds, get a heavy weight penalty
            int myweight = 100000 + oedge - edge + abs(upper-oupper)+abs(lower-olower);
            if (myweight < weight) {
                foundwin = *it;
                exposure = 0;
                weight = myweight;
            }
        } else if ((oedge - edge) < weight) {
            foundwin = *it;
            weight = oedge - edge;
            exposure = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
        } else if (foundwin && oedge - edge == weight) {
            int myexp = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
            if (myexp > exposure) {
                foundwin = *it;
                // weight is same
                exposure = myexp;
            }
        } // else not improvement
    }

    if (foundwin) 
        foundwin->setInputFocus();
}
void BScreen::initMenus() {
    m_workspacemenu.reset(MenuCreator::createMenuType("workspacemenu", screenNumber()));
    initMenu();
}

void BScreen::initMenu() {
	
    if (m_rootmenu.get()) {
        // since all menus in root is submenus in m_rootmenu
        // just remove every item in m_rootmenu and then clear m_rootmenu_list
        while (m_rootmenu->numberOfItems())
            m_rootmenu->remove(0); 
        m_rootmenu_list.clear();

    } else
        m_rootmenu.reset(createMenu(""));

    Fluxbox * const fb = Fluxbox::instance();
    if (fb->getMenuFilename().size() > 0) {
        m_rootmenu.reset(MenuCreator::createFromFile(fb->getMenuFilename(),
                                                     screenNumber(), true));

    }

    if (m_rootmenu.get() == 0) {
        _FB_USES_NLS;
        m_rootmenu.reset(createMenu(_FBTEXT(Menu, DefaultRootMenu, "Fluxbox default menu", "Title of fallback root menu")));
        FbTk::RefCount<FbTk::Command> restart_fb(CommandParser::instance().parseLine("restart"));
        FbTk::RefCount<FbTk::Command> exit_fb(CommandParser::instance().parseLine("exit"));
        FbTk::RefCount<FbTk::Command> execute_xterm(CommandParser::instance().parseLine("exec xterm"));
        m_rootmenu->setInternalMenu();
        m_rootmenu->insert(_FBTEXT(Menu, xterm, "xterm", "xterm - in fallback menu"),
                           execute_xterm);
        m_rootmenu->insert(_FBTEXT(Menu, Restart, "Restart", "Restart command"),
                           restart_fb);
        m_rootmenu->insert(_FBTEXT(Menu, Exit, "Exit", "Exit command"),
                           exit_fb);
    }

    m_rootmenu->updateMenu();
}


void BScreen::addConfigMenu(const char *label, FbTk::Menu &menu) {
    m_configmenu_list.push_back(std::make_pair(label, &menu));
    setupConfigmenu(*m_configmenu.get());
}

void BScreen::removeConfigMenu(FbTk::Menu &menu) {
    Configmenus::iterator erase_it = find_if(m_configmenu_list.begin(),
                                             m_configmenu_list.end(),
                                             FbTk::Compose(bind2nd(equal_to<FbTk::Menu *>(), &menu),
                                                           FbTk::Select2nd<Configmenus::value_type>()));
    if (erase_it != m_configmenu_list.end())
        m_configmenu_list.erase(erase_it);
    setupConfigmenu(*m_configmenu.get());

}    

void BScreen::setupConfigmenu(FbTk::Menu &menu) {
    _FB_USES_NLS;

    menu.removeAll();

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), 
                                                                              &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command> reconf_cmd(CommandParser::instance().parseLine("reconfigure"));
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    // create focus menu
    // we don't set this to internal menu so will 
    // be deleted toghether with the parent
    const char *focusmenu_label = _FBTEXT(Configmenu, FocusModel,
                                          "Focus Model", "Method used to give focus to windows");
    FbTk::Menu *focus_menu = createMenu(focusmenu_label ? focusmenu_label : "");

#define _FOCUSITEM(a, b, c, d, e) focus_menu->insert(new FocusModelMenuItem(_FBTEXT(a, b, c, d), *this, e, save_and_reconfigure))

    _FOCUSITEM(Configmenu, ClickToFocus,
               "Click To Focus", "Click to focus",
               CLICKTOFOCUS);
    _FOCUSITEM(Configmenu, SloppyFocus,
               "Sloppy Focus", "Sloppy Focus",
               SLOPPYFOCUS);
    _FOCUSITEM(Configmenu, SemiSloppyFocus,
               "Semi Sloppy Focus", "Semi Sloppy Focus",
               SEMISLOPPYFOCUS);
#undef _FOCUSITEM

    focus_menu->insert(new BoolMenuItem(_FBTEXT(Configmenu, 
                                                AutoRaise,
                                                "Auto Raise",
                                                "Auto Raise windows on sloppy"),
                                        *resource.auto_raise,
                                        save_and_reconfigure));

    focus_menu->updateMenu();

    menu.insert(focusmenu_label, focus_menu);
#ifdef SLIT
    if (slit() != 0) {
        slit()->menu().setInternalMenu();
        menu.insert("Slit", &slit()->menu());
    }
#endif // SLIT

    Configmenus::iterator it = m_configmenu_list.begin();
    Configmenus::iterator it_end = m_configmenu_list.end();
    for (; it != it_end; ++it)
        menu.insert(it->first, it->second);

#define _BOOLITEM(a, b, c, d, e, f) menu.insert(new BoolMenuItem(_FBTEXT(a, b, c, d), e, f))
                             
    _BOOLITEM(Configmenu, ImageDithering,
              "Image Dithering", "Image Dithering",
              *resource.image_dither, save_and_reconfigure);
    _BOOLITEM(Configmenu, OpaqueMove,
              "Opaque Window Moving", "Window Moving with whole window visible (as opposed to outline moving)",
              *resource.opaque_move, saverc_cmd);
    _BOOLITEM(Configmenu, FullMax,
              "Full Maximization", "Maximise over slit, toolbar, etc",
              *resource.full_max, saverc_cmd);
    _BOOLITEM(Configmenu, FocusNew,
              "Focus New Windows", "Focus newly created windows",
              *resource.focus_new, saverc_cmd);
    _BOOLITEM(Configmenu, FocusLast,
              "Focus Last Window on Workspace", "Focus Last Window on Workspace",
              *resource.focus_last, saverc_cmd);
    _BOOLITEM(Configmenu, WorkspaceWarping,
              "Workspace Warping", "Workspace Warping - dragging windows to the edge and onto the next workspace",
              *resource.workspace_warping, saverc_cmd);
    _BOOLITEM(Configmenu, DesktopWheeling,
              "Desktop MouseWheel Switching", "Workspace switching using mouse wheel",
              *resource.desktop_wheeling, saverc_cmd);
    _BOOLITEM(Configmenu, DecorateTransient,
              "Decorate Transient Windows", "Decorate Transient Windows",
              *resource.decorate_transient, saverc_cmd);
    _BOOLITEM(Configmenu, ClickRaises,
              "Click Raises", "Click Raises", 
              *resource.click_raises, saverc_cmd);

#ifdef HAVE_XRENDER
    if (FbTk::Transparent::haveRender() ||
        FbTk::Transparent::haveComposite()) {

        const char *alphamenu_label = _FBTEXT(Configmenu, Transparency,
                                          "Transparency", "Menu containing various transparency options");
        FbTk::Menu *alpha_menu = createMenu(alphamenu_label ? alphamenu_label : "");

        if (FbTk::Transparent::haveComposite(true)) {
            alpha_menu->insert(new BoolMenuItem(_FBTEXT(Configmenu, ForcePseudoTrans,
                               "Force Pseudo-Transparency", "When composite is available, still use old pseudo-transparency"),
                    Fluxbox::instance()->getPseudoTrans(), save_and_reconfigure));
        }

        FbTk::MenuItem *focused_alpha_item = new IntResMenuItem(_FBTEXT(Configmenu, FocusedAlpha, "Focused Window Alpha", "Transparency level of the focused window"),
                    resource.focused_alpha, 0, 255, *alpha_menu);
        focused_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(focused_alpha_item);

        FbTk::MenuItem *unfocused_alpha_item = new IntResMenuItem(_FBTEXT(Configmenu, UnfocusedAlpha, "Unfocused Window Alpha", "Transparency level of unfocused windows"),
                    resource.unfocused_alpha, 0, 255, *alpha_menu);
        unfocused_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(unfocused_alpha_item);

        FbTk::MenuItem *menu_alpha_item = new IntResMenuItem(_FBTEXT(Configmenu, MenuAlpha, "Menu Alpha", "Transparency level of menu"),
                    resource.menu_alpha, 0, 255, *alpha_menu);
        menu_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(menu_alpha_item);

        alpha_menu->updateMenu();
        menu.insert(alphamenu_label, alpha_menu);
    }
#endif // HAVE_XRENDER
#undef _BOOLITEM

    // finaly update menu 
    menu.updateMenu();
}


void BScreen::shutdown() {
    rootWindow().setEventMask(NoEventMask);
    FbTk::App::instance()->sync(false);
    m_shutdown = true;
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::shutdown));
}


void BScreen::showPosition(int x, int y) {
        if (!doShowWindowPos())
        return;

    if (! pos_visible) {
        if (hasXinerama()) {
            unsigned int head = getCurrHead();

            m_pos_window.move(getHeadX(head) + (getHeadWidth(head) - m_pos_window.width()) / 2,
                              getHeadY(head) + (getHeadHeight(head) - m_pos_window.height()) / 2);
                            
        } else {
            m_pos_window.move((width() - m_pos_window.width()) / 2, (height() - m_pos_window.height()) / 2);
        }

        m_pos_window.show();
        m_pos_window.raise();

        pos_visible = true;
    }
    char label[256];

    _FB_USES_NLS;

    sprintf(label,
            _FBTEXT(Screen, PositionFormat,
                    "X: %4d x Y: %4d",
                    "Format for screen coordinates - %4d for X, and %4d for Y"), x, y);

    m_pos_window.clear();

    winFrameTheme().font().drawText(m_pos_window,
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    winFrameTheme().bevelWidth(), 
                                    winFrameTheme().bevelWidth() + 
                                    winFrameTheme().font().ascent());
		
}


void BScreen::hidePosition() {
    if (pos_visible) {
        m_pos_window.hide();
        pos_visible = false;
    }
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
    if (!doShowWindowPos())
        return;

    if (! geom_visible) {
        if (hasXinerama()) {
            unsigned int head = getCurrHead();

            m_geom_window.move(getHeadX(head) + (getHeadWidth(head) - m_geom_window.width()) / 2,
                             getHeadY(head) + (getHeadHeight(head) - m_geom_window.height()) / 2);
        } else {
            m_geom_window.move((width() - m_geom_window.width()) / 2, (height() - m_geom_window.height()) / 2);

        }
        m_geom_window.show();
        m_geom_window.raise();

        geom_visible = true;
    }
	
    char label[256];
    _FB_USES_NLS;

    sprintf(label,
            _FBTEXT(Screen, GeometryFormat,
                    "W: %4d x H: %4d",
                    "Format for width and height window, %4d for width, and %4d for height"),
            gx, gy);

    m_geom_window.clear();

    //!! TODO: geom window again?! repeated
    winFrameTheme().font().drawText(m_geom_window,
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    winFrameTheme().bevelWidth(), 
                                    winFrameTheme().bevelWidth() + 
                                    winFrameTheme().font().ascent());	
}


void BScreen::hideGeometry() {
    if (geom_visible) {
        m_geom_window.hide();
        geom_visible = false;
    }
}

void BScreen::setLayer(FbTk::XLayerItem &item, int layernum) {
    m_layermanager.moveToLayer(item, layernum);
}


/**
 Goes to the workspace "right" of the current
*/
void BScreen::nextWorkspace(const int delta) {
    changeWorkspaceID( (currentWorkspaceID() + delta) % getCount());
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::prevWorkspace(const int delta) {
    changeWorkspaceID( (currentWorkspaceID() - delta + getCount()) % getCount());
}

/**
 Goes to the workspace "right" of the current
*/
void BScreen::rightWorkspace(const int delta) {
    if (currentWorkspaceID()+delta < getCount())
        changeWorkspaceID(currentWorkspaceID()+delta);
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::leftWorkspace(const int delta) {
    if (currentWorkspaceID() >= static_cast<unsigned int>(delta))
        changeWorkspaceID(currentWorkspaceID()-delta);
}

/**
  @return true if the windows should be skiped else false
*/
bool BScreen::doSkipWindow(const WinClient &winclient, int opts) {
    const FluxboxWindow *win = winclient.fbwindow();
    return (!win ||
            (opts & CYCLESKIPSTUCK) != 0 && win->isStuck() || // skip if stuck
            // skip if not active client (i.e. only visit each fbwin once)
            (opts & CYCLEGROUPS) != 0 && win->winClient().window() != winclient.window() ||
            (opts & CYCLESKIPSHADED) != 0 && win->isShaded() || // skip if shaded
            win->isFocusHidden()
            ); 
}

void BScreen::renderGeomWindow() {
    _FB_USES_NLS;

    const char *s = _FBTEXT(Screen, 
                            GeometryLength,
                            "W: 0000 x H: 0000",
                            "Representative maximum sized text for width and height dialog");
    int l = strlen(s);

    int geom_h = winFrameTheme().font().height() + winFrameTheme().bevelWidth()*2;
    int geom_w = winFrameTheme().font().textWidth(s, l) + winFrameTheme().bevelWidth()*2;
    m_geom_window.resize(geom_w, geom_h);

    m_geom_window.setBorderWidth(winFrameTheme().border().width());
    m_geom_window.setBorderColor(winFrameTheme().border().color());


    Pixmap tmp = geom_pixmap;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (!winFrameTheme().titleFocusTexture().usePixmap()) {
            geom_pixmap = None;
            m_geom_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            geom_pixmap = imageControl().renderImage(m_geom_window.width(), m_geom_window.height(),
                                                     winFrameTheme().titleFocusTexture());
            m_geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (!winFrameTheme().labelFocusTexture().usePixmap()) {
            geom_pixmap = None;
            m_geom_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            geom_pixmap = imageControl().renderImage(m_geom_window.width(), m_geom_window.height(),
                                                     winFrameTheme().labelFocusTexture());
            m_geom_window.setBackgroundPixmap(geom_pixmap);
        }
    }

    if (tmp)
        imageControl().removeImage(tmp);

}


void BScreen::renderPosWindow() {
    _FB_USES_NLS;

    const char *s = _FBTEXT(Screen, 
                            PositionLength,
                            "0: 0000 x 0: 0000",
                            "Representative maximum sized text for X and Y dialog");
    int l = strlen(s);

    int pos_h = winFrameTheme().font().height() + winFrameTheme().bevelWidth()*2;
    int pos_w = winFrameTheme().font().textWidth(s, l) + winFrameTheme().bevelWidth()*2;
    m_pos_window.resize(pos_w, pos_h);

    m_pos_window.setBorderWidth(winFrameTheme().border().width());
    m_pos_window.setBorderColor(winFrameTheme().border().color());


    Pixmap tmp = pos_pixmap;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (!winFrameTheme().titleFocusTexture().usePixmap()) {
            pos_pixmap = None;
            m_pos_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            pos_pixmap = imageControl().renderImage(m_pos_window.width(), m_pos_window.height(),
                                                    winFrameTheme().titleFocusTexture());
            m_pos_window.setBackgroundPixmap(pos_pixmap);
        }
    } else {
        if (!winFrameTheme().labelFocusTexture().usePixmap()) {
            pos_pixmap = None;
            m_pos_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            pos_pixmap = imageControl().renderImage(m_pos_window.width(), m_pos_window.height(),
                                                     winFrameTheme().labelFocusTexture());
            m_pos_window.setBackgroundPixmap(pos_pixmap);
        }
    }

    if (tmp)
        imageControl().removeImage(tmp);

}




/**
   Called when a set of watched modifiers has been released
*/
void BScreen::notifyReleasedKeys(XKeyEvent &ke) {
    if (cycling_focus) {
        cycling_focus = false;
        cycling_last = 0;
        // put currently focused window to top
        // the iterator may be invalid if the window died
        // in which case we'll do a proper revert focus
        if (cycling_window != focused_list.end()) {
            WinClient *client = *cycling_window;
            focused_list.erase(cycling_window);
            focused_list.push_front(client);
            client->fbwindow()->raise();
        } else {
            Fluxbox::instance()->revertFocus(*this);
        }
    }
}

/**
 * Used to find out which window was last focused on the given workspace
 * If workspace is outside the ID range, then the absolute last focused window
 * is given.
 */
WinClient *BScreen::getLastFocusedWindow(int workspace) {
    if (focused_list.empty()) return 0;
    if (workspace < 0 || workspace >= (int) getCount())
        return focused_list.front();

    FocusedWindows::iterator it = focused_list.begin();    
    FocusedWindows::iterator it_end = focused_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() &&
            (((int)(*it)->fbwindow()->workspaceNumber()) == workspace 
             && !(*it)->fbwindow()->isIconic()
             && (!(*it)->fbwindow()->isStuck() || (*it)->fbwindow()->isFocused())))
            // only give focus to a stuck window if it is currently focused
            // otherwise they tend to override normal workspace focus
            return *it;
    }
    return 0;
}

/**
 * Used to find out which window was last active in the given group
 * If ignore_client is given, it excludes that client.
 * Stuck, iconic etc don't matter within a group
 */
WinClient *BScreen::getLastFocusedWindow(FluxboxWindow &group, WinClient *ignore_client) {
    if (focused_list.empty()) return 0;

    FocusedWindows::iterator it = focused_list.begin();    
    FocusedWindows::iterator it_end = focused_list.end();
    for (; it != it_end; ++it) {
        if (((*it)->fbwindow() == &group) &&
            (*it) != ignore_client)
            return *it;
    }
    return 0;
}

void BScreen::updateSize() {
    // force update geometry
    rootWindow().updateGeometry();

    // reset background
    m_root_theme->reconfigTheme();

    // send resize notify
    m_resize_sig.notify();
}


/**
 * Find the group of windows to this window's left
 * So, we check the leftgroup hint, and see if we know any windows
 */
FluxboxWindow *BScreen::findGroupLeft(WinClient &winclient) {
    Window w = winclient.getGroupLeftWindow();
    if (w == None)
        return 0;

    WinClient *have_client = Fluxbox::instance()->searchWindow(w);

    if (!have_client) {
        // not found, add it to expecting
        m_expecting_groups[w] = &winclient;
    } else if (&have_client->screen() != &winclient.screen())
        // something is not consistent
        return 0;

    if (have_client)
        return have_client->fbwindow();
    else
        return 0;
}

FluxboxWindow *BScreen::findGroupRight(WinClient &winclient) {
    Groupables::iterator it = m_expecting_groups.find(winclient.window());
    if (it == m_expecting_groups.end())
        return 0;

    // yay, this'll do.
    WinClient *other = it->second;
    m_expecting_groups.erase(it); // don't expect it anymore

    // forget about it if it isn't the left-most client in the group, plus
    // it must have the atom set on it (i.e. previously encountered by fluxbox)
    // for us to check our expecting
    if (!winclient.hasGroupLeftWindow() ||
        other->getGroupLeftWindow() != None)
        return 0;

    return other->fbwindow();
}
void BScreen::initXinerama() {
#ifdef XINERAMA
    Display *display = FbTk::App::instance()->display();

    if (!XineramaIsActive(display)) {
#ifdef DEBUG
        cerr<<"BScreen::initXinerama(): dont have Xinerama"<<endl;
#endif // DEBUG    
        m_xinerama_avail = false;
        m_xinerama_headinfo = 0;
        m_xinerama_num_heads = 0;
        return;
    }
#ifdef DEBUG
    cerr<<"BScreen::initXinerama(): have Xinerama"<<endl;
#endif // DEBUG    
    m_xinerama_avail = true;

    XineramaScreenInfo *screen_info;
    int number;
    screen_info = XineramaQueryScreens(display, &number);
    m_xinerama_headinfo = new XineramaHeadInfo[number];
    m_xinerama_num_heads = number;
    for (int i=0; i < number; i++) {
        m_xinerama_headinfo[i].x = screen_info[i].x_org;
        m_xinerama_headinfo[i].y = screen_info[i].y_org;
        m_xinerama_headinfo[i].width = screen_info[i].width;
        m_xinerama_headinfo[i].height = screen_info[i].height;
    }
    XFree(screen_info);
#ifdef DEBUG
    cerr<<"BScreen::initXinerama(): number of heads ="<<number<<endl;
#endif // DEBUG

#else // XINERAMA
    // no xinerama
    m_xinerama_avail = false;
    m_xinerama_num_heads = 0;
#endif // XINERAMA

}

int BScreen::getHead(int x, int y) const {
    if (!hasXinerama()) return 0;
#ifdef XINERAMA

    for (int i=0; i < m_xinerama_num_heads; i++) {
        if (x >= m_xinerama_headinfo[i].x &&
            x < (m_xinerama_headinfo[i].x + m_xinerama_headinfo[i].width) &&
            y >= m_xinerama_headinfo[i].y &&
            y < (m_xinerama_headinfo[i].y + m_xinerama_headinfo[i].height)) {
            return i+1;
        }
    }

#endif // XINERAMA
    return 0;
}

int BScreen::getHead(FbTk::FbWindow &win) const {
    if (hasXinerama())
        return getHead(win.x() + win.width()/2, win.y() + win.height()/2);
    else 
        return 0;
}


int BScreen::getCurrHead() const {
    if (!hasXinerama()) return 0;
    int root_x = 0, root_y = 0;
#ifdef XINERAMA
    int ignore_i;
    unsigned int ignore_ui;

    Window ignore_w;

    XQueryPointer(FbTk::App::instance()->display(),
                  rootWindow().window(), &ignore_w, 
                  &ignore_w, &root_x, &root_y,
                  &ignore_i, &ignore_i, &ignore_ui);
#endif // XINERAMA
    return getHead(root_x, root_y);
}

int BScreen::getHeadX(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return 0;
    return m_xinerama_headinfo[head-1].x;
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadY(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return 0;
    return m_xinerama_headinfo[head-1].y;
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadWidth(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return width();
    return m_xinerama_headinfo[head-1].width;
#else
    return width();
#endif // XINERAMA
}

int BScreen::getHeadHeight(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return height();
    return m_xinerama_headinfo[head-1].height;
#else
    return height();
#endif // XINERAMA
}

pair<int,int> BScreen::clampToHead(int head, int x, int y, int w, int h) const {

    // if there are multiple heads, head=0 is not valid
    // a better way would be to search the closest head
    if (head == 0 && numHeads() != 0)
	head = 1;
    
    int hx = getHeadX(head);
    int hy = getHeadY(head);
    int hw = getHeadWidth(head);
    int hh = getHeadHeight(head);
 
    if (x + w > hx + hw)
        x = hx + hw - w;
    if (y + h > hy + hh)
        y = hy + hh - h;
 
    if (x < hx)
        x = hx;
    if (y < hy)
       y = hy;

    return make_pair(x,y);
}

// TODO: when toolbar gets its resources moved into Toolbar.hh/cc, then
// this can be gone and a consistent interface for the two used
// on the actual objects

template<>
void BScreen::setOnHead<FluxboxWindow>(FluxboxWindow& win, int head) {
    if (head > 0 && head <= numHeads()) {
        int current_head = getHead(win.fbWindow());
        win.move(getHeadX(head) + win.frame().x() - getHeadX(current_head),
                 getHeadY(head) + win.frame().y() - getHeadY(current_head));
    }
}

