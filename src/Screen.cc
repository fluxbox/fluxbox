// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Screen.cc,v 1.118 2003/03/22 05:13:08 rathnor Exp $


#include "Screen.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "ImageControl.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "StringUtil.hh"
#include "Netizen.hh"
#include "DirHelper.hh"
#include "WinButton.hh"
#include "SimpleCommand.hh"
#include "MenuTheme.hh"
#include "FbCommands.hh"
#include "BoolMenuItem.hh"
#include "IntResMenuItem.hh"
#include "MacroCommand.hh"
#include "XLayerItem.hh"
#include "MultLayers.hh"
#include "FbMenu.hh"
#include "LayerMenu.hh"

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

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

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

#ifndef  MAXPATHLEN
#define	 MAXPATHLEN 255
#endif // MAXPATHLEN

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <iostream>
#include <memory>
#include <algorithm>

using namespace std;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
    fprintf(stderr,
            I18n::instance()->
            getMessage(
                FBNLS::ScreenSet, FBNLS::ScreenAnotherWMRunning,
                "BScreen::BScreen: an error occured while querying the X server.\n"
                "	another window manager already running on display %s.\n"),
            DisplayString(display));

    running = false;

    return(-1);
}

FbTk::Menu *createMenuFromScreen(BScreen &screen) {
    FbTk::Menu *menu = new FbMenu(*screen.menuTheme(), 
                                       screen.getScreenNumber(), 
                                       *screen.getImageControl(), 
                                     *screen.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()));
    return menu;
}

class FocusModelMenuItem : public FbTk::MenuItem {
public:
    FocusModelMenuItem(const char *label, BScreen &screen, Fluxbox::FocusModel model, FbTk::RefCount<FbTk::Command> &cmd):
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

//---------- resource manipulators ---------
template<>
void Resource<Tab::Alignment>::
setFromString(const char *strval) {	
    m_value = Tab::getTabAlignmentNum(strval);
}

template<>
void Resource<Tab::Placement>::
setFromString(const char *strval) {	
    m_value = Tab::getTabPlacementNum(strval);
}

template<>
void Resource<Toolbar::Placement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Toolbar::TOPLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Toolbar::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Toolbar::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Toolbar::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Toolbar::TOPRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Toolbar::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop") == 0)
        m_value = Toolbar::LEFTTOP;
    else if (strcasecmp(strval, "LeftCenter") == 0)
        m_value = Toolbar::LEFTCENTER;
    else if (strcasecmp(strval, "LeftBottom") == 0)
        m_value = Toolbar::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop") == 0)
        m_value = Toolbar::RIGHTTOP;
    else if (strcasecmp(strval, "RightCenter") == 0)
        m_value = Toolbar::RIGHTCENTER;
    else if (strcasecmp(strval, "RightBottom") == 0)
        m_value = Toolbar::RIGHTBOTTOM;
    else
        setDefaultValue();
}

template<>
void Resource<ToolbarHandler::ToolbarMode>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "Off") == 0) 
        m_value = ToolbarHandler::OFF;
    else if (strcasecmp(strval, "None") == 0) 
        m_value = ToolbarHandler::NONE;
    else if (strcasecmp(strval, "Icons") == 0) 
        m_value = ToolbarHandler::ICONS;
    else if (strcasecmp(strval, "WorkspaceIcons") == 0) 
        m_value = ToolbarHandler::WORKSPACEICONS;
    else if (strcasecmp(strval, "Workspace") == 0) 
        m_value = ToolbarHandler::WORKSPACE;
    else if (strcasecmp(strval, "AllWindows") == 0) 
        m_value = ToolbarHandler::ALLWINDOWS;
    else
        setDefaultValue();
}


//--------- resource accessors --------------
template<>
string Resource<Tab::Alignment>::
getString() {
    return Tab::getTabAlignmentString(m_value);
}

template<>
string Resource<Tab::Placement>::
getString() {
    return Tab::getTabPlacementString(m_value);
}

