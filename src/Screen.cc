// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Screen.cc,v 1.227 2003/08/25 16:07:09 fluxgen Exp $


#include "Screen.hh"

#include "I18n.hh"
#include "fluxbox.hh"
#include "ImageControl.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "StringUtil.hh"
#include "Netizen.hh"
#include "Directory.hh"
#include "SimpleCommand.hh"
#include "FbWinFrameTheme.hh"
#include "MenuTheme.hh"
#include "RootTheme.hh"
#include "WinButtonTheme.hh"
#include "FbCommands.hh"
#include "BoolMenuItem.hh"
#include "IntResMenuItem.hh"
#include "MacroCommand.hh"
#include "XLayerItem.hh"
#include "MultLayers.hh"
#include "FbMenu.hh"
#include "LayerMenu.hh"
#include "WinClient.hh"
#include "Subject.hh"
#include "FbWinFrame.hh"
#include "FbWindow.hh"
#include "Strut.hh"
#include "SlitTheme.hh"
#include "CommandParser.hh"
#include "MenuTheme.hh"
#include "IconMenuItem.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
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

using namespace std;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
    cerr<<I18n::instance()->
        getMessage(FBNLS::ScreenSet, FBNLS::ScreenAnotherWMRunning,
                   "BScreen::BScreen: an error occured while querying the X server.\n"
                   "	another window manager already running on display ")<<DisplayString(display)<<endl;

    running = false;

    return -1;
}

FbTk::Menu *createMenuFromScreen(BScreen &screen, const char *label = 0) {
    FbTk::Menu *menu = new FbMenu(*screen.menuTheme(), 
                                  screen.screenNumber(), 
                                  screen.imageControl(), 
                                  *screen.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()));
    if (label)
        menu->setLabel(label);

    return menu;
}

class FocusModelMenuItem : public FbTk::MenuItem {
public:
    FocusModelMenuItem(const char *label, BScreen &screen, 
                       Fluxbox::FocusModel model, 
                       FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_screen(screen), m_focusmodel(model) {
    }
    bool isEnabled() const { return m_screen.getFocusModel() != m_focusmodel; }
    void click(int button, int time) {
        m_screen.saveFocusModel(m_focusmodel);
        FbTk::MenuItem::click(button, time);
    }

private:
    BScreen &m_screen;
    Fluxbox::FocusModel m_focusmodel;
};


}; // End anonymous namespace



namespace {

class StyleMenuItem: public FbTk::MenuItem {
public:
    StyleMenuItem(const std::string &label, const std::string &filename):FbTk::MenuItem(label.c_str()), 
                                                                         m_filename(FbTk::StringUtil::
                                                                                    expandFilename(filename)) {
        // perform shell style ~ home directory expansion
        // and insert style      
        FbTk::RefCount<FbTk::Command> 
            setstyle_cmd(new FbCommands::
                         SetStyleCmd(m_filename));
        setCommand(setstyle_cmd);
    }
    bool isSelected() const {
        return Fluxbox::instance()->getStyleFilename() == m_filename;
    }
private:
    const std::string m_filename;
};

class AddWorkspaceCmd:public FbTk::Command {
public:
    explicit AddWorkspaceCmd(BScreen &scrn):m_screen(scrn) { }
    void execute() {
        m_screen.addWorkspace();
    }
private:
    BScreen &m_screen;
};

class RemoveLastWorkspaceCmd:public FbTk::Command {
public:
    explicit RemoveLastWorkspaceCmd(BScreen &scrn):m_screen(scrn) { }
    void execute() {
        m_screen.removeLastWorkspace();
    }
private:
    BScreen &m_screen;
};

class ReloadStyleCmd: public FbTk::Command {
public:
    void execute() {
        FbCommands::SetStyleCmd cmd(Fluxbox::instance()->getStyleFilename());
        cmd.execute();
    }
};

void setupWorkspacemenu(BScreen &scr, FbTk::Menu &menu) {
    menu.removeAll(); // clear all items
    using namespace FbTk;
    menu.setLabel("Workspace");
    RefCount<Command> new_workspace(new AddWorkspaceCmd(scr));
    RefCount<Command> remove_last(new RemoveLastWorkspaceCmd(scr));
    //!! TODO: NLS
    menu.insert("New Workspace", new_workspace);
    menu.insert("Remove Last", remove_last);
    // for each workspace add workspace name and it's menu to our workspace menu
    for (size_t workspace = 0; workspace < scr.getCount(); ++workspace) {
        Workspace *wkspc = scr.getWorkspace(workspace);
        menu.insert(wkspc->name().c_str(), &wkspc->menu());
    }

    // update graphics
    menu.update();
}

};


BScreen::ScreenResource::ScreenResource(FbTk::ResourceManager &rm, 
                                        const std::string &scrname, 
                                        const std::string &altscrname):

    image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
    opaque_move(rm, false, "session.opaqueMove", "Session.OpaqueMove"),
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
    rootcommand(rm, "", scrname+".rootCommand", altscrname+".RootCommand"),
    focus_model(rm, Fluxbox::CLICKTOFOCUS, scrname+".focusModel", altscrname+".FocusModel"),
    workspaces(rm, 1, scrname+".workspaces", altscrname+".Workspaces"),
    edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    menu_alpha(rm, 255, scrname+".menuAlpha", altscrname+".MenuAlpha") {

};

