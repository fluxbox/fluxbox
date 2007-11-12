// fluxbox.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// blackbox.cc for blackbox - an X11 Window manager
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

#include "fluxbox.hh"

#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "AtomHandler.hh"
#include "FbCommands.hh"
#include "WinClient.hh"
#include "Keys.hh"
#include "FbAtoms.hh"
#include "FocusControl.hh"
#include "Layer.hh"

#include "defaults.hh"

#include "FbTk/I18n.hh"
#include "FbTk/Image.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Resource.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/XrmDatabaseHelper.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/CompareEqual.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/Compose.hh"
#include "FbTk/KeyUtil.hh"

//Use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#endif // SLIT
#ifdef USE_GNOME
#include "Gnome.hh"
#endif // USE_GNOME
#ifdef USE_NEWWMSPEC
#include "Ewmh.hh"
#endif // USE_NEWWMSPEC
#ifdef REMEMBER
#include "Remember.hh"
#endif // REMEMBER
#ifdef USE_TOOLBAR
#include "Toolbar.hh"
#else
class Toolbar { };
#endif // USE_TOOLBAR

// X headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

// X extensions
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE
#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_RANDR

// system headers

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <sys/wait.h>

#include <iostream>
#include <memory>
#include <algorithm>
#include <typeinfo>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::list;
using std::pair;
using std::bind2nd;
using std::mem_fun;
using std::equal_to;

#ifdef DEBUG
using std::hex;
using std::dec;
#endif // DEBUG

using namespace FbTk;

namespace {

Window last_bad_window = None;

// *** NOTE: if you want to debug here the X errors are
//     coming from, you should turn on the XSynchronise call below
int handleXErrors(Display *d, XErrorEvent *e) {
    if (e->error_code == BadWindow)
        last_bad_window = e->resourceid;
#ifdef DEBUG
    else {
        // ignore bad window ones, they happen a lot
        // when windows close themselves
        char errtxt[128];

        XGetErrorText(d, e->error_code, errtxt, 128);
        cerr<<"Fluxbox: X Error: "<<errtxt<<"("<<(int)e->error_code<<") opcodes "<<
            (int)e->request_code<<"/"<<(int)e->minor_code<<" resource 0x"<<hex<<(int)e->resourceid<<dec<<endl;
//        if (e->error_code != 9 && e->error_code != 183)
//            kill(0, 2);
    }
#endif // !DEBUG

    return False;
}

} // end anonymous

//static singleton var
Fluxbox *Fluxbox::s_singleton=0;