template<>
string Resource<Toolbar::Placement>::
getString() {
    switch (m_value) {
    case Toolbar::TOPLEFT:
        return string("TopLeft");
        break;
    case Toolbar::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Toolbar::TOPCENTER:
        return string("TopCenter");
        break;			
    case Toolbar::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Toolbar::TOPRIGHT:
        return string("TopRight");
        break;
    case Toolbar::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case Toolbar::LEFTTOP:
        return string("LeftTop");
        break;
    case Toolbar::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Toolbar::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case Toolbar::RIGHTTOP:
        return string("RightTop");
        break;
    case Toolbar::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Toolbar::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("BottomCenter");
}

template<>
string Resource<ToolbarHandler::ToolbarMode>::
getString() {
    switch (m_value) {
    case ToolbarHandler::OFF:
        return string("Off");
        break;
    case ToolbarHandler::NONE:
        return string("None");
        break;
    case ToolbarHandler::LASTMODE:
    case ToolbarHandler::ICONS:
        return string("Icons");
        break;
    case ToolbarHandler::WORKSPACEICONS:
        return string("Workspace");
        break;
    case ToolbarHandler::WORKSPACE:
        return string("Workspace");
        break;
    case ToolbarHandler::ALLWINDOWS:
        return string("AllWindows");
        break;
    }
    // default string
    return string("Icons");
}

namespace {

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

/// for root command
class RootTheme: public FbTk::Theme {
public:
    explicit RootTheme(BScreen &scr):FbTk::Theme(scr.getScreenNumber()),
                                     m_root_command(*this, "rootCommand", "RootCommand"), 
                                     m_scr(scr) { }
    void reconfigTheme() {
        // override resource root command?
        if (m_scr.getRootCommand() == "") { 
            // do root command
            FbCommands::ExecuteCmd cmd(*m_root_command);
            cmd.execute();
        } else {
            FbCommands::ExecuteCmd cmd(m_scr.getRootCommand());
            cmd.execute();
        }
    }
private:
    FbTk::ThemeItem<std::string> m_root_command;
    BScreen &m_scr;
};

template <>
void FbTk::ThemeItem<std::string>::load() { }

template <>
void FbTk::ThemeItem<std::string>::setDefaultValue() { 
    *(*this) = ""; 
}

template <>
void FbTk::ThemeItem<std::string>::setFromString(const char *str) { 
    *(*this) = (str ? str : ""); 
}

BScreen::ScreenResource::ScreenResource(ResourceManager &rm, 
                                        const std::string &scrname, 
                                        const std::string &altscrname):
    toolbar_auto_hide(rm, false, scrname+".toolbar.autoHide", altscrname+".Toolbar.AutoHide"),
    image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
    opaque_move(rm, false, "session.opaqueMove", "Session.OpaqueMove"),
    full_max(rm, true, scrname+".fullMaximization", altscrname+".FullMaximization"),
    max_over_slit(rm, true, scrname+".maxOverSlit",altscrname+".MaxOverSlit"),
    tab_rotate_vertical(rm, true, scrname+".tab.rotatevertical", altscrname+".Tab.RotateVertical"),
    sloppy_window_grouping(rm, true, scrname+".sloppywindowgrouping", altscrname+".SloppyWindowGrouping"),
    workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
    desktop_wheeling(rm, true, scrname+".desktopwheeling", altscrname+".DesktopWheeling"),
    show_window_pos(rm, true, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
    focus_last(rm, true, scrname+".focusLastWindow", altscrname+".FocusLastWindow"),
    focus_new(rm, true, scrname+".focusNewWindows", altscrname+".FocusNewWindows"),
    antialias(rm, false, scrname+".antialias", altscrname+".Antialias"),
    auto_raise(rm, false, scrname+".autoRaise", altscrname+".AutoRaise"),
    rootcommand(rm, "", scrname+".rootCommand", altscrname+".RootCommand"),
    focus_model(rm, Fluxbox::CLICKTOFOCUS, scrname+".focusModel", altscrname+".FocusModel"),
    workspaces(rm, 1, scrname+".workspaces", altscrname+".Workspaces"),
    toolbar_width_percent(rm, 65, scrname+".toolbar.widthPercent", altscrname+".Toolbar.WidthPercent"),
    edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    tab_width(rm, 64, scrname+".tab.width", altscrname+".Tab.Width"),
    tab_height(rm, 16, scrname+".tab.height", altscrname+".Tab.Height"),
    slit_layernum(rm, Fluxbox::instance()->getDockLayer(), scrname+".slit.layer", altscrname+".Slit.Layer"),
    toolbar_layernum(rm, Fluxbox::instance()->getDesktopLayer(), scrname+".toolbar.layer", altscrname+".Toolbar.Layer"),
    tab_placement(rm, Tab::PTOP, scrname+".tab.placement", altscrname+".Tab.Placement"),
    tab_alignment(rm, Tab::ALEFT, scrname+".tab.alignment", altscrname+".Tab.Alignment"),
    toolbar_mode(rm, ToolbarHandler::ICONS, scrname+".toolbar.mode", altscrname+".Toolbar.Mode"),
    toolbar_on_head(rm, 0, scrname+".toolbar.onhead", altscrname+".Toolbar.onHead"),
    toolbar_placement(rm, Toolbar::BOTTOMCENTER, scrname+".toolbar.placement", altscrname+".Toolbar.Placement")
{

};

BScreen::BScreen(ResourceManager &rm,
                 const string &screenname, const string &altscreenname,
                 int scrn, int num_layers) : ScreenInfo(scrn),
                             m_clientlist_sig(*this),  // client signal
                             m_workspacecount_sig(*this), // workspace count signal
                             m_workspacenames_sig(*this), // workspace names signal 
                             m_currentworkspace_sig(*this), // current workspace signal
                             m_layermanager(num_layers),
                             theme(0), m_windowtheme(scrn), 
                             m_menutheme(new FbTk::MenuTheme(scrn)),
                             resource(rm, screenname, altscreenname),
                             m_root_theme(new RootTheme(*this)),
                             m_toolbarhandler(0)
{
    Display *disp = FbTk::App::instance()->display();

    event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
        SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask;

    XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
    XSelectInput(disp, getRootWindow(), event_mask);
    XSync(disp, False);
    XSetErrorHandler((XErrorHandler) old);

    managed = running;
    if (! managed)
        return;
	
    I18n *i18n = I18n::instance();
	
    fprintf(stderr,
            i18n->
            getMessage(
                       FBNLS::ScreenSet, FBNLS::ScreenManagingScreen,
                       "BScreen::BScreen: managing screen %d "
                       "using visual 0x%lx, depth %d\n"),
            getScreenNumber(), XVisualIDFromVisual(getVisual()),
            getDepth());

    Fluxbox * const fluxbox = Fluxbox::instance();
#ifdef HAVE_GETPID
    pid_t bpid = getpid();

    XChangeProperty(disp, getRootWindow(),
                    Fluxbox::instance()->getFluxboxPidAtom(), XA_CARDINAL,
                    sizeof(pid_t) * 8, PropModeReplace,
                    (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID


    XDefineCursor(disp, getRootWindow(), fluxbox->getSessionCursor());

    image_control =
        new FbTk::ImageControl(scrn, true, fluxbox->colorsPerChannel(),
                               fluxbox->getCacheLife(), fluxbox->getCacheMax());
    image_control->installRootColormap();
    root_colormap_installed = true;

    fluxbox->load_rc(this);

    image_control->setDither(*resource.image_dither);
    theme = new Theme(disp, getRootWindow(), colormap(), getScreenNumber(), 
                      fluxbox->getStyleFilename().c_str(), getRootCommand().c_str());

    theme->reconfigure(*resource.antialias);
    // setup windowtheme, toolbartheme for antialias
    m_windowtheme.font().setAntialias(*resource.antialias);
    m_menutheme->titleFont().setAntialias(*resource.antialias);
    m_menutheme->frameFont().setAntialias(*resource.antialias);

    // set database for new Theme Engine
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename().c_str());

    // special case for tab rotated
    if (*resource.tab_rotate_vertical && 
        ( *resource.tab_placement == Tab::PLEFT || *resource.tab_placement == Tab::PRIGHT)) {
        theme->getWindowStyle().tab.font.rotate(90);
    } else  {
        theme->getWindowStyle().tab.font.rotate(0);
    }

    const char *s = i18n->getMessage(
                                     FBNLS::ScreenSet, FBNLS::ScreenPositionLength,
                                     "W: 0000 x H: 0000");
	
    int l = strlen(s);

    geom_h = m_windowtheme.font().height();
    geom_w = m_windowtheme.font().textWidth(s, l);
	
    geom_w += getBevelWidth()*2;
    geom_h += getBevelWidth()*2;

    XSetWindowAttributes attrib;
    unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
    attrib.border_pixel = getBorderColor()->pixel();
    attrib.colormap = colormap();
    attrib.save_under = true;

    geom_window =
        XCreateWindow(disp, getRootWindow(),
                      0, 0, geom_w, geom_h, theme->getBorderWidth(), getDepth(),
                      InputOutput, getVisual(), mask, &attrib);
    geom_visible = false;

    if (m_windowtheme.labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (m_windowtheme.titleFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(m_windowtheme.titleFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     m_windowtheme.titleFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (m_windowtheme.labelFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(m_windowtheme.labelFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     m_windowtheme.labelFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    }

    workspacemenu.reset(createMenuFromScreen(*this));

    if (*resource.workspaces != 0) {
        for (int i = 0; i < *resource.workspaces; ++i) {
            Workspace *wkspc = new Workspace(*this, m_layermanager, workspacesList.size());
            workspacesList.push_back(wkspc);
        }
    } else { // create at least one workspace
        Workspace *wkspc = new Workspace(*this, m_layermanager, workspacesList.size());
        workspacesList.push_back(wkspc);
    }

    current_workspace = workspacesList.front();

#ifdef SLIT
    m_slit.reset(new Slit(*this, *layerManager().getLayer(getSlitLayerNum())));
#endif // SLIT

    m_toolbarhandler = new ToolbarHandler(*this, getToolbarMode());

    if (getToolbar()) 
        getToolbar()->setPlacement(*resource.toolbar_placement);

    setupWorkspacemenu(*this, *workspacemenu);

    m_configmenu.reset(createMenuFromScreen(*this));
    setupConfigmenu(*m_configmenu.get());

    workspacemenu->setItemSelected(2, true);

    if (getToolbar() != 0)
        getToolbar()->reconfigure();

    initMenu(); // create and initiate rootmenu

    //update menus
    m_rootmenu->update();
    m_configmenu->update();

#ifdef SLIT
    if (m_slit.get())
        m_slit->reconfigure();
#endif // SLIT

    // start with workspace 0
    changeWorkspaceID(0);
    updateNetizenWorkspaceCount();
	
    int i;
    unsigned int nchild;
    Window r, p, *children;
    XQueryTree(disp, getRootWindow(), &r, &p, &children, &nchild);

    // preen the window list of all icon windows... for better dockapp support
    for (i = 0; i < (int) nchild; i++) {
		
        if (children[i] == None) continue;

        XWMHints *wmhints = XGetWMHints(getBaseDisplay()->getXDisplay(),
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

    if (! isSloppyFocus() && getToolbar() != 0) {
        XSetInputFocus(disp, getToolbar()->getWindowID(),
                       RevertToParent, CurrentTime);
    }

    // set the toolbarhandler after the windows are setup, so it catches their state properly

    XFree(children);
    XFlush(disp);
}

BScreen::~BScreen() {
    if (! managed)
        return;

    if (geom_pixmap != None)
        image_control->removeImage(geom_pixmap);


    removeWorkspaceNames();

    Workspaces::iterator w_it = workspacesList.begin();
    Workspaces::iterator w_it_end = workspacesList.end();
    for(; w_it != w_it_end; ++w_it) {
        delete (*w_it);
    }
    workspacesList.clear();
	
    Icons::iterator i_it = iconList.begin();
    Icons::iterator i_it_end = iconList.end();
    for(; i_it != i_it_end; ++i_it) {
        delete (*i_it);
    }
    iconList.clear();
	
    Netizens::iterator n_it = netizenList.begin();
    Netizens::iterator n_it_end = netizenList.end();
    for(; n_it != n_it_end; ++n_it) {
        delete (*n_it);
    }
    netizenList.clear();

    delete image_control;

    delete theme;

}

const FbTk::Menu &BScreen::getToolbarModemenu() const {
    return m_toolbarhandler->getModeMenu();
}

FbTk::Menu &BScreen::getToolbarModemenu() {
    return m_toolbarhandler->getModeMenu();
}

/// TODO
unsigned int BScreen::getMaxLeft() const {
    return 0;
}

///!! TODO
unsigned int BScreen::getMaxRight() const {
    return getWidth();
}

///!! TODO
unsigned int BScreen::getMaxTop() const {
    return 0;
}
///!! TODO
unsigned int BScreen::getMaxBottom() const {
    return getHeight();
}

void BScreen::reconfigure() {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): BScreen::reconfigure"<<endl;
#endif // DEBUG
    Fluxbox::instance()->loadRootCommand(this);
    theme->setRootCommand(getRootCommand());
    const string &filename = Fluxbox::instance()->getStyleFilename();
    theme->load(filename.c_str()); // old theme engine
    // setup windowtheme, toolbartheme for antialias
    m_windowtheme.font().setAntialias(*resource.antialias);
    m_menutheme->titleFont().setAntialias(*resource.antialias);
    m_menutheme->frameFont().setAntialias(*resource.antialias);

    FbTk::ThemeManager::instance().load(filename.c_str()); // new theme engine

    if (getToolbar())
        getToolbar()->theme().font().setAntialias(*resource.antialias);

    theme->reconfigure(*resource.antialias);
    
    I18n *i18n = I18n::instance();

    const char *s = i18n->getMessage(
                                     FBNLS::ScreenSet, 
                                     FBNLS::ScreenPositionLength,
                                     "W: 0000 x H: 0000");
    int l = strlen(s);

    //TODO: repeated from somewhere else?
    geom_h = m_windowtheme.font().height();
    geom_w = m_windowtheme.font().textWidth(s, l);
    geom_w += getBevelWidth()*2;
    geom_h += getBevelWidth()*2;

    Pixmap tmp = geom_pixmap;
    if (m_windowtheme.labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (m_windowtheme.titleFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(m_windowtheme.titleFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     m_windowtheme.titleFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (m_windowtheme.labelFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(m_windowtheme.labelFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     m_windowtheme.labelFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    }
    if (tmp)
        image_control->removeImage(tmp);

    geom_window.setBorderWidth(theme->getBorderWidth());
    geom_window.setBorderColor(theme->getBorderColor());

    //reconfigure menus
    workspacemenu->reconfigure();
    m_configmenu->reconfigure();
	
    initMenu();
    m_rootmenu->reconfigure();		


    //    m_toolbar->setPlacement(*resource.toolbar_placement);
    if (getToolbar() != 0) {
        getToolbar()->reconfigure();
        if (getToolbar()->theme().font().isAntialias() != *resource.antialias)
            getToolbar()->theme().font().setAntialias(*resource.antialias);
    }
#ifdef SLIT    
    if (m_slit.get()) {
        m_slit->setPlacement(static_cast<Slit::Placement>(getSlitPlacement()));
        m_slit->setDirection(static_cast<Slit::Direction>(getSlitDirection()));
        m_slit->reconfigure();
    }
#endif // SLIT

    //reconfigure workspaces
    Workspaces::iterator wit = workspacesList.begin();
    Workspaces::iterator wit_end = workspacesList.end();
    for (; wit != wit_end; ++wit) {
        (*wit)->reconfigure();
    }

    //reconfigure Icons
    Icons::iterator iit = iconList.begin();
    Icons::iterator iit_end = iconList.end();
    for (; iit != iit_end; ++iit) {
        if ((*iit)->validateClient())
            (*iit)->reconfigure();
    }

    image_control->timeout();
}


void BScreen::rereadMenu() {
    initMenu();

    m_rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames() {
    workspaceNames.erase(workspaceNames.begin(), workspaceNames.end());
}

void BScreen::updateWorkspaceNamesAtom() {
    m_workspacenames_sig.notify();

}

void BScreen::addIcon(FluxboxWindow *w) {
    if (! w) return;

    w->setWindowNumber(iconList.size());

    iconList.push_back(w);
}


void BScreen::removeIcon(FluxboxWindow *w) {
    if (! w)
        return;
	
    {
	Icons::iterator it = iconList.begin();
	Icons::iterator it_end = iconList.end();
	for (; it != it_end; ++it) {
            if (*it == w) {
                iconList.erase(it);
                break;
            }
        }
    }
    
    Icons::iterator it = iconList.begin();
    Icons::iterator it_end = iconList.end();
    for (int i = 0; it != it_end; ++it, ++i) {
        (*it)->setWindowNumber(i);
    }
}

void BScreen::removeWindow(FluxboxWindow *win) {
    Workspaces::iterator it = workspacesList.begin();
    Workspaces::iterator it_end = workspacesList.end();
    for (; it != it_end; ++it) {
        (*it)->removeWindow(win);
    }
}

FluxboxWindow *BScreen::getIcon(unsigned int index) {
    if (index < iconList.size())
        return iconList[index];

    return 0;
}

void BScreen::setAntialias(bool value) {
    if (*resource.antialias == value)
        return;
    resource.antialias = value;
    reconfigure();
}

int BScreen::addWorkspace() {
    Workspace *wkspc = new Workspace(*this, m_layermanager, workspacesList.size());
    workspacesList.push_back(wkspc);
    addWorkspaceName(wkspc->name().c_str()); // update names
    //add workspace to workspacemenu
    workspacemenu->insert(wkspc->name().c_str(), &wkspc->menu(),
                          wkspc->workspaceID() + 2); //+2 so we add it after "remove last"
		
    workspacemenu->update();
    saveWorkspaces(workspacesList.size());
    if (getToolbar() != 0)
        getToolbar()->reconfigure();
    
    updateNetizenWorkspaceCount();	
	
	
    return workspacesList.size();
	
}

/// removes last workspace
/// @return number of desktops left
int BScreen::removeLastWorkspace() {
    if (workspacesList.size() <= 1)
        return 0;
    Workspace *wkspc = workspacesList.back();

    if (current_workspace->workspaceID() == wkspc->workspaceID())
        changeWorkspaceID(current_workspace->workspaceID() - 1);

    wkspc->removeAll();

    workspacemenu->remove(wkspc->workspaceID()+2); // + 2 is where workspaces starts
    workspacemenu->update();
	
    //remove last workspace
    workspacesList.pop_back();		
    delete wkspc;

    if (getToolbar() != 0)
        getToolbar()->reconfigure();

    updateNetizenWorkspaceCount();
    saveWorkspaces(workspacesList.size());

    return workspacesList.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {
    if (! current_workspace || id >= workspacesList.size())
        return;
	
    if (id != current_workspace->workspaceID()) {
        XSync(BaseDisplay::getXDisplay(), true);
        FluxboxWindow *focused = Fluxbox::instance()->getFocusedWindow();
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): focused = "<<focused<<endl;
#endif // DEBUG

        if (focused && focused->isMoving()) {
            if (doOpaqueMove())
                reassociateGroup(focused, id, true);
            // don't reassociate if not opaque moving
            focused->pauseMoving();
        }

        Workspace *wksp = getCurrentWorkspace();
        Workspace::Windows wins = wksp->getWindowList();
        Workspace::Windows::iterator it = wins.begin();
        for (; it != wins.end(); ++it) {
            if ((*it)->isStuck()) {
                reassociateGroup(*it,id,true);
            }
        }

        current_workspace->hideAll();

        workspacemenu->setItemSelected(current_workspace->workspaceID() + 2, false);

        if (focused && focused->getScreen() == this &&
            (! focused->isStuck()) && (!focused->isMoving())) {
            current_workspace->setLastFocusedWindow(focused);
            Fluxbox::instance()->setFocusedWindow(0); // set focused window to none
        }

        // set new workspace
        current_workspace = getWorkspace(id);

        workspacemenu->setItemSelected(current_workspace->workspaceID() + 2, true);
        if (getToolbar() != 0)
            getToolbar()->redrawWorkspaceLabel(true);

        current_workspace->showAll();

        if (*resource.focus_last && current_workspace->getLastFocusedWindow() &&
            !(focused && focused->isMoving())) {
            current_workspace->getLastFocusedWindow()->setInputFocus();		

        } else if (focused && (focused->isStuck() || focused->isMoving())) {
            focused->setInputFocus();
        }

        if (focused && focused->isMoving()) {
            focused->resumeMoving();
        }
    }

    updateNetizenCurrentWorkspace();
}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
    if (! current_workspace || id >= workspacesList.size())
        return;

    if (!win)
        win = Fluxbox::instance()->getFocusedWindow();

    if (id != current_workspace->workspaceID()) {
        XSync(BaseDisplay::getXDisplay(), True);

        if (win && win->getScreen() == this &&
            (! win->isStuck())) {

            if ( win->getTab() ) {
                Tab *tab = win->getTab();
                tab->disconnect();
                tab->setPosition();
            }

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
#ifdef DEBUG
            cerr<<__FILE__<<": Sending to id = "<<id<<endl;
            cerr<<__FILE__<<": win->workspaceId="<<win->getWorkspaceNumber()<<endl;
#endif //DEBUG

        }

    }
}


void BScreen::addNetizen(Netizen *n) {
    netizenList.push_back(n);

    n->sendWorkspaceCount();
    n->sendCurrentWorkspace();

    Workspaces::iterator it = workspacesList.begin();
    Workspaces::iterator it_end = workspacesList.end();
    for (; it != it_end; ++it) {
        for (int i = 0; i < (*it)->getCount(); ++i) {
            n->sendWindowAdd((*it)->getWindow(i)->getClientWindow(),
                             (*it)->workspaceID());
        }
    }

    Window f = ((Fluxbox::instance()->getFocusedWindow()) ?
		Fluxbox::instance()->getFocusedWindow()->getClientWindow() : None);
    n->sendWindowFocus(f);
}

void BScreen::removeNetizen(Window w) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        if ((*it)->getWindowID() == w) {
            Netizen *n = *it;
            delete n;
            netizenList.erase(it);			
            break;
        }
    }
}


void BScreen::updateNetizenCurrentWorkspace() {
    m_currentworkspace_sig.notify();
	
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendCurrentWorkspace();
    }

}


void BScreen::updateNetizenWorkspaceCount() {

    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendWorkspaceCount();
    }

    m_workspacecount_sig.notify();	
	
}


void BScreen::updateNetizenWindowFocus() {

    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    Window f = ((Fluxbox::instance()->getFocusedWindow()) ?
                Fluxbox::instance()->getFocusedWindow()->getClientWindow() : None);
    for (; it != it_end; ++it) {
        (*it)->sendWindowFocus(f);
    }
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowAdd(w, p);
    }

    m_clientlist_sig.notify();
	
}


void BScreen::updateNetizenWindowDel(Window w) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowDel(w);
    }
	
    m_clientlist_sig.notify();
}


void BScreen::updateNetizenWindowRaise(Window w) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowRaise(w);
    }
}


void BScreen::updateNetizenWindowLower(Window w) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowLower(w);
    }
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
    Netizens::iterator it = netizenList.begin();
    Netizens::iterator it_end = netizenList.end();
    for (; it != it_end; ++it) {
        (*it)->sendConfigNotify(e);
    }
}

FluxboxWindow *BScreen::createWindow(Window client) {
    FluxboxWindow *win = new FluxboxWindow(client, this, getScreenNumber(), *getImageControl(), 
                                           winFrameTheme(), *menuTheme(),
                                           *layerManager().getLayer(Fluxbox::instance()->getNormalLayer()));
 
#ifdef SLIT
    if (win->initialState() == WithdrawnState)
        getSlit()->addClient(win->getClientWindow());
#endif // SLIT

    if (!win->isManaged()) {
        delete win;
        return 0;
    } else {
        Fluxbox::instance()->saveWindowSearch(client, win);
        Fluxbox::instance()->attachSignals(*win);
        setupWindowActions(*win);
    }
    if (win->getWorkspaceNumber() == getCurrentWorkspaceID() || win->isStuck()) {
        win->show();      
    }
    XSync(FbTk::App::instance()->display(), False);
    return win;
}

void BScreen::setupWindowActions(FluxboxWindow &win) {

    FbWinFrame &frame = win.frame();


    typedef FbTk::RefCount<FbTk::Command> CommandRef;

    using namespace FbTk;
    typedef RefCount<Command> CommandRef;
    typedef SimpleCommand<FluxboxWindow> WindowCmd;

    CommandRef iconify_cmd(new WindowCmd(win, &FluxboxWindow::iconify));
    CommandRef maximize_cmd(new WindowCmd(win, &FluxboxWindow::maximize));
    CommandRef maximize_vert_cmd(new WindowCmd(win, &FluxboxWindow::maximizeVertical));
    CommandRef maximize_horiz_cmd(new WindowCmd(win, &FluxboxWindow::maximizeHorizontal));
    CommandRef close_cmd(new WindowCmd(win, &FluxboxWindow::close));
    CommandRef shade_cmd(new WindowCmd(win, &FluxboxWindow::shade));
    CommandRef raise_cmd(new WindowCmd(win, &FluxboxWindow::raise));
    CommandRef lower_cmd(new WindowCmd(win, &FluxboxWindow::lower));
    CommandRef raise_and_focus_cmd(new WindowCmd(win, &FluxboxWindow::raiseAndFocus));
    CommandRef stick_cmd(new WindowCmd(win, &FluxboxWindow::stick));
    CommandRef show_menu_cmd(new WindowCmd(win, &FluxboxWindow::popupMenu));

    // clear old buttons from frame
    frame.removeAllButtons();
    //!! TODO: fix this ugly hack
    // get titlebar configuration
    const vector<Fluxbox::Titlebar> *dir = &Fluxbox::instance()->getTitlebarLeft();
    for (char c=0; c<2; c++) {
        for (size_t i=0; i< dir->size(); ++i) {
            //create new buttons
            FbTk::Button *newbutton = 0;
            if (win.isIconifiable() && (*dir)[i] == Fluxbox::MINIMIZE) {
                newbutton = new WinButton(WinButton::MINIMIZE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);
                newbutton->setOnClick(iconify_cmd);


            } else if (win.isMaximizable() && (*dir)[i] == Fluxbox::MAXIMIZE) {
                newbutton = new WinButton(WinButton::MAXIMIZE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);

                newbutton->setOnClick(maximize_cmd, 1);
                newbutton->setOnClick(maximize_horiz_cmd, 3);
                newbutton->setOnClick(maximize_vert_cmd, 2);

            } else if (win.isClosable() && (*dir)[i] == Fluxbox::CLOSE) {
                newbutton = new WinButton(WinButton::CLOSE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);

                newbutton->setOnClick(close_cmd);
#ifdef DEBUG
                cerr<<__FILE__<<": Creating close button"<<endl;
#endif // DEBUG
            } else if ((*dir)[i] == Fluxbox::STICK) {
                newbutton = new WinButton(WinButton::STICK,
                                          frame.titlebar(),
                                          0, 0, 10, 10);
                newbutton->setOnClick(stick_cmd);

            } else if ((*dir)[i] == Fluxbox::SHADE) {
                newbutton = new WinButton(WinButton::SHADE,
                                          frame.titlebar(),
                                          0, 0, 10, 10);
                newbutton->setOnClick(shade_cmd);

            }
        
            if (newbutton != 0) {
                newbutton->show();
                if (c == 0)
                    frame.addLeftButton(newbutton);
                else
                    frame.addRightButton(newbutton);
            }
        } //end for i
        dir = &Fluxbox::instance()->getTitlebarRight();
    } // end for c

    // setup titlebar
    frame.setOnClickTitlebar(raise_and_focus_cmd, 1, false, true); // on press with button 1
    frame.setOnClickTitlebar(shade_cmd, 1, true); // doubleclick with button 1
    frame.setOnClickTitlebar(show_menu_cmd, 3); // on release with button 3
    frame.setOnClickTitlebar(lower_cmd, 2, false, true);  // on press with button 2
    frame.setDoubleClickTime(Fluxbox::instance()->getDoubleClickInterval());
    // setup menu
    FbTk::Menu &menu = win.getWindowmenu();
    menu.removeAll(); // clear old items
    menu.disableTitle(); // not titlebar

    // set new menu items
    menu.insert("Shade", shade_cmd);
    menu.insert("Stick", stick_cmd);
    menu.insert("Maximize", maximize_cmd);
    menu.insert("Maximize Vertical", maximize_vert_cmd);
    menu.insert("Maximize Horizontal", maximize_horiz_cmd);
    menu.insert("Iconify", iconify_cmd);
    menu.insert("Raise", raise_cmd);
    menu.insert("Lower", lower_cmd);
    menu.insert("Layer...", win.getLayermenu());
    menu.insert("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
    menu.insert("Close", close_cmd);

    menu.reconfigure(); // update graphics

}


void BScreen::saveStrftimeFormat(const char *format) {
    //make sure std::string don't get 0 string
    resource.strftime_format = (format ? format : "");
}


void BScreen::addWorkspaceName(const char *name) {
    workspaceNames.push_back(name);
}


string BScreen::getNameOfWorkspace(unsigned int workspace) const {
    if (workspace < workspaceNames.size()) {
        return workspaceNames[workspace];
    } else {
        return "";
    }
}

void BScreen::reassociateGroup(FluxboxWindow *w, unsigned int wkspc_id, bool ignore_sticky) {
    if (w->hasTab() && (w->getTab()->next() || w->getTab()->prev())) {
        Tab *tab_it = w->getTab()->first();
        for (; tab_it; tab_it = tab_it->next()) {
            reassociateWindow(tab_it->getWindow(), wkspc_id, ignore_sticky);
        }
    } else {
        // no tab, juts move this one
        reassociateWindow(w, wkspc_id, ignore_sticky);
    }
}

void BScreen::reassociateWindow(FluxboxWindow *w, unsigned int wkspc_id, bool ignore_sticky) {
    if (! w) return;

    if (wkspc_id >= getCount()) {
        wkspc_id = current_workspace->workspaceID();
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): wkspc_id >= getCount()"<<endl;
#endif // DEBUG
    }

    if (!w->isIconic() && w->getWorkspaceNumber() == wkspc_id)
        return;


    if (w->isIconic()) {
        removeIcon(w);
        getWorkspace(wkspc_id)->addWindow(w);
    } else if (ignore_sticky || ! w->isStuck()) {
        getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
        getWorkspace(wkspc_id)->addWindow(w);
    }
}


void BScreen::nextFocus(int opts) {
    bool have_focused = false;
    int focused_window_number = -1;
    FluxboxWindow *focused = Fluxbox::instance()->getFocusedWindow();
    const int num_windows = getCurrentWorkspace()->getCount();

    if (focused != 0) {
        if (focused->getScreen()->getScreenNumber() == 
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = focused->getWindowNumber();
        }
    }

    if (num_windows >= 1) {
        Workspace *wksp = getCurrentWorkspace();
        Workspace::Windows &wins = wksp->getWindowList();
        Workspace::Windows::iterator it = wins.begin();

        if (!have_focused) {
            focused = *it;
        } else {
            for (; *it != focused; ++it) //get focused window iterator
                continue;
        }
        do {
            ++it;
            if (it == wins.end())
                it = wins.begin();
            // see if the window should be skipped
            if (! (doSkipWindow(*it, opts) || !(*it)->setInputFocus()) )
                break;
        } while (*it != focused);

        if (*it != focused && it != wins.end())
            (*it)->raise();

    }

}


void BScreen::prevFocus(int opts) {
    bool have_focused = false;
    int focused_window_number = -1;
    FluxboxWindow *focused;
    int num_windows = getCurrentWorkspace()->getCount();
	
    if ((focused = Fluxbox::instance()->getFocusedWindow())) {
        if (focused->getScreen()->getScreenNumber() ==
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = focused->getWindowNumber();
        }
    }

    if (num_windows >= 1) {
        Workspace *wksp = getCurrentWorkspace();
        Workspace::Windows &wins = wksp->getWindowList();
        Workspace::Windows::iterator it = wins.begin();

        if (!have_focused) {
            focused = *it;
        } else {
            for (; *it != focused; ++it) //get focused window iterator
                continue;
        }
		
        do {
            if (it == wins.begin())
                it = wins.end();
            --it;
            // see if the window should be skipped
            if (! (doSkipWindow(*it, opts) || !(*it)->setInputFocus()) )
                break;
        } while (*it != focused);

        if (*it != focused && it != wins.end())
            (*it)->raise();

    }
}


void BScreen::raiseFocus() {
    bool have_focused = false;
    int focused_window_number = -1;
    Fluxbox * const fb = Fluxbox::instance();
	
    if (fb->getFocusedWindow())
        if (fb->getFocusedWindow()->getScreen()->getScreenNumber() ==
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = fb->getFocusedWindow()->getWindowNumber();
        }

    if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
        fb->getFocusedWindow()->raise();
}

void BScreen::initMenu() {
    I18n *i18n = I18n::instance();
	
    if (m_rootmenu.get()) {
        rootmenuList.erase(rootmenuList.begin(), rootmenuList.end());

        while (m_rootmenu->numberOfItems())
            m_rootmenu->remove(0);			
    } else
        m_rootmenu.reset(createMenuFromScreen(*this));

    bool defaultMenu = true;
    Fluxbox * const fb = Fluxbox::instance();
    if (fb->getMenuFilename()) {
        ifstream menu_file(fb->getMenuFilename());

        if (!menu_file.fail()) {
            if (! menu_file.eof()) {
                string line;
                int row = 0;
                while (getline(menu_file, line) && ! menu_file.eof()) {
                    row++;
                    if (line[0] != '#') {
                        string key;
                        int pos=0;
                        int err = StringUtil::getStringBetween(key, line.c_str(), '[', ']');
						
                        if (key == "begin") {
                            pos += err;
                            string label;
                            err = StringUtil::getStringBetween(label, line.c_str()+pos, '(', ')');
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
                        fb->getMenuFilename());
            }
            menu_file.close();
        } else
            perror(fb->getMenuFilename());
    }

    if (defaultMenu) {
        FbTk::RefCount<FbTk::Command> restart_fb(new FbCommands::RestartFluxboxCmd());
        FbTk::RefCount<FbTk::Command> exit_fb(new FbCommands::ExitFluxboxCmd());
        FbTk::RefCount<FbTk::Command> execute_xterm(new FbCommands::ExecuteCmd("xterm"));
        m_rootmenu->setInternalMenu();
        m_rootmenu->insert(i18n->getMessage(
                                            FBNLS::ScreenSet, FBNLS::Screenxterm,
                                            "xterm"),
                           execute_xterm);
        m_rootmenu->insert(i18n->getMessage(
                                            FBNLS::ScreenSet, FBNLS::ScreenRestart,
                                            "Restart"),
                           restart_fb);
        m_rootmenu->insert(i18n->getMessage(
                                            FBNLS::ScreenSet, FBNLS::ScreenExit,
                                            "Exit"),
                           exit_fb);
    } else
        fb->saveMenuFilename(fb->getMenuFilename());
}

/// looks through a menufile and adds correct items to the root-menu.
bool BScreen::parseMenuFile(ifstream &file, FbTk::Menu &menu, int &row) {
	
    string line;
    FbTk::RefCount<FbTk::Command> 
        hide_menu(new FbTk::SimpleCommand<FbTk::Menu>(menu, &FbTk::Menu::hide));

    while (! file.eof()) {

        if (getline(file, line)) {
            row++;
            if (line[0] != '#') { //the line is commented
                int parse_pos = 0, err = 0;


                std::string str_key, str_label, str_cmd;
				
                err = StringUtil::getStringBetween(str_key, line.c_str(), '[', ']');
                if (err > 0 ) {
                    parse_pos += err;	
                    err = StringUtil::getStringBetween(str_label, line.c_str() + parse_pos, '(', ')');
                    if (err>0) {
                        parse_pos += err;	
                        StringUtil::getStringBetween(str_cmd, line.c_str() + parse_pos, '{', '}');
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
                                i18n->getMessage(
                                                 FBNLS::ScreenSet, FBNLS::ScreenEXECError,
                                                 "BScreen::parseMenuFile: [exec] error, "
                                                 "no menu label and/or command defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {
                        FbTk::RefCount<FbTk::Command> exec_cmd(new FbCommands::ExecuteCmd(str_cmd));
                        FbTk::MacroCommand *exec_and_hide = new FbTk::MacroCommand();
                        exec_and_hide->add(hide_menu);
                        exec_and_hide->add(exec_cmd);
                        FbTk::RefCount<FbTk::Command> exec_and_hide_cmd(exec_and_hide);
                        menu.insert(str_label.c_str(), exec_and_hide_cmd);
                    }
                } else if (str_key == "exit") { // exit
                    if (!str_label.size()) {
                        fprintf(stderr,
                                i18n->getMessage(
                                                 FBNLS::ScreenSet, FBNLS::ScreenEXITError,
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
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenSTYLEError,
                                           "BScreen::parseMenuFile: [style] error, "
                                           "no menu label and/or filename defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {
                        // perform shell style ~ home directory expansion
                        // and insert style                        
                        FbTk::RefCount<FbTk::Command> 
                            setstyle_cmd(new FbCommands::
                                         SetStyleCmd(StringUtil::
                                                     expandFilename(str_cmd)));
                        menu.insert(str_label.c_str(), setstyle_cmd);
						
                    }
                } else if (str_key == "config") {
                    if (! str_label.size()) {
                        fprintf(stderr,
                                i18n->
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenCONFIGError,
                                           "BScreen::parseMenufile: [config] error, "
                                           "no label defined"));
                        cerr<<"Row: "<<row<<endl;
                    } else {
                        cerr<<"inserts configmenu: "<<m_configmenu.get()<<endl;
                        menu.insert(str_label.c_str(), m_configmenu.get());
                    }
                } // end of config                
                else if ( str_key == "include") { // include
                    if (!str_label.size()) {
                        fprintf(stderr,
                                i18n->
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenINCLUDEError,
                                           "BScreen::parseMenuFile: [include] error, "
                                           "no filename defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {	// start of else 'x'
                        // perform shell style ~ home directory expansion
                        string newfile(StringUtil::expandFilename(str_label));

                        if (newfile.size() != 0) {
                            FILE *submenufile = fopen(newfile.c_str(), "r");

                            if (submenufile) {
                                struct stat buf;
                                if (fstat(fileno(submenufile), &buf) ||
                                    (! S_ISREG(buf.st_mode))) {
                                    fprintf(stderr,
                                            i18n->
                                            getMessage(
                                                       FBNLS::ScreenSet, 
                                                       FBNLS::ScreenINCLUDEErrorReg,
                                                       "BScreen::parseMenuFile: [include] error: "
                                                       "'%s' is not a regular file\n"), 
                                            newfile.c_str());

                                    cerr<<"Row: "<<row<<endl;
                                }
								
                                if (! feof(submenufile)) {
                                    fclose(submenufile);
                                    ifstream subfile(newfile.c_str());
                                    if (! parseMenuFile(subfile, menu, row))
                                        Fluxbox::instance()->saveMenuFilename(newfile.c_str());
                                }
                            } else
                                perror(newfile.c_str());
                        } 
                    } // end of else 'x'
                } // end of include
                else if (str_key == "submenu") { // sub
                    if (!str_label.size()) {
                        fprintf(stderr,
                                i18n->
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenSUBMENUError,
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
                        rootmenuList.push_back(submenu);
					
                    }
                } // end of sub
                else if (str_key == "restart") {
                    if (!str_label.size()) {
                        fprintf(stderr,
                                i18n->
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenRESTARTError,
                                           "BScreen::parseMenuFile: [restart] error, "
                                           "no menu label defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {
                        /*  if (str_cmd.size())
                            menu.insert(str_label.c_str(), BScreen::RESTARTOTHER, str_cmd.c_str());
                            else
                        */
                        FbTk::RefCount<FbTk::Command> restart_fb(new FbCommands::RestartFluxboxCmd());
                        menu.insert(str_label.c_str(), restart_fb);
                    }
                } // end of restart
                else if (str_key == "reconfig") { // reconf
                    if (!str_label.c_str()) {
                        fprintf(stderr,
                                i18n->
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenRECONFIGError,
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
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRError,
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
                                i18n->getMessage(
                                                 FBNLS::ScreenSet, FBNLS::ScreenWORKSPACESError,
                                                 "BScreen:parseMenuFile: [workspaces] error, "
                                                 "no menu label defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else
                        menu.insert(str_label.c_str(), workspacemenu.get());
                } // end of work
            }
        }
    }

    return ((menu.numberOfItems() == 0) ? true : false);
}

void BScreen::setupConfigmenu(FbTk::Menu &menu) {
    I18n *i18n = I18n::instance();
    using namespace FBNLS;

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), 
                                                                              &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command> reconf_cmd(new FbCommands::ReconfigureFluxboxCmd());
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    // create focus menu
    FbTk::Menu *focus_menu = createMenuFromScreen(*this);

    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(
                                                         ConfigmenuSet, 
                                                         ConfigmenuClickToFocus,
                                                         "Click To Focus"), 
                                              *this,
                                              Fluxbox::CLICKTOFOCUS,
                                              save_and_reconfigure));
    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(
        ConfigmenuSet, 
                                                         ConfigmenuSloppyFocus,
                                                         "Sloppy Focus"), 
                                              *this,
                                              Fluxbox::SLOPPYFOCUS,
                                              save_and_reconfigure));
    focus_menu->insert(new FocusModelMenuItem(i18n->getMessage(
                                                         ConfigmenuSet, 
                                                         ConfigmenuSemiSloppyFocus,
                                                         "Semi Sloppy Focus"),
                                              *this,
                                              Fluxbox::SEMISLOPPYFOCUS,
                                              save_and_reconfigure));
    focus_menu->insert(new BoolMenuItem(i18n->getMessage(
                                                         ConfigmenuSet, 
                                                         ConfigmenuAutoRaise,
                                                         "Auto Raise"),
                                        *resource.auto_raise,
                                        save_and_reconfigure));

    focus_menu->update();
    rootmenuList.push_back(focus_menu);

    menu.insert(i18n->getMessage(
                                 ConfigmenuSet, ConfigmenuFocusModel,
                                 "Focus Model"), 
                focus_menu);
#ifdef SLIT
    if (getSlit() != 0)
        menu.insert("Slit", &getSlit()->menu());
#endif // SLIT
    menu.insert(i18n->getMessage(
        ToolbarSet, ToolbarToolbarTitle,
        "Toolbar"), &m_toolbarhandler->getToolbarMenu());
    menu.insert(new
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuImageDithering,
                                              "Image Dithering"),
                             *resource.image_dither, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(
                             i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuOpaqueMove,
                                              "Opaque Window Moving"),
                             *resource.opaque_move, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuFullMax,
                                              "Full Maximization"),
                             *resource.full_max, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuFocusNew,
                                              "Focus New Windows"),
                             *resource.focus_new, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuFocusLast,
                                              "Focus Last Window on Workspace"),
                             *resource.focus_last, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuMaxOverSlit,
                                              "Maximize Over Slit"),
                             *resource.max_over_slit, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuWorkspaceWarping,
                                              "Workspace Warping"),
                             *resource.workspace_warping, save_and_reconfigure));
    menu.insert(new 
                BoolMenuItem(i18n->getMessage(
                                              ConfigmenuSet, ConfigmenuDesktopWheeling,
                                              "Desktop MouseWheel Switching"),
                             *resource.desktop_wheeling, save_and_reconfigure));

    // setup antialias cmd to reload style and save resource on toggle
    menu.insert(new BoolMenuItem("antialias", *resource.antialias, 
                                 save_and_reconfigure));


    // finaly update menu 
    menu.update();
}