BScreen::BScreen(FbTk::ResourceManager &rm,
                 const string &screenname, const string &altscreenname,
                 int scrn, int num_layers) : 
    m_clientlist_sig(*this),  // client signal
    m_workspacecount_sig(*this), // workspace count signal
    m_workspacenames_sig(*this), // workspace names signal 
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
    resource(rm, screenname, altscreenname),
    m_name(screenname),
    m_altname(altscreenname),
    m_resource_manager(rm),
    m_available_workspace_area(new Strut(0, 0, 0, 0)),
    m_xinerama_headinfo(0),
    m_shutdown(false) {

    Display *disp = FbTk::App::instance()->display();

    initXinerama();

    // setup error handler to catch "screen already managed by other wm"
    XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);

    rootWindow().setEventMask(ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
                              SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
                              ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask);

    XSync(disp, False);

    XSetErrorHandler((XErrorHandler) old);

    managed = running;
    if (! managed)
        return;
	
    I18n *i18n = I18n::instance();
	
    fprintf(stderr, i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenManagingScreen,
                                     "BScreen::BScreen: managing screen %d "
                                     "using visual 0x%lx, depth %d\n"),
            screenNumber(), XVisualIDFromVisual(rootWindow().visual()),
            rootWindow().depth());

    cycling_window = focused_list.end();
    
    rootWindow().setCursor(XCreateFontCursor(disp, XC_left_ptr));

    Fluxbox *fluxbox = Fluxbox::instance();
    // setup image cache engine
    m_image_control.reset(new FbTk::ImageControl(scrn, true, fluxbox->colorsPerChannel(),
                                                 fluxbox->getCacheLife(), fluxbox->getCacheMax()));
    imageControl().installRootColormap();
    root_colormap_installed = true;

    // load this screens resources
    fluxbox->load_rc(*this);

    FbTk::ThemeManager::instance().load(Fluxbox::instance()->getStyleFilename());

#ifdef SLIT
    if (slit()) // this will load theme and reconfigure slit
        FbTk::ThemeManager::instance().loadTheme(slit()->theme());
#endif // SLIT


    m_menutheme->setAlpha(*resource.menu_alpha);

    imageControl().setDither(*resource.image_dither);

    // setup windowtheme for antialias
    // before we load the theme

    winFrameTheme().font().setAntialias(*resource.antialias);
    menuTheme()->titleFont().setAntialias(*resource.antialias);
    menuTheme()->frameFont().setAntialias(*resource.antialias);


    // create geometry window 

    int geom_h = 10;
    int geom_w = 100; // just initial, will be fixed in render

    XSetWindowAttributes attrib;
    unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
    attrib.border_pixel = winFrameTheme().border().color().pixel();
    attrib.colormap = rootWindow().colormap();
    attrib.save_under = true;

    winFrameTheme().reconfigSig().attach(this);// for geom window

    m_geom_window = 
        XCreateWindow(disp, rootWindow().window(),
                      0, 0, geom_w, geom_h, winFrameTheme().border().width(), rootWindow().depth(),
                      InputOutput, rootWindow().visual(), mask, &attrib);
    geom_visible = false;
    geom_pixmap = 0;

    renderGeomWindow();

    // setup workspaces and workspace menu

    workspacemenu.reset(createMenuFromScreen(*this));
    workspacemenu->setInternalMenu();
    //!! TODO: NLS
    m_iconmenu.reset(createMenuFromScreen(*this, "Icons"));
    m_iconmenu->setInternalMenu();

    if (*resource.workspaces != 0) {
        for (int i = 0; i < *resource.workspaces; ++i) {
            Workspace *wkspc = new Workspace(*this, m_layermanager, 
                                             getNameOfWorkspace(m_workspaces_list.size()),
                                             m_workspaces_list.size());
            m_workspaces_list.push_back(wkspc);
        }
    } else { // create at least one workspace
        Workspace *wkspc = new Workspace(*this, m_layermanager, 
                                         getNameOfWorkspace(m_workspaces_list.size()),
                                         m_workspaces_list.size());
        m_workspaces_list.push_back(wkspc);
    }

    setupWorkspacemenu(*this, *workspacemenu);
    //!! TODO: NLS
    workspacemenu->insert("Icons", m_iconmenu.get());
    workspacemenu->update();

    m_current_workspace = m_workspaces_list.front();

#ifdef SLIT
    m_slit.reset(new Slit(*this, *layerManager().getLayer(Fluxbox::instance()->getDesktopLayer()),
                 Fluxbox::instance()->getSlitlistFilename().c_str()));

#endif // SLIT


    //!! TODO: we shouldn't do this more than once, but since slit handles their
    // own resources we must do this.
    fluxbox->load_rc(*this);

    // TODO: nls
    m_configmenu.reset(createMenuFromScreen(*this, "Configuration"));
    setupConfigmenu(*m_configmenu.get());
    m_configmenu->setInternalMenu();

    workspacemenu->setItemSelected(2, true);
    // create and initiate rootmenu
    rereadMenu();
    
    m_configmenu->update();

    // start with workspace 0
    changeWorkspaceID(0);
    updateNetizenWorkspaceCount();
	
    int i;
    unsigned int nchild;
    Window r, p, *children;
    XQueryTree(disp, rootWindow().window(), &r, &p, &children, &nchild);

    // preen the window list of all icon windows... for better dockapp support
    for (i = 0; i < (int) nchild; i++) {
		
        if (children[i] == None) continue;

        XWMHints *wmhints = XGetWMHints(FbTk::App::instance()->display(),
                                        children[i]);

        if (wmhints) {
            if ((wmhints->flags & IconWindowHint) &&
                (wmhints->icon_window != children[i]))
                for (int j = 0; j < (int) nchild; j++) {
                    if (children[j] == wmhints->icon_window) {
                        children[j] = None;
                        break;
                    }
                }
            XFree(wmhints);
        }
    }

    // manage shown windows
    for (i = 0; i < (int) nchild; ++i) {
        if (children[i] == None || (! fluxbox->validateWindow(children[i])))
            continue;

        XWindowAttributes attrib;
        if (XGetWindowAttributes(disp, children[i],
                                 &attrib)) {
            if (attrib.override_redirect) 
                continue;

            if (attrib.map_state != IsUnmapped) {
                FluxboxWindow *win = createWindow(children[i]);

                if (win) {
                    XMapRequestEvent mre;
                    mre.window = children[i];
                    win->mapRequestEvent(mre);
                }
            }
        }
    }

    rm.unlock();

    XFree(children);

    XFlush(disp);
}