Fluxbox::Fluxbox(int argc, char **argv, const char *dpy_name, const char *rcfilename)
    : FbTk::App(dpy_name),
      m_fbatoms(new FbAtoms()),
      m_resourcemanager(rcfilename, true),
      // TODO: shouldn't need a separate one for screen
      m_screen_rm(m_resourcemanager),
      m_rc_ignoreborder(m_resourcemanager, false, "session.ignoreBorder", "Session.IgnoreBorder"),
      m_rc_pseudotrans(m_resourcemanager, false, "session.forcePseudoTransparency", "Session.forcePseudoTransparency"),
      m_rc_colors_per_channel(m_resourcemanager, 4,
                              "session.colorsPerChannel", "Session.ColorsPerChannel"),
      m_rc_double_click_interval(m_resourcemanager, 250, "session.doubleClickInterval", "Session.DoubleClickInterval"),
      m_rc_tabs_padding(m_resourcemanager, 0, "session.tabPadding", "Session.TabPadding"),
      m_rc_stylefile(m_resourcemanager, DEFAULTSTYLE, "session.styleFile", "Session.StyleFile"),
      m_rc_styleoverlayfile(m_resourcemanager, "~/." + realProgramName("fluxbox") + "/overlay", "session.styleOverlay", "Session.StyleOverlay"),
      m_rc_menufile(m_resourcemanager, DEFAULTMENU, "session.menuFile", "Session.MenuFile"),
      m_rc_keyfile(m_resourcemanager, DEFAULTKEYSFILE, "session.keyFile", "Session.KeyFile"),
      m_rc_slitlistfile(m_resourcemanager, "~/." + realProgramName("fluxbox") + "/slitlist", "session.slitlistFile", "Session.SlitlistFile"),
      m_rc_appsfile(m_resourcemanager, "~/." + realProgramName("fluxbox") + "/apps", "session.appsFile", "Session.AppsFile"),
      m_rc_tabs_attach_area(m_resourcemanager, ATTACH_AREA_WINDOW, "session.tabsAttachArea", "Session.TabsAttachArea"),
      m_rc_cache_life(m_resourcemanager, 5, "session.cacheLife", "Session.CacheLife"),
      m_rc_cache_max(m_resourcemanager, 200, "session.cacheMax", "Session.CacheMax"),
      m_rc_auto_raise_delay(m_resourcemanager, 250, "session.autoRaiseDelay", "Session.AutoRaiseDelay"),
      m_masked_window(0),
      m_mousescreen(0),
      m_keyscreen(0),
      m_last_time(0),
      m_masked(0),
      m_rc_file(rcfilename ? rcfilename : ""),
      m_argv(argv), m_argc(argc),
      m_revert_screen(0),
      m_showing_dialog(false),
      m_starting(true),
      m_restarting(false),
      m_shutdown(false),
      m_server_grabs(0),
      m_randr_event_type(0),
      m_RC_PATH(realProgramName("fluxbox")),
      m_RC_INIT_FILE("init") {

    _FB_USES_NLS;
    if (s_singleton != 0)
        throw _FB_CONSOLETEXT(Fluxbox, FatalSingleton, "Fatal! There can only one instance of fluxbox class.", "Error displayed on weird error where an instance of the Fluxbox class already exists!");

    if (display() == 0) {
        throw _FB_CONSOLETEXT(Fluxbox, NoDisplay,
                      "Can not connect to X server.\nMake sure you started X before you start Fluxbox.",
                      "Error message when no X display appears to exist");
    }

    Display *disp = FbTk::App::instance()->display();
    // For KDE dock applets
    // KDE v1.x
    m_kwm1_dockwindow = XInternAtom(disp,
                                    "KWM_DOCKWINDOW", False);
    // KDE v2.x
    m_kwm2_dockwindow = XInternAtom(disp,
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
    // setup X error handler
    XSetErrorHandler((XErrorHandler) handleXErrors);

    //catch system signals
    SignalHandler &sigh = SignalHandler::instance();
    sigh.registerHandler(SIGSEGV, this);
    sigh.registerHandler(SIGFPE, this);
    sigh.registerHandler(SIGPIPE, this); // e.g. output sent to grep
    sigh.registerHandler(SIGTERM, this);
    sigh.registerHandler(SIGINT, this);
    sigh.registerHandler(SIGCHLD, this);
    sigh.registerHandler(SIGHUP, this);
    sigh.registerHandler(SIGUSR1, this);
    sigh.registerHandler(SIGUSR2, this);
    //
    // setup timer
    // This timer is used to we can issue a safe reconfig command.
    // Because when the command is executed we shouldn't do reconfig directly
    // because it could affect ongoing menu stuff so we need to reconfig in
    // the next event "round".
    FbTk::RefCount<FbTk::Command> reconfig_cmd(new FbTk::SimpleCommand<Fluxbox>(*this, &Fluxbox::timed_reconfigure));
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 1;
    m_reconfig_timer.setTimeout(to);
    m_reconfig_timer.setCommand(reconfig_cmd);
    m_reconfig_timer.fireOnce(true);

    // set a timer to revert focus on FocusOut, in case no FocusIn arrives
    FbTk::RefCount<FbTk::Command> revert_cmd(new FbTk::SimpleCommand<Fluxbox>(*this, &Fluxbox::revert_focus));
    m_revert_timer.setCommand(revert_cmd);
    m_revert_timer.setTimeout(to);
    m_revert_timer.fireOnce(true);

    // XSynchronize(disp, True);

    s_singleton = this;
    m_have_shape = false;
    m_shape_eventbase = 0;
#ifdef SHAPE
    int shape_err;
    m_have_shape = XShapeQueryExtension(disp, &m_shape_eventbase, &shape_err);
#endif // SHAPE

#ifdef HAVE_RANDR
    // get randr event type
    int randr_error_base;
    XRRQueryExtension(disp, &m_randr_event_type, &randr_error_base);
#endif // HAVE_RANDR

    load_rc();

    grab();

    setupConfigFiles();

    if (! XSupportsLocale())
        cerr<<_FB_CONSOLETEXT(Fluxbox, WarningLocale, 
                              "Warning: X server does not support locale", 
                              "XSupportsLocale returned false")<<endl;

    if (XSetLocaleModifiers("") == 0)
        cerr<<_FB_CONSOLETEXT(Fluxbox, WarningLocaleModifiers, 
                              "Warning: cannot set locale modifiers", 
                              "XSetLocaleModifiers returned false")<<endl;


#ifdef HAVE_GETPID
    m_fluxbox_pid = XInternAtom(disp, "_BLACKBOX_PID", False);
#endif // HAVE_GETPID


    // Create keybindings handler and load keys file
    // Note: this needs to be done before creating screens
    m_key.reset(new Keys);
    m_key->load(StringUtil::expandFilename(*m_rc_keyfile).c_str());

    vector<int> screens;
    int i;

    // default is "use all screens"
    for (i = 0; i < ScreenCount(disp); i++)
        screens.push_back(i);

    // find out, on what "screens" fluxbox should run
    // FIXME(php-coder): maybe it worths moving this code to main.cc, where command line is parsed?
    for (i = 1; i < m_argc; i++) {
        if (! strcmp(m_argv[i], "-screen")) {
            if ((++i) >= m_argc) {
                cerr << _FB_CONSOLETEXT(main, ScreenRequiresArg, 
                                        "error, -screen requires argument", 
                                        "the -screen option requires a file argument") << endl;
                exit(1);
            }

            // "all" is default
            if (! strcmp(m_argv[i], "all"))
                break;

            vector<string> vals;
            vector<int> scrtmp;
            int scrnr = 0;
            FbTk::StringUtil::stringtok(vals, m_argv[i], ",:");
            for (vector<string>::iterator scrit = vals.begin();
                 scrit != vals.end(); scrit++) {
                scrnr = atoi(scrit->c_str());
                if (scrnr >= 0 && scrnr < ScreenCount(disp))
                    scrtmp.push_back(scrnr);
            }

            if (!vals.empty())
                swap(scrtmp, screens);
        }
    }

    // create screens
    for (size_t s = 0; s < screens.size(); s++) {
        char scrname[128], altscrname[128];
        sprintf(scrname, "session.screen%d", screens[s]);
        sprintf(altscrname, "session.Screen%d", screens[s]);
        BScreen *screen = new BScreen(m_screen_rm.lock(),
                                      scrname, altscrname,
                                      screens[s], ::Layer::NUM_LAYERS);

        // already handled
        if (! screen->isScreenManaged()) {
            delete screen;
            continue;
        }

        // add to our list
        m_screen_list.push_back(screen);
    }

    if (m_screen_list.empty()) {
        throw _FB_CONSOLETEXT(Fluxbox, ErrorNoScreens,
                             "Couldn't find screens to manage.\nMake sure you don't have another window manager running.",
                             "Error message when no unmanaged screens found - usually means another window manager is running");
    }

    m_keyscreen = m_mousescreen = m_screen_list.front();

    // parse apps file after creating screens but before creating windows
#ifdef REMEMBER
        addAtomHandler(new Remember(), "remember"); // for remembering window attribs
#endif // REMEMBER
    // ewmh handler needs to be added after apps file handler, or else some
    // window properties are set incorrectly on new windows
    // this dependency should probably be made more robust
#ifdef USE_NEWWMSPEC
    addAtomHandler(new Ewmh(), "ewmh"); // for Extended window manager atom support
#endif // USE_NEWWMSPEC
#ifdef USE_GNOME
    addAtomHandler(new Gnome(), "gnome"); // for gnome 1 atom support
#endif //USE_GNOME

    // init all "screens"
    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for(; it != it_end; ++it)
        initScreen(*it);

    XAllowEvents(disp, ReplayPointer, CurrentTime);

    // setup theme manager to have our style file ready to be scanned
    FbTk::ThemeManager::instance().load(getStyleFilename(), getStyleOverlayFilename());

    //XSynchronize(disp, False);
    sync(false);

    m_reconfigure_wait = m_reread_menu_wait = false;

    m_resourcemanager.unlock();
    ungrab();

#ifdef DEBUG
    if (m_resourcemanager.lockDepth() != 0)
        cerr<<"--- resource manager lockdepth = "<<m_resourcemanager.lockDepth()<<endl;
#endif //DEBUG
    m_starting = false;
    //
    // For dumping theme items
    // FbTk::ThemeManager::instance().listItems();
    //
    //    m_resourcemanager.dump();

#ifdef USE_TOOLBAR
    // finally, show toolbar
    Toolbars::iterator toolbar_it = m_toolbars.begin();
    Toolbars::iterator toolbar_it_end = m_toolbars.end();
    for (; toolbar_it != toolbar_it_end; ++toolbar_it)
        (*toolbar_it)->updateVisibleState();
#endif // USE_TOOLBAR

}


Fluxbox::~Fluxbox() {

    // destroy toolbars
    while (!m_toolbars.empty()) {
        delete m_toolbars.back();
        m_toolbars.pop_back();
    }

    // destroy atomhandlers
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         it++) {
        delete (*it).first;
    }
    m_atomhandler.clear();

    // this needs to be destroyed before screens; otherwise, menus stored in
    // key commands cause a segfault when the XLayerItem is destroyed
    m_key.reset(0);

    // destroy screens (after others, as they may do screen things)
    while (!m_screen_list.empty()) {
        delete m_screen_list.back();
        m_screen_list.pop_back();
    }


    clearMenuFilenames();
}


