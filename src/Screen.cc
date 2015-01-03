// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Screen.hh"

#include "fluxbox.hh"
#include "Keys.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "Workspace.hh"

#include "Layer.hh"
#include "FocusControl.hh"
#include "ScreenPlacement.hh"

// menu items
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/IntMenuItem.hh"
#include "FbTk/MenuSeparator.hh"
#include "FocusModelMenuItem.hh"
#include "FbTk/RadioMenuItem.hh"

// menus
#include "FbMenu.hh"
#include "LayerMenu.hh"

#include "MenuCreator.hh"

#include "WinClient.hh"
#include "FbWinFrame.hh"
#include "Strut.hh"
#include "FbTk/CommandParser.hh"
#include "AtomHandler.hh"
#include "HeadArea.hh"
#include "RectangleUtil.hh"
#include "FbCommands.hh"
#ifdef USE_SYSTRAY
#include "SystemTray.hh"
#endif
#include "Debug.hh"

#include "FbTk/I18n.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/MultLayers.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/Compose.hh"
#include "FbTk/FbString.hh"
#include "FbTk/STLUtil.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/Util.hh"

#ifdef USE_SLIT
#include "Slit.hh"
#include "SlitClient.hh"
#else
// fill it in
class Slit {};
#endif // USE_SLIT

#ifdef USE_TOOLBAR
#include "Toolbar.hh"
#else
class Toolbar {};
#endif // USE_TOOLBAR

#ifdef STDC_HEADERS
#include <sys/types.h>
#endif // STDC_HEADERS

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

#if defined(HAVE_RANDR) || defined(HAVE_RANDR1_2)
#include <X11/extensions/Xrandr.h>
#endif // HAVE_RANDR

#include <iostream>
#include <algorithm>
#include <functional>
#include <stack>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::cerr;
using std::endl;
using std::string;
using std::make_pair;
using std::pair;
using std::list;
using std::vector;
using std::mem_fun;
using std::bind2nd;
using std::equal_to;

using std::hex;
using std::dec;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
    _FB_USES_NLS;
    cerr<<_FB_CONSOLETEXT(Screen, AnotherWMRunning,
                  "BScreen::BScreen: an error occured while querying the X server.\n"
                  "	another window manager already running on display ",
                  "Message when another WM is found already active on all screens")
        <<DisplayString(display)<<endl;

    running = false;

    return -1;
}


int calcSquareDistance(int x1, int y1, int x2, int y2) {
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
}

class TabPlacementMenuItem: public FbTk::RadioMenuItem {
public:
    TabPlacementMenuItem(const FbTk::FbString & label, BScreen &screen,
                         FbWinFrame::TabPlacement place,
                         FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd),
        m_screen(screen),
        m_place(place) {
        setCloseOnClick(false);
    }

    bool isSelected() const { return m_screen.getTabPlacement() == m_place; }
    void click(int button, int time, unsigned int mods) {
        m_screen.saveTabPlacement(m_place);
        FbTk::RadioMenuItem::click(button, time, mods);
    }


private:
    BScreen &m_screen;
    FbWinFrame::TabPlacement m_place;
};

void clampMenuDelay(int& delay) {
    delay = FbTk::Util::clamp(delay, 0, 5000);
}


struct TabPlacementString {
    FbWinFrame::TabPlacement placement;
    const char* str;
};

const TabPlacementString placement_strings[] = {
    { FbWinFrame::TOPLEFT, "TopLeft" },
    { FbWinFrame::TOP, "Top" },
    { FbWinFrame::TOPRIGHT, "TopRight" },
    { FbWinFrame::BOTTOMLEFT, "BottomLeft" },
    { FbWinFrame::BOTTOM, "Bottom" },
    { FbWinFrame::BOTTOMRIGHT, "BottomRight" },
    { FbWinFrame::LEFTBOTTOM, "LeftBottom" },
    { FbWinFrame::LEFT, "Left" },
    { FbWinFrame::LEFTTOP, "LeftTop" },
    { FbWinFrame::RIGHTBOTTOM, "RightBottom" },
    { FbWinFrame::RIGHT, "Right" },
    { FbWinFrame::RIGHTTOP, "RightTop" }
};

Atom atom_fbcmd = 0;
Atom atom_wm_check = 0;
Atom atom_net_desktop = 0;
Atom atom_utf8_string = 0;
Atom atom_kde_systray = 0;
Atom atom_kwm1 = 0;

void initAtoms(Display* dpy) {
    atom_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    atom_net_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    atom_fbcmd = XInternAtom(dpy, "_FLUXBOX_ACTION", False);
    atom_utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
    atom_kde_systray = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
    atom_kwm1 = XInternAtom(dpy, "KWM_DOCKWINDOW", False);
}


} // end anonymous namespace



namespace FbTk {

template<>
string FbTk::Resource<FbWinFrame::TabPlacement>::
getString() const {

    size_t i = (m_value == FbTk::Util::clamp(m_value, FbWinFrame::TOPLEFT, FbWinFrame::RIGHTTOP)
                ? m_value 
                : FbWinFrame::DEFAULT) - 1;
    return placement_strings[i].str;
}

template<>
void FbTk::Resource<FbWinFrame::TabPlacement>::
setFromString(const char *strval) {

    size_t i;
    for (i = 0; i < sizeof(placement_strings)/sizeof(TabPlacementString); ++i) {
        if (strcasecmp(strval, placement_strings[i].str) == 0) {
            m_value = placement_strings[i].placement;
            return;
        }
    }
    setDefaultValue();
}

} // end namespace FbTk