BScreen::~BScreen() {
    if (! managed)
        return;

    if (geom_pixmap != None)
        imageControl().removeImage(geom_pixmap);

    removeWorkspaceNames();

    Workspaces::iterator w_it = m_workspaces_list.begin();
    Workspaces::iterator w_it_end = m_workspaces_list.end();
    for(; w_it != w_it_end; ++w_it) {
        delete (*w_it);
    }
    m_workspaces_list.clear();
	
    Icons::iterator i_it = m_icon_list.begin();
    Icons::iterator i_it_end = m_icon_list.end();
    for(; i_it != i_it_end; ++i_it) {
        delete (*i_it);
    }
    m_icon_list.clear();
	
    Netizens::iterator n_it = m_netizen_list.begin();
    Netizens::iterator n_it_end = m_netizen_list.end();
    for(; n_it != n_it_end; ++n_it) {
        delete (*n_it);
    }

    m_netizen_list.clear();

    if (hasXinerama() && m_xinerama_headinfo) {
        delete [] m_xinerama_headinfo;
    }
}

unsigned int BScreen::currentWorkspaceID() const { 
    return m_current_workspace->workspaceID(); 
}

Pixmap BScreen::rootPixmap() const {

    Pixmap root_pm = 0;
    Display *disp = FbTk::App::instance()->display();
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned int *data;
    if (rootWindow().property(XInternAtom(disp, "_XROOTPMAP_ID", false),
                              0L, 1L, 
                              false, XA_PIXMAP, &real_type,
                              &real_format, &items_read, &items_left, 
                              (unsigned char **) &data) && 
        items_read) { 
        root_pm = (Pixmap) (*data);                  
        XFree(data);
    }

    return root_pm;

}
    
unsigned int BScreen::maxLeft(int head) const {
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) : 
            getHeadX(head) + m_available_workspace_area->left();
    else
        return doFullMax() ? 0 : m_available_workspace_area->left();
}

unsigned int BScreen::maxRight(int head) const {
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) + getHeadWidth(head) : 
            getHeadX(head) + getHeadWidth(head) - m_available_workspace_area->right();
    else
        return doFullMax() ? width() : width() - m_available_workspace_area->right();
}

unsigned int BScreen::maxTop(int head) const {

    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) : getHeadY(head) + m_available_workspace_area->top();
    else
        return doFullMax() ? 0 : m_available_workspace_area->top();
}

unsigned int BScreen::maxBottom(int head) const {
    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) + getHeadHeight(head) :
            getHeadY(head) + getHeadHeight(head) - m_available_workspace_area->bottom();
    else
        return doFullMax() ? height() : height() - m_available_workspace_area->bottom();
}

void BScreen::update(FbTk::Subject *subj) {
    // for now we're only listening to the theme sig, so no object check
    // if another signal is added later, will need to differentiate here

    renderGeomWindow();
}

void BScreen::reconfigure() {
    m_menutheme->setAlpha(*resource.menu_alpha);
    Fluxbox::instance()->loadRootCommand(*this);

    // setup windowtheme, toolbartheme for antialias
    winFrameTheme().font().setAntialias(*resource.antialias);
    m_menutheme->titleFont().setAntialias(*resource.antialias);
    m_menutheme->frameFont().setAntialias(*resource.antialias);

    // load theme
    //    std::string theme_filename(Fluxbox::instance()->getStyleFilename());
    //    FbTk::ThemeManager::instance().load(theme_filename.c_str());

    renderGeomWindow();

    //reconfigure menus
    workspacemenu->reconfigure();
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


#ifdef SLIT    
    if (slit())
        slit()->reconfigure();
#endif // SLIT

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
    if (! w) return;

    m_icon_list.push_back(w);
    updateIconMenu();
}


void BScreen::removeIcon(FluxboxWindow *w) {
    if (! w)
        return;
	
    Icons::iterator erase_it = remove_if(m_icon_list.begin(),
                                         m_icon_list.end(),
                                         bind2nd(equal_to<FluxboxWindow *>(), w));
    if (erase_it != m_icon_list.end())
        m_icon_list.erase(erase_it);

    updateIconMenu();
}


void BScreen::updateIconMenu() {
    m_iconmenu->removeAll();
    Icons::iterator it = m_icon_list.begin();
    Icons::iterator it_end = m_icon_list.end();
    for (; it != it_end; ++it) {
        FluxboxWindow::ClientList::iterator client_it = (*it)->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end = (*it)->clientList().end();
        for (; client_it != client_it_end; ++client_it)
            m_iconmenu->insert(new IconMenuItem(**client_it));
    }
    m_iconmenu->update();
}