void BScreen::createStyleMenu(FbTk::Menu &menu, 
                              const char *label, const char *directory) {
    
    // perform shell style ~ home directory expansion
    string stylesdir(StringUtil::expandFilename(directory ? directory : ""));

    I18n *i18n = I18n::instance();						
    struct stat statbuf;

    if (! stat(stylesdir.c_str(), &statbuf)) {
        if (S_ISDIR(statbuf.st_mode)) { // is a directory?

            DirHelper d(stylesdir.c_str());

            // create a vector of all the filenames in the directory
            // add sort it
            std::vector<std::string> filelist(d.entries());
            for (size_t file_index = 0; file_index < d.entries(); ++file_index)
                filelist[file_index] = d.readFilename();
            std::sort(filelist.begin(), filelist.end(), less<string>());

            int slen = stylesdir.size();
            // for each file in directory add filename and path to menu
            for (size_t file_index = 0; file_index < d.entries(); file_index++) {
                int nlen = filelist[file_index].size();
                char style[MAXPATHLEN + 1];

                strncpy(style, stylesdir.c_str(), slen);
                *(style + slen) = '/';
                strncpy(style + slen + 1, filelist[file_index].c_str(), nlen + 1);

                if ( !stat(style, &statbuf) && S_ISREG(statbuf.st_mode)) {
                    FbTk::RefCount<FbTk::Command> setstyle_cmd(new FbCommands::
                                                               SetStyleCmd(style));
                    menu.insert(filelist[file_index].c_str(), setstyle_cmd);
                }
            } 
            // update menu graphics
            menu.update();
            Fluxbox::instance()->saveMenuFilename(stylesdir.c_str());
        } else { // no directory
            fprintf(stderr,
                    i18n->
                    getMessage(
                        FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRErrorNotDir,
                        "BScreen::parseMenuFile:"
                        " [stylesdir/stylesmenu] error, %s is not a"
                        " directory\n"), stylesdir.c_str());
        } // end of directory check
    } else { // stat failed
        fprintf(stderr,
		i18n->
		getMessage(
                    FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRErrorNoExist,		
                    "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                    " error, %s does not exist\n"), stylesdir.c_str());
    } // end of stat
   
}

void BScreen::shutdown() {
    XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), NoEventMask);
    XSync(getBaseDisplay()->getXDisplay(), False);


    Workspaces::iterator it = workspacesList.begin();
    Workspaces::iterator it_end = workspacesList.end();
    for (; it != it_end; ++it) {
        (*it)->shutdown();
    }



    while (!iconList.empty()) {
        iconList.back()->restore(true); // restore with remap
        delete iconList.back(); // the window removes it self from iconlist
    }


#ifdef SLIT
    if (m_slit.get())
        m_slit->shutdown();
#endif // SLIT

}