void Fluxbox::initScreen(BScreen *screen) {

    Display* disp = display();

    // now we can create menus (which needs this screen to be in screen_list)
    screen->initMenus();

#ifdef HAVE_GETPID
    pid_t bpid = getpid();

    screen->rootWindow().changeProperty(getFluxboxPidAtom(), XA_CARDINAL,
                                        sizeof(pid_t) * 8, PropModeReplace,
                                        (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID

#ifdef HAVE_RANDR
    // setup RANDR for this screens root window
    // we need to determine if we should use old randr select input function or not
#ifdef X_RRScreenChangeSelectInput
    // use old set randr event
    XRRScreenChangeSelectInput(disp, screen->rootWindow().window(), True);
#else
    XRRSelectInput(disp, screen->rootWindow().window(),
                   RRScreenChangeNotifyMask);
#endif // X_RRScreenChangeSelectInput

#endif // HAVE_RANDR


#ifdef USE_TOOLBAR
    m_toolbars.push_back(new Toolbar(*screen,
                                     *screen->layerManager().
                                     getLayer(::Layer::NORMAL)));
#endif // USE_TOOLBAR

    // must do this after toolbar is created
    screen->initWindows();

    // attach screen signals to this
    screen->currentWorkspaceSig().attach(this);
    screen->focusedWindowSig().attach(this);
    screen->workspaceCountSig().attach(this);
    screen->workspaceNamesSig().attach(this);
    screen->workspaceAreaSig().attach(this);
    screen->clientListSig().attach(this);

    // initiate atomhandler for screen specific stuff
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         it++) {
        (*it).first->initForScreen(*screen);
    }

    FocusControl::revertFocus(*screen); // make sure focus style is correct
#ifdef SLIT
    if (screen->slit())
        screen->slit()->show();
#endif // SLIT

}


void Fluxbox::eventLoop() {
    Display *disp = display();
    while (!m_shutdown) {
        if (XPending(disp)) {
            XEvent e;
            XNextEvent(disp, &e);

            if (last_bad_window != None && e.xany.window == last_bad_window &&
                e.type != DestroyNotify) { // we must let the actual destroys through
                if (e.type == FocusOut)
                    m_revert_timer.start();
#ifdef DEBUG
                else
                    cerr<<"Fluxbox::eventLoop(): removing bad window from event queue"<<endl;
#endif // DEBUG
            } else {
                last_bad_window = None;
                handleEvent(&e);
            }
        } else {
            FbTk::Timer::updateTimers(ConnectionNumber(disp)); //handle all timers
        }
    }
}

bool Fluxbox::validateWindow(Window window) const {
    XEvent event;
    if (XCheckTypedWindowEvent(display(), window, DestroyNotify, &event)) {
        XPutBackEvent(display(), &event);
        return false;
    }

    return true;
}

void Fluxbox::grab() {
    if (! m_server_grabs++)
       XGrabServer(display());
}

void Fluxbox::ungrab() {
    if (! --m_server_grabs)
        XUngrabServer(display());

    if (m_server_grabs < 0)
        m_server_grabs = 0;
}

/**
 setup the configutation files in
 home directory
*/
void Fluxbox::setupConfigFiles() {

    bool create_init = false, create_keys = false, create_menu = false;

    string dirname = getenv("HOME") + string("/.") + m_RC_PATH + "/";
    string init_file, keys_file, menu_file, slitlist_file;
    init_file = dirname + m_RC_INIT_FILE;
    keys_file = dirname + "keys";
    menu_file = dirname + "menu";

    struct stat buf;

    // is file/dir already there?
    if (! stat(dirname.c_str(), &buf)) {

        // check if anything with those name exists, if not create new
        if (stat(init_file.c_str(), &buf))
            create_init = true;
        if (stat(keys_file.c_str(), &buf))
            create_keys = true;
        if (stat(menu_file.c_str(), &buf))
            create_menu = true;

    } else {
#ifdef DEBUG
        cerr <<__FILE__<<"("<<__LINE__<<"): Creating dir: " << dirname.c_str() << endl;
#endif // DEBUG
        _FB_USES_NLS;
        // create directory with perm 700
        if (mkdir(dirname.c_str(), 0700)) {
            fprintf(stderr, _FB_CONSOLETEXT(Fluxbox, ErrorCreatingDirectory,
                                    "Can't create %s directory",
                                    "Can't create a directory, one %s for directory name").c_str(),
                    dirname.c_str());
            cerr<<endl;
            return;
        }

        //mark creation of files
        create_init = create_keys = create_menu = true;
    }


    // copy key configuration
    if (create_keys)
        FbTk::FileUtil::copyFile(DEFAULTKEYSFILE, keys_file.c_str());

    // copy menu configuration
    if (create_menu)
        FbTk::FileUtil::copyFile(DEFAULTMENU, menu_file.c_str());

    // copy init file
    if (create_init)
        FbTk::FileUtil::copyFile(DEFAULT_INITFILE, init_file.c_str());

#define CONFIG_VERSION 5
    FbTk::Resource<int> config_version(m_resourcemanager, 0,
            "session.configVersion", "Session.ConfigVersion");
    if (*config_version < CONFIG_VERSION) {
        // configs are out of date, so run fluxbox-update_configs

        string commandargs = realProgramName("fluxbox-update_configs");
        commandargs += " -rc " + init_file;

#ifdef HAVE_GETPID
        // add the fluxbox pid so fbuc can have us reload rc if necessary
        pid_t bpid = getpid();
        char intbuff[64];
        sprintf(intbuff, "%d", bpid);
        commandargs += " -pid ";
        commandargs += intbuff;
#endif // HAVE_GETPID

        FbCommands::ExecuteCmd fbuc(commandargs, 0);
        fbuc.execute();
    }

}

void Fluxbox::handleEvent(XEvent * const e) {
    _FB_USES_NLS;
    m_last_event = *e;

    // it is possible (e.g. during moving) for a window
    // to mask all events to go to it
    if ((m_masked == e->xany.window) && m_masked_window) {
        if (e->type == MotionNotify) {
            m_last_time = e->xmotion.time;
            m_masked_window->motionNotifyEvent(e->xmotion);
            return;
        } else if (e->type == ButtonRelease) {
            e->xbutton.window = m_masked_window->fbWindow().window();
        }

    }

    // update key/mouse screen and last time before we enter other eventhandlers
    if (e->type == KeyPress ||
        e->type == KeyRelease) {
        m_keyscreen = searchScreen(e->xkey.root);
    } else if (e->type == ButtonPress ||
               e->type == ButtonRelease ||
               e->type == MotionNotify ) {
        if (e->type == MotionNotify)
            m_last_time = e->xmotion.time;
        else
            m_last_time = e->xbutton.time;

        m_mousescreen = searchScreen(e->xbutton.root);
    } else if (e->type == EnterNotify ||
               e->type == LeaveNotify) {
        m_last_time = e->xcrossing.time;
        m_mousescreen = searchScreen(e->xcrossing.root);
    } else if (e->type == PropertyNotify) {
        m_last_time = e->xproperty.time;
        // check transparency atoms if it's a root pm

        BScreen *screen = searchScreen(e->xproperty.window);
        if (screen) {
            screen->propertyNotify(e->xproperty.atom);
        }
    }

    // we need to check focus out for menus before
    // we call FbTk eventhandler
    // so we can get FbTk::Menu::focused() before it sets to 0
    if (e->type == FocusOut &&
        e->xfocus.mode != NotifyGrab &&
        e->xfocus.detail != NotifyPointer &&
        e->xfocus.detail != NotifyInferior &&
        FbTk::Menu::focused() != 0 &&
        FbTk::Menu::focused()->window() == e->xfocus.window) {

        // find screen num
        ScreenList::iterator it = m_screen_list.begin();
        ScreenList::iterator it_end = m_screen_list.end();
        for (; it != it_end; ++it) {
            if ( (*it)->screenNumber() ==
                 FbTk::Menu::focused()->fbwindow().screenNumber()) {
                FocusControl::setFocusedWindow(0);
                m_revert_screen = *it;
                m_revert_timer.start();
                break; // found the screen, no more search
            }
        }
    }

    // try FbTk::EventHandler first
    FbTk::EventManager::instance()->handleEvent(*e);

    switch (e->type) {
    case ButtonRelease:
    case ButtonPress:
        break;
    case ConfigureRequest: {

        if (!searchWindow(e->xconfigurerequest.window)) {

            grab();

            if (validateWindow(e->xconfigurerequest.window)) {
                XWindowChanges xwc;

                xwc.x = e->xconfigurerequest.x;
                xwc.y = e->xconfigurerequest.y;
                xwc.width = e->xconfigurerequest.width;
                xwc.height = e->xconfigurerequest.height;
                xwc.border_width = e->xconfigurerequest.border_width;
                xwc.sibling = e->xconfigurerequest.above;
                xwc.stack_mode = e->xconfigurerequest.detail;

                XConfigureWindow(FbTk::App::instance()->display(),
                                 e->xconfigurerequest.window,
                                 e->xconfigurerequest.value_mask, &xwc);
            }

            ungrab();
        } // else already handled in FluxboxWindow::handleEvent

    }
        break;
    case MapRequest: {

#ifdef DEBUG
        cerr<<"MapRequest for 0x"<<hex<<e->xmaprequest.window<<dec<<endl;

#endif // DEBUG

        WinClient *winclient = searchWindow(e->xmaprequest.window);

        if (! winclient) {
            BScreen *screen = 0;
            int screen_num;
            XWindowAttributes attr;
            // find screen
            if (XGetWindowAttributes(display(),
                                     e->xmaprequest.window,
                                     &attr) && attr.screen != 0) {
                screen_num = XScreenNumberOfScreen(attr.screen);

                // find screen
                ScreenList::iterator screen_it = find_if(m_screen_list.begin(),
                                                         m_screen_list.end(),
                                                         FbTk::CompareEqual<BScreen>(&BScreen::screenNumber, screen_num));
                if (screen_it != m_screen_list.end())
                    screen = *screen_it;
            }
            // try with parent if we failed to find screen num
            if (screen == 0)
               screen = searchScreen(e->xmaprequest.parent);

            if (screen == 0) {
                cerr<<"Fluxbox "<<_FB_CONSOLETEXT(Fluxbox, CantMapWindow, "Warning! Could not find screen to map window on!", "")<<endl;
            } else
                screen->createWindow(e->xmaprequest.window);

        } else {
            // we don't handle MapRequest in FluxboxWindow::handleEvent
            if (winclient->fbwindow())
                winclient->fbwindow()->mapRequestEvent(e->xmaprequest);
        }

    }
        break;
    case MapNotify:
        // handled directly in FluxboxWindow::handleEvent
        break;
    case UnmapNotify:
        handleUnmapNotify(e->xunmap);
	break;
    case MappingNotify:
        // Update stored modifier mapping
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): MappingNotify"<<endl;
#endif // DEBUG
        if (e->xmapping.request == MappingKeyboard
            || e->xmapping.request == MappingModifier) {
            XRefreshKeyboardMapping(&e->xmapping);
            FbTk::KeyUtil::instance().init(); // reinitialise the key utils
            // reconfigure keys (if the mapping changes, they don't otherwise update
            m_key->reconfigure(StringUtil::expandFilename(*m_rc_keyfile).c_str());
        }
        break;
    case CreateNotify:
	break;
    case DestroyNotify: {
        WinClient *winclient = searchWindow(e->xdestroywindow.window);
        if (winclient != 0) {
            FluxboxWindow *win = winclient->fbwindow();
            if (win)
                win->destroyNotifyEvent(e->xdestroywindow);

            delete winclient;

            if (win && win->numClients() == 0)
                delete win;
        }

    }
        break;
    case MotionNotify:
        m_last_time = e->xmotion.time;
        break;
    case PropertyNotify: {
        m_last_time = e->xproperty.time;
        WinClient *winclient = searchWindow(e->xproperty.window);
        if (winclient == 0)
            break;
        // most of them are handled in FluxboxWindow::handleEvent
        // but some special cases like ewmh propertys needs to be checked
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++) {
            if ( (*it).first->propertyNotify(*winclient, e->xproperty.atom))
                break;
        }
    } break;
    case EnterNotify: {

        m_last_time = e->xcrossing.time;
        BScreen *screen = 0;

        if (e->xcrossing.mode == NotifyGrab)
            break;

        if ((e->xcrossing.window == e->xcrossing.root) &&
            (screen = searchScreen(e->xcrossing.window))) {
            screen->imageControl().installRootColormap();

        }

    } break;
    case LeaveNotify:
        m_last_time = e->xcrossing.time;
        break;
    case Expose:
        break;
    case KeyRelease:
    case KeyPress:
	break;
    case ColormapNotify: {
        BScreen *screen = searchScreen(e->xcolormap.window);

        if (screen != 0) {
            screen->setRootColormapInstalled((e->xcolormap.state ==
                                              ColormapInstalled) ? true : false);
        }
    } break;
    case FocusIn: {

        // a grab is something of a pseudo-focus event, so we ignore
        // them, here we ignore some window receiving it
        if (e->xfocus.mode == NotifyGrab ||
            e->xfocus.detail == NotifyPointer ||
            e->xfocus.detail == NotifyInferior)
            break;
        WinClient *winclient = searchWindow(e->xfocus.window);
        if (winclient && FocusControl::focusedWindow() != winclient)
            FocusControl::setFocusedWindow(winclient);

    } break;
    case FocusOut:{
        // and here we ignore some window losing the special grab focus
        if (e->xfocus.mode == NotifyGrab ||
            e->xfocus.detail == NotifyPointer ||
            e->xfocus.detail == NotifyInferior)
            break;

        WinClient *winclient = searchWindow(e->xfocus.window);
        if (winclient == 0 && FbTk::Menu::focused() == 0) {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__FUNCTION__<<") Focus out is not a FluxboxWindow !!"<<endl;
#endif // DEBUG
        } else if (winclient && (winclient == FocusControl::focusedWindow() ||
                                 FocusControl::focusedWindow() == 0) &&
                   (winclient->fbwindow() == 0
                    || !winclient->fbwindow()->isMoving())) {
            // we don't unfocus a moving window
            FocusControl::setFocusedWindow(0);
            m_revert_screen = &winclient->screen();
            m_revert_timer.start();
        }
    }
	break;
    case ClientMessage:
        handleClientMessage(e->xclient);
	break;
    default: {

#ifdef HAVE_RANDR
        if (e->type == m_randr_event_type) {
            // update root window size in screen
            BScreen *scr = searchScreen(e->xany.window);
            if (scr != 0)
                scr->updateSize();
        }
#endif // HAVE_RANDR

    }

    }
}