void BScreen::removeWindow(FluxboxWindow *win) {
    if (win->isIconic())
        removeIcon(win);
    else
        getWorkspace(win->workspaceNumber())->removeWindow(win);
}


void BScreen::removeClient(WinClient &client) {

    WinClient *cyc = *cycling_window;
    WinClient *focused = Fluxbox::instance()->getFocusedWindow();
    focused_list.remove(&client);
    if (cyc == &client) {
        cycling_window = focused_list.end();
    } else if (focused == &client) {
        // if we are focused, then give our focus to our transient parent
        // or revert normally
        if (client.transientFor() && client.transientFor()->fbwindow())
            client.transientFor()->fbwindow()->setInputFocus();
        else
            Fluxbox::instance()->revertFocus(focused->screen());
    }

    for_each(getWorkspacesList().begin(), getWorkspacesList().end(),
             mem_fun(&Workspace::updateClientmenu));

    // remove any grouping this is expecting
    Groupables::iterator it = m_expecting_groups.begin();
    Groupables::iterator it_end = m_expecting_groups.end();
    for (; it != it_end; ++it) {
        if (it->second == &client) {
            m_expecting_groups.erase(it);
            // it should only be in there a maximum of once
            break;
        }
    }
    // the client could be on icon menu so we update it
    updateIconMenu();

}

FluxboxWindow *BScreen::getIcon(unsigned int index) {
    if (index < m_icon_list.size())
        return m_icon_list[index];

    return 0;
}

void BScreen::setAntialias(bool value) {
    if (*resource.antialias == value)
        return;
    resource.antialias = value;
    reconfigure();
}