BScreen::ScreenResource::ScreenResource(FbTk::ResourceManager &rm,
                                        const string &scrname,
                                        const string &altscrname):
    opaque_move(rm, true, scrname + ".opaqueMove", altscrname+".OpaqueMove"),
    full_max(rm, false, scrname+".fullMaximization", altscrname+".FullMaximization"),
    max_ignore_inc(rm, true, scrname+".maxIgnoreIncrement", altscrname+".MaxIgnoreIncrement"),
    max_disable_move(rm, false, scrname+".maxDisableMove", altscrname+".MaxDisableMove"),
    max_disable_resize(rm, false, scrname+".maxDisableResize", altscrname+".MaxDisableResize"),
    workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
    show_window_pos(rm, false, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
    auto_raise(rm, true, scrname+".autoRaise", altscrname+".AutoRaise"),
    click_raises(rm, true, scrname+".clickRaises", altscrname+".ClickRaises"),
    default_deco(rm, "NORMAL", scrname+".defaultDeco", altscrname+".DefaultDeco"),
    tab_placement(rm, FbWinFrame::TOPLEFT, scrname+".tab.placement", altscrname+".Tab.Placement"),
    windowmenufile(rm, Fluxbox::instance()->getDefaultDataFilename("windowmenu"), scrname+".windowMenu", altscrname+".WindowMenu"),
    typing_delay(rm, 0, scrname+".noFocusWhileTypingDelay", altscrname+".NoFocusWhileTypingDelay"),
    workspaces(rm, 4, scrname+".workspaces", altscrname+".Workspaces"),
    edge_snap_threshold(rm, 10, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    focused_alpha(rm, 255, scrname+".window.focus.alpha", altscrname+".Window.Focus.Alpha"),
    unfocused_alpha(rm, 255, scrname+".window.unfocus.alpha", altscrname+".Window.Unfocus.Alpha"),
    menu_alpha(rm, 255, scrname+".menu.alpha", altscrname+".Menu.Alpha"),
    menu_delay(rm, 200, scrname + ".menuDelay", altscrname+".MenuDelay"),
    tab_width(rm, 64, scrname + ".tab.width", altscrname+".Tab.Width"),
    tooltip_delay(rm, 500, scrname + ".tooltipDelay", altscrname+".TooltipDelay"),
    allow_remote_actions(rm, false, scrname+".allowRemoteActions", altscrname+".AllowRemoteActions"),
    clientmenu_use_pixmap(rm, true, scrname+".clientMenu.usePixmap", altscrname+".ClientMenu.UsePixmap"),
    tabs_use_pixmap(rm, true, scrname+".tabs.usePixmap", altscrname+".Tabs.UsePixmap"),
    max_over_tabs(rm, false, scrname+".tabs.maxOver", altscrname+".Tabs.MaxOver"),
    default_internal_tabs(rm, true /* TODO: autoconf option? */ , scrname+".tabs.intitlebar", altscrname+".Tabs.InTitlebar") {


}

BScreen::BScreen(FbTk::ResourceManager &rm,
                 const string &screenname,
                 const string &altscreenname,
                 int scrn, int num_layers) :
    m_layermanager(num_layers),
    m_image_control(0),
    m_focused_windowtheme(new FbWinFrameTheme(scrn, ".focus", ".Focus")),
    m_unfocused_windowtheme(new FbWinFrameTheme(scrn, ".unfocus", ".Unfocus")),
    // the order of windowtheme and winbutton theme is important
    // because winbutton need to rescale the pixmaps in winbutton theme
    // after fbwinframe have resized them
    m_focused_winbutton_theme(new WinButtonTheme(scrn, "", "", *m_focused_windowtheme)),
    m_unfocused_winbutton_theme(new WinButtonTheme(scrn, ".unfocus", ".Unfocus", *m_unfocused_windowtheme)),
    m_pressed_winbutton_theme(new WinButtonTheme(scrn, ".pressed", ".Pressed", *m_focused_windowtheme)),
    m_menutheme(new FbTk::MenuTheme(scrn)),
    m_root_window(scrn),
    m_geom_window(new OSDWindow(m_root_window, *this, *m_focused_windowtheme)),
    m_pos_window(new OSDWindow(m_root_window, *this, *m_focused_windowtheme)),
    m_tooltip_window(new TooltipWindow(m_root_window, *this, *m_focused_windowtheme)),
    m_dummy_window(scrn, -1, -1, 1, 1, 0, true, false, CopyFromParent, InputOnly),
    resource(rm, screenname, altscreenname),
    m_resource_manager(rm),
    m_name(screenname),
    m_altname(altscreenname),
    m_focus_control(new FocusControl(*this)),
    m_placement_strategy(new ScreenPlacement(*this)),
    m_cycling(false), m_cycle_opts(0),
    m_xinerama_headinfo(0),
    m_restart(false),
    m_shutdown(false) {


    Fluxbox *fluxbox = Fluxbox::instance();
    Display *disp = fluxbox->display();

    initAtoms(disp);


    // TODO fluxgen: check if this is the right place (it was not -lis)
    //
    // Create the first one, initXinerama will expand this if needed.
    m_head_areas.resize(1);
    m_head_areas[0] = new HeadArea();

    initXinerama();

    // setup error handler to catch "screen already managed by other wm"
    XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);

    rootWindow().setEventMask(ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
                              SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
                              ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask);

    fluxbox->sync(false);

    XSetErrorHandler((XErrorHandler) old);

    managed = running;
    if (! managed) {
        delete m_placement_strategy; m_placement_strategy = 0;
        delete m_focus_control; m_focus_control = 0;
        return;
    }

    // we're going to manage the screen, so now add our pid
#ifdef HAVE_GETPID
    unsigned long bpid = static_cast<unsigned long>(getpid());

    rootWindow().changeProperty(fluxbox->getFluxboxPidAtom(), XA_CARDINAL,
                                sizeof(pid_t) * 8, PropModeReplace,
                                (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID

    // check if we're the first EWMH compliant window manager on this screen
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems, ret_bytes_after;
    unsigned char *ret_prop;
    if (rootWindow().property(atom_wm_check, 0l, 1l,
            False, XA_WINDOW, &xa_ret_type, &ret_format, &ret_nitems,
            &ret_bytes_after, &ret_prop) ) {
        m_restart = (ret_prop != NULL);
        XFree(ret_prop);
    }


// setup RANDR for this screens root window
#if defined(HAVE_RANDR)
    int randr_mask = RRScreenChangeNotifyMask;
# ifdef RRCrtcChangeNotifyMask
    randr_mask |= RRCrtcChangeNotifyMask;
# endif
# ifdef RROutputChangeNotifyMask
    randr_mask |= RROutputChangeNotifyMask;
# endif
# ifdef RROutputPropertyNotifyMask
    randr_mask |= RROutputPropertyNotifyMask;
# endif
    XRRSelectInput(disp, rootWindow().window(), randr_mask);
#endif // HAVE_RANDR


    _FB_USES_NLS;

#ifdef DEBUG
    fprintf(stderr, _FB_CONSOLETEXT(Screen, ManagingScreen,
                            "BScreen::BScreen: managing screen %d "
                            "using visual 0x%lx, depth %d\n",
                            "informational message saying screen number (%d), visual (%lx), and colour depth (%d)").c_str(),
            screenNumber(), XVisualIDFromVisual(rootWindow().visual()),
            rootWindow().depth());
#endif // DEBUG

    FbTk::EventManager *evm = FbTk::EventManager::instance();
    evm->add(*this, rootWindow());
    Keys *keys = fluxbox->keys();
    if (keys)
        keys->registerWindow(rootWindow().window(), *this,
                             Keys::GLOBAL|Keys::ON_DESKTOP);
    rootWindow().setCursor(XCreateFontCursor(disp, XC_left_ptr));

    // load this screens resources
    fluxbox->load_rc(*this);

    // setup image cache engine
    m_image_control.reset(new FbTk::ImageControl(scrn,
                                                 fluxbox->colorsPerChannel(),
                                                 fluxbox->getCacheLife(), fluxbox->getCacheMax()));
    imageControl().installRootColormap();
    root_colormap_installed = true;

    m_root_theme.reset(new RootTheme(imageControl()));
    m_root_theme->reconfigTheme();

    focusedWinFrameTheme()->setAlpha(*resource.focused_alpha);
    unfocusedWinFrameTheme()->setAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);

    clampMenuDelay(*resource.menu_delay);

    m_menutheme->setDelay(*resource.menu_delay);

    m_tracker.join(focusedWinFrameTheme()->reconfigSig(),
            FbTk::MemFun(*this, &BScreen::focusedWinFrameThemeReconfigured));


    renderGeomWindow();
    renderPosWindow();
    m_tooltip_window->setDelay(*resource.tooltip_delay);

    // setup workspaces and workspace menu
    int nr_ws = *resource.workspaces;
    addWorkspace(); // at least one
    for (int i = 1; i < nr_ws; ++i) {
        addWorkspace();
    }

    m_current_workspace = m_workspaces_list.front();

    m_windowmenu.reset(createMenu(""));
    m_windowmenu->setInternalMenu();
    m_windowmenu->setReloadHelper(new FbTk::AutoReloadHelper());
    m_windowmenu->reloadHelper()->setReloadCmd(FbTk::RefCount<FbTk::Command<void> >(new FbTk::SimpleCommand<BScreen>(*this, &BScreen::rereadWindowMenu)));

    m_rootmenu.reset(createMenu(""));
    m_rootmenu->setReloadHelper(new FbTk::AutoReloadHelper());
    m_rootmenu->reloadHelper()->setReloadCmd(FbTk::RefCount<FbTk::Command<void> >(new FbTk::SimpleCommand<BScreen>(*this, &BScreen::rereadMenu)));

    m_configmenu.reset(createMenu(_FB_XTEXT(Menu, Configuration,
                                  "Configuration", "Title of configuration menu")));
    setupConfigmenu(*m_configmenu.get());
    m_configmenu->setInternalMenu();

    // check which desktop we should start on
    unsigned int first_desktop = 0;
    if (m_restart) {
        bool exists;
        unsigned int ret=static_cast<unsigned int>(rootWindow().cardinalProperty(atom_net_desktop, &exists));
        if (exists) {
            if (ret < static_cast<unsigned int>(nr_ws))
                first_desktop = ret;
        }
    }

    changeWorkspaceID(first_desktop);

#ifdef USE_SLIT
    m_slit.reset(new Slit(*this, *layerManager().getLayer(ResourceLayer::DESKTOP),
                 fluxbox->getSlitlistFilename().c_str()));
#endif // USE_SLIT

    rm.unlock();

    XFlush(disp);
}



BScreen::~BScreen() {

    if (! managed)
        return;
    
    m_configmenu.reset(0);
    
    m_toolbar.reset(0);

    FbTk::EventManager *evm = FbTk::EventManager::instance();
    evm->remove(rootWindow());
    Keys *keys = Fluxbox::instance()->keys();
    if (keys)
        keys->unregisterWindow(rootWindow().window());

    if (m_rootmenu.get() != 0)
        m_rootmenu->removeAll();

    // Since workspacemenu holds client list menus (from workspace)
    // we need to destroy it before we destroy workspaces
    m_workspacemenu.reset(0);

    if (m_extramenus.size()) {
        // check whether extramenus are included in windowmenu
        // if not, we clean them ourselves
        bool extramenus_in_windowmenu = false;
        for (size_t i = 0, n = m_windowmenu->numberOfItems(); i < n; i++)
            if (m_windowmenu->find(i)->submenu() == m_extramenus.begin()->second) {
                extramenus_in_windowmenu = true;
                break;
            }

        ExtraMenus::iterator mit = m_extramenus.begin();
        ExtraMenus::iterator mit_end = m_extramenus.end();
        for (; mit != mit_end; ++mit) {
            // we set them to NOT internal so that they will be deleted when the
            // menu is cleaned up. We can't delete them here because they are
            // still in the menu
            // (They need to be internal for most of the time so that if we
            // rebuild the menu, then they won't be removed.
            if (! extramenus_in_windowmenu) {
                // not attached to our windowmenu
                // so we clean it up
                delete mit->second;
            } else {
                // let the parent clean it up
                mit->second->setInternalMenu(false);
            }
        }
    }

    removeWorkspaceNames();
    using namespace FbTk::STLUtil;
    destroyAndClear(m_workspaces_list);
    destroyAndClear(m_managed_resources);

    //why not destroyAndClear(m_icon_list); ?
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
    

    delete m_rootmenu.release();
    delete m_workspacemenu.release();
    delete m_windowmenu.release();
    
    // TODO fluxgen: check if this is the right place
    for (size_t i = 0; i < m_head_areas.size(); i++)
        delete m_head_areas[i];

    delete m_focus_control;
    delete m_placement_strategy;

}

bool BScreen::isRestart() {
    return Fluxbox::instance()->isStartup() && m_restart;
}

void BScreen::initWindows() {

#ifdef USE_TOOLBAR
    m_toolbar.reset(new Toolbar(*this,
                                *layerManager().getLayer(::ResourceLayer::NORMAL)));
#endif // USE_TOOLBAR

    unsigned int nchild;
    Window r, p, *children;
    Display *disp = FbTk::App::instance()->display();
    XQueryTree(disp, rootWindow().window(), &r, &p, &children, &nchild);

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

                        fbdbg<<"BScreen::initWindows(): children[j] = 0x"<<hex<<children[j]<<dec<<endl;
                        fbdbg<<"BScreen::initWindows(): = icon_window"<<endl;

                        children[j] = None;
                        break;
                    }
                }
            XFree(wmhints);
        }

    }

    Fluxbox *fluxbox = Fluxbox::instance();

    // manage shown windows
    Window transient_for = 0;
    bool safety_flag = false;
    unsigned int num_transients = 0;
    for (unsigned int i = 0; i <= nchild; ++i) {
        if (i == nchild) {
            if (num_transients) {
                if (num_transients == nchild)
                    safety_flag = true;
                nchild = num_transients;
                i = num_transients = 0;
            } else
                break;
        }

        if (children[i] == None)
            continue;
        else if (!fluxbox->validateWindow(children[i])) {

            fbdbg<<"BScreen::initWindows(): not valid window = "<<hex<<children[i]<<dec<<endl;

            children[i] = None;
            continue;
        }

        // if we have a transient_for window and it isn't created yet...
        // postpone creation of this window until after all others
        if (XGetTransientForHint(disp, children[i], &transient_for) &&
            fluxbox->searchWindow(transient_for) == 0 && !safety_flag) {
            // add this window back to the beginning of the list of children
            children[num_transients] = children[i];
            num_transients++;

            fbdbg<<"BScreen::initWindows(): postpone creation of 0x"<<hex<<children[i]<<dec<<endl;
            fbdbg<<"BScreen::initWindows(): transient_for = 0x"<<hex<<transient_for<<dec<<endl;

            continue;
        }


        XWindowAttributes attrib;
        if (XGetWindowAttributes(disp, children[i],
                                 &attrib)) {
            if (attrib.override_redirect) {
                children[i] = None; // we dont need this anymore, since we already created a window for it
                continue;
            }

            if (attrib.map_state != IsUnmapped)
                createWindow(children[i]);

        }
        children[i] = None; // we dont need this anymore, since we already created a window for it
    }

    XFree(children);

    // now, show slit and toolbar