void Fluxbox::handleUnmapNotify(XUnmapEvent &ue) {

    BScreen *screen = searchScreen(ue.event);

    if (ue.event != ue.window && (!screen || !ue.send_event)) {
        return;
    }

    WinClient *winclient = 0;

    if ((winclient = searchWindow(ue.window)) != 0) {

        if (winclient != 0) {
            FluxboxWindow *win = winclient->fbwindow();

            if (!win) {
                delete winclient;
                return;
            }

            // this should delete client and adjust m_focused_window if necessary
            win->unmapNotifyEvent(ue);

            winclient = 0; // it's invalid now when win destroyed the client

            // finally destroy window if empty
            if (win->numClients() == 0) {
                delete win;
                win = 0;
            }
        }

    // according to http://tronche.com/gui/x/icccm/sec-4.html#s-4.1.4
    // a XWithdrawWindow is
    //   1) unmapping the window (which leads to the upper branch
    //   2) sends an synthetic unampevent (which is handled below)
    } else if (screen && ue.send_event) {
        XDeleteProperty(display(), ue.window, FbAtoms::instance()->getWMStateAtom());
        XUngrabButton(display(), AnyButton, AnyModifier, ue.window);
    }

}

/**
 * Handles XClientMessageEvent
 */
void Fluxbox::handleClientMessage(XClientMessageEvent &ce) {

#ifdef DEBUG
    char * atom = 0;
    if (ce.message_type)
        atom = XGetAtomName(FbTk::App::instance()->display(), ce.message_type);

    cerr<<__FILE__<<"("<<__LINE__<<"): ClientMessage. data.l[0]=0x"<<hex<<ce.data.l[0]<<
	"  message_type=0x"<<ce.message_type<<dec<<" = \""<<atom<<"\""<<endl;

    if (ce.message_type && atom) XFree((char *) atom);
#endif // DEBUG


    if (ce.format != 32)
        return;

    if (ce.message_type == m_fbatoms->getWMChangeStateAtom()) {
        WinClient *winclient = searchWindow(ce.window);
        if (! winclient || !winclient->fbwindow() || ! winclient->validateClient())
            return;

        if (ce.data.l[0] == IconicState)
            winclient->fbwindow()->iconify();
        if (ce.data.l[0] == NormalState)
            winclient->fbwindow()->deiconify();
    } else {
        WinClient *winclient = searchWindow(ce.window);
        BScreen *screen = searchScreen(ce.window);
        // note: we dont need screen nor winclient to be non-null,
        // it's up to the atomhandler to check that
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++) {
            (*it).first->checkClientMessage(ce, screen, winclient);
        }

    }
}