void BScreen::showPosition(int x, int y) {
    if (! geom_visible) {
#ifdef XINERAMA
        unsigned int head = hasXinerama() ? getCurrHead() : 0;

        geom_window.moveResize(getHeadX(head) + (getHeadWidth(head) - geom_w) / 2,
                               getHeadY(head) + (getHeadHeight(head) - geom_h) / 2, 
                               geom_w, geom_h);
#else // !XINERMA
        geom_window.moveResize((getWidth() - geom_w) / 2,
                               (getHeight() - geom_h) / 2, geom_w, geom_h);
#endif // XINERAMA

        geom_window.show();
        geom_window.raise();

        geom_visible = true;
    }
    char label[256];
	
    sprintf(label,
             I18n::instance()->getMessage(
                                          FBNLS::ScreenSet, FBNLS::ScreenPositionFormat,
                                          "X: %4d x Y: %4d"), x, y);

    geom_window.clear();

    m_windowtheme.font().drawText(
                                  geom_window.window(),
                                  getScreenNumber(),
                                  m_windowtheme.labelTextFocusGC(),
                                  label, strlen(label),
                                  theme->getBevelWidth(), theme->getBevelWidth() + 
                                  m_windowtheme.font().ascent());
		
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
    if (! geom_visible) {
#ifdef XINERAMA
        unsigned int head = hasXinerama() ? getCurrHead() : 0;

        geom_window.moveResize(getHeadX(head) + (getHeadWidth(head) - geom_w) / 2,
                               getHeadY(head) + (getHeadHeight(head) - geom_h) / 2, 
                               geom_w, geom_h);
#else // !XINERMA
        geom_window.moveResize((getWidth() - geom_w) / 2,
                               (getHeight() - geom_h) / 2, geom_w, geom_h);
#endif // XINERAMA
        geom_window.show();
        geom_window.raise();

        geom_visible = true;
    }
	
    char label[256];

    sprintf(label,
            I18n::instance()->getMessage(
                                         FBNLS::ScreenSet, FBNLS::ScreenGeometryFormat,
                                         "W: %4d x H: %4d"), gx, gy);

    geom_window.clear();

    //TODO: geom window again?! repeated
    m_windowtheme.font().drawText(
                                  geom_window.window(),
                                  getScreenNumber(),
                                  m_windowtheme.labelTextFocusGC(),
                                  label, strlen(label),
                                  theme->getBevelWidth(), theme->getBevelWidth() + 
                                  m_windowtheme.font().ascent());	
}