#ifdef USE_SLIT
    if (slit())
        slit()->show();
#endif // USE_SLIT

}

unsigned int BScreen::currentWorkspaceID() const {
    return m_current_workspace->workspaceID();
}

const Strut* BScreen::availableWorkspaceArea(int head) const {
    if (head > numHeads()) {
        /* May this ever happen? */
        static Strut whole(-1 /* should never be used */, 0, width(), 0, height());
        return &whole;
    }
    return m_head_areas[head ? head-1 : 0]->availableWorkspaceArea();
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

void BScreen::focusedWinFrameThemeReconfigured() {
    renderGeomWindow();
    renderPosWindow();

    Fluxbox *fluxbox = Fluxbox::instance();
    const std::list<Focusable *> winlist =
            focusControl().focusedOrderWinList().clientList();
    std::list<Focusable *>::const_iterator it = winlist.begin(),
                                           it_end = winlist.end();
    for (; it != it_end; ++it)
        fluxbox->updateFrameExtents(*(*it)->fbwindow());

}

void BScreen::propertyNotify(Atom atom) {

    if (allowRemoteActions() && atom == atom_fbcmd) {
        Atom xa_ret_type;
        int ret_format;
        unsigned long ret_nitems, ret_bytes_after;
        char *str;
        if (rootWindow().property(atom_fbcmd, 0l, 64l,
                True, XA_STRING, &xa_ret_type, &ret_format, &ret_nitems,
                &ret_bytes_after, (unsigned char **)&str) && str) {

            if (ret_bytes_after) {
                XFree(str);
                long len = 64 + (ret_bytes_after + 3)/4;
                rootWindow().property(atom_fbcmd, 0l, len,
                    True, XA_STRING, &xa_ret_type, &ret_format, &ret_nitems,
                    &ret_bytes_after, (unsigned char **)&str);
            }

            static std::auto_ptr<FbTk::Command<void> > cmd(0);
            cmd.reset(FbTk::CommandParser<void>::instance().parse(str, false));
            if (cmd.get()) {
                cmd->execute();
            }
            XFree(str);

        }
    // TODO: this doesn't belong in FbPixmap
    } else if (FbTk::FbPixmap::rootwinPropertyNotify(screenNumber(), atom))
        m_bg_change_sig.emit(*this);
}

void BScreen::keyPressEvent(XKeyEvent &ke) {
    Fluxbox::instance()->keys()->doAction(ke.type, ke.state, ke.keycode,
                                          Keys::GLOBAL|Keys::ON_DESKTOP);
}

void BScreen::keyReleaseEvent(XKeyEvent &ke) {
    if (m_cycling) {
        unsigned int state = FbTk::KeyUtil::instance().cleanMods(ke.state);
        state &= ~FbTk::KeyUtil::instance().keycodeToModmask(ke.keycode);

        if (state) // still cycling
            return;

        m_cycling = false;
        focusControl().stopCyclingFocus();
    }
    if (!Fluxbox::instance()->keys()->inKeychain())
        FbTk::EventManager::instance()->ungrabKeyboard();
}

void BScreen::buttonPressEvent(XButtonEvent &be) {
    if (be.button == 1 && !isRootColormapInstalled())
        imageControl().installRootColormap();

    Keys *keys = Fluxbox::instance()->keys();
    keys->doAction(be.type, be.state, be.button, Keys::GLOBAL|Keys::ON_DESKTOP,
                   0, be.time);
}

void BScreen::cycleFocus(int options, const ClientPattern *pat, bool reverse) {
    // get modifiers from event that causes this for focus order cycling
    XEvent ev = Fluxbox::instance()->lastEvent();
    unsigned int mods = 0;
    if (ev.type == KeyPress)
        mods = FbTk::KeyUtil::instance().cleanMods(ev.xkey.state);
    else if (ev.type == ButtonPress)
        mods = FbTk::KeyUtil::instance().cleanMods(ev.xbutton.state);

    if (!m_cycling && mods) {
        m_cycling = true;
        FbTk::EventManager::instance()->grabKeyboard(rootWindow().window());
    }

    if (mods == 0) // can't stacked cycle unless there is a mod to grab
        options |= FocusableList::STATIC_ORDER;

    const FocusableList *win_list =
        FocusableList::getListFromOptions(*this, options);
    focusControl().cycleFocus(*win_list, pat, reverse);

}

FbMenu *BScreen::createMenu(const string &label) {
    FbTk::Layer* layer = layerManager().getLayer(ResourceLayer::MENU);
    FbMenu *menu = new FbMenu(menuTheme(), imageControl(), *layer);
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

FbMenu *BScreen::createToggleMenu(const string &label) {
    FbTk::Layer* layer = layerManager().getLayer(ResourceLayer::MENU);
    FbMenu *menu = new ToggleMenu(menuTheme(), imageControl(), *layer);
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

void BScreen::addExtraWindowMenu(const FbTk::FbString &label, FbTk::Menu *menu) {
    menu->setInternalMenu();
    menu->disableTitle();
    m_extramenus.push_back(make_pair(label, menu));
    rereadWindowMenu();
}

void BScreen::reconfigure() {
    Fluxbox *fluxbox = Fluxbox::instance();

    focusedWinFrameTheme()->setAlpha(*resource.focused_alpha);
    unfocusedWinFrameTheme()->setAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);

    clampMenuDelay(*resource.menu_delay);

    m_menutheme->setDelay(*resource.menu_delay);

    // realize the number of workspaces from the init-file
    const unsigned int nr_ws = *resource.workspaces;
    if (nr_ws > m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            addWorkspace();
        }
    } else if (nr_ws < m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            removeLastWorkspace();
        }
    }

    // update menu filenames
    m_rootmenu->reloadHelper()->setMainFile(fluxbox->getMenuFilename());
    m_windowmenu->reloadHelper()->setMainFile(windowMenuFilename());

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
    m_reconfigure_sig.emit(*this);

    // Reload style
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename(),
                                        fluxbox->getStyleOverlayFilename(),
                                        m_root_theme->screenNum());

    reconfigureTabs();
}