/// handle system signals
void Fluxbox::handleSignal(int signum) {
    _FB_USES_NLS;

    static int re_enter = 0;

    switch (signum) {
    case SIGCHLD: // we don't want the child process to kill us
        // more than one process may have terminated
        while (waitpid(-1, 0, WNOHANG | WUNTRACED) > 0);
        break;
    case SIGHUP:
        restart();
        break;
    case SIGUSR1:
        load_rc();
        break;
    case SIGUSR2:
        reconfigure();
        break;
    case SIGSEGV:
        abort();
        break;
    case SIGFPE:
    case SIGINT:
    case SIGPIPE:
    case SIGTERM:
        shutdown();
        break;
    default:
        fprintf(stderr,
                _FB_CONSOLETEXT(BaseDisplay, SignalCaught, "%s:      signal %d caught\n", "signal catch debug message. Include %s for command and %d for signal number").c_str(),
                m_argv[0], signum);

        if (! m_starting && ! re_enter) {
            re_enter = 1;
            cerr<<_FB_CONSOLETEXT(BaseDisplay, ShuttingDown, "Shutting Down\n", "Quitting because of signal, end with newline");
            shutdown();
        }


        cerr<<_FB_CONSOLETEXT(BaseDisplay, Aborting, "Aborting... dumping core\n", "Aboring and dumping core, end with newline");
        abort();
        break;
    }
}