void BScreen::hideGeometry() {
    if (geom_visible) {
        geom_window.hide();
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
    changeWorkspaceID( (getCurrentWorkspaceID()+delta) % getCount());
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::prevWorkspace(const int delta) {
    changeWorkspaceID( (getCurrentWorkspaceID()-delta+getCount()) % getCount());
}

/**
 Goes to the workspace "right" of the current
*/
void BScreen::rightWorkspace(const int delta) {
    if (getCurrentWorkspaceID()+delta < getCount())
        changeWorkspaceID(getCurrentWorkspaceID()+delta);
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::leftWorkspace(const int delta) {
    if (getCurrentWorkspaceID() >= static_cast<unsigned int>(delta))
        changeWorkspaceID(getCurrentWorkspaceID()-delta);
}

/**
  @return true if the windows should be skiped else false
*/
bool BScreen::doSkipWindow(const FluxboxWindow *w, int opts) {
    return ((opts & CYCLESKIPSTUCK) != 0 && w->isStuck() || // skip if stuck
            (opts & CYCLESKIPLOWERTABS) != 0 && w->isLowerTab() || // skip if lower tab
            (opts & CYCLESKIPSHADED) != 0 && w->isShaded()); // skip if shaded
}

/**
 Access and clear the auto-group window
*/
FluxboxWindow* BScreen::useAutoGroupWindow() {
    Window w = auto_group_window;
    auto_group_window = 0;
    return w ? Fluxbox::instance()->searchWindow(w) : 0;
}