void BScreen::reconfigureTabs() {
    const std::list<Focusable *> winlist =
            focusControl().focusedOrderWinList().clientList();
    std::list<Focusable *>::const_iterator it = winlist.begin(),
                                           it_end = winlist.end();
    for (; it != it_end; ++it)
        (*it)->fbwindow()->applyDecorations();
}

void BScreen::updateWorkspaceName(unsigned int w) {
    Workspace *space = getWorkspace(w);
    if (space) {
        m_workspace_names[w] = space->name();
        m_workspacenames_sig.emit(*this);
        Fluxbox::instance()->save_rc();
    }
}

void BScreen::removeWorkspaceNames() {
    m_workspace_names.clear();
}

void BScreen::addIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    // make sure we have a unique list
    if (find(iconList().begin(), iconList().end(), w) != iconList().end())
        return;

    iconList().push_back(w);

    // notify listeners
    iconListSig().emit(*this);
}


void BScreen::removeIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    Icons::iterator erase_it = remove_if(iconList().begin(),
                                         iconList().end(),
                                         bind2nd(equal_to<FluxboxWindow *>(), w));
    // no need to send iconlist signal if we didn't
    // change the iconlist
    if (erase_it != m_icon_list.end()) {
        iconList().erase(erase_it);
        iconListSig().emit(*this);
    }
}

void BScreen::removeWindow(FluxboxWindow *win) {

    fbdbg<<"BScreen::removeWindow("<<win<<")"<<endl;

    // extra precaution, if for some reason, the
    // icon list should be out of sync
    removeIcon(win);
    // remove from workspace
    Workspace *space = getWorkspace(win->workspaceNumber());
    if (space != 0)
        space->removeWindow(win, false);
}


void BScreen::removeClient(WinClient &client) {

    focusControl().removeClient(client);

    if (client.fbwindow() && client.fbwindow()->isIconic())
        iconListSig().emit(*this);

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

    bool save_name = getNameOfWorkspace(m_workspaces_list.size()) == "";
    Workspace *wkspc = new Workspace(*this,
                                     getNameOfWorkspace(m_workspaces_list.size()),
                                     m_workspaces_list.size());
    m_workspaces_list.push_back(wkspc);

    if (save_name) {
        addWorkspaceName(wkspc->name().c_str());
        m_workspacenames_sig.emit(*this);
    }

    saveWorkspaces(m_workspaces_list.size());
    workspaceCountSig().emit( *this );

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

    wkspc->removeAll(wkspc->workspaceID()-1);

    Icons::iterator it = iconList().begin();
    const Icons::iterator it_end = iconList().end();
    for (; it != it_end; ++it) {
        if ((*it)->workspaceNumber() == wkspc->workspaceID())
            (*it)->setWorkspace(wkspc->workspaceID()-1);
    }
    m_clientlist_sig.emit(*this);

    //remove last workspace
    m_workspaces_list.pop_back();

    saveWorkspaces(m_workspaces_list.size());
    workspaceCountSig().emit( *this );
    // must be deleted after we send notify!!
    // so we dont get bad pointers somewhere
    // while processing the notify signal
    delete wkspc;

    return m_workspaces_list.size();
}


void BScreen::changeWorkspaceID(unsigned int id, bool revert) {

    if (! m_current_workspace || id >= m_workspaces_list.size() ||
        id == m_current_workspace->workspaceID())
        return;

    /* Ignore all EnterNotify events until the pointer actually moves */
    this->focusControl().ignoreAtPointer();

    FbTk::App::instance()->sync(false);

    FluxboxWindow *focused = FocusControl::focusedFbWindow();

    if (focused && focused->isMoving() && doOpaqueMove())
        // don't reassociate if not opaque moving
        reassociateWindow(focused, id, true);

    // set new workspace
    Workspace *old = currentWorkspace();
    m_current_workspace = getWorkspace(id);

    // we show new workspace first in order to appear faster
    currentWorkspace()->showAll();

    // reassociate all windows that are stuck to the new workspace
    Workspace::Windows wins = old->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it)->isStuck()) {
            reassociateWindow(*it, id, true);
        }
    }

    // change workspace ID of stuck iconified windows, too
    Icons::iterator icon_it = iconList().begin();
    for (; icon_it != iconList().end(); ++icon_it) {
        if ((*icon_it)->isStuck())
            (*icon_it)->setWorkspace(id);
    }

    if (focused && focused->isMoving() && doOpaqueMove())
        focused->focus();
    else if (revert)
        FocusControl::revertFocus(*this);

    old->hideAll(false);

    FbTk::App::instance()->sync(false);

    m_currentworkspace_sig.emit(*this);

    // do this after atom handlers, so scripts can access new workspace number
    Fluxbox::instance()->keys()->doAction(FocusIn, 0, 0, Keys::ON_DESKTOP);
}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
    if (! m_current_workspace || id >= m_workspaces_list.size())
        return;

    if (!win)
        win = FocusControl::focusedFbWindow();

    if (!win || &win->screen() != this || win->isStuck())
        return;

    FbTk::App::instance()->sync(false);

    windowMenu().hide();
    reassociateWindow(win, id, true);

    // change workspace ?
    if (changeWS)
        changeWorkspaceID(id, false);

    // if the window is on current workspace, show it; else hide it.
    if (id == currentWorkspace()->workspaceID() && !win->isIconic())
        win->show();
    else {
        win->hide(true);
        FocusControl::revertFocus(*this);
    }

    // send all the transients too
    FluxboxWindow::ClientList::iterator client_it = win->clientList().begin();
    FluxboxWindow::ClientList::iterator client_it_end = win->clientList().end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient::TransientList::const_iterator it = (*client_it)->transientList().begin();
        WinClient::TransientList::const_iterator it_end = (*client_it)->transientList().end();
        for (; it != it_end; ++it) {
            if ((*it)->fbwindow())
                sendToWorkspace(id, (*it)->fbwindow(), false);
        }
    }

}