void Fluxbox::update(FbTk::Subject *changedsub) {
    //TODO: fix signaling, this does not look good
    FluxboxWindow *fbwin = 0;
    WinClient *client = 0;

    if (typeid(*changedsub) == typeid(FluxboxWindow::WinSubject)) {
        FluxboxWindow::WinSubject *winsub = dynamic_cast<FluxboxWindow::WinSubject *>(changedsub);
        fbwin = &winsub->win();
    } else if (typeid(*changedsub) == typeid(Focusable::FocusSubject)) {
        Focusable::FocusSubject *winsub = dynamic_cast<Focusable::FocusSubject *>(changedsub);
        fbwin = winsub->win().fbwindow();
        if (typeid(winsub->win()) == typeid(WinClient))
            client = dynamic_cast<WinClient *>(&winsub->win());
    }

    if (fbwin && &fbwin->stateSig() == changedsub) { // state signal
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateState(*fbwin);
        }
        // if window changed to iconic state
        // add to icon list
        if (fbwin->isIconic()) {
            fbwin->screen().addIcon(fbwin);
            Workspace *space = fbwin->screen().getWorkspace(fbwin->workspaceNumber());
            if (space != 0)
                space->removeWindow(fbwin, true);
        }

        if (fbwin->isStuck()) {
            // if we're sticky then reassociate window
            // to all workspaces
            BScreen &scr = fbwin->screen();
            if (scr.currentWorkspaceID() != fbwin->workspaceNumber()) {
                scr.reassociateWindow(fbwin,
                                      scr.currentWorkspaceID(),
                                      true);
            }
        }
    } else if (fbwin && &fbwin->layerSig() == changedsub) { // layer signal
        AtomHandlerContainerIt it= m_atomhandler.begin();
        for (; it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateLayer(*fbwin);
        }
    } else if (fbwin && &fbwin->dieSig() == changedsub) { // window death signal
        AtomHandlerContainerIt it= m_atomhandler.begin();
        for (; it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateFrameClose(*fbwin);
        }

        // make sure each workspace get this
        BScreen &scr = fbwin->screen();
        scr.removeWindow(fbwin);
        if (FocusControl::focusedFbWindow() == fbwin)
            FocusControl::setFocusedFbWindow(0);
    } else if (fbwin && &fbwin->workspaceSig() == changedsub) {  // workspace signal
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateWorkspace(*fbwin);
        }
    } else if (client && &client->dieSig() == changedsub) { // client death
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateClientClose(*client);
        }

        BScreen &screen = client->screen();

        // At this point, we trust that this client is no longer in the
        // client list of its frame (but it still has reference to the frame)
        // We also assume that any remaining active one is the last focused one

        // This is where we revert focus on window close
        // NOWHERE ELSE!!!
        if (FocusControl::focusedWindow() == client) {
            FocusControl::unfocusWindow(*client);
            // make sure nothing else uses this window before focus reverts
            FocusControl::setFocusedWindow(0);
            m_revert_screen = &screen;
            m_revert_timer.start();
        }

        screen.removeClient(*client);
    } else if (typeid(*changedsub) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject *subj = dynamic_cast<BScreen::ScreenSubject *>(changedsub);
        BScreen &screen = subj->screen();
        if ((&(screen.workspaceCountSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkspaceCount(screen);
            }
        } else if ((&(screen.workspaceNamesSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkspaceNames(screen);
            }
        } else if ((&(screen.currentWorkspaceSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateCurrentWorkspace(screen);
            }
        } else if ((&(screen.focusedWindowSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); it++) {
                (*it).first->updateFocusedWindow(screen,
                        (FocusControl::focusedWindow() ?
                         FocusControl::focusedWindow()->window() :
                         0));
            }
        } else if ((&(screen.workspaceAreaSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkarea(screen);
            }
        } else if ((&(screen.clientListSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateClientList(screen);
            }
        }
    }
}

void Fluxbox::attachSignals(FluxboxWindow &win) {
    win.hintSig().attach(this);
    win.stateSig().attach(this);
    win.workspaceSig().attach(this);
    win.layerSig().attach(this);
    win.dieSig().attach(this);
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end(); ++it) {
        (*it).first->setupFrame(win);
    }
}

void Fluxbox::attachSignals(WinClient &winclient) {
    winclient.dieSig().attach(this);

    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end(); ++it) {
        (*it).first->setupClient(winclient);
    }
}

BScreen *Fluxbox::searchScreen(Window window) {

    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for (; it != it_end; ++it) {
        if (*it && (*it)->rootWindow() == window)
            return *it;
    }

    return 0;
}


AtomHandler* Fluxbox::getAtomHandler(const string &name) {
    if ( name != "" ) {
        using namespace FbTk;
        AtomHandlerContainerIt it = find_if(m_atomhandler.begin(),
                                            m_atomhandler.end(),
                                            Compose(bind2nd(equal_to<string>(), name),
                                                    Select2nd<AtomHandlerContainer::value_type>()));
        if (it != m_atomhandler.end())
            return (*it).first;
    }
    return 0;
}
void Fluxbox::addAtomHandler(AtomHandler *atomh, const string &name) {
    m_atomhandler[atomh]= name;;
}

void Fluxbox::removeAtomHandler(AtomHandler *atomh) {
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         ++it) {
        if ((*it).first == atomh) {
            m_atomhandler.erase(it);
            return;
        }
    }
}

WinClient *Fluxbox::searchWindow(Window window) {
    WinClientMap::iterator it = m_window_search.find(window);
    if (it != m_window_search.end())
        return it->second;

    WindowMap::iterator git = m_window_search_group.find(window);
    return git == m_window_search_group.end() ? 0 : &git->second->winClient();
}


/* Not implemented until we know how it'll be used
 * Recall that this refers to ICCCM groups, not fluxbox tabgroups
 * See ICCCM 4.1.11 for details
 */
/*
WinClient *Fluxbox::searchGroup(Window window) {
}
*/

void Fluxbox::saveWindowSearch(Window window, WinClient *data) {
    m_window_search[window] = data;
}

/* some windows relate to the whole group */
void Fluxbox::saveWindowSearchGroup(Window window, FluxboxWindow *data) {
    m_window_search_group[window] = data;
}

void Fluxbox::saveGroupSearch(Window window, WinClient *data) {
    m_group_search.insert(pair<Window, WinClient *>(window, data));
}


void Fluxbox::removeWindowSearch(Window window) {
    m_window_search.erase(window);
}

void Fluxbox::removeWindowSearchGroup(Window window) {
    m_window_search_group.erase(window);
}

void Fluxbox::removeGroupSearch(Window window) {
    m_group_search.erase(window);
}

/// restarts fluxbox
void Fluxbox::restart(const char *prog) {
    shutdown();

    m_restarting = true;

    if (prog) {
        m_restart_argument = prog;
    }
}

/// prepares fluxbox for a shutdown
void Fluxbox::shutdown() {
    if (m_shutdown)
        return;

    m_shutdown = true;

    XSetInputFocus(FbTk::App::instance()->display(), PointerRoot, None, CurrentTime);

    //send shutdown to all screens
    for_each(m_screen_list.begin(),
             m_screen_list.end(), mem_fun(&BScreen::shutdown));

    sync(false);
}

/// saves resources
void Fluxbox::save_rc() {
    _FB_USES_NLS;
    XrmDatabase new_blackboxrc = 0;

    char rc_string[1024];

    string dbfile(getRcFilename());

    if (!dbfile.empty()) {
        m_resourcemanager.save(dbfile.c_str(), dbfile.c_str());
        m_screen_rm.save(dbfile.c_str(), dbfile.c_str());
    } else
        cerr<<_FB_CONSOLETEXT(Fluxbox, BadRCFile, "rc filename is invalid!", "Bad settings file")<<endl;

    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();

    //Save screen resources

    for (; it != it_end; ++it) {
        BScreen *screen = *it;
        int screen_number = screen->screenNumber();

        // these are static, but may not be saved in the users resource file,
        // writing these resources will allow the user to edit them at a later
        // time... but loading the defaults before saving allows us to rewrite the
        // users changes...

        // write out the users workspace names
        sprintf(rc_string, "session.screen%d.workspaceNames: ", screen_number);
        string workspaces_string(rc_string);

        const vector<string> names = screen->getWorkspaceNames();
        for (size_t i=0; i < names.size(); i++) {
            workspaces_string.append(FbTk::FbStringUtil::FbStrToLocale(names[i]));
            workspaces_string.append(",");
        }

        XrmPutLineResource(&new_blackboxrc, workspaces_string.c_str());

    }

    XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.c_str());

    XrmMergeDatabases(new_blackboxrc, &old_blackboxrc); //merge database together
    XrmPutFileDatabase(old_blackboxrc, dbfile.c_str());
    XrmDestroyDatabase(old_blackboxrc);
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ------------ SAVING DONE"<<endl;
#endif // DEBUG
}