int BScreen::addWorkspace() {
    Workspace *wkspc = new Workspace(*this, m_layermanager, 
                                     "",
                                     m_workspaces_list.size());
    m_workspaces_list.push_back(wkspc);
    addWorkspaceName(wkspc->name().c_str()); // update names
    //add workspace to workspacemenu
    workspacemenu->insert(wkspc->name().c_str(), &wkspc->menu(),
                          wkspc->workspaceID() + 2); //+2 so we add it after "remove last" item
		
    workspacemenu->update();
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

    workspacemenu->remove(wkspc->workspaceID()+2); // + 2 is where workspaces starts
    workspacemenu->update();
	
    //remove last workspace
    m_workspaces_list.pop_back();		
    delete wkspc;


    updateNetizenWorkspaceCount();
    saveWorkspaces(m_workspaces_list.size());

    return m_workspaces_list.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {
    if (! m_current_workspace || id >= m_workspaces_list.size() ||
        id == m_current_workspace->workspaceID())
        return;

    XSync(FbTk::App::instance()->display(), true);
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

    currentWorkspace()->hideAll();

    workspacemenu->setItemSelected(currentWorkspace()->workspaceID() + 2, false);

    // set new workspace
    m_current_workspace = getWorkspace(id);

    workspacemenu->setItemSelected(currentWorkspace()->workspaceID() + 2, true);

    currentWorkspace()->showAll();

    if (focused && (focused->isStuck() || focused->isMoving())) {
        focused->setInputFocus();
    } else
        Fluxbox::instance()->revertFocus(*this);

    if (focused && focused->isMoving()) {
        focused->resumeMoving();
    }

    updateNetizenCurrentWorkspace();
}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
    if (! m_current_workspace || id >= m_workspaces_list.size())
        return;

    if (!win) {
        WinClient *client = Fluxbox::instance()->getFocusedWindow();
        if (client) 
            win = client->fbwindow();
    }

    if (id != currentWorkspace()->workspaceID()) {
        XSync(FbTk::App::instance()->display(), True);

        if (win && &win->screen() == this &&
            (! win->isStuck())) {

            if (win->isIconic()) {
                win->deiconify();
            }

            win->withdraw();
            reassociateWindow(win, id, true);
			
            // change workspace ?
            if (changeWS) {
                changeWorkspaceID(id);
                win->setInputFocus();
            }

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
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowAdd(w, p);
    }

    m_clientlist_sig.notify();
	
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

FluxboxWindow *BScreen::createWindow(Window client) {
    XSync(FbTk::App::instance()->display(), false);

#ifdef SLIT
#ifdef KDE
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
                iskdedockapp = True;
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

        if (iskdedockapp) {
            XSelectInput(disp, client, StructureNotifyMask);

            if (slit())
                slit()->addClient(client);

            return 0; // dont create a FluxboxWindow for this one
        }
#endif // KDE
#endif // SLIT

    WinClient *winclient = new WinClient(client, *this);

    if (winclient->initial_state == WithdrawnState) {
        delete winclient;
#ifdef SLIT
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
            win = new FluxboxWindow(*winclient, *this, 
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
    
    if (new_win) {
        Fluxbox::instance()->attachSignals(*win);
    }

    // we also need to check if another window expects this window to the left
    // and if so, then join it.
    FluxboxWindow *otherwin = 0;
    // TODO: does this do the right stuff focus-wise?
    if ((otherwin = findGroupRight(*winclient)) && otherwin != win) {
        win->attachClient(otherwin->winClient());
    }

    if (!win->isIconic() && (win->workspaceNumber() == currentWorkspaceID() || win->isStuck())) {
        win->show();
    }

    XSync(FbTk::App::instance()->display(), False);
    return win;
}

FluxboxWindow *BScreen::createWindow(WinClient &client) {
    FluxboxWindow *win = new FluxboxWindow(client, *this, 
                                           winFrameTheme(),
                                           *layerManager().getLayer(Fluxbox::instance()->getNormalLayer()));
#ifdef SLIT
    if (win->initialState() == WithdrawnState)
        slit()->addClient(win->clientWindow());
#endif // SLIT
    if (!win->isManaged()) {
        delete win;
        return 0;
    }
    // don't add to focused_list, as it should already be in there (since the
    // WinClient already exists).
    
    Fluxbox::instance()->attachSignals(*win);
    // winclient actions should have been setup when the WinClient was created
    if (win->workspaceNumber() == currentWorkspaceID() || win->isStuck()) {
        win->show();      
    }
    return win;
}

Strut *BScreen::requestStrut(int left, int right, int top, int bottom) {
    Strut *str = new Strut(left, right, top, bottom);
    m_strutlist.push_back(str);
    return str;
}

void BScreen::clearStrut(Strut *str) {
    if (str == 0)
        return;
    // find strut and erase it
    std::list<Strut *>::iterator pos = find(m_strutlist.begin(),
                                            m_strutlist.end(),
                                            str);
    if (pos == m_strutlist.end())
        return;
    m_strutlist.erase(pos);
    delete str;
}

/// helper class for for_each in BScreen::updateAvailableWorkspaceArea()
namespace {
class MaxArea {
public:
    MaxArea(Strut &max_area):m_max_area(max_area) { }
    void operator ()(const Strut *str) {
        static int left, right, bottom, top;
        left = std::max(m_max_area.left(), str->left());
        right = std::max(m_max_area.right(), str->right());
        bottom = std::max(m_max_area.bottom(), str->bottom());
        top = std::max(m_max_area.top(), str->top());
        m_max_area = Strut(left, right, top, bottom);
    }
private:
    Strut &m_max_area;
};

}; // end anonymous namespace

void BScreen::updateAvailableWorkspaceArea() {
    // find max of left, right, top and bottom and set avaible workspace area

    // clear old area
    m_available_workspace_area.reset(new Strut(0, 0, 0, 0));
    
    // calculate max area
    for_each(m_strutlist.begin(),
             m_strutlist.end(),
             MaxArea(*m_available_workspace_area.get()));
}

void BScreen::addWorkspaceName(const char *name) {
    m_workspace_names.push_back(name);
}


string BScreen::getNameOfWorkspace(unsigned int workspace) const {
    if (workspace < m_workspace_names.size()) {
        return m_workspace_names[workspace];
    } else {
        return "";
    }
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
    } else if (ignore_sticky || ! w->isStuck()) {
        getWorkspace(w->workspaceNumber())->removeWindow(w);
        getWorkspace(wkspc_id)->addWindow(*w);
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

            FluxboxWindow *fbwin = (*it)->m_win;
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

            FluxboxWindow *fbwin = (*it)->m_win;
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

void BScreen::dirFocus(FluxboxWindow &win, FocusDir dir) {
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
        if ((*it) == &win) continue; // skip slef
        
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

void BScreen::initMenu() {
    I18n *i18n = I18n::instance();
	
    if (m_rootmenu.get()) {
        // since all menus in root is submenus in m_rootmenu
        // just remove every item in m_rootmenu and then clear m_rootmenu_list
        while (m_rootmenu->numberOfItems())
            m_rootmenu->remove(0); 
        m_rootmenu_list.clear();

    } else
        m_rootmenu.reset(createMenuFromScreen(*this));

    bool defaultMenu = true;
    Fluxbox * const fb = Fluxbox::instance();
    if (fb->getMenuFilename().size() > 0) {
        std::string menufilestr = fb->getMenuFilename();
        menufilestr = FbTk::StringUtil::expandFilename(menufilestr);
        ifstream menu_file(menufilestr.c_str());

        if (!menu_file.fail()) {
            if (! menu_file.eof()) {
                string line;
                int row = 0;
                while (getline(menu_file, line) && ! menu_file.eof()) {
                    row++;
                    if (line[0] != '#') {
                        string key;
                        int pos=0;
                        int err = FbTk::StringUtil::
                            getStringBetween(key, 
                                             line.c_str(), 
                                             '[', ']');
						
                        if (key == "begin") {
                            pos += err;
                            string label;
                            err = FbTk::StringUtil::
                                getStringBetween(label, 
                                                 line.c_str()+pos, 
                                                 '(', ')');
                            if (err>0) {
                                m_rootmenu->setLabel(label.c_str());
                                defaultMenu = parseMenuFile(menu_file, *m_rootmenu.get(), row);
                            } else
                                cerr<<"Error in menufile. Line("<<row<<")"<<endl;
                            break;
                        }
                    }
                }
            } else {
                fprintf(stderr,
                        i18n->getMessage(
                                         FBNLS::ScreenSet, FBNLS::ScreenEmptyMenuFile,
                                         "%s: Empty menu file"),
                        menufilestr.c_str());
            }
            menu_file.close();
        } else
            perror(menufilestr.c_str());
    }

    if (defaultMenu) {
        FbTk::RefCount<FbTk::Command> restart_fb(new FbCommands::RestartFluxboxCmd(""));
        FbTk::RefCount<FbTk::Command> exit_fb(new FbCommands::ExitFluxboxCmd());
        FbTk::RefCount<FbTk::Command> execute_xterm(new FbCommands::ExecuteCmd("xterm", screenNumber()));
        m_rootmenu->setInternalMenu();
        m_rootmenu->insert(i18n->getMessage(FBNLS::ScreenSet, FBNLS::Screenxterm,
                                            "xterm"),
                           execute_xterm);
        m_rootmenu->insert(i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenRestart,
                                            "Restart"),
                           restart_fb);
        m_rootmenu->insert(i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenExit,
                                            "Exit"),
                           exit_fb);
    }
}

/// looks through a menufile and adds correct items to the root-menu.
bool BScreen::parseMenuFile(ifstream &file, FbTk::Menu &menu, int &row) {
	
    string line;
    FbTk::RefCount<FbTk::Command> 
        hide_menu(new FbTk::SimpleCommand<FbTk::Menu>(menu, &FbTk::Menu::hide));

    while (! file.eof()) {
        
        if (!getline(file, line))
            continue;

        row++;
        if (line[0] == '#') //the line is commented
            continue;

            int parse_pos = 0, err = 0;


            std::string str_key, str_label, str_cmd;
				
            err = FbTk::StringUtil::
                getStringBetween(str_key, 
                                 line.c_str(),
                                 '[', ']');
            if (err > 0 ) {
                parse_pos += err;	
                err = FbTk::StringUtil::
                    getStringBetween(str_label, 
                                     line.c_str() + parse_pos,
                                     '(', ')');
                if (err>0) {
                    parse_pos += err;	
                    FbTk::StringUtil::
                        getStringBetween(str_cmd, 
                                         line.c_str() + parse_pos,
                                         '{', '}');
                }
            } else 
                continue; //read next line
				
            if (!str_key.size()) 
                continue;	//read next line

            I18n *i18n = I18n::instance();
            if (str_key == "end") {
                return ((menu.numberOfItems() == 0) ? true : false);
            } else if (str_key == "nop") { 
                menu.insert(str_label.c_str());
            } else if (str_key == "exec") { // exec
                if (!(str_label.size() && str_cmd.size())) {
                    fprintf(stderr,
                            i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenEXECError,
                                             "BScreen::parseMenuFile: [exec] error, "
                                             "no menu label and/or command defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {
                    FbTk::RefCount<FbTk::Command> exec_cmd(new FbCommands::ExecuteCmd(str_cmd, screenNumber()));
                    FbTk::MacroCommand *exec_and_hide = new FbTk::MacroCommand();
                    exec_and_hide->add(hide_menu);
                    exec_and_hide->add(exec_cmd);
                    FbTk::RefCount<FbTk::Command> exec_and_hide_cmd(exec_and_hide);
                    menu.insert(str_label.c_str(), exec_and_hide_cmd);
                }
            } else if (str_key == "exit") { // exit
                if (!str_label.size()) {
                    fprintf(stderr,
                            i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenEXITError,
                                             "BScreen::parseMenuFile: [exit] error, "
                                             "no menu label defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {
                    FbTk::RefCount<FbTk::Command> exit_fb_cmd(new FbCommands::ExitFluxboxCmd());
                    menu.insert(str_label.c_str(), exit_fb_cmd);
                }
            } else if (str_key == "style") {	// style
                if (!( str_label.size() && str_cmd.size())) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenSTYLEError,
                                       "BScreen::parseMenuFile: [style] error, "
                                       "no menu label and/or filename defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else
                    menu.insert(new StyleMenuItem(str_label, str_cmd));						

            } else if (str_key == "config") {
                if (! str_label.size()) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenCONFIGError,
                                       "BScreen::parseMenufile: [config] error, "
                                       "no label defined"));
                    cerr<<"Row: "<<row<<endl;
                } else {
#ifdef DEBUG
                    cerr<<__FILE__<<"("<<__FUNCTION__<<
                        "): inserts configmenu: "<<m_configmenu.get()<<endl;
#endif // DEBUG
                    menu.insert(str_label.c_str(), m_configmenu.get());
                }
            } // end of config                
            else if ( str_key == "include") { // include
                if (!str_label.size()) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenINCLUDEError,
                                       "BScreen::parseMenuFile: [include] error, "
                                       "no filename defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {	// start of else 'x'
                    // perform shell style ~ home directory expansion
                    string newfile(FbTk::StringUtil::expandFilename(str_label));

                    if (newfile.size() != 0) {
                        if (!FbTk::Directory::isRegularFile(newfile)) {
                            fprintf(stderr,
                                    i18n->
                                    getMessage(
                                               FBNLS::ScreenSet, 
                                               FBNLS::ScreenINCLUDEErrorReg,
                                               "BScreen::parseMenuFile: [include] error: "
                                               "'%s' is not a regular file\n"), 
                                    newfile.c_str());
                            cerr<<"Row: "<<row<<endl;
                        } else {
                            // the file is a regular file, lets open and parse it
                            ifstream subfile(newfile.c_str());
                            if (!parseMenuFile(subfile, menu, row))
                                Fluxbox::instance()->saveMenuFilename(newfile.c_str());
                        }
                    } 
                } // end of else 'x'
            } // end of include
            else if (str_key == "submenu") { // sub
                if (!str_label.size()) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenSUBMENUError,
                                       "BScreen::parseMenuFile: [submenu] error, "
                                       "no menu label defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {
                    FbTk::Menu *submenu = createMenuFromScreen(*this);

                    if (str_cmd.size())
                        submenu->setLabel(str_cmd.c_str());
                    else
                        submenu->setLabel(str_label.c_str());

                    parseMenuFile(file, *submenu, row);				
                    submenu->update();
                    menu.insert(str_label.c_str(), submenu);
                    // save to list so we can delete it later
                    m_rootmenu_list.push_back(submenu);
					
                }
            } // end of sub
            else if (str_key == "restart") {
                if (!str_label.size()) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenRESTARTError,
                                       "BScreen::parseMenuFile: [restart] error, "
                                       "no menu label defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {
                    FbTk::RefCount<FbTk::Command> restart_fb(new FbCommands::RestartFluxboxCmd(str_cmd));
                    menu.insert(str_label.c_str(), restart_fb);
                }
            } // end of restart
            else if (str_key == "reconfig") { // reconf
                if (!str_label.c_str()) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenRECONFIGError,
                                       "BScreen::parseMenuFile: [reconfig] error, "
                                       "no menu label defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else {
                    FbTk::RefCount<FbTk::Command> 
                        reconfig_fb_cmd(new FbCommands::ReconfigureFluxboxCmd());
                    menu.insert(str_label.c_str(), reconfig_fb_cmd);
                }
            } else if (str_key == "stylesdir" || str_key == "stylesmenu") {

                bool newmenu = (str_key == "stylesmenu");

                if (!( str_label.size() && str_cmd.size()) && newmenu) {
                    fprintf(stderr,
                            i18n->
                            getMessage(FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRError,
                                       "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                                       " error, no directory defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else { 
                    createStyleMenu(menu, str_label.c_str(), 
                                    newmenu ? str_cmd.c_str() : str_label.c_str());
                }
            } // end of stylesdir
            else if (str_key == "workspaces") {
                if (!str_label.size()) {
                    fprintf(stderr,
                            i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenWORKSPACESError,
                                             "BScreen:parseMenuFile: [workspaces] error, "
                                             "no menu label defined\n"));
                    cerr<<"Row: "<<row<<endl;
                } else
                    menu.insert(str_label.c_str(), workspacemenu.get());
            } // end of workspaces
            else { // ok, if we didn't find any special menu item we try with command parser
                // we need to attach command with arguments so command parser can parse it
                string line = str_key + " " + str_cmd;
                FbTk::RefCount<FbTk::Command> command(CommandParser::instance().parseLine(line));
                if (*command != 0)
                    menu.insert(str_label.c_str(), command);
            }
    } // end of while not eof

    return ((menu.numberOfItems() == 0) ? true : false);
}

void BScreen::addConfigMenu(const char *label, FbTk::Menu &menu) {
    m_configmenu_list.push_back(std::make_pair(label, &menu));
    setupConfigmenu(*m_configmenu.get());
}

void BScreen::removeConfigMenu(FbTk::Menu &menu) {
    Configmenus::iterator it = m_configmenu_list.begin();
    Configmenus::iterator it_end = m_configmenu_list.end();
    for (; it != it_end; ++it) {
        if (it->second == &menu) {
            m_configmenu_list.erase(it);
            break;
        }
    }
    setupConfigmenu(*m_configmenu.get());
}    

void BScreen::setupConfigmenu(FbTk::Menu &menu) {
    I18n *i18n = I18n::instance();
    using namespace FBNLS;

    menu.removeAll();

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), 
                                                                              &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command> reconf_cmd(new FbCommands::ReconfigureFluxboxCmd());
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    // create focus menu
    // we don't set this to internal menu so will 
    // be deleted toghether with the parent
    const char *focusmenu_label = i18n->getMessage(ConfigmenuSet, ConfigmenuFocusModel,
                                                   "Focus Model");
    FbTk::Menu *focus_menu = createMenuFromScreen(*this, focusmenu_label);

    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(ConfigmenuSet, 
                                                               ConfigmenuClickToFocus,
                                                               "Click To Focus"), 
                                              *this,
                                              Fluxbox::CLICKTOFOCUS,
                                              save_and_reconfigure));

    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(ConfigmenuSet, 
                                                               ConfigmenuSloppyFocus,
                                                               "Sloppy Focus"), 
                                              *this,
                                              Fluxbox::SLOPPYFOCUS,
                                              save_and_reconfigure));

    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(ConfigmenuSet, 
                                                               ConfigmenuSemiSloppyFocus,
                                                               "Semi Sloppy Focus"),
                                              *this,
                                              Fluxbox::SEMISLOPPYFOCUS,
                                              save_and_reconfigure));

    focus_menu->insert(new BoolMenuItem(i18n->getMessage(ConfigmenuSet, 
                                                         ConfigmenuAutoRaise,
                                                         "Auto Raise"),
                                        *resource.auto_raise,
                                        save_and_reconfigure));

    focus_menu->update();

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

    menu.insert(new
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuImageDithering,
                                              "Image Dithering"),
                             *resource.image_dither, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuOpaqueMove,
                                              "Opaque Window Moving"),
                             *resource.opaque_move, saverc_cmd));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuFullMax,
                                              "Full Maximization"),
                             *resource.full_max, saverc_cmd));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusNew,
                                              "Focus New Windows"),
                             *resource.focus_new, saverc_cmd));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusLast,
                                              "Focus Last Window on Workspace"),
                             *resource.focus_last, saverc_cmd));

    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuWorkspaceWarping,
                                              "Workspace Warping"),
                             *resource.workspace_warping, saverc_cmd));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuDesktopWheeling,
                                              "Desktop MouseWheel Switching"),
                             *resource.desktop_wheeling, saverc_cmd));

    menu.insert(new BoolMenuItem("Click Raises",
				 *resource.click_raises,
				 saverc_cmd));    
    // setup antialias cmd to reload style and save resource on toggle
    menu.insert(new BoolMenuItem(i18n->getMessage(ConfigmenuSet, ConfigmenuAntiAlias,
                                                  "AntiAlias"), 
                                 *resource.antialias, 
                                 save_and_reconfigure));
    //!! TODO: antialias
    FbTk::MenuItem *menu_alpha_item = new IntResMenuItem("Menu Alpha", resource.menu_alpha,
                                              0, 255);
    menu_alpha_item->setCommand(saverc_cmd);
    menu.insert(menu_alpha_item);



    // finaly update menu 
    menu.update();
}