bool BScreen::isKdeDockapp(Window client) const {
    //Check and see if client is KDE dock applet.
    bool iskdedockapp = false;
    Atom ajunk;
    int ijunk;
    unsigned long *data = 0, uljunk;
    Display *disp = FbTk::App::instance()->display();
    // Check if KDE v2.x dock applet
    if (XGetWindowProperty(disp, client, atom_kde_systray,
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
        if (XGetWindowProperty(disp, client,
                               atom_kwm1, 0l, 1l, False,
                               atom_kwm1, &ajunk, &ijunk, &uljunk,
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
    FbTk::EventHandler *evh  = 0;
    FbTk::EventManager *evm = FbTk::EventManager::instance();

    AtomHandler* handler = 0;
#if USE_SYSTRAY
    handler = Fluxbox::instance()->getAtomHandler(SystemTray::getNetSystemTrayAtom(screenNumber()));
#endif
    if (handler == 0) {
#ifdef USE_SLIT
        if (slit() != 0 && slit()->acceptKdeDockapp())
            slit()->addClient(client);
        else
#endif // USE_SLIT
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

    if (winclient->initial_state == WithdrawnState ||
        winclient->getWMClassClass() == "DockApp") {
        delete winclient;
#ifdef USE_SLIT
        if (slit() && !isKdeDockapp(client))
            slit()->addClient(client);
#endif // USE_SLIT
        return 0;
    }

    // check if it should be grouped with something else
    FluxboxWindow *win;
    WinClient *other;
    if ((other = findGroupLeft(*winclient)) && (win = other->fbwindow())) {
        win->attachClient(*winclient);
        Fluxbox::instance()->attachSignals(*winclient);
    } else {

        Fluxbox::instance()->attachSignals(*winclient);
        if (winclient->fbwindow()) { // may have been set in an atomhandler
            win = winclient->fbwindow();
            Workspace *workspace = getWorkspace(win->workspaceNumber());
            if (workspace)
                workspace->updateClientmenu();
        } else {
            win = new FluxboxWindow(*winclient);

            if (!win->isManaged()) {
                delete win;
                return 0;
            }
        }
    }

    // add the window to the focus list
    // always add to front on startup to keep the focus order the same
    if (win->isFocused() || Fluxbox::instance()->isStartup())
        focusControl().addFocusFront(*winclient);
    else
        focusControl().addFocusBack(*winclient);

    // we also need to check if another window expects this window to the left
    // and if so, then join it.
    if ((other = findGroupRight(*winclient)) && other->fbwindow() != win)
        win->attachClient(*other);
    else if (other) // should never happen
        win->moveClientRightOf(*other, *winclient);

    m_clientlist_sig.emit(*this);

    FbTk::App::instance()->sync(false);
    return win;
}


FluxboxWindow *BScreen::createWindow(WinClient &client) {

    if (isKdeDockapp(client.window()) && addKdeDockapp(client.window())) {
        return 0;
    }

    FluxboxWindow *win = new FluxboxWindow(client);

#ifdef SLIT
    if (slit() != 0) {

        if (win->initialState() == WithdrawnState) {
            slit()->addClient(client.window());
        } else if (client->getWMClassClass() == "DockApp") {
            slit()->addClient(client.window());
        }
    }
#endif // SLIT


    if (!win->isManaged()) {
        delete win;
        return 0;
    }

    win->show();
    // don't ask me why, but client doesn't seem to keep focus in new window
    // and we don't seem to get a FocusIn event from setInputFocus
    if ((focusControl().focusNew() || FocusControl::focusedWindow() == &client)
            && win->focus())
        FocusControl::setFocusedWindow(&client);

    m_clientlist_sig.emit(*this);

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
        next = m_head_areas[i]->requestStrut(i+1, left, right, top, bottom, next);
    }

    return next;
}

void BScreen::clearStrut(Strut *str) {
    if (str->next())
        clearStrut(str->next());
    int head = str->head() ? str->head() - 1 : 0;
    /* The number of heads may have changed, be careful. */
    if (head < (numHeads() ? numHeads() : 1))
        m_head_areas[head]->clearStrut(str);
    // str is invalid now
}

void BScreen::updateAvailableWorkspaceArea() {
    size_t n = (numHeads() ? numHeads() : 1);
    bool updated = false;

    for (size_t i = 0; i < n; i++) {
        updated = m_head_areas[i]->updateAvailableWorkspaceArea() || updated;
    }

    if (updated)
        m_workspace_area_sig.emit(*this);
}

void BScreen::addWorkspaceName(const char *name) {
    m_workspace_names.push_back(FbTk::FbStringUtil::LocaleStrToFb(name));
    Workspace *wkspc = getWorkspace(m_workspace_names.size()-1);
    if (wkspc)
        wkspc->setName(m_workspace_names.back());
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

    if (wkspc_id >= numberOfWorkspaces())
        wkspc_id = currentWorkspace()->workspaceID();

    if (!w->isIconic() && w->workspaceNumber() == wkspc_id)
        return;


    if (w->isIconic()) {
        removeIcon(w);
        getWorkspace(wkspc_id)->addWindow(*w);
    } else if (ignore_sticky || ! w->isStuck()) {
        // fresh windows have workspaceNumber == -1, which leads to
        // an invalid workspace (unsigned int)
        Workspace* ws = getWorkspace(w->workspaceNumber());
        if (ws)
            ws->removeWindow(w, true);
        getWorkspace(wkspc_id)->addWindow(*w);
    }
}

void BScreen::initMenus() {
    m_workspacemenu.reset(MenuCreator::createMenuType("workspacemenu", screenNumber()));
    m_rootmenu->reloadHelper()->setMainFile(Fluxbox::instance()->getMenuFilename());
    m_windowmenu->reloadHelper()->setMainFile(windowMenuFilename());
}


void BScreen::rereadMenu() {

    m_rootmenu->removeAll();
    m_rootmenu->setLabel(FbTk::BiDiString(""));

    Fluxbox * const fb = Fluxbox::instance();
    if (!fb->getMenuFilename().empty())
        MenuCreator::createFromFile(fb->getMenuFilename(), *m_rootmenu,
                                    m_rootmenu->reloadHelper());

    if (m_rootmenu->numberOfItems() == 0) {
        _FB_USES_NLS;
        m_rootmenu->setLabel(_FB_XTEXT(Menu, DefaultRootMenu, "Fluxbox default menu", "Title of fallback root menu"));
        FbTk::RefCount<FbTk::Command<void> > restart_fb(FbTk::CommandParser<void>::instance().parse("restart"));
        FbTk::RefCount<FbTk::Command<void> > exit_fb(FbTk::CommandParser<void>::instance().parse("exit"));
        FbTk::RefCount<FbTk::Command<void> > execute_xterm(FbTk::CommandParser<void>::instance().parse("exec xterm"));
        m_rootmenu->setInternalMenu();
        m_rootmenu->insert("xterm", execute_xterm);
        m_rootmenu->insert(_FB_XTEXT(Menu, Reconfigure, "Reconfigure",
                                     "Reload Configuration command")),
        m_rootmenu->insert(_FB_XTEXT(Menu, Restart, "Restart", "Restart command"),
                           restart_fb);
        m_rootmenu->insert(_FB_XTEXT(Menu, Exit, "Exit", "Exit command"),
                           exit_fb);
    }

}

const std::string BScreen::windowMenuFilename() const {
    if ((*resource.windowmenufile).empty())
        return Fluxbox::instance()->getDefaultDataFilename("windowmenu");
    return *resource.windowmenufile;
}

void BScreen::rereadWindowMenu() {

    m_windowmenu->removeAll();
    if (!windowMenuFilename().empty())
        MenuCreator::createFromFile(windowMenuFilename(), *m_windowmenu,
                                    m_windowmenu->reloadHelper());

}

void BScreen::addConfigMenu(const FbTk::FbString &label, FbTk::Menu &menu) {
    m_configmenu_list.push_back(make_pair(label, &menu));
    if (m_configmenu.get())
        setupConfigmenu(*m_configmenu.get());
}

void BScreen::removeConfigMenu(FbTk::Menu &menu) {
    Configmenus::iterator erase_it = find_if(m_configmenu_list.begin(),
                                             m_configmenu_list.end(),
                                             FbTk::Compose(bind2nd(equal_to<FbTk::Menu *>(), &menu),
                                                           FbTk::Select2nd<Configmenus::value_type>()));
    if (erase_it != m_configmenu_list.end())
        m_configmenu_list.erase(erase_it);

    if (!isShuttingdown() && m_configmenu.get())
        setupConfigmenu(*m_configmenu.get());

}


void BScreen::addManagedResource(FbTk::Resource_base *resource) {
    m_managed_resources.push_back(resource);
}

int BScreen::getGap(int head, const char type) {
    return type == 'w' ? getXGap(head) : getYGap(head);
}

int BScreen::calRelativeSize(int head, int i, char type) {
    // return floor(i * getGap(head, type) / 100 + 0.5);
    return FbTk::RelCalcHelper::calPercentageValueOf(i, getGap(head, type));
}
int BScreen::calRelativeWidth(int head, int i) {
    return calRelativeSize(head, i, 'w');
}
int BScreen::calRelativeHeight(int head, int i) {
    return calRelativeSize(head, i, 'h');
}

int BScreen::calRelativePosition(int head, int i, char type) {
    int max = type == 'w' ? maxLeft(head) : maxTop(head);
    // return floor((i - min) / getGap(head, type)  * 100 + 0.5);
    return FbTk::RelCalcHelper::calPercentageOf((i - max), getGap(head, type));
}
// returns a pixel, which is relative to the width of the screen
// screen starts from 0, 1000 px width, if i is 10 then it should return 100
int BScreen::calRelativePositionWidth(int head, int i) {
    return calRelativePosition(head, i, 'w');
}
// returns a pixel, which is relative to the height of th escreen
// screen starts from 0, 1000 px height, if i is 10 then it should return 100
int BScreen::calRelativePositionHeight(int head, int i) {
    return calRelativePosition(head, i, 'h');
}

int BScreen::calRelativeDimension(int head, int i, char type) {
    // return floor(i / getGap(head, type) * 100 + 0.5);
    return FbTk::RelCalcHelper::calPercentageOf(i, getGap(head, type));
  }
int BScreen::calRelativeDimensionWidth(int head, int i) {
    return calRelativeDimension(head, i, 'w');
}
int BScreen::calRelativeDimensionHeight(int head, int i) {
    return calRelativeDimension(head, i, 'h');
}

float BScreen::getXGap(int head) {
    return maxRight(head) - maxLeft(head);
}
float BScreen::getYGap(int head) {
    return maxBottom(head) - maxTop(head);
}

void BScreen::setupConfigmenu(FbTk::Menu &menu) {
    _FB_USES_NLS;

    menu.removeAll();

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconftabs_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command<void> > saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                                 *Fluxbox::instance(),
                                                 &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command<void> > reconf_cmd(FbTk::CommandParser<void>::instance().parse("reconfigure"));

    FbTk::RefCount<FbTk::Command<void> > reconftabs_cmd(new FbTk::SimpleCommand<BScreen>(
                                                 *this,
                                                 &BScreen::reconfigureTabs));
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    s_a_reconftabs_macro->add(saverc_cmd);
    s_a_reconftabs_macro->add(reconftabs_cmd);
    FbTk::RefCount<FbTk::Command<void> > save_and_reconfigure(s_a_reconf_macro);
    FbTk::RefCount<FbTk::Command<void> > save_and_reconftabs(s_a_reconftabs_macro);
    // create focus menu
    // we don't set this to internal menu so will
    // be deleted toghether with the parent
    FbTk::FbString focusmenu_label = _FB_XTEXT(Configmenu, FocusModel,
                                          "Focus Model",
                                          "Method used to give focus to windows");
    FbTk::Menu *focus_menu = createMenu(focusmenu_label);

#define _BOOLITEM(m,a, b, c, d, e, f) (m).insert(new FbTk::BoolMenuItem(_FB_XTEXT(a, b, c, d), e, f))


#define _FOCUSITEM(a, b, c, d, e) \
    focus_menu->insert(new FocusModelMenuItem(_FB_XTEXT(a, b, c, d), focusControl(), \
                                              e, save_and_reconfigure))

    _FOCUSITEM(Configmenu, ClickFocus,
               "Click To Focus", "Click to focus",
               FocusControl::CLICKFOCUS);
    _FOCUSITEM(Configmenu, MouseFocus,
               "Mouse Focus (Keyboard Friendly)",
               "Mouse Focus (Keyboard Friendly)",
               FocusControl::MOUSEFOCUS);
    _FOCUSITEM(Configmenu, StrictMouseFocus,
               "Mouse Focus (Strict)",
               "Mouse Focus (Strict)",
               FocusControl::STRICTMOUSEFOCUS);
#undef _FOCUSITEM

    focus_menu->insert(new FbTk::MenuSeparator());
    focus_menu->insert(new TabFocusModelMenuItem(_FB_XTEXT(Configmenu,
        ClickTabFocus, "ClickTabFocus", "Click tab to focus windows"),
        focusControl(), FocusControl::CLICKTABFOCUS, save_and_reconfigure));
    focus_menu->insert(new TabFocusModelMenuItem(_FB_XTEXT(Configmenu,
        MouseTabFocus, "MouseTabFocus", "Hover over tab to focus windows"),
        focusControl(), FocusControl::MOUSETABFOCUS, save_and_reconfigure));
    focus_menu->insert(new FbTk::MenuSeparator());

    try {
        focus_menu->insert(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, FocusNew,
            "Focus New Windows", "Focus newly created windows"),
            m_resource_manager.getResource<bool>(name() + ".focusNewWindows"),
            saverc_cmd));
    } catch (FbTk::ResourceException & e) {
        cerr<<e.what()<<endl;
    }
   
#ifdef XINERAMA
    try {
        focus_menu->insert(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, FocusSameHead,
            "Keep Head", "Only revert focus on same head"),
            m_resource_manager.getResource<bool>(name() + ".focusSameHead"),
            saverc_cmd));
    } catch (FbTk::ResourceException e) {
        cerr<<e.what()<<endl;
    }
#endif // XINERAMA

    _BOOLITEM(*focus_menu, Configmenu, AutoRaise,
              "Auto Raise", "Auto Raise windows on sloppy",
              resource.auto_raise, saverc_cmd);
    _BOOLITEM(*focus_menu, Configmenu, ClickRaises,
              "Click Raises", "Click Raises",
              resource.click_raises, saverc_cmd);

    focus_menu->updateMenu();

    menu.insert(focusmenu_label, focus_menu);

    // END focus menu

    // BEGIN maximize menu

    FbTk::FbString maxmenu_label = _FB_XTEXT(Configmenu, MaxMenu,
            "Maximize Options", "heading for maximization options");
    FbTk::Menu *maxmenu = createMenu(maxmenu_label);

    _BOOLITEM(*maxmenu, Configmenu, FullMax,
              "Full Maximization", "Maximise over slit, toolbar, etc",
              resource.full_max, saverc_cmd);
    _BOOLITEM(*maxmenu, Configmenu, MaxIgnoreInc,
              "Ignore Resize Increment",
              "Maximizing Ignores Resize Increment (e.g. xterm)",
              resource.max_ignore_inc, saverc_cmd);
    _BOOLITEM(*maxmenu, Configmenu, MaxDisableMove,
              "Disable Moving", "Don't Allow Moving While Maximized",
              resource.max_disable_move, saverc_cmd);
    _BOOLITEM(*maxmenu, Configmenu, MaxDisableResize,
              "Disable Resizing", "Don't Allow Resizing While Maximized",
              resource.max_disable_resize, saverc_cmd);

    maxmenu->updateMenu();
    menu.insert(maxmenu_label, maxmenu);

    // END maximize menu

    // BEGIN tab menu

    FbTk::FbString tabmenu_label = _FB_XTEXT(Configmenu, TabMenu,
                                        "Tab Options",
                                        "heading for tab-related options");
    FbTk::Menu *tab_menu = createMenu(tabmenu_label);
    FbTk::FbString tabplacement_label = _FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu");
    FbTk::Menu *tabplacement_menu = createToggleMenu(tabplacement_label);

    tab_menu->insert(tabplacement_label, tabplacement_menu);

    _BOOLITEM(*tab_menu,Configmenu, TabsInTitlebar,
              "Tabs in Titlebar", "Tabs in Titlebar",
              resource.default_internal_tabs, save_and_reconftabs);
    tab_menu->insert(new FbTk::BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,
              "Maximize Over", "Maximize over this thing when maximizing"),
              resource.max_over_tabs, save_and_reconfigure));
    tab_menu->insert(new FbTk::BoolMenuItem(_FB_XTEXT(Toolbar, ShowIcons,
              "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
              resource.tabs_use_pixmap, save_and_reconfigure));

    FbTk::MenuItem *tab_width_item =
        new FbTk::IntMenuItem(_FB_XTEXT(Configmenu, ExternalTabWidth,
                                       "External Tab Width",
                                       "Width of external-style tabs"),
                               resource.tab_width, 10, 3000, /* silly number */
                               *tab_menu);
    tab_width_item->setCommand(save_and_reconftabs);
    tab_menu->insert(tab_width_item);

    // menu is 3 wide, 5 down
    struct PlacementP {
         const FbTk::FbString label;
         FbWinFrame::TabPlacement placement;
    };
    static const PlacementP place_menu[] = {

        { _FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), FbWinFrame::TOPLEFT},
        { _FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), FbWinFrame::LEFTTOP},
        { _FB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), FbWinFrame::LEFT},
        { _FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), FbWinFrame::LEFTBOTTOM},
        { _FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), FbWinFrame::BOTTOMLEFT},
        { _FB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), FbWinFrame::TOP},
        { "", FbWinFrame::TOPLEFT},
        { "", FbWinFrame::TOPLEFT},
        { "", FbWinFrame::TOPLEFT},
        { _FB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), FbWinFrame::BOTTOM},
        { _FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), FbWinFrame::TOPRIGHT},
        { _FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), FbWinFrame::RIGHTTOP},
        { _FB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), FbWinFrame::RIGHT},
        { _FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), FbWinFrame::RIGHTBOTTOM},
        { _FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), FbWinFrame::BOTTOMRIGHT}
    };

    tabplacement_menu->setMinimumColumns(3);
    // create items in sub menu
    for (size_t i=0; i< sizeof(place_menu)/sizeof(PlacementP); ++i) {
        const PlacementP& p = place_menu[i];
        if (p.label == "") {
            tabplacement_menu->insert(p.label);
            tabplacement_menu->setItemEnabled(i, false);
        } else
            tabplacement_menu->insert(new TabPlacementMenuItem(p.label, *this, p.placement, save_and_reconftabs));
    }
    tabplacement_menu->updateMenu();

    menu.insert(tabmenu_label, tab_menu);