/// @return filename of resource file
string Fluxbox::getRcFilename() {

    if (m_rc_file.empty()) { // set default filename
        string defaultfile(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + m_RC_INIT_FILE);
        return defaultfile;
    }

    return m_rc_file;
}

/// Provides default filename of data file
void Fluxbox::getDefaultDataFilename(const char *name, string &filename) const {
    filename = string(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + name);
}

/// loads resources
void Fluxbox::load_rc() {
    _FB_USES_NLS;
    //get resource filename
    string dbfile(getRcFilename());

    if (!dbfile.empty()) {
        if (!m_resourcemanager.load(dbfile.c_str())) {
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "Failed trying to read rc file")<<":"<<dbfile<<endl;
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFileTrying, "Retrying with", "Retrying rc file loading with (the following file)")<<": "<<DEFAULT_INITFILE<<endl;
            if (!m_resourcemanager.load(DEFAULT_INITFILE))
                cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_resourcemanager.load(DEFAULT_INITFILE))
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
    }

    if (m_rc_menufile->empty())
        m_rc_menufile.setDefaultValue();

    FbTk::Transparent::usePseudoTransparent(*m_rc_pseudotrans);

    if (!m_rc_slitlistfile->empty()) {
        *m_rc_slitlistfile = StringUtil::expandFilename(*m_rc_slitlistfile);
    } else {
        string filename;
        getDefaultDataFilename("slitlist", filename);
        m_rc_slitlistfile.setFromString(filename.c_str());
    }

    if (*m_rc_colors_per_channel < 2)
        *m_rc_colors_per_channel = 2;
    else if (*m_rc_colors_per_channel > 6)
        *m_rc_colors_per_channel = 6;

    if (m_rc_stylefile->empty())
        *m_rc_stylefile = DEFAULTSTYLE;
}

