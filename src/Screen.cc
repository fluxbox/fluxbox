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

// $Id: Screen.cc,v 1.141 2003/04/28 12:58:08 rathnor Exp $


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
#include "FbWinFrameTheme.hh"
#include "MenuTheme.hh"
#include "RootTheme.hh"
//#include "WinButtonTheme.hh"
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
#include <functional>

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
void Resource<Slit::Placement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Slit::TOPLEFT;
    else if (strcasecmp(strval, "CenterLeft")==0)
        m_value = Slit::CENTERLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Slit::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Slit::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Slit::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Slit::TOPRIGHT;
    else if (strcasecmp(strval, "CenterRight")==0)
        m_value = Slit::CENTERRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Slit::BOTTOMRIGHT;
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

template<>
void Resource<Slit::Direction>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "Vertical") == 0) 
        m_value = Slit::VERTICAL;
    else if (strcasecmp(strval, "Horizontal") == 0) 
        m_value = Slit::HORIZONTAL;
    else
        setDefaultValue();
}

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


string Resource<Slit::Placement>::
getString() {
    switch (m_value) {
    case Slit::TOPLEFT:
        return string("TopLeft");
        break;
    case Slit::CENTERLEFT:
        return string("CenterLeft");
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
    case Slit::CENTERRIGHT:
        return string("CenterRight");
        break;
    case Slit::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    }
    //default string
    return string("BottomRight");
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

template<>
string Resource<Slit::Direction>::
getString() {
    switch (m_value) {
    case Slit::VERTICAL:
        return string("Vertical");
        break;
    case Slit::HORIZONTAL:
        return string("Horizontal");
        break;
    }
    // default string
    return string("Vertical");
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

template <>
void FbTk::ThemeItem<int>::load() { }

template <>
void FbTk::ThemeItem<int>::setDefaultValue() {
    *(*this) = 0;
}

template <>
void FbTk::ThemeItem<int>::setFromString(const char *str) {
    if (str == 0)
        return;
    sscanf(str, "%d", &m_value);
}

BScreen::ScreenResource::ScreenResource(ResourceManager &rm, 
                                        const std::string &scrname, 
                                        const std::string &altscrname):
    toolbar_auto_hide(rm, false, scrname+".toolbar.autoHide", altscrname+".Toolbar.AutoHide"),
    image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
    opaque_move(rm, false, "session.opaqueMove", "Session.OpaqueMove"),
    full_max(rm, true, scrname+".fullMaximization", altscrname+".FullMaximization"),
    max_over_slit(rm, true, scrname+".maxOverSlit",altscrname+".MaxOverSlit"),
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
    toolbar_width_percent(rm, 65, 
                          scrname+".toolbar.widthPercent", altscrname+".Toolbar.WidthPercent"),
    edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    menu_alpha(rm, 255, scrname+".menuAlpha", altscrname+".MenuAlpha"),
    slit_layernum(rm, Fluxbox::Layer(Fluxbox::instance()->getDockLayer()), 
                  scrname+".slit.layer", altscrname+".Slit.Layer"),
    toolbar_layernum(rm, Fluxbox::Layer(Fluxbox::instance()->getDesktopLayer()), 
                     scrname+".toolbar.layer", altscrname+".Toolbar.Layer"),
    toolbar_mode(rm, ToolbarHandler::ICONS, scrname+".toolbar.mode", altscrname+".Toolbar.Mode"),
    toolbar_on_head(rm, 0, scrname+".toolbar.onhead", altscrname+".Toolbar.onHead"),
    toolbar_placement(rm, Toolbar::BOTTOMCENTER, 
                      scrname+".toolbar.placement", altscrname+".Toolbar.Placement"),
    slit_auto_hide(rm, false, scrname+".slit.autoHide", altscrname+".Slit.AutoHide"),
    slit_placement(rm, Slit::BOTTOMRIGHT,
                   scrname+".slit.placement", altscrname+".Slit.Placement"),
    slit_direction(rm, Slit::VERTICAL, scrname+".slit.direction", altscrname+".Slit.Direction")

{

};

BScreen::BScreen(ResourceManager &rm,
                 const string &screenname, const string &altscreenname,
                 int scrn, int num_layers) : 
    ScreenInfo(scrn),
    m_clientlist_sig(*this),  // client signal
    m_workspacecount_sig(*this), // workspace count signal
    m_workspacenames_sig(*this), // workspace names signal 
    m_currentworkspace_sig(*this), // current workspace signal
    m_layermanager(num_layers),
    cycling_focus(false),
    cycling_last(0),
    m_windowtheme(new FbWinFrameTheme(scrn)), 
    m_menutheme(new FbTk::MenuTheme(scrn)),
    resource(rm, screenname, altscreenname),
    m_root_theme(new 
                 RootTheme(scrn, 
                           *resource.rootcommand)),
    //    m_winbutton_theme(new WinButtonTheme(scrn)),
    m_toolbarhandler(0) {


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


    cycling_window = focused_list.end();

    XDefineCursor(disp, getRootWindow(), fluxbox->getSessionCursor());

    image_control =
        new FbTk::ImageControl(scrn, true, fluxbox->colorsPerChannel(),
                               fluxbox->getCacheLife(), fluxbox->getCacheMax());
    image_control->installRootColormap();
    root_colormap_installed = true;

    fluxbox->load_rc(*this);
    FbTk::Menu::setAlpha(*resource.menu_alpha);

    image_control->setDither(*resource.image_dither);

    // setup windowtheme, toolbartheme for antialias
    winFrameTheme().font().setAntialias(*resource.antialias);
    menuTheme()->titleFont().setAntialias(*resource.antialias);
    menuTheme()->frameFont().setAntialias(*resource.antialias);

    // set database for new Theme Engine
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename().c_str());

    const char *s = i18n->getMessage(FBNLS::ScreenSet, FBNLS::ScreenPositionLength,
                                     "W: 0000 x H: 0000");
	
    int l = strlen(s);

    geom_h = winFrameTheme().font().height();
    geom_w = winFrameTheme().font().textWidth(s, l);
	
    geom_w += m_root_theme->bevelWidth()*2;
    geom_h += m_root_theme->bevelWidth()*2;

    XSetWindowAttributes attrib;
    unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
    attrib.border_pixel = m_root_theme->borderColor().pixel();
    attrib.colormap = colormap();
    attrib.save_under = true;
    //!! TODO border width
    geom_window = 
        XCreateWindow(disp, getRootWindow(),
                      0, 0, geom_w, geom_h, rootTheme().borderWidth(), getDepth(),
                      InputOutput, getVisual(), mask, &attrib);
    geom_visible = false;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (winFrameTheme().titleFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     winFrameTheme().titleFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (winFrameTheme().labelFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     winFrameTheme().labelFocusTexture());
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

    setupWorkspacemenu(*this, *workspacemenu);

    m_configmenu.reset(createMenuFromScreen(*this));
    setupConfigmenu(*m_configmenu.get());
    m_configmenu->setInternalMenu();

    workspacemenu->setItemSelected(2, true);

    if (getToolbar()) {
        getToolbar()->setPlacement(*resource.toolbar_placement);
        getToolbar()->theme().font().setAntialias(*resource.antialias);
        getToolbar()->reconfigure();
    }

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

    if (! isSloppyFocus() && getToolbar() != 0) {
        XSetInputFocus(disp, getToolbar()->window().window(),
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

}

const FbTk::Menu &BScreen::getToolbarModemenu() const {
    return m_toolbarhandler->getModeMenu();
}

FbTk::Menu &BScreen::getToolbarModemenu() {
    return m_toolbarhandler->getModeMenu();
}

unsigned int BScreen::getCurrentWorkspaceID() const { 
    return current_workspace->workspaceID(); 
}

Pixmap BScreen::rootPixmap() const {

    Pixmap root_pm = 0;
    Display *disp = FbTk::App::instance()->display();
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned int *data;
    if (XGetWindowProperty(disp, getRootWindow(),
                           XInternAtom(disp, "_XROOTPMAP_ID", false),
                           0L, 1L, 
                           false, XA_PIXMAP, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) { 
        root_pm = (Pixmap) (*data);                  
        XFree(data);
    }

    return root_pm;

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
    FbTk::Menu::setAlpha(*resource.menu_alpha);
    Fluxbox::instance()->loadRootCommand(*this);

    // setup windowtheme, toolbartheme for antialias
    winFrameTheme().font().setAntialias(*resource.antialias);
    m_menutheme->titleFont().setAntialias(*resource.antialias);
    m_menutheme->frameFont().setAntialias(*resource.antialias);
    // load theme
    std::string theme_filename(Fluxbox::instance()->getStyleFilename());
    FbTk::ThemeManager::instance().load(theme_filename.c_str());

    I18n *i18n = I18n::instance();

    const char *s = i18n->getMessage(
                                     FBNLS::ScreenSet, 
                                     FBNLS::ScreenPositionLength,
                                     "W: 0000 x H: 0000");
    int l = strlen(s);

    //TODO: repeated from somewhere else?
    geom_h = winFrameTheme().font().height();
    geom_w = winFrameTheme().font().textWidth(s, l);
    geom_w += m_root_theme->bevelWidth()*2;
    geom_h += m_root_theme->bevelWidth()*2;

    Pixmap tmp = geom_pixmap;
    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (winFrameTheme().titleFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     winFrameTheme().titleFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (winFrameTheme().labelFocusTexture().type() ==
            (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            geom_pixmap = None;
            geom_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            geom_pixmap = image_control->renderImage(geom_w, geom_h,
                                                     winFrameTheme().labelFocusTexture());
            geom_window.setBackgroundPixmap(geom_pixmap);
        }
    }
    if (tmp)
        image_control->removeImage(tmp);

    geom_window.setBorderWidth(m_root_theme->borderWidth());
    geom_window.setBorderColor(m_root_theme->borderColor());

    //reconfigure menus
    workspacemenu->reconfigure();
    m_configmenu->reconfigure();
	
    initMenu();
    m_rootmenu->reconfigure();		


    if (getToolbar()) {
        getToolbar()->setPlacement(*resource.toolbar_placement);
        if (getToolbar()->theme().font().isAntialias() != *resource.antialias)
            getToolbar()->theme().font().setAntialias(*resource.antialias);
        getToolbar()->reconfigure();
    }

#ifdef SLIT    
    if (m_slit.get()) {
        m_slit->setPlacement(static_cast<Slit::Placement>(getSlitPlacement()));
        m_slit->setDirection(static_cast<Slit::Direction>(getSlitDirection()));
        m_slit->reconfigure();
    }
#endif // SLIT

    //reconfigure workspaces
    for_each(workspacesList.begin(),
             workspacesList.end(),
             mem_fun(&Workspace::reconfigure));

    //reconfigure Icons
    for_each(iconList.begin(),
             iconList.end(),
             mem_fun(&FluxboxWindow::reconfigure));

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
	

    Icons::iterator erase_it = remove_if(iconList.begin(),
                                         iconList.end(),
                                         bind2nd(equal_to<FluxboxWindow *>(), w));
    if (erase_it != iconList.end())
        iconList.erase(erase_it);

    
    Icons::iterator it = iconList.begin();
    Icons::iterator it_end = iconList.end();
    for (int i = 0; it != it_end; ++it, ++i) {
        (*it)->setWindowNumber(i);
    }
}

void BScreen::removeWindow(FluxboxWindow *win) {
    Workspaces::iterator it = workspacesList.begin();
    Workspaces::iterator it_end = workspacesList.end();
    for (; it != it_end; ++it)
        (*it)->removeWindow(win);
}


void BScreen::removeClient(WinClient &client) {
    WinClient *cyc = *cycling_window;
    focused_list.remove(&client);
    if (cyc == &client) {
        cycling_window = focused_list.end();
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
        XSync(FbTk::App::instance()->display(), true);
        FluxboxWindow *focused = Fluxbox::instance()->getFocusedWindow();
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): focused = "<<focused<<endl;
#endif // DEBUG

        if (focused && focused->isMoving()) {
            if (doOpaqueMove())
                reassociateWindow(focused, id, true);
            // don't reassociate if not opaque moving
            focused->pauseMoving();
        }

        // reassociate all windows that are stuck to the new workspace
        Workspace *wksp = getCurrentWorkspace();
        Workspace::Windows wins = wksp->getWindowList();
        Workspace::Windows::iterator it = wins.begin();
        for (; it != wins.end(); ++it) {
            if ((*it)->isStuck()) {
                reassociateWindow(*it, id, true);
            }
        }

        current_workspace->hideAll();

        workspacemenu->setItemSelected(current_workspace->workspaceID() + 2, false);

        if (focused && &focused->getScreen() == this &&
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
        XSync(FbTk::App::instance()->display(), True);

        if (win && &win->getScreen() == this &&
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
    for_each(netizenList.begin(),
             netizenList.end(),
             mem_fun(&Netizen::sendCurrentWorkspace));
}


void BScreen::updateNetizenWorkspaceCount() {
    for_each(netizenList.begin(),
             netizenList.end(),
             mem_fun(&Netizen::sendWorkspaceCount));

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
    FluxboxWindow *win = new FluxboxWindow(client, *this, 
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
        // always put on end of focused list, if it gets focused it'll get pushed up
        // there is only the one win client at this stage
        focused_list.push_back(&win->winClient());

        //TODO: is next line needed?
        Fluxbox::instance()->saveWindowSearch(client, win);
        setupWindowActions(*win);
        Fluxbox::instance()->attachSignals(*win);
    }
    if (win->getWorkspaceNumber() == getCurrentWorkspaceID() || win->isStuck()) {
        win->show();
    }
    XSync(FbTk::App::instance()->display(), False);
    return win;
}

FluxboxWindow *BScreen::createWindow(WinClient &client) {
    FluxboxWindow *win = new FluxboxWindow(client, *this, 
                                           winFrameTheme(), *menuTheme(),
                                           *layerManager().getLayer(Fluxbox::instance()->getNormalLayer()));
#ifdef SLIT
    if (win->initialState() == WithdrawnState)
        getSlit()->addClient(win->getClientWindow());
#endif // SLIT
    if (!win->isManaged()) {
        delete win;
        return 0;
    }
    // don't add to focused_list, as it should already be in there (since the
    // WinClient already exists).
    
    Fluxbox::instance()->saveWindowSearch(client.window(), win);
    setupWindowActions(*win);
    Fluxbox::instance()->attachSignals(*win);
    if (win->getWorkspaceNumber() == getCurrentWorkspaceID() || win->isStuck()) {
        win->show();      
    }
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
                newbutton = new WinButton(win, //*m_winbutton_theme.get(),
                                          WinButton::MINIMIZE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);
                newbutton->setOnClick(iconify_cmd);

            } else if (win.isMaximizable() && (*dir)[i] == Fluxbox::MAXIMIZE) {
                newbutton = new WinButton(win, //*m_winbutton_theme.get(),
                                          WinButton::MAXIMIZE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);

                newbutton->setOnClick(maximize_cmd, 1);
                newbutton->setOnClick(maximize_horiz_cmd, 3);
                newbutton->setOnClick(maximize_vert_cmd, 2);

            } else if (win.isClosable() && (*dir)[i] == Fluxbox::CLOSE) {
                newbutton = new WinButton(win, //*m_winbutton_theme.get(),
                                          WinButton::CLOSE, 
                                          frame.titlebar(), 
                                          0, 0, 10, 10);

                newbutton->setOnClick(close_cmd);
#ifdef DEBUG
                cerr<<__FILE__<<": Creating close button"<<endl;
#endif // DEBUG
            } else if ((*dir)[i] == Fluxbox::STICK) {
                WinButton *winbtn = new WinButton(win, // *m_winbutton_theme.get(),
                                                  WinButton::STICK,
                                                  frame.titlebar(),
                                                  0, 0, 10, 10);
                win.stateSig().attach(winbtn);
                winbtn->setOnClick(stick_cmd);
                newbutton = winbtn;                
            } else if ((*dir)[i] == Fluxbox::SHADE) {
                WinButton *winbtn = new WinButton(win, // *m_winbutton_theme.get(),
                                                  WinButton::SHADE,
                                                  frame.titlebar(),
                                                  0, 0, 10, 10);               
                winbtn->setOnClick(shade_cmd);
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

    frame.reconfigure();

    // setup titlebar
    frame.setOnClickTitlebar(raise_and_focus_cmd, 1, false, true); // on press with button 1
    frame.setOnClickTitlebar(shade_cmd, 1, true); // doubleclick with button 1
    frame.setOnClickTitlebar(show_menu_cmd, 3); // on release with button 3
    frame.setOnClickTitlebar(lower_cmd, 2); // on release with button 2
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
    menu.insert("Layer...", &win.getLayermenu());
    CommandRef next_client_cmd(new WindowCmd(win, &FluxboxWindow::nextClient));
    CommandRef prev_client_cmd(new WindowCmd(win, &FluxboxWindow::prevClient));
    menu.insert("Next Client", next_client_cmd);
    menu.insert("Prev Client", prev_client_cmd);

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

void BScreen::reassociateWindow(FluxboxWindow *w, unsigned int wkspc_id, 
                                bool ignore_sticky) {
    if (w == 0)
        return;

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
        getWorkspace(wkspc_id)->addWindow(*w);
    } else if (ignore_sticky || ! w->isStuck()) {
        getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
        getWorkspace(wkspc_id)->addWindow(*w);
    }
}


void BScreen::nextFocus(int opts) {
    bool have_focused = false;
    int focused_window_number = -1;
    FluxboxWindow *focused = Fluxbox::instance()->getFocusedWindow();
    const int num_windows = getCurrentWorkspace()->getCount();

    if (focused != 0) {
        if (focused->getScreen().getScreenNumber() == 
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = focused->getWindowNumber();
        }
    }

    if (num_windows >= 1) {
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
            FocusedWindows::iterator it_end = focused_list.end();

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
                     || fbwin->getWorkspaceNumber() == getCurrentWorkspaceID())) {
                    // either on this workspace, or stuck

                    // keep track of the originally selected window in a set
                    WinClient &last_client = fbwin->winClient();

                    if (! (doSkipWindow(fbwin, opts) || !fbwin->setCurrentClient(**it)) ) {
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
            Workspace *wksp = getCurrentWorkspace();
            Workspace::Windows &wins = wksp->getWindowList();
            Workspace::Windows::iterator it = wins.begin();
            
            if (!have_focused) {
                focused = (*it);
            } else {
                for (; (*it) != focused; ++it) //get focused window iterator
                    continue;
            }
            do {
                ++it;
                if (it == wins.end())
                    it = wins.begin();
                // see if the window should be skipped
                if (! (doSkipWindow((*it), opts) || !(*it)->setInputFocus()) )
                    break;
            } while ((*it) != focused);
            if ((*it) != focused && it != wins.end())
                (*it)->raise();
        }

    }

}


void BScreen::prevFocus(int opts) {
    bool have_focused = false;
    int focused_window_number = -1;
    FluxboxWindow *focused;
    int num_windows = getCurrentWorkspace()->getCount();
	
    if ((focused = Fluxbox::instance()->getFocusedWindow())) {
        if (focused->getScreen().getScreenNumber() ==
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = focused->getWindowNumber();
        }
    }

    if (num_windows >= 1) {
        if (!(opts & CYCLELINEAR)) {
            if (!cycling_focus) {
                cycling_focus = True;
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
                     || fbwin->getWorkspaceNumber() == getCurrentWorkspaceID())) {
                    // either on this workspace, or stuck

                    // keep track of the originally selected window in a set
                    WinClient &last_client = fbwin->winClient();


                    if (! (doSkipWindow(fbwin, opts) || !fbwin->setCurrentClient(**it)) ) {
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
            
            Workspace *wksp = getCurrentWorkspace();
            Workspace::Windows &wins = wksp->getWindowList();
            Workspace::Windows::iterator it = wins.begin();
            
            if (!have_focused) {
                focused = (*it);
            } else {
                for (; (*it) != focused; ++it) //get focused window iterator
                    continue;
            }
            
            do {
                if (it == wins.begin())
                    it = wins.end();
                --it;
                // see if the window should be skipped
                if (! (doSkipWindow((*it), opts) || !(*it)->setInputFocus()) )
                    break;
            } while ((*it) != focused);
            
            if ((*it) != focused && it != wins.end())
                (*it)->raise();
        }
    }
}


void BScreen::raiseFocus() {
    bool have_focused = false;
    int focused_window_number = -1;
    Fluxbox * const fb = Fluxbox::instance();
	
    if (fb->getFocusedWindow())
        if (fb->getFocusedWindow()->getScreen().getScreenNumber() ==
            getScreenNumber()) {
            have_focused = true;
            focused_window_number = fb->getFocusedWindow()->getWindowNumber();
        }

    if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
        fb->getFocusedWindow()->raise();
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
    int borderW = m_root_theme->borderWidth(),
        top = win.getYFrame(), 
        bottom = win.getYFrame() + win.getHeight() + 2*borderW,
        left = win.getXFrame(),
        right = win.getXFrame() + win.getWidth() + 2*borderW;

    Workspace::Windows &wins = getCurrentWorkspace()->getWindowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it) == &win) continue; // skip self
        
        // we check things against an edge, and within the bounds (draw a picture)
        int edge=0, upper=0, lower=0, oedge=0, oupper=0, olower=0;

        int otop = (*it)->getYFrame(), 
            obottom = (*it)->getYFrame() + (*it)->getHeight() + 2*borderW,
            oleft = (*it)->getXFrame(),
            oright = (*it)->getXFrame() + (*it)->getWidth() + 2*borderW;
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
        // just remove every item in m_rootmenu and then clear rootmenuList
        while (m_rootmenu->numberOfItems())
            m_rootmenu->remove(0); 
        rootmenuList.clear();

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
                        fb->getMenuFilename());
            }
            menu_file.close();
        } else
            perror(fb->getMenuFilename());
    }

    if (defaultMenu) {
        FbTk::RefCount<FbTk::Command> restart_fb(new FbCommands::RestartFluxboxCmd());
        FbTk::RefCount<FbTk::Command> exit_fb(new FbCommands::ExitFluxboxCmd());
        FbTk::RefCount<FbTk::Command> execute_xterm(new FbCommands::ExecuteCmd("xterm", getScreenNumber()));
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
                                i18n->getMessage(
                                                 FBNLS::ScreenSet, FBNLS::ScreenEXECError,
                                                 "BScreen::parseMenuFile: [exec] error, "
                                                 "no menu label and/or command defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {
                        FbTk::RefCount<FbTk::Command> exec_cmd(new FbCommands::ExecuteCmd(str_cmd, getScreenNumber()));
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
                                         SetStyleCmd(FbTk::StringUtil::
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
                                getMessage(
                                           FBNLS::ScreenSet, FBNLS::ScreenINCLUDEError,
                                           "BScreen::parseMenuFile: [include] error, "
                                           "no filename defined\n"));
                        cerr<<"Row: "<<row<<endl;
                    } else {	// start of else 'x'
                        // perform shell style ~ home directory expansion
                        string newfile(FbTk::StringUtil::expandFilename(str_label));

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
    // we don't set this to internal menu so will 
    // be deleted toghether with the parent
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

    menu.insert(i18n->getMessage(
                                 ConfigmenuSet, ConfigmenuFocusModel,
                                 "Focus Model"), 
                focus_menu);
#ifdef SLIT
    if (getSlit() != 0) {
        getSlit()->menu().setInternalMenu();
        menu.insert("Slit", &getSlit()->menu());
    }
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

    menu.insert(new BoolMenuItem("Click Raises",
				 *resource.click_raises,
				 save_and_reconfigure));    
    // setup antialias cmd to reload style and save resource on toggle
    menu.insert(new BoolMenuItem("antialias", *resource.antialias, 
                                 save_and_reconfigure));

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
    Display *disp = FbTk::App::instance()->display();
    XSelectInput(disp, getRootWindow(), NoEventMask);
    XSync(disp, False);

    for_each(workspacesList.begin(),
             workspacesList.end(),
             mem_fun(&Workspace::shutdown));

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

    winFrameTheme().font().drawText(
                                  geom_window.window(),
                                  getScreenNumber(),
                                  winFrameTheme().labelTextFocusGC(),
                                  label, strlen(label),
                                  m_root_theme->bevelWidth(), 
                                  m_root_theme->bevelWidth() + 
                                  winFrameTheme().font().ascent());
		
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
    winFrameTheme().font().drawText(geom_window.window(),
                                  getScreenNumber(),
                                  winFrameTheme().labelTextFocusGC(),
                                  label, strlen(label),
                                  m_root_theme->bevelWidth(), 
                                  m_root_theme->bevelWidth() + 
                                  winFrameTheme().font().ascent());	
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
            /* (opts & CYCLESKIPLOWERTABS) != 0 && w->isLowerTab() || // skip if lower tab
             */
            (opts & CYCLESKIPSHADED) != 0 && w->isShaded()); // skip if shaded
}

/**
   Called when a set of watched modifiers has been released
*/
void BScreen::notifyReleasedKeys(XKeyEvent &ke) {
    if (cycling_focus) {
        cycling_focus = false;
        cycling_last = 0;
        // put currently focused window to top
        WinClient *client = *cycling_window;
        focused_list.erase(cycling_window);
        focused_list.push_front(client);
        client->fbwindow()->raise();
    }
}

/**
 Access and clear the auto-group window
*/
FluxboxWindow* BScreen::useAutoGroupWindow() {
    Window w = auto_group_window;
    auto_group_window = 0;
    return w ? Fluxbox::instance()->searchWindow(w) : 0;
}