void BScreen::createStyleMenu(FbTk::Menu &menu, 
                              const char *label, const char *directory) {
    
    // perform shell style ~ home directory expansion
    string stylesdir(FbTk::StringUtil::expandFilename(directory ? directory : ""));

    if (!FbTk::Directory::isDirectory(stylesdir)) {
        //!! TODO: NLS
        cerr<<"Error creating style menu! Stylesdir: "<<stylesdir<<" does not exist or is not a directory!"<<endl;
        return;
    }
        

    FbTk::Directory dir(stylesdir.c_str());

    // create a vector of all the filenames in the directory
    // add sort it
    std::vector<std::string> filelist(dir.entries());
    for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
        filelist[file_index] = dir.readFilename();

    std::sort(filelist.begin(), filelist.end(), less<string>());

    // for each file in directory add filename and path to menu
    for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
        std::string style(stylesdir + '/' + filelist[file_index]);
        // add to menu only if the file is a regular file
        if (FbTk::Directory::isRegularFile(style))
            menu.insert(new StyleMenuItem(filelist[file_index], style));
    } 
    // update menu graphics
    menu.update();
    Fluxbox::instance()->saveMenuFilename(stylesdir.c_str());
   
}

void BScreen::shutdown() {
    Display *disp = FbTk::App::instance()->display();
    rootWindow().setEventMask(NoEventMask);
    XSync(disp, False);
    m_shutdown = true;
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::shutdown));