#ifdef HAVE_XRENDER
    if (FbTk::Transparent::haveRender() ||
        FbTk::Transparent::haveComposite()) {

        FbTk::FbString alphamenu_label = _FB_XTEXT(Configmenu, Transparency,
                                          "Transparency",
                                           "Menu containing various transparency options");
        FbTk::Menu *alpha_menu = createMenu(alphamenu_label);

        if (FbTk::Transparent::haveComposite(true)) {
            static FbTk::SimpleAccessor<bool> s_pseudo(Fluxbox::instance()->getPseudoTrans());
            alpha_menu->insert(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, ForcePseudoTrans,
                               "Force Pseudo-Transparency",
                               "When composite is available, still use old pseudo-transparency"),
                    s_pseudo, save_and_reconfigure));
        }

        // in order to save system resources, don't save or reconfigure alpha
        // settings until after the user is done changing them
        FbTk::RefCount<FbTk::Command<void> > delayed_save_and_reconf(
            new FbTk::DelayedCmd(save_and_reconfigure));

        FbTk::MenuItem *focused_alpha_item =
            new FbTk::IntMenuItem(_FB_XTEXT(Configmenu, FocusedAlpha,
                                       "Focused Window Alpha",
                                       "Transparency level of the focused window"),
                    resource.focused_alpha, 0, 255, *alpha_menu);
        focused_alpha_item->setCommand(delayed_save_and_reconf);
        alpha_menu->insert(focused_alpha_item);

        FbTk::MenuItem *unfocused_alpha_item =
            new FbTk::IntMenuItem(_FB_XTEXT(Configmenu,
                                       UnfocusedAlpha,
                                       "Unfocused Window Alpha",
                                       "Transparency level of unfocused windows"),

                    resource.unfocused_alpha, 0, 255, *alpha_menu);
        unfocused_alpha_item->setCommand(delayed_save_and_reconf);
        alpha_menu->insert(unfocused_alpha_item);

        FbTk::MenuItem *menu_alpha_item =
            new FbTk::IntMenuItem(_FB_XTEXT(Configmenu, MenuAlpha,
                                       "Menu Alpha", "Transparency level of menu"),
                    resource.menu_alpha, 0, 255, *alpha_menu);
        menu_alpha_item->setCommand(delayed_save_and_reconf);
        alpha_menu->insert(menu_alpha_item);

        alpha_menu->updateMenu();
        menu.insert(alphamenu_label, alpha_menu);
    }