void Fluxbox::load_rc(BScreen &screen) {
    //get resource filename
    _FB_USES_NLS;
    string dbfile(getRcFilename());

    XrmDatabaseHelper database;

    database = XrmGetFileDatabase(dbfile.c_str());
    if (database==0)
        database = XrmGetFileDatabase(DEFAULT_INITFILE);

    XrmValue value;
    char *value_type, name_lookup[1024], class_lookup[1024];
    int screen_number = screen.screenNumber();


    screen.removeWorkspaceNames();

    sprintf(name_lookup, "session.screen%d.workspaceNames", screen_number);
    sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {

        string values(value.addr);
        BScreen::WorkspaceNames names;

        StringUtil::removeTrailingWhitespace(values);
        StringUtil::removeFirstWhitespace(values);
        StringUtil::stringtok<BScreen::WorkspaceNames>(names, values, ",");
        BScreen::WorkspaceNames::iterator it;
        for(it = names.begin(); it != names.end(); it++) {
            if (!(*it).empty() && (*it) != "")
            screen.addWorkspaceName((*it).c_str());
        }

    }

    FbTk::Image::removeAllSearchPaths();
    sprintf(name_lookup, "session.screen%d.imageSearchPath", screen_number);
    sprintf(class_lookup, "Session.Screen%d.imageSearchPath", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value) && value.addr) {
        vector<string> paths;
        StringUtil::stringtok(paths, value.addr, ", ");
        for (size_t i = 0; i < paths.size(); ++i)
            FbTk::Image::addSearchPath(paths[i]);
    }

    if (!dbfile.empty()) {
        if (!m_screen_rm.load(dbfile.c_str())) {
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "Failed trying to read rc file")<<":"<<dbfile<<endl;
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFileTrying, "Retrying with", "Retrying rc file loading with (the following file)")<<": "<<DEFAULT_INITFILE<<endl;
            if (!m_screen_rm.load(DEFAULT_INITFILE))
                cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_screen_rm.load(DEFAULT_INITFILE))
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
    }
}

void Fluxbox::reconfigure() {
    load_rc();
    m_reconfigure_wait = true;
    m_reconfig_timer.start();
}


void Fluxbox::real_reconfigure() {

    FbTk::Transparent::usePseudoTransparent(*m_rc_pseudotrans);

    ScreenList::iterator screen_it = m_screen_list.begin();
    ScreenList::iterator screen_it_end = m_screen_list.end();
    for (; screen_it != screen_it_end; ++screen_it)
        load_rc(*(*screen_it));

    // reconfigure all screens
    for_each(m_screen_list.begin(), m_screen_list.end(), mem_fun(&BScreen::reconfigure));

    //reconfigure keys
    m_key->reconfigure(StringUtil::expandFilename(*m_rc_keyfile).c_str());

    // and atomhandlers
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         it++) {
        (*it).first->reconfigure();
    }
}

BScreen *Fluxbox::findScreen(int id) {
    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->screenNumber() == id)
            break;
    }

    if (it == m_screen_list.end())
        return 0;

    return *it;
}

bool Fluxbox::menuTimestampsChanged() const {
    list<MenuTimestamp *>::const_iterator it = m_menu_timestamps.begin();
    list<MenuTimestamp *>::const_iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {

        time_t timestamp = FbTk::FileUtil::getLastStatusChangeTimestamp((*it)->filename.c_str());

        if (timestamp >= 0) {
            if (timestamp != (*it)->timestamp)
                return true;
        } else
            return true;
    }

    // no timestamp changed
    return false;
}

void Fluxbox::hideExtraMenus(BScreen &screen) {

#ifdef USE_TOOLBAR
        // hide toolbar that matches screen
        for (size_t toolbar = 0; toolbar < m_toolbars.size(); ++toolbar) {
            if (&(m_toolbars[toolbar]->screen()) == &screen)
                m_toolbars[toolbar]->menu().hide();
        }

#endif // USE_TOOLBAR

}

void Fluxbox::rereadMenu(bool show_after_reread) {
    m_reread_menu_wait = true;
    m_show_menu_after_reread = show_after_reread;
    m_reconfig_timer.start();
}


void Fluxbox::real_rereadMenu() {

    clearMenuFilenames();

    for_each(m_screen_list.begin(),
             m_screen_list.end(),
             mem_fun(&BScreen::rereadMenu));

    if(m_show_menu_after_reread) {

        FbCommands::ShowRootMenuCmd showcmd;
        showcmd.execute();

        m_show_menu_after_reread = false;
    }
}

void Fluxbox::saveMenuFilename(const char *filename) {
    if (filename == 0)
        return;

    bool found = false;

    list<MenuTimestamp *>::iterator it = m_menu_timestamps.begin();
    list<MenuTimestamp *>::iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {
        if ((*it)->filename == filename) {
            found = true;
            break;
        }
    }

    if (! found) {
        time_t timestamp = FbTk::FileUtil::getLastStatusChangeTimestamp(filename);

        if (timestamp >= 0) {
            MenuTimestamp *ts = new MenuTimestamp;

            ts->filename = filename;
            ts->timestamp = timestamp;

            m_menu_timestamps.push_back(ts);
        }
    }
}

void Fluxbox::clearMenuFilenames() {
    while(!m_menu_timestamps.empty()) {
        delete m_menu_timestamps.back();
        m_menu_timestamps.pop_back();
    }
}

void Fluxbox::timed_reconfigure() {
    if (m_reconfigure_wait)
        real_reconfigure();

    if (m_reread_menu_wait)
        real_rereadMenu();

    m_reconfigure_wait = m_reread_menu_wait = false;
}

void Fluxbox::revert_focus() {
    if (!m_revert_screen || FocusControl::focusedWindow() ||
            FbTk::Menu::focused() || m_showing_dialog)
        return;

    Window win;
    int revert;
    Display *disp = display();

    XGetInputFocus(disp, &win, &revert);

    // we only want to revert focus if it's left dangling, as some other
    // application may have set the focus to an unmanaged window
    if (win == None || win == PointerRoot ||
            win == m_revert_screen->rootWindow().window())
        FocusControl::revertFocus(*m_revert_screen);
}

bool Fluxbox::validateClient(const WinClient *client) const {
    WinClientMap::const_iterator it =
        find_if(m_window_search.begin(),
                m_window_search.end(),
                Compose(bind2nd(equal_to<WinClient *>(), client),
                        Select2nd<WinClientMap::value_type>()));
    return it != m_window_search.end();
}

void Fluxbox::updateFrameExtents(FluxboxWindow &win) {
    AtomHandlerContainerIt it = m_atomhandler.begin();
    AtomHandlerContainerIt it_end = m_atomhandler.end();
    for (; it != it_end; ++it ) {
        (*it).first->updateFrameExtents(win);
    }
}