#ifdef SLIT
    if (m_slit.get())
        m_slit->shutdown();
#endif // SLIT

}


void BScreen::showPosition(int x, int y) {
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
	
    sprintf(label,
            I18n::instance()->getMessage(FBNLS::ScreenSet, FBNLS::ScreenPositionFormat,
                                         "X: %4d x Y: %4d"), x, y);

    m_geom_window.clear();

    winFrameTheme().font().drawText(m_geom_window.window(),
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    m_root_theme->bevelWidth(), 
                                    m_root_theme->bevelWidth() + 
                                    winFrameTheme().font().ascent());
		
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
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

    sprintf(label,
            I18n::instance()->getMessage(FBNLS::ScreenSet, FBNLS::ScreenGeometryFormat,
                                         "W: %4d x H: %4d"), gx, gy);

    m_geom_window.clear();

    //!! TODO: geom window again?! repeated
    winFrameTheme().font().drawText(m_geom_window.window(),
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    m_root_theme->bevelWidth(), 
                                    m_root_theme->bevelWidth() + 
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
            (opts & CYCLESKIPSHADED) != 0 && win->isShaded() // skip if shaded
            ); 
}

void BScreen::renderGeomWindow() {

    const char *s = I18n::instance()->getMessage(FBNLS::ScreenSet, 
                                     FBNLS::ScreenPositionLength,
                                     "W: 0000 x H: 0000");
    int l = strlen(s);

    int geom_h = winFrameTheme().font().height() + m_root_theme->bevelWidth()*2;
    int geom_w = winFrameTheme().font().textWidth(s, l) + m_root_theme->bevelWidth()*2;
    m_geom_window.resize(geom_w, geom_h);

    m_geom_window.setBorderWidth(winFrameTheme().border().width());
    m_geom_window.setBorderColor(winFrameTheme().border().color());


    Pixmap tmp = geom_pixmap;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (winFrameTheme().titleFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            m_geom_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            geom_pixmap = imageControl().renderImage(m_geom_window.width(), m_geom_window.height(),
                                                     winFrameTheme().titleFocusTexture());
            m_geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (winFrameTheme().labelFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
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
    for (; it != it_end; ++it)
        if ((*it)->fbwindow() &&
            (((int)(*it)->fbwindow()->workspaceNumber()) == workspace 
             && !(*it)->fbwindow()->isIconic()
             && (!(*it)->fbwindow()->isStuck() || (*it)->fbwindow()->isFocused())))
            // only give focus to a stuck window if it is currently focused
            // otherwise they tend to override normal workspace focus
            return *it;
    return 0;
}

void BScreen::updateSize() {
    rootWindow().updateGeometry();

    // reconfigure anything that depends on root window size

    // reset background
    m_root_theme->reconfigTheme();

#ifdef SLIT
    if (slit())
        slit()->reconfigure();
#endif // SLIT

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

    return other->m_win;
}
void BScreen::initXinerama() {
#ifdef XINERAMA
    Display *display = FbTk::App::instance()->display();

    if (!XineramaIsActive(display)) {
        m_xinerama_avail = false;
        m_xinerama_headinfo = 0;
        m_xinerama_num_heads = 0;
        return;
    }
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

// TODO: when toolbar gets its resources moved into Toolbar.hh/cc, then
// this can be gone and a consistent interface for the two used
// on the actual objects


template <>
int BScreen::getOnHead<Slit>(Slit &slit) {
    return 0;
}

template <>
void BScreen::setOnHead<Slit>(Slit &slit, int head) {
    //    slit.saveOnHead(head);
    slit.reconfigure();
}