#endif // HAVE_XRENDER

    Configmenus::iterator it = m_configmenu_list.begin();
    Configmenus::iterator it_end = m_configmenu_list.end();
    for (; it != it_end; ++it)
        menu.insert(it->first, it->second);

    _BOOLITEM(menu, Configmenu, OpaqueMove,
              "Opaque Window Moving",
              "Window Moving with whole window visible (as opposed to outline moving)",
              resource.opaque_move, saverc_cmd);
    _BOOLITEM(menu, Configmenu, WorkspaceWarping,
              "Workspace Warping",
              "Workspace Warping - dragging windows to the edge and onto the next workspace",
              resource.workspace_warping, saverc_cmd);

#undef _BOOLITEM

    // finaly update menu
    menu.updateMenu();
}


void BScreen::shutdown() {
    rootWindow().setEventMask(NoEventMask);
    FbTk::App::instance()->sync(false);
    m_shutdown = true;
    m_focus_control->shutdown();
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::shutdown));
}


void BScreen::showPosition(int x, int y) {
    if (!doShowWindowPos())
        return;

    char buf[256];
    sprintf(buf, "X:%5d x Y:%5d", x, y);

    FbTk::BiDiString label(buf);
    m_pos_window->showText(label);
}


void BScreen::hidePosition() {
    m_pos_window->hide();
}

void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
    if (!doShowWindowPos())
        return;

    char buf[256];
    _FB_USES_NLS;

    sprintf(buf,
            _FB_XTEXT(Screen, GeometryFormat,
                    "W: %4d x H: %4d",
                    "Format for width and height window, %4d for width, and %4d for height").c_str(),
            gx, gy);

    FbTk::BiDiString label(buf);
    m_geom_window->showText(label);
}


void BScreen::showTooltip(const FbTk::BiDiString &text) {
    if (*resource.tooltip_delay >= 0)
        m_tooltip_window->showText(text);
}

void BScreen::hideTooltip() {
    if (*resource.tooltip_delay >= 0)
        m_tooltip_window->hide();
}


void BScreen::hideGeometry() {
    m_geom_window->hide();
}

void BScreen::setLayer(FbTk::LayerItem &item, int layernum) {
    m_layermanager.moveToLayer(item, layernum);
}


/**
 Goes to the workspace "right" of the current
*/
void BScreen::nextWorkspace(int delta) {
    changeWorkspaceID( (currentWorkspaceID() + delta) % numberOfWorkspaces());
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::prevWorkspace(int delta) {
    changeWorkspaceID( (static_cast<signed>(numberOfWorkspaces()) + currentWorkspaceID() - (delta % numberOfWorkspaces())) % numberOfWorkspaces());
}

/**
 Goes to the workspace "right" of the current
*/
void BScreen::rightWorkspace(int delta) {
    if (currentWorkspaceID()+delta < numberOfWorkspaces())
        changeWorkspaceID(currentWorkspaceID()+delta);
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::leftWorkspace(int delta) {
    if (currentWorkspaceID() >= static_cast<unsigned int>(delta))
        changeWorkspaceID(currentWorkspaceID()-delta);
}


void BScreen::renderGeomWindow() {

    char buf[256];
    _FB_USES_NLS;

    const std::string msg = _FB_XTEXT(Screen, GeometrySpacing,
            "W: %04d x H: %04d", "Representative maximum sized text for width and height dialog");
    const int n = snprintf(buf, msg.size(), msg.c_str(), 0, 0);

    FbTk::BiDiString label(std::string(buf, n));
    m_geom_window->resizeForText(label);
    m_geom_window->reconfigTheme();
}


void BScreen::renderPosWindow() {
    m_pos_window->resizeForText(FbTk::BiDiString("0:00000 x 0:00000"));
    m_pos_window->reconfigTheme();
}

void BScreen::updateSize() {
    // update xinerama layout
    initXinerama();

    // check if window geometry has changed
    if (rootWindow().updateGeometry()) {
        // reset background
        m_root_theme->reset();

        // send resize notify
        m_resize_sig.emit(*this);
        m_workspace_area_sig.emit(*this);

        // move windows out of inactive heads
        clearHeads();
    }
}


/**
 * Find the winclient to this window's left
 * So, we check the leftgroup hint, and see if we know any windows
 */
WinClient *BScreen::findGroupLeft(WinClient &winclient) {
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

    return have_client;
}

WinClient *BScreen::findGroupRight(WinClient &winclient) {
    Groupables::iterator it = m_expecting_groups.find(winclient.window());
    if (it == m_expecting_groups.end())
        return 0;

    // yay, this'll do.
    WinClient *other = it->second;
    m_expecting_groups.erase(it); // don't expect it anymore

    // forget about it if it isn't the left-most client in the group
    Window leftwin = other->getGroupLeftWindow();
    if (leftwin != None && leftwin != winclient.window())
        return 0;

    return other;
}
void BScreen::clearXinerama() {
    fbdbg<<"BScreen::initXinerama(): dont have Xinerama"<<endl;

    m_xinerama_avail = false;
    if (m_xinerama_headinfo)
        delete [] m_xinerama_headinfo;
    m_xinerama_headinfo = 0;
    m_xinerama_num_heads = 0;
}

void BScreen::initXinerama() {
#ifdef XINERAMA
    Display *display = FbTk::App::instance()->display();

    if (!XineramaIsActive(display)) {
        clearXinerama();
        return;
    }

    fbdbg<<"BScreen::initXinerama(): have Xinerama"<<endl;

    m_xinerama_avail = true;

    XineramaScreenInfo *screen_info;
    int number;
    screen_info = XineramaQueryScreens(display, &number);

    /* The call may have actually failed. If this is the first time we init
     * Xinerama, fall back to turning it off. If not, pretend nothing
     * happened -- another event will tell us and it will work then. */
    if (!screen_info) {
        if (!m_xinerama_headinfo)
            clearXinerama();
        return;
    }

    if (m_xinerama_headinfo)
        delete [] m_xinerama_headinfo;

    m_xinerama_headinfo = new XineramaHeadInfo[number];
    m_xinerama_num_heads = number;
    for (int i=0; i < number; i++) {
        m_xinerama_headinfo[i]._x = screen_info[i].x_org;
        m_xinerama_headinfo[i]._y = screen_info[i].y_org;
        m_xinerama_headinfo[i]._width = screen_info[i].width;
        m_xinerama_headinfo[i]._height = screen_info[i].height;
    }
    XFree(screen_info);

    fbdbg<<"BScreen::initXinerama(): number of heads ="<<number<<endl;

    /* Reallocate to the new number of heads. */
    int ha_num = numHeads() ? numHeads() : 1;
    int ha_oldnum = m_head_areas.size();
    if (ha_num > ha_oldnum) {
        m_head_areas.resize(ha_num);
        for (int i = ha_oldnum; i < ha_num; i++)
            m_head_areas[i] = new HeadArea();
    } else if (ha_num < ha_oldnum) {
        for (int i = ha_num; i < ha_oldnum; i++)
            delete m_head_areas[i];
        m_head_areas.resize(ha_num);
    }

#else // XINERAMA
    // no xinerama
    m_xinerama_avail = false;
    m_xinerama_num_heads = 0;
#endif // XINERAMA

}

/* Move windows out of inactive heads */
void BScreen::clearHeads() {
    if (!hasXinerama()) return;

    for (Workspaces::iterator i = m_workspaces_list.begin();
            i != m_workspaces_list.end(); ++i) {
        for (Workspace::Windows::iterator win = (*i)->windowList().begin();
                win != (*i)->windowList().end(); ++win) {

            FluxboxWindow& w = *(*win);

            // check if the window is invisible
            bool invisible = true;
            int j;
            for (j = 0; j < m_xinerama_num_heads; ++j) {
                XineramaHeadInfo& hi = m_xinerama_headinfo[j];
                if (RectangleUtil::overlapRectangles(hi, w)) {
                    invisible = false;
                    break;
                }
            }

            if (invisible) { // get closest head and replace the (now invisible) cwindow
                int closest_head = getHead(w.fbWindow());
                if (closest_head == 0) {
                    closest_head = 1; // first head is a safe bet here
                }
                w.placeWindow(closest_head);
            }
        }
    }
}

int BScreen::getHead(int x, int y) const {

#ifdef XINERAMA
    if (hasXinerama()) {
        for (int i=0; i < m_xinerama_num_heads; i++) {
            if (RectangleUtil::insideBorder(m_xinerama_headinfo[i], x, y, 0)) {
                return i+1;
            }
        }
    }
#endif // XINERAMA
    return 0;
}


int BScreen::getHead(const FbTk::FbWindow &win) const {

    int head = 0; // whole screen

#ifdef XINERAMA
    if (hasXinerama()) {

        // cast needed to prevent win.x() become "unsigned int" which is bad
        // since it might be negative
        int cx = win.x() + static_cast<int>(win.width() / 2);
        int cy = win.y() + static_cast<int>(win.height() / 2);

        head = getHead(cx, cy);
        if ( head == 0 ) {
            // if the center of the window is not on any head then select
            // the head which center is nearest to the window center
            long dist = -1;
            int i;
            for (i = 0; i < m_xinerama_num_heads; ++i) {
                XineramaHeadInfo& hi = m_xinerama_headinfo[i];
                int d = calcSquareDistance(cx, cy,
                    hi.x() + (hi.width() / 2), hi.y() + (hi.height() / 2));
                if (dist == -1 || d < dist) { // found a closer head
                    head = i + 1;
                    dist = d;
                }
            }
        }
    }
#endif

    return head;
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
    return m_xinerama_headinfo[head-1].x();
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadY(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return 0;
    return m_xinerama_headinfo[head-1].y();
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadWidth(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return width();
    return m_xinerama_headinfo[head-1].width();
#else
    return width();
#endif // XINERAMA
}

int BScreen::getHeadHeight(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return height();
    return m_xinerama_headinfo[head-1].height();
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

    x = FbTk::Util::clamp(x, hx, hx + hw - w);
    y = FbTk::Util::clamp(y, hy, hy + hh - h);

    return make_pair(x,y);
}
