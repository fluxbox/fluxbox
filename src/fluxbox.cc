// fluxbox.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: fluxbox.cc,v 1.164 2003/06/25 13:06:04 fluxgen Exp $

#include "fluxbox.hh"

#include "I18n.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "StringUtil.hh"
#include "Resource.hh"
#include "XrmDatabaseHelper.hh"
#include "AtomHandler.hh"
#include "ImageControl.hh"
#include "EventManager.hh"
#include "FbCommands.hh"
#include "WinClient.hh"
#include "Keys.hh"
#include "FbAtoms.hh"

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
#include "ToolbarHandler.hh"
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

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

#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif // HAVE_LIBGEN_H

#include <sys/wait.h>

#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <typeinfo>

using namespace std;
using namespace FbTk;

#ifndef	 HAVE_BASENAME
namespace {

char *basename(char *s) {
    char *save = s;

    while (*s) {
        if (*s++ == '/')
            save = s;
    }

    return save;
}

}; // end anonymous namespace

#endif // HAVE_BASENAME

//-----------------------------------------------------------------
//---- accessors for int, bool, and some enums with Resource ------
//-----------------------------------------------------------------

template<>
void FbTk::Resource<int>::
setFromString(const char* strval) {
    int val;
    if (sscanf(strval, "%d", &val)==1)
        *this = val;
}

template<>
void FbTk::Resource<std::string>::
setFromString(const char *strval) {
    *this = strval;
}

template<>
void FbTk::Resource<bool>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "true")==0)
        *this = true;
    else
        *this = false;
}

template<>
void FbTk::Resource<Fluxbox::FocusModel>::
setFromString(char const *strval) {
    // auto raise options here for backwards read compatibility
    // they are not supported for saving purposes. Nor does the "AutoRaise" 
    // part actually do anything
    if (strcasecmp(strval, "SloppyFocus") == 0 
        || strcasecmp(strval, "AutoRaiseSloppyFocus") == 0) 
        m_value = Fluxbox::SLOPPYFOCUS;
    else if (strcasecmp(strval, "SemiSloppyFocus") == 0
        || strcasecmp(strval, "AutoRaiseSemiSloppyFocus") == 0) 
        m_value = Fluxbox::SEMISLOPPYFOCUS;
    else if (strcasecmp(strval, "ClickToFocus") == 0) 
        m_value = Fluxbox::CLICKTOFOCUS;
    else
        setDefaultValue();
}

template<>
void FbTk::Resource<Fluxbox::TitlebarList>::
setFromString(char const *strval) {
    vector<std::string> val;
    StringUtil::stringtok(val, strval);
    int size=val.size();
    //clear old values
    m_value.clear();
		
    for (int i=0; i<size; i++) {
        if (strcasecmp(val[i].c_str(), "Maximize")==0)
            m_value.push_back(Fluxbox::MAXIMIZE);
        else if (strcasecmp(val[i].c_str(), "Minimize")==0)
            m_value.push_back(Fluxbox::MINIMIZE);
        else if (strcasecmp(val[i].c_str(), "Shade")==0)
            m_value.push_back(Fluxbox::SHADE);
        else if (strcasecmp(val[i].c_str(), "Stick")==0)
            m_value.push_back(Fluxbox::STICK);
        else if (strcasecmp(val[i].c_str(), "Menu")==0)
            m_value.push_back(Fluxbox::MENU);
        else if (strcasecmp(val[i].c_str(), "Close")==0)
            m_value.push_back(Fluxbox::CLOSE);
    }
}

template<>
void FbTk::Resource<unsigned int>::
setFromString(const char *strval) {	
    if (sscanf(strval, "%ul", &m_value) != 1)
        setDefaultValue();
}

//-----------------------------------------------------------------
//---- manipulators for int, bool, and some enums with Resource ---
//-----------------------------------------------------------------
template<>
std::string FbTk::Resource<bool>::
getString() {				
    return std::string(**this == true ? "true" : "false");
}

template<>
std::string FbTk::Resource<int>::
getString() {
    char strval[256];
    sprintf(strval, "%d", **this);
    return std::string(strval);
}

template<>
std::string FbTk::Resource<std::string>::
getString() { return **this; }

template<>
std::string FbTk::Resource<Fluxbox::FocusModel>::
getString() {
    switch (m_value) {
    case Fluxbox::SLOPPYFOCUS:
        return string("SloppyFocus");
    case Fluxbox::SEMISLOPPYFOCUS:
        return string("SemiSloppyFocus");
    case Fluxbox::CLICKTOFOCUS:
        return string("ClickToFocus");
    }
    // default string
    return string("ClickToFocus");
}

template<>
std::string FbTk::Resource<Fluxbox::TitlebarList>::
getString() {
    string retval;
    int size=m_value.size();
    for (int i=0; i<size; i++) {
        switch (m_value[i]) {
        case Fluxbox::SHADE:
            retval.append("Shade");
            break;
        case Fluxbox::MINIMIZE:
            retval.append("Minimize");
            break;
        case Fluxbox::MAXIMIZE:
            retval.append("Maximize");
            break;
        case Fluxbox::CLOSE:
            retval.append("Close");
            break;
        case Fluxbox::STICK:
            retval.append("Stick");
            break;
        case Fluxbox::MENU:
            retval.append("Menu");
            break;
        default:
            break;
        }
        retval.append(" ");
    }

    return retval;
}

template<>
string FbTk::Resource<unsigned int>::
getString() {
    char tmpstr[128];
    sprintf(tmpstr, "%ul", m_value);
    return string(tmpstr);
}

template<>
void FbTk::Resource<Fluxbox::Layer>::
setFromString(const char *strval) {
    int tempnum = 0;
    if (sscanf(strval, "%d", &tempnum) == 1)
        m_value = tempnum;
    else if (strcasecmp(strval, "Menu") == 0)
        m_value = Fluxbox::instance()->getMenuLayer();
    else if (strcasecmp(strval, "AboveDock") == 0)
        m_value = Fluxbox::instance()->getAboveDockLayer();
    else if (strcasecmp(strval, "Dock") == 0)
        m_value = Fluxbox::instance()->getDockLayer();
    else if (strcasecmp(strval, "Top") == 0)
        m_value = Fluxbox::instance()->getTopLayer();
    else if (strcasecmp(strval, "Normal") == 0)
        m_value = Fluxbox::instance()->getNormalLayer();
    else if (strcasecmp(strval, "Bottom") == 0)
        m_value = Fluxbox::instance()->getBottomLayer();
    else if (strcasecmp(strval, "Desktop") == 0)
        m_value = Fluxbox::instance()->getDesktopLayer();
    else 
        setDefaultValue();
}


template<>
string FbTk::Resource<Fluxbox::Layer>::
getString() {

    if (m_value.getNum() == Fluxbox::instance()->getMenuLayer()) 
        return string("Menu");
    else if (m_value.getNum() == Fluxbox::instance()->getAboveDockLayer()) 
        return string("AboveDock");
    else if (m_value.getNum() == Fluxbox::instance()->getDockLayer()) 
        return string("Dock");
    else if (m_value.getNum() == Fluxbox::instance()->getTopLayer()) 
        return string("Top");
    else if (m_value.getNum() == Fluxbox::instance()->getNormalLayer()) 
        return string("Normal");
    else if (m_value.getNum() == Fluxbox::instance()->getBottomLayer()) 
        return string("Bottom");
    else if (m_value.getNum() == Fluxbox::instance()->getDesktopLayer()) 
        return string("Desktop");
    else {
        char tmpstr[128];
        sprintf(tmpstr, "%d", m_value.getNum());
        return string(tmpstr);
    }
}

static Window last_bad_window = None;


static int handleXErrors(Display *d, XErrorEvent *e) {
#ifdef DEBUG
    /*    
    char errtxt[128];
    
    XGetErrorText(d, e->error_code, errtxt, 128);
    cerr<<"Fluxbox: X Error: "<<errtxt<<"("<<(int)e->error_code<<") opcodes "<<
        (int)e->request_code<<"/"<<(int)e->minor_code<<" resource 0x"<<hex<<(int)e->resourceid<<dec<<endl;
    */
#endif // !DEBUG

    if (e->error_code == BadWindow)
        last_bad_window = e->resourceid;

    return False;
}


//static singleton var
Fluxbox *Fluxbox::s_singleton=0;

//default values for titlebar left and right
//don't forget to change last value in m_rc_titlebar_* if you add more to these
Fluxbox::Titlebar Fluxbox::s_titlebar_left[] = {STICK};
Fluxbox::Titlebar Fluxbox::s_titlebar_right[] = {MINIMIZE, MAXIMIZE, CLOSE};

Fluxbox::Fluxbox(int argc, char **argv, const char *dpy_name, const char *rcfilename)
    : FbTk::App(dpy_name),
      m_fbatoms(new FbAtoms()),
      m_resourcemanager(), m_screen_rm(),
      m_rc_tabs(m_resourcemanager, true, "session.tabs", "Session.Tabs"),
      m_rc_ignoreborder(m_resourcemanager, false, "session.ignoreBorder", "Session.IgnoreBorder"),
      m_rc_colors_per_channel(m_resourcemanager, 4, 
                              "session.colorsPerChannel", "Session.ColorsPerChannel"),
      m_rc_numlayers(m_resourcemanager, 13, "session.numLayers", "Session.NumLayers"),
      m_rc_stylefile(m_resourcemanager, "", "session.styleFile", "Session.StyleFile"),
      m_rc_menufile(m_resourcemanager, DEFAULTMENU, "session.menuFile", "Session.MenuFile"),
      m_rc_keyfile(m_resourcemanager, DEFAULTKEYSFILE, "session.keyFile", "Session.KeyFile"),
      m_rc_slitlistfile(m_resourcemanager, "", "session.slitlistFile", "Session.SlitlistFile"),
      m_rc_groupfile(m_resourcemanager, "", "session.groupFile", "Session.GroupFile"),
      m_rc_titlebar_left(m_resourcemanager, 
                         TitlebarList(&s_titlebar_left[0], &s_titlebar_left[1]), 
                         "session.titlebar.left", "Session.Titlebar.Left"),
      m_rc_titlebar_right(m_resourcemanager, 
                          TitlebarList(&s_titlebar_right[0], &s_titlebar_right[3]), 
                          "session.titlebar.right", "Session.Titlebar.Right"),
      m_rc_cache_life(m_resourcemanager, 5, "session.cacheLife", "Session.CacheLife"),
      m_rc_cache_max(m_resourcemanager, 200, "session.cacheMax", "Session.CacheMax"),
      m_focused_window(0), m_masked_window(0),
      m_timer(this),
      m_watching_screen(0), m_watch_keyrelease(0),
      m_last_time(0),
      m_masked(0),
      m_rc_file(rcfilename ? rcfilename : ""),
      m_argv(argv), m_argc(argc),
      m_starting(true),
      m_shutdown(false),
      m_server_grabs(0),
      m_randr_event_type(0),
      m_RC_PATH("fluxbox"),
      m_RC_INIT_FILE("init") {
      

    if (s_singleton != 0)
        throw string("Fatal! There can only one instance of fluxbox class.");

    if (display() == 0) {
        //!! TODO: NLS
        throw string("Can not connect to X server.\n"
                     "Make sure you started X before you start Fluxbox.");
    }

    // setup X error handler
    XSetErrorHandler((XErrorHandler) handleXErrors);

    //catch system signals
    SignalHandler &sigh = SignalHandler::instance();	
    sigh.registerHandler(SIGSEGV, this);
    sigh.registerHandler(SIGFPE, this);
    sigh.registerHandler(SIGTERM, this);
    sigh.registerHandler(SIGINT, this);
    sigh.registerHandler(SIGCHLD, this);
    sigh.registerHandler(SIGHUP, this);
    sigh.registerHandler(SIGUSR1, this);	
    sigh.registerHandler(SIGUSR2, this);

    Display *disp = FbTk::App::instance()->display();

    s_singleton = this;
    m_have_shape = false;
    m_shape_eventbase = 0;
#ifdef SHAPE
    int shape_err; 
    m_have_shape = XShapeQueryExtension(disp, &m_shape_eventbase, &shape_err);
#endif // SHAPE

#ifdef HAVE_RANDR
    // get randr event type
    int error_base;
    XRRQueryExtension(disp, &m_randr_event_type, &error_base);
#endif // HAVE_RANDR

    // setup atom handlers before we create any windows
#ifdef USE_GNOME
    addAtomHandler(new Gnome()); // for gnome 1 atom support
#endif //USE_GNOME

#ifdef USE_NEWWMSPEC
    addAtomHandler(new Ewmh()); // for Extended window manager atom support
#endif // USE_NEWWMSPEC

#ifdef REMEMBER
    addAtomHandler(new Remember()); // for remembering window attribs
#endif // REMEMBER

    grab();
	
    setupConfigFiles();
	
    if (! XSupportsLocale())
        cerr<<"Warning: X server does not support locale"<<endl;

    if (XSetLocaleModifiers("") == 0)
        cerr<<"Warning: cannot set locale modifiers"<<endl;


    resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec = 0;
	
#ifdef HAVE_GETPID
    m_fluxbox_pid = XInternAtom(disp, "_BLACKBOX_PID", False);
#endif // HAVE_GETPID

    load_rc();
    // Allocate screens
    for (int i = 0; i < ScreenCount(display()); i++) {
        char scrname[128], altscrname[128];
        sprintf(scrname, "session.screen%d", i);
        sprintf(altscrname, "session.Screen%d", i);
        BScreen *screen = new BScreen(m_screen_rm, 
                                      scrname, altscrname,
                                      i, getNumberOfLayers());
        if (! screen->isScreenManaged()) {
            delete screen;			
            continue;
        }

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

        m_screen_list.push_back(screen);
#ifdef USE_TOOLBAR
        m_atomhandler.push_back(new ToolbarHandler(*screen));
#endif // USE_TOOLBAR
        
        // attach screen signals to this
        screen->currentWorkspaceSig().attach(this);
        screen->workspaceCountSig().attach(this);
        screen->workspaceNamesSig().attach(this);
        screen->clientListSig().attach(this);
		
        // initiate atomhandler for screen specific stuff
        for (size_t atomh=0; atomh<m_atomhandler.size(); ++atomh) {
            m_atomhandler[atomh]->initForScreen(*screen);
        }
    }

    if (m_screen_list.size() == 0) {
        //!! TODO: NLS
        throw string("Couldn't find screens to manage.\n"
                     "Make sure you don't have another window manager running.");
    }

    XSynchronize(disp, False);
    XSync(disp, False);

    m_reconfigure_wait = m_reread_menu_wait = false;
	
    m_timer.setTimeout(0);
    m_timer.fireOnce(true);

    // Create keybindings handler and load keys file	
    m_key.reset(new Keys(StringUtil::expandFilename(*m_rc_keyfile).c_str()));

    ungrab();
    m_starting = false;
}


Fluxbox::~Fluxbox() {
    // destroy atomhandlers
    while (!m_atomhandler.empty()) {
        delete m_atomhandler.back();
        m_atomhandler.pop_back();
    }

    clearMenuFilenames();	
}

void Fluxbox::eventLoop() {
    while (!m_shutdown) {
        if (XPending(display())) {
            XEvent e;
            XNextEvent(display(), &e);

            if (last_bad_window != None && e.xany.window == last_bad_window && 
                e.type != DestroyNotify) { // we must let the actual destroys through
#ifdef DEBUG
                cerr<<"Fluxbox::eventLoop(): removing bad window from event queue"<<endl;
#endif // DEBUG
            } else {
                last_bad_window = None;
                handleEvent(&e);
            }
        } else {
            FbTk::Timer::updateTimers(ConnectionNumber(display())); //handle all timers
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

    string dirname = getenv("HOME")+string("/.")+string(m_RC_PATH) + "/";
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

        // create directory with perm 700
        if (mkdir(dirname.c_str(), 0700)) {
            cerr << "Can't create " << dirname << " directory!" << endl;
            return;	
        }
		
        //mark creation of files
        create_init = create_keys = create_menu = true;
    }


    // should we copy key configuraion?
    if (create_keys) {
        ifstream from(DEFAULTKEYSFILE);
        ofstream to(keys_file.c_str());

        if (! to.good()) {
            cerr << "Can't write file" << endl;			
        } else if (from.good()) {
#ifdef DEBUG
            cerr << "Copying file: " << DEFAULTKEYSFILE << endl;
#endif // DEBUG
            to<<from.rdbuf(); //copy file
			
        } else {
            cerr<<"Can't copy default keys file."<<endl;
        }		
    }

    // should we copy menu configuraion?
    if (create_menu) {
        ifstream from(DEFAULTMENU);
        ofstream to(menu_file.c_str());

        if (! to.good()) {
            cerr << "Can't open " << menu_file.c_str() << "for writing" << endl;
        } else if (from.good()) {
#ifdef DEBUG
            cerr << "Copying file: " << DEFAULTMENU << endl;
#endif // DEBUG
            to<<from.rdbuf(); //copy file

        } else {
            cerr<<"Can't copy default menu file."<<endl;
        }		
    }	

    // should we copy default init file?
    if (create_init) {
        ifstream from(DEFAULT_INITFILE);
        ofstream to(init_file.c_str());

        if (! to.good()) {
            cerr << "Can't open " << init_file.c_str() << "for writing" << endl;
        } else if (from.good()) {
#ifdef DEBUG
            cerr << "Copying file: " << DEFAULT_INITFILE << endl;
#endif // DEBUG
            to<<from.rdbuf(); //copy file
        } else {
            cerr<<"Can't copy default init file."<<endl;	
        }
    }
}

void Fluxbox::handleEvent(XEvent * const e) {

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
    // try FbTk::EventHandler first
    FbTk::EventManager::instance()->handleEvent(*e);

    switch (e->type) {
    case ButtonRelease:
    case ButtonPress:
        handleButtonEvent(e->xbutton);
	break;	
    case ConfigureRequest: {
        FluxboxWindow *win = (FluxboxWindow *) 0;

        if ((win = searchWindow(e->xconfigurerequest.window))) {
            // already handled in FluxboxWindow::handleEvent
        } else { 
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
        }

    }
        break;
    case MapRequest: {
#ifdef DEBUG
        cerr<<"MapRequest for 0x"<<hex<<e->xmaprequest.window<<dec<<endl;
#endif // DEBUG

        FluxboxWindow *win = searchWindow(e->xmaprequest.window);

        if (! win) {
            //!!! TODO
            BScreen *scr = searchScreen(e->xmaprequest.parent);
            if (scr != 0)
                win = scr->createWindow(e->xmaprequest.window);
            else
                cerr<<"Fluxbox Warning! Could not find screen to map window on!"<<endl;

        }
        // we don't handle MapRequest in FluxboxWindow::handleEvent
        if (win)
            win->mapRequestEvent(e->xmaprequest);
    }
        break;
    case MapNotify: {
        // handled directly in FluxboxWindow::handleEvent
        FluxboxWindow *win = searchWindow(e->xmap.window);
        if (win)
            win->mapNotifyEvent(e->xmap);
    } break;
    case UnmapNotify:
        handleUnmapNotify(e->xunmap);
	break;	
    case MappingNotify:
        // Update stored modifier mapping
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): MappingNotify"<<endl;
#endif // DEBUG        
        if (m_key.get()) {
            m_key->loadModmap();
        }
        break;
    case CreateNotify:
	break;
    case DestroyNotify: {
        FluxboxWindow *win = searchWindow(e->xdestroywindow.window);        
        if (win != 0) {
            WinClient *client = win->findClient(e->xdestroywindow.window);
            if (client != 0) {
                win->destroyNotifyEvent(e->xdestroywindow);

                delete client;
                
                if (win->numClients() == 0 ||
                    &win->winClient() == client && win->numClients() == 1) {
                    delete win;
                }

            }
        } 

    }
        break;
    case MotionNotify: 
        break;
    case PropertyNotify: {
        m_last_time = e->xproperty.time;
        FluxboxWindow *win = searchWindow(e->xproperty.window);
        if (win == 0)
            break;
        // most of them are handled in FluxboxWindow::handleEvent
        // but some special cases like ewmh propertys needs to be checked 
        for (size_t i=0; i<m_atomhandler.size(); ++i) {
            if (m_atomhandler[i]->propertyNotify(*win, e->xproperty.atom))
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
        // handled directly in FluxboxWindow::exposeEvent
        break;
    case KeyRelease:
    case KeyPress:
        handleKeyEvent(e->xkey);
	break;
    case ColormapNotify: {
        BScreen *screen = searchScreen(e->xcolormap.window);

        if (screen != 0) {
            screen->setRootColormapInstalled((e->xcolormap.state ==
                                              ColormapInstalled) ? true : false);
        }
    } break;
    case FocusIn: {
        if (e->xfocus.mode == NotifyUngrab ||
            e->xfocus.detail == NotifyPointer)
            break;

        FluxboxWindow *win = searchWindow(e->xfocus.window);
        if (win && ! win->isFocused())
            setFocusedWindow(win);
	
    } break;
    case FocusOut:
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

void Fluxbox::handleButtonEvent(XButtonEvent &be) {
		
    switch (be.type) {
    case ButtonPress: {
        m_last_time = be.time;

        BScreen *screen = searchScreen(be.window);
        if (screen == 0)
            break; // end case
#ifdef SLIT
        // hide slit menu
        if (screen->slit())
            screen->slit()->menu().hide();
#endif // SLIT

        if (be.button == 1) {
            if (! screen->isRootColormapInstalled())
                screen->imageControl().installRootColormap();

            if (screen->getWorkspacemenu()->isVisible())
                screen->getWorkspacemenu()->hide();
            if (screen->getRootmenu()->isVisible())
                screen->getRootmenu()->hide();

        } else if (be.button == 2) {
            int mx = be.x_root -
                (screen->getWorkspacemenu()->width() / 2);
            int my = be.y_root -
                (screen->getWorkspacemenu()->titleHeight() / 2);
	
            if (mx < 0) mx = 0;
            if (my < 0) my = 0;

            if (mx + screen->getWorkspacemenu()->width() >
                screen->width()) {
                mx = screen->width() -
                    screen->getWorkspacemenu()->width() -
                    screen->getWorkspacemenu()->fbwindow().borderWidth();
            }

            if (my + screen->getWorkspacemenu()->height() >
                screen->height()) {
                my = screen->height() -
                    screen->getWorkspacemenu()->height() -
                    screen->getWorkspacemenu()->fbwindow().borderWidth();
            }
            screen->getWorkspacemenu()->move(mx, my);

            if (! screen->getWorkspacemenu()->isVisible()) {
                screen->getWorkspacemenu()->removeParent();
                screen->getWorkspacemenu()->show();
            }
        } else if (be.button == 3) { 
            //calculate placement of workspace menu
            //and show/hide it				
            int mx = be.x_root -
                (screen->getRootmenu()->width() / 2);
            int my = be.y_root -
                (screen->getRootmenu()->titleHeight() / 2);

            if (mx < 0) mx = 0;
            if (my < 0) my = 0;

            if (mx + screen->getRootmenu()->width() > screen->width()) {
                mx = screen->width() -
                    screen->getRootmenu()->width() -
                    screen->getRootmenu()->fbwindow().borderWidth();
            }

            if (my + screen->getRootmenu()->height() >
                screen->height()) {
                my = screen->height() -
                    screen->getRootmenu()->height() -
                    screen->getRootmenu()->fbwindow().borderWidth();
            }
            screen->getRootmenu()->move(mx, my);

            if (! screen->getRootmenu()->isVisible()) {
                checkMenu();
                screen->getRootmenu()->show();
            }
        } else if (screen->isDesktopWheeling() && be.button == 4) {
            screen->nextWorkspace(1);
        } else if (screen->isDesktopWheeling() && be.button == 5) {
            screen->prevWorkspace(1);
        }
        
    } break;
    case ButtonRelease:
        break;	
    default:
        break;
    }
}

void Fluxbox::handleUnmapNotify(XUnmapEvent &ue) {

		
    FluxboxWindow *win = 0;
	
    BScreen *screen = searchScreen(ue.event);
	
    if ( ue.event != ue.window && (screen != 0 || !ue.send_event))
        return;

    if ((win = searchWindow(ue.window)) != 0) {
        WinClient *client = win->findClient(ue.window);

        if (client != 0) {

            win->unmapNotifyEvent(ue);
            client = 0; // it's invalid now when win destroyed the client

            if (win == m_focused_window)
                revertFocus(win->screen());

            // finaly destroy window if empty
            if (win->numClients() == 0) {
                delete win;
                win = 0;
            }
        }  
    }

}

/**
 * Handles XClientMessageEvent
 */
void Fluxbox::handleClientMessage(XClientMessageEvent &ce) {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ClientMessage. data.l[0]=0x"<<hex<<ce.data.l[0]<<
	"  message_type=0x"<<ce.message_type<<dec<<endl;
#endif // DEBUG

    if (ce.format != 32)
        return;
	
    if (ce.message_type == m_fbatoms->getWMChangeStateAtom()) {
        FluxboxWindow *win = searchWindow(ce.window);
        if (! win || ! win->validateClient())
            return;

        if (ce.data.l[0] == IconicState)
            win->iconify();
        if (ce.data.l[0] == NormalState)
            win->deiconify();
    } else if (ce.message_type == m_fbatoms->getFluxboxChangeWorkspaceAtom()) {
        BScreen *screen = searchScreen(ce.window);

        if (screen && ce.data.l[0] >= 0 &&
            ce.data.l[0] < (signed)screen->getCount())
            screen->changeWorkspaceID(ce.data.l[0]);
				
    } else if (ce.message_type == m_fbatoms->getFluxboxChangeWindowFocusAtom()) {
        FluxboxWindow *win = searchWindow(ce.window);
        if (win && win->isVisible() && win->setInputFocus())
            win->installColormap(true);
    } else if (ce.message_type == m_fbatoms->getFluxboxCycleWindowFocusAtom()) {
        BScreen *screen = searchScreen(ce.window);

        if (screen) {
            if (! ce.data.l[0])
                screen->prevFocus();
            else
                screen->nextFocus();
        }		
    } else if (ce.message_type == m_fbatoms->getFluxboxChangeAttributesAtom()) {
		
        FluxboxWindow *win = searchWindow(ce.window);

        if (win && win->validateClient()) {
            FluxboxWindow::BlackboxHints net;
            net.flags = ce.data.l[0];
            net.attrib = ce.data.l[1];
            net.workspace = ce.data.l[2];
            net.stack = ce.data.l[3];
            net.decoration = static_cast<int>(ce.data.l[4]);
            win->changeBlackboxHints(net);
        }
    } else {
        FluxboxWindow *win = searchWindow(ce.window);
        BScreen *screen = searchScreen(ce.window);
		
        for (size_t i=0; i<m_atomhandler.size(); ++i) {
            m_atomhandler[i]->checkClientMessage(ce, screen, win);
        }
    }
}

/**
 Handles KeyRelease and KeyPress events
*/
void Fluxbox::handleKeyEvent(XKeyEvent &ke) {
    switch (ke.type) {
    case KeyPress: {
        BScreen *keyscreen = searchScreen(ke.window);

        BScreen *mousescreen = keyscreen;
        Window root, ignorew; 
        int ignored;
        if (!XQueryPointer(FbTk::App::instance()->display(),
                           ke.window, &root, &ignorew, &ignored, &ignored, 
                           &ignored, &ignored, (unsigned int *)&ignored))
            // pointer on different screen to ke.window
            mousescreen = searchScreen(root);

        if (keyscreen == 0 || mousescreen == 0)
            break;

#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): KeyEvent"<<endl;
#endif
        //find action
        Keys::KeyAction action = m_key->getAction(&ke);
#ifdef DEBUG
        const char *actionstr = m_key->getActionStr(action);
        if (actionstr)
            cerr<<"KeyAction("<<actionstr<<")"<<endl;				
#endif
        if (action==Keys::LASTKEYGRAB) //if action not found end case
            break;

        // what to allow if moving
        if (m_focused_window && m_focused_window->isMoving()) {
            int allowed = false;
            switch (action) {
            case Keys::WORKSPACE:
            case Keys::SENDTOWORKSPACE:
            case Keys::WORKSPACE1:
            case Keys::WORKSPACE2:
            case Keys::WORKSPACE3:
            case Keys::WORKSPACE4:
            case Keys::WORKSPACE5:
            case Keys::WORKSPACE6:
            case Keys::WORKSPACE7:
            case Keys::WORKSPACE8:
            case Keys::WORKSPACE9:
            case Keys::WORKSPACE10:
            case Keys::WORKSPACE11:
            case Keys::WORKSPACE12:
            case Keys::NEXTWORKSPACE:
            case Keys::PREVWORKSPACE:
            case Keys::LEFTWORKSPACE:
            case Keys::RIGHTWORKSPACE:
                allowed = true;
                break;
            default:
                allowed = false;
            }
            if (!allowed) break;
        }

        switch (action) {					
        case Keys::WORKSPACE:
            // Workspace1 has id 0, hence -1
            mousescreen->changeWorkspaceID(m_key->getParam()-1);
            break;
        case Keys::SENDTOWORKSPACE:
            // Workspace1 has id 0, hence -1
            keyscreen->sendToWorkspace(m_key->getParam()-1);
            break;
            // NOTE!!! The WORKSPACEn commands are not needed anymore
        case Keys::WORKSPACE1:
            mousescreen->changeWorkspaceID(0);
            break;
        case Keys::WORKSPACE2:
            mousescreen->changeWorkspaceID(1);
            break;
        case Keys::WORKSPACE3:
            mousescreen->changeWorkspaceID(2);
            break;
        case Keys::WORKSPACE4:
            mousescreen->changeWorkspaceID(3);
            break;
        case Keys::WORKSPACE5:
            mousescreen->changeWorkspaceID(4);
            break;
        case Keys::WORKSPACE6:
            mousescreen->changeWorkspaceID(5);
            break;
        case Keys::WORKSPACE7:            
            mousescreen->changeWorkspaceID(6);
            break;
        case Keys::WORKSPACE8:
            mousescreen->changeWorkspaceID(7);
            break;
        case Keys::WORKSPACE9:
            mousescreen->changeWorkspaceID(8);
            break;
        case Keys::WORKSPACE10:
            mousescreen->changeWorkspaceID(9);
            break;
        case Keys::WORKSPACE11:
            mousescreen->changeWorkspaceID(10);
            break;
        case Keys::WORKSPACE12:
            mousescreen->changeWorkspaceID(11);
            break;
        case Keys::NEXTWORKSPACE:
            mousescreen->nextWorkspace(m_key->getParam());
            break;
        case Keys::PREVWORKSPACE:
            mousescreen->prevWorkspace(m_key->getParam());
            break;
        case Keys::LEFTWORKSPACE:
            mousescreen->leftWorkspace(m_key->getParam());
            break;
        case Keys::RIGHTWORKSPACE:
            mousescreen->rightWorkspace(m_key->getParam());
            break;
        case Keys::KILLWINDOW: //kill the current window
            if (m_focused_window) {
                XKillClient(FbTk::App::instance()->display(),
                            m_focused_window->clientWindow());
            }
            break;
        case Keys::NEXTGROUP:    //activate next group (params set right in Keys)
        case Keys::NEXTWINDOW: { //activate next window
            unsigned int mods = Keys::cleanMods(ke.state);
            if (mousescreen == 0)
                break;
            if (mods == 0) { // can't stacked cycle unless there is a mod to grab
                mousescreen->nextFocus(m_key->getParam() | BScreen::CYCLELINEAR);
                break;
            }
            if (!m_watching_screen && !(m_key->getParam() & BScreen::CYCLELINEAR)) {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                watchKeyRelease(*mousescreen, mods);
            }
            mousescreen->nextFocus(m_key->getParam());
            break;
        }
        case Keys::PREVGROUP:    //activate prev group (params set right in Keys)
        case Keys::PREVWINDOW:	{//activate prev window
            unsigned int mods = Keys::cleanMods(ke.state);
            if (mousescreen == 0)
                break;
            if (mods == 0) { // can't stacked cycle unless there is a mod to grab
                mousescreen->prevFocus(m_key->getParam() | BScreen::CYCLELINEAR);
                break;
            }
            if (!m_watching_screen && !(m_key->getParam() & BScreen::CYCLELINEAR)) {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                watchKeyRelease(*mousescreen, mods);
            }
            mousescreen->prevFocus(m_key->getParam());
            break;
        }
        case Keys::FOCUSUP:
            if (m_focused_window) 
                keyscreen->dirFocus(*m_focused_window, BScreen::FOCUSUP);
            break;
        case Keys::FOCUSDOWN:
            if (m_focused_window) 
                keyscreen->dirFocus(*m_focused_window, BScreen::FOCUSDOWN);
            break;
        case Keys::FOCUSLEFT:
            if (m_focused_window) 
                keyscreen->dirFocus(*m_focused_window, BScreen::FOCUSLEFT);
            break;
        case Keys::FOCUSRIGHT:
            if (m_focused_window) 
                keyscreen->dirFocus(*m_focused_window, BScreen::FOCUSRIGHT);
            break;
        case Keys::NEXTTAB: 
            if (m_focused_window && m_focused_window->numClients() > 1)
                m_focused_window->nextClient();                        
            break;						
        case Keys::PREVTAB: 
            if (m_focused_window && m_focused_window->numClients() > 1)
                m_focused_window->prevClient();

            break;
        case Keys::FIRSTTAB:
            cerr<<"FIRSTTAB TODO!"<<endl;
            break;
        case Keys::LASTTAB:
            cerr<<"LASTTAB TODO!"<<endl;
            break;
        case Keys::MOVETABPREV:
            cerr<<"MOVETABPREV TODO!"<<endl;
            break;
        case Keys::MOVETABNEXT:
            cerr<<"MOVETABNEXT TODO!"<<endl;
            break;
        case Keys::ATTACHLAST:
            //!! just attach last window to focused window
            if (m_focused_window) {
                Workspace *space = keyscreen->currentWorkspace();
                Workspace::Windows &wins = space->windowList();
                if (wins.size() == 1)
                    break;
                BScreen::FocusedWindows &fwins = keyscreen->getFocusedList();
                BScreen::FocusedWindows::iterator it = fwins.begin();
                for (; it != fwins.end(); ++it) {
                    if ((*it)->fbwindow() != m_focused_window &&
                        (*it)->fbwindow()->workspaceNumber() == 
                        keyscreen->currentWorkspaceID()) {
                        m_focused_window->attachClient(**it);
                        break;
                    }
                }
            }
            break;
        case Keys::DETACHCLIENT:
            if (m_focused_window) {                        
                m_focused_window->detachClient(m_focused_window->winClient());
            }
            break;
        case Keys::EXECUTE: { //execute command on keypress
            FbCommands::ExecuteCmd cmd(m_key->getExecCommand(), mousescreen->screenNumber());
            cmd.execute();			
        } break;
        case Keys::RECONFIGURE: 
            reload_rc();
            break;
        case Keys::RESTART: {
            FbCommands::RestartFluxboxCmd cmd(m_key->getExecCommand());
            cmd.execute();			
        } break;
        case Keys::QUIT:
            shutdown();
            break;
        case Keys::ROOTMENU: { //show root menu
						
            //calculate placement of workspace menu
            //and show/hide it				
            int mx = ke.x_root -
                (mousescreen->getRootmenu()->width() / 2);
            int my = ke.y_root -
                (mousescreen->getRootmenu()->titleHeight() / 2);

            if (mx < 0) mx = 0;
            if (my < 0) my = 0;

            if (mx + mousescreen->getRootmenu()->width() > mousescreen->width()) {
                mx = mousescreen->width() -
                    mousescreen->getRootmenu()->width() -
                    mousescreen->getRootmenu()->fbwindow().borderWidth();
            }

            if (my + mousescreen->getRootmenu()->height() >
                mousescreen->height()) {
                my = mousescreen->height() -
                    mousescreen->getRootmenu()->height() -
                    mousescreen->getRootmenu()->fbwindow().borderWidth();
            }
            mousescreen->getRootmenu()->move(mx, my);

            if (! mousescreen->getRootmenu()->isVisible()) {
                checkMenu();
                mousescreen->getRootmenu()->show();
            }

        } break;
        default: //try to see if its a window action
            doWindowAction(action, m_key->getParam());
        }
            
          
    } break;
    case KeyRelease: {
        // we ignore most key releases unless we need to use
        // a release to stop something (e.g. window cycling).

        // we notify if _all_ of the watched modifiers are released
        if (m_watching_screen && m_watch_keyrelease) {
            // mask the mod of the released key out
            // won't mask anything if it isn't a mod
            ke.state &= ~m_key->keycodeToModmask(ke.keycode);
            
            if ((m_watch_keyrelease & ke.state) == 0) {
                
                m_watching_screen->notifyReleasedKeys(ke);
                XUngrabKeyboard(FbTk::App::instance()->display(), CurrentTime);
                
                // once they are released, we drop the watch
                m_watching_screen = 0;
                m_watch_keyrelease = 0;
            }
        }

        break;
    }	
    default:
        break;
    }
	
	
}
void Fluxbox::doWindowAction(int action, const int param) {
    if (!m_focused_window)
        return;

    switch (action) {
    case Keys::ICONIFY:
        m_focused_window->iconify();
        break;
    case Keys::RAISE:
        m_focused_window->raise();
        break;
    case Keys::LOWER:
        m_focused_window->lower();
        break;
    case Keys::RAISELAYER:
        m_focused_window->raiseLayer();
        break;
    case Keys::LOWERLAYER:
        m_focused_window->lowerLayer();
        break;
    case Keys::TOPLAYER:
        m_focused_window->moveToLayer(getBottomLayer());
        break;
    case Keys::BOTTOMLAYER:
        m_focused_window->moveToLayer(getTopLayer());
        break;
    case Keys::CLOSE:
        m_focused_window->close();
        break;
    case Keys::SHADE:		
        m_focused_window->shade(); // this has to be done in THIS order
        break;
    case Keys::MAXIMIZE:
        m_focused_window->maximize();
        break;
    case Keys::STICK:
        m_focused_window->stick();
        break;								
    case Keys::VERTMAX:
        if (m_focused_window->isResizable())
            m_focused_window->maximizeVertical();
        break;
    case Keys::HORIZMAX:
        if (m_focused_window->isResizable())
            m_focused_window->maximizeHorizontal();
        break;
    case Keys::NUDGERIGHT:	
        m_focused_window->moveResize(
            m_focused_window->x() + param, m_focused_window->y(),
            m_focused_window->width(), m_focused_window->height());
        break;
    case Keys::NUDGELEFT:			
        m_focused_window->moveResize(
            m_focused_window->x() - param, m_focused_window->y(),
            m_focused_window->width(), m_focused_window->height());
        break;
    case Keys::NUDGEUP:
        m_focused_window->moveResize(
            m_focused_window->x(), m_focused_window->y() - param,
            m_focused_window->width(), m_focused_window->height());
        break;
    case Keys::NUDGEDOWN:
        m_focused_window->moveResize(
            m_focused_window->x(), m_focused_window->y() + param,
            m_focused_window->width(), m_focused_window->height());
        break;
        // NOTE !!! BIGNUDGExxxx is not needed, just use 10 as a parameter
    case Keys::BIGNUDGERIGHT:		
        m_focused_window->moveResize(
            m_focused_window->x() + 10, m_focused_window->y(),
            m_focused_window->width(), m_focused_window->height());
        break;
    case Keys::BIGNUDGELEFT:				
        m_focused_window->moveResize(
            m_focused_window->x() - 10, m_focused_window->y(),
            m_focused_window->width(), m_focused_window->height());
        break;
    case Keys::BIGNUDGEUP:								
        m_focused_window->moveResize(
            m_focused_window->x(), m_focused_window->y()-10,
            m_focused_window->width(), m_focused_window->height());
        break;								
    case Keys::BIGNUDGEDOWN:			
        m_focused_window->moveResize(
            m_focused_window->x(), m_focused_window->y()+10,
            m_focused_window->width(), m_focused_window->height());								
        break;												
    case Keys::HORIZINC:
           m_focused_window->moveResize(
                m_focused_window->x(), m_focused_window->y(),
                m_focused_window->width() + 10, m_focused_window->height());

        break;								
    case Keys::VERTINC:
            m_focused_window->moveResize(
                m_focused_window->x(), m_focused_window->y(),
                m_focused_window->width(), m_focused_window->height()+10);
        break;
    case Keys::HORIZDEC:				
        m_focused_window->moveResize(
                m_focused_window->x(), m_focused_window->y(),
                m_focused_window->width() - 10, m_focused_window->height());
        break;								
    case Keys::VERTDEC:
        m_focused_window->moveResize(
                m_focused_window->x(), m_focused_window->y(),
                m_focused_window->width(), m_focused_window->height()-10);

        break;
    case Keys::TOGGLEDECOR:
        m_focused_window->toggleDecoration();
        break;
    case Keys::TOGGLETAB:
        cerr<<"TOGGLETAB TODO!"<<endl;
        break;
    default: //do nothing
        break;							
    }

}

/// handle system signals
void Fluxbox::handleSignal(int signum) {
    I18n *i18n = I18n::instance();
    static int re_enter = 0;

    switch (signum) {
    case SIGCHLD: // we don't want the child process to kill us
        waitpid(-1, 0, WNOHANG | WUNTRACED);
        break;
    case SIGHUP:
        load_rc();
        break;
    case SIGUSR1:
        reload_rc();
        break;
    case SIGUSR2:
        rereadMenu();
        break;
    case SIGSEGV:
        abort();
        break;
    case SIGFPE:
    case SIGINT:
    case SIGTERM:
        shutdown();
        break;
    default:
        fprintf(stderr,
                i18n->getMessage(
                    FBNLS::BaseDisplaySet, FBNLS::BaseDisplaySignalCaught,
                    "%s:	signal %d caught\n"),
                m_argv[0], signum);

        if (! m_starting && ! re_enter) {
            re_enter = 1;
            fprintf(stderr,
                    i18n->getMessage(
                        FBNLS::BaseDisplaySet, FBNLS::BaseDisplayShuttingDown,
                        "shutting down\n"));
            shutdown();
        }

			
        fprintf(stderr,
                i18n->getMessage(
                    FBNLS::BaseDisplaySet, FBNLS::BaseDisplayAborting,
                    "aborting... dumping core\n"));
        abort();
        break;
    }
}


void Fluxbox::update(FbTk::Subject *changedsub) {
    //TODO: fix signaling, this does not look good
    if (typeid(*changedsub) == typeid(FluxboxWindow::WinSubject)) {
        FluxboxWindow::WinSubject *winsub = dynamic_cast<FluxboxWindow::WinSubject *>(changedsub);
        FluxboxWindow &win = winsub->win();
        if ((&(win.hintSig())) == changedsub) { // hint signal
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateHints(win);
            }
        } else if ((&(win.stateSig())) == changedsub) { // state signal
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateState(win);
            }
            // if window changed to iconic state
            // add to icon list
            if (win.isIconic()) {
                Workspace *space = win.screen().getWorkspace(win.workspaceNumber());
                if (space != 0)
                    space->removeWindow(&win);
                win.screen().addIcon(&win);
            }

            if (win.isStuck()) {
                // if we're sticky then reassociate window
                // to all workspaces
                BScreen &scr = win.screen();
                if (scr.currentWorkspaceID() != win.workspaceNumber()) {
                    scr.reassociateWindow(&win, 
                                          scr.currentWorkspaceID(),
                                          true);
                }
            }
        } else if ((&(win.layerSig())) == changedsub) { // layer signal

            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateLayer(win);
            }
        } else if ((&(win.dieSig())) == changedsub) { // window death signal
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateWindowClose(win);
            }
            // make sure each workspace get this 
            BScreen &scr = win.screen();
            scr.removeWindow(&win);
            if (m_focused_window == &win) 
                revertFocus(scr);
            
        } else if ((&(win.workspaceSig())) == changedsub) {  // workspace signal
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateWorkspace(win);
            }            
        } else {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__LINE__<<"): WINDOW uncought signal from "<<&win<<endl;
#endif // DEBUG
        }
		
    } else if (typeid(*changedsub) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject *subj = dynamic_cast<BScreen::ScreenSubject *>(changedsub);
        BScreen &screen = subj->screen();
        if ((&(screen.workspaceCountSig())) == changedsub) {
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateWorkspaceCount(screen);
            }
        } else if ((&(screen.workspaceNamesSig())) == changedsub) {
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateWorkspaceNames(screen);
            }
        } else if ((&(screen.currentWorkspaceSig())) == changedsub) {
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateCurrentWorkspace(screen);
            }
        } else if ((&(screen.clientListSig())) == changedsub) {
            for (size_t i=0; i<m_atomhandler.size(); ++i) {
                if (m_atomhandler[i]->update())
                    m_atomhandler[i]->updateClientList(screen);
            }
        }
    } else if (typeid(*changedsub) == typeid(WinClient::WinClientSubj)) {
        WinClient::WinClientSubj *subj = dynamic_cast<WinClient::WinClientSubj *>(changedsub);
        WinClient &client = subj->winClient();

        BScreen &screen = client.screen();
        screen.updateNetizenWindowDel(client.window());
        screen.removeClient(client);

        removeWindowSearch(client.window());        
        //!! TODO
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<") TODO: signal stuff for client death!!"<<endl;
#endif // DEBUG
    }
}

void Fluxbox::attachSignals(FluxboxWindow &win) {
    win.hintSig().attach(this);
    win.stateSig().attach(this);
    win.workspaceSig().attach(this);
    win.layerSig().attach(this);
    win.winClient().dieSig().attach(this);
    win.dieSig().attach(this);
    for (size_t i=0; i<m_atomhandler.size(); ++i) {
        m_atomhandler[i]->setupWindow(win);
    }
}

BScreen *Fluxbox::searchScreen(Window window) {
    BScreen *screen = 0;
    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();

    for (; it != it_end; ++it) {
        if (*it) {
            if ((*it)->rootWindow() == window) {
                screen = (*it);
                return screen;
            }
        }
    }

    return 0;
}

void Fluxbox::addAtomHandler(AtomHandler *atomh) {
    for (unsigned int handler = 0; handler < m_atomhandler.size(); handler++) {
        if (m_atomhandler[handler] == atomh) 
            return;
    }
    m_atomhandler.push_back(atomh);
}

void Fluxbox::removeAtomHandler(AtomHandler *atomh) {
    std::vector<AtomHandler *>::iterator it = m_atomhandler.begin();        
    for (; it != m_atomhandler.end(); ++it) {
        if (*it == atomh) {
            m_atomhandler.erase(it);
            return;
        }
    }
}

FluxboxWindow *Fluxbox::searchWindow(Window window) {
    std::map<Window, FluxboxWindow *>::iterator it = m_window_search.find(window);
    return it == m_window_search.end() ? 0 : it->second;
}


FluxboxWindow *Fluxbox::searchGroup(Window window, FluxboxWindow *win) {
    std::map<Window, FluxboxWindow *>::iterator it = m_group_search.find(window);
    return it == m_group_search.end() ? 0 : it->second;
}


void Fluxbox::saveWindowSearch(Window window, FluxboxWindow *data) {
    m_window_search[window] = data;
}


void Fluxbox::saveGroupSearch(Window window, FluxboxWindow *data) {
    m_group_search[window] = data;
}


void Fluxbox::removeWindowSearch(Window window) {
    m_window_search.erase(window);
}

void Fluxbox::removeGroupSearch(Window window) {
    m_group_search.erase(window);
}

/// restarts fluxbox
void Fluxbox::restart(const char *prog) {
    shutdown();

    if (prog) {
        execlp(prog, prog, 0);
        perror(prog);
    }

    // fall back in case the above execlp doesn't work
    execvp(m_argv[0], m_argv);
    execvp(basename(m_argv[0]), m_argv);
}

/// prepares fluxbox for a shutdown
void Fluxbox::shutdown() {

    XSetInputFocus(FbTk::App::instance()->display(), PointerRoot, None, CurrentTime);

    //send shutdown to all screens
    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for (; it != it_end; ++it) {
        if(*it)
            (*it)->shutdown();
    }
    m_shutdown = true;
    XSync(FbTk::App::instance()->display(), False);

}

/// saves resources
void Fluxbox::save_rc() {

    XrmDatabase new_blackboxrc = 0;
	
    char rc_string[1024];

    string dbfile(getRcFilename());
	
    if (dbfile.size() != 0) {
        m_resourcemanager.save(dbfile.c_str(), dbfile.c_str());
        m_screen_rm.save(dbfile.c_str(), dbfile.c_str());
    } else
        cerr<<"database filename is invalid!"<<endl;
	

    sprintf(rc_string, "session.doubleClickInterval:	%lu",
            resource.double_click_interval);
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.autoRaiseDelay:	%lu",
            ((resource.auto_raise_delay.tv_sec * 1000) +
             (resource.auto_raise_delay.tv_usec / 1000)));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();

    //Save screen resources

    for (; it != it_end; ++it) {
        BScreen *screen = *it;
        int screen_number = screen->screenNumber();
  
        sprintf(rc_string, "session.screen%d.rowPlacementDirection: %s", screen_number,
                ((screen->getRowPlacementDirection() == BScreen::LEFTRIGHT) ?
                 "LeftToRight" : "RightToLeft"));
        XrmPutLineResource(&new_blackboxrc, rc_string);

        sprintf(rc_string, "session.screen%d.colPlacementDirection: %s", screen_number,
                ((screen->getColPlacementDirection() == BScreen::TOPBOTTOM) ?
                 "TopToBottom" : "BottomToTop"));
        XrmPutLineResource(&new_blackboxrc, rc_string);

        string placement;
		
        switch (screen->getPlacementPolicy()) {
        case BScreen::CASCADEPLACEMENT:
            placement = "CascadePlacement";
            break;

        case BScreen::COLSMARTPLACEMENT:
            placement = "ColSmartPlacement";
            break;

        case BScreen::UNDERMOUSEPLACEMENT:
            placement = "UnderMousePlacement";
            break;

        default:
        case BScreen::ROWSMARTPLACEMENT:
            placement = "RowSmartPlacement";
            break;
        }
		
        sprintf(rc_string, "session.screen%d.windowPlacement: %s", screen_number,
                placement.c_str());
        XrmPutLineResource(&new_blackboxrc, rc_string);

        //		load_rc(screen);
        // these are static, but may not be saved in the users resource file,
        // writing these resources will allow the user to edit them at a later
        // time... but loading the defaults before saving allows us to rewrite the
        // users changes...

#ifdef		HAVE_STRFTIME
        sprintf(rc_string, "session.screen%d.strftimeFormat: %s", screen_number,
                screen->getStrftimeFormat());
        XrmPutLineResource(&new_blackboxrc, rc_string);
#else // !HAVE_STRFTIME
        sprintf(rc_string, "session.screen%d.dateFormat:	%s", screen_number,
                ((screen->getDateFormat() == B_EUROPEANDATE) ?
                 "European" : "American"));
        XrmPutLineResource(&new_blackboxrc, rc_string);

        sprintf(rc_string, "session.screen%d.clockFormat:	%d", screen_number,
                ((screen->isClock24Hour()) ? 24 : 12));
        XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // HAVE_STRFTIME

        // write out the users workspace names
        sprintf(rc_string, "session.screen%d.workspaceNames: ", screen_number);
        string workspaces_string(rc_string);

        for (unsigned int workspace=0; workspace < screen->getCount(); workspace++) {
            if (screen->getWorkspace(workspace)->name().size()!=0)
                workspaces_string.append(screen->getWorkspace(workspace)->name());
            else
                workspaces_string.append("Null");
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
 
    if (m_rc_file.size() == 0) { // set default filename
        string defaultfile(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + m_RC_INIT_FILE);
        return defaultfile;
    }

    return m_rc_file;
}

/// Provides default filename of data file
void Fluxbox::getDefaultDataFilename(char *name, string &filename) {
    filename = string(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + name);
}

/// loads resources
void Fluxbox::load_rc() {
    XrmDatabaseHelper database;
	
    //get resource filename
    string dbfile(getRcFilename());


    if (dbfile.size() != 0) {
        if (!m_resourcemanager.load(dbfile.c_str())) {
            cerr<<"Failed to load database:"<<dbfile<<endl;
            cerr<<"Trying with: "<<DEFAULT_INITFILE<<endl;
            if (!m_resourcemanager.load(DEFAULT_INITFILE))
                cerr<<"Failed to load database: "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_resourcemanager.load(DEFAULT_INITFILE))
            cerr<<"Failed to load database: "<<DEFAULT_INITFILE<<endl;
    }
	
    XrmValue value;
    char *value_type;

    if (m_rc_menufile->size()) {
        *m_rc_menufile = StringUtil::expandFilename(*m_rc_menufile);
        if (!m_rc_menufile->size())
            m_rc_menufile.setDefaultValue();
    } else
        m_rc_menufile.setDefaultValue();
 
    if (m_rc_slitlistfile->size() != 0) {
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

    if (m_rc_stylefile->size() == 0)
        *m_rc_stylefile = DEFAULTSTYLE;
    else // expand tilde
        *m_rc_stylefile = StringUtil::expandFilename(*m_rc_stylefile);

    //load file
    database = XrmGetFileDatabase(dbfile.c_str());
    if (database==0) {
        cerr<<"Fluxbox: Cant open "<<dbfile<<" !"<<endl;
        cerr<<"Using: "<<DEFAULT_INITFILE<<endl;
        database = XrmGetFileDatabase(DEFAULT_INITFILE);
    }

    if (XrmGetResource(*database, "session.doubleClickInterval",
                       "Session.DoubleClickInterval", &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &resource.double_click_interval) != 1)
            resource.double_click_interval = 250;
    } else
        resource.double_click_interval = 250;

    if (XrmGetResource(*database, "session.autoRaiseDelay", "Session.AutoRaiseDelay", 
                       &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &resource.auto_raise_delay.tv_usec) != 1)
            resource.auto_raise_delay.tv_usec = 250;
    } else
        resource.auto_raise_delay.tv_usec = 250;

    resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec / 1000;
    resource.auto_raise_delay.tv_usec -=
        (resource.auto_raise_delay.tv_sec * 1000);
    resource.auto_raise_delay.tv_usec *= 1000;

    // expand tilde
    *m_rc_groupfile = StringUtil::expandFilename(*m_rc_groupfile);

#ifdef DEBUG
    cerr<<__FILE__<<": Loading groups ("<<*m_rc_groupfile<<")"<<endl;
#endif // DEBUG
    if (!Workspace::loadGroups(*m_rc_groupfile)) {
        cerr<<"Failed to load groupfile: "<<*m_rc_groupfile<<endl;
    }
}

void Fluxbox::load_rc(BScreen &screen) {
    //get resource filename
    string dbfile(getRcFilename());
    if (dbfile.size() != 0) {
        if (!m_screen_rm.load(dbfile.c_str())) {
            cerr<<"Failed to load database:"<<dbfile<<endl;
            cerr<<"Trying with: "<<DEFAULT_INITFILE<<endl;
            if (!m_screen_rm.load(DEFAULT_INITFILE))
                cerr<<"Failed to load database: "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_screen_rm.load(DEFAULT_INITFILE))
            cerr<<"Failed to load database: "<<DEFAULT_INITFILE<<endl;
    }
	
    XrmDatabaseHelper database;

    database = XrmGetFileDatabase(dbfile.c_str());
    if (database==0)
        database = XrmGetFileDatabase(DEFAULT_INITFILE);
		
    XrmValue value;
    char *value_type, name_lookup[1024], class_lookup[1024];
    int screen_number = screen.screenNumber();

    sprintf(name_lookup, "session.screen%d.rowPlacementDirection", screen_number);
    sprintf(class_lookup, "Session.Screen%d.RowPlacementDirection", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        if (! strncasecmp(value.addr, "righttoleft", value.size))
            screen.saveRowPlacementDirection(BScreen::RIGHTLEFT);
        else	
            screen.saveRowPlacementDirection(BScreen::LEFTRIGHT);
    } else
        screen.saveRowPlacementDirection(BScreen::LEFTRIGHT);

    sprintf(name_lookup, "session.screen%d.colPlacementDirection", screen_number);
    sprintf(class_lookup, "Session.Screen%d.ColPlacementDirection", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        if (! strncasecmp(value.addr, "bottomtotop", value.size))
            screen.saveColPlacementDirection(BScreen::BOTTOMTOP);
        else
            screen.saveColPlacementDirection(BScreen::TOPBOTTOM);
    } else
        screen.saveColPlacementDirection(BScreen::TOPBOTTOM);

    screen.removeWorkspaceNames();

    sprintf(name_lookup, "session.screen%d.workspaceNames", screen_number);
    sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): Workspaces="<<
            screen.getNumberOfWorkspaces()<<endl;
#endif // DEBUG
        char *search = StringUtil::strdup(value.addr);

        int i;
        for (i = 0; i < screen.getNumberOfWorkspaces(); i++) {
            char *nn;

            if (! i) nn = strtok(search, ",");
            else nn = strtok(0, ",");

            if (nn)
                screen.addWorkspaceName(nn);	
            else break;
			
        }

        delete [] search;
    }

    sprintf(name_lookup, "session.screen%d.windowPlacement", screen_number);
    sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        if (! strncasecmp(value.addr, "RowSmartPlacement", value.size))
            screen.savePlacementPolicy(BScreen::ROWSMARTPLACEMENT);
        else if (! strncasecmp(value.addr, "ColSmartPlacement", value.size))
            screen.savePlacementPolicy(BScreen::COLSMARTPLACEMENT);
        else if (! strncasecmp(value.addr, "UnderMousePlacement", value.size))
            screen.savePlacementPolicy(BScreen::UNDERMOUSEPLACEMENT);
        else
            screen.savePlacementPolicy(BScreen::CASCADEPLACEMENT);
    } else
        screen.savePlacementPolicy(BScreen::ROWSMARTPLACEMENT);
    
#ifdef HAVE_STRFTIME
    sprintf(name_lookup, "session.screen%d.strftimeFormat", screen_number);
    sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value))
        screen.saveStrftimeFormat(value.addr);
    else
        screen.saveStrftimeFormat("%I:%M %p");
#else //	HAVE_STRFTIME

    sprintf(name_lookup, "session.screen%d.dateFormat", screen_number);
    sprintf(class_lookup, "Session.Screen%d.DateFormat", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        if (strncasecmp(value.addr, "european", value.size))
            screen.saveDateFormat(B_AMERICANDATE);
        else
            screen.saveDateFormat(B_EUROPEANDATE);
    } else
        screen.saveDateFormat(B_AMERICANDATE);

    sprintf(name_lookup, "session.screen%d.clockFormat", screen_number);
    sprintf(class_lookup, "Session.Screen%d.ClockFormat", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        int clock;
        if (sscanf(value.addr, "%d", &clock) != 1)
            screen.saveClock24Hour(false);
        else if (clock == 24) 
            screen.saveClock24Hour(true);
        else 
            screen.saveClock24Hour(false);
    } else
        screen.saveClock24Hour(false);
#endif // HAVE_STRFTIME

}

void Fluxbox::loadRootCommand(BScreen &screen)	{
 
    string dbfile(getRcFilename());

    XrmDatabaseHelper database(dbfile.c_str());
    if (!*database) 
        database = XrmGetFileDatabase(DEFAULT_INITFILE);

    XrmValue value;
    char *value_type, name_lookup[1024], class_lookup[1024];
    sprintf(name_lookup, "session.screen%d.rootCommand", screen.screenNumber());
    sprintf(class_lookup, "Session.Screen%d.RootCommand", screen.screenNumber());
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {										 
        screen.saveRootCommand(value.addr==0 ? "": value.addr);
    } else
        screen.saveRootCommand("");		
	
}

void Fluxbox::reload_rc() {
    load_rc();
    reconfigure();
}


void Fluxbox::reconfigure() {
    m_reconfigure_wait = true;

    if (! m_timer.isTiming()) 
        m_timer.start();
}


void Fluxbox::real_reconfigure() {

    XrmDatabase new_blackboxrc = (XrmDatabase) 0;

    string dbfile(getRcFilename());
    XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.c_str());

    XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
    XrmPutFileDatabase(old_blackboxrc, dbfile.c_str());
	
    if (old_blackboxrc)
        XrmDestroyDatabase(old_blackboxrc);


    ScreenList::iterator sit = m_screen_list.begin();
    ScreenList::iterator sit_end = m_screen_list.end();
    for (; sit != sit_end; ++sit)
        (*sit)->reconfigure();

    //reconfigure keys
    m_key->reconfigure(StringUtil::expandFilename(*m_rc_keyfile).c_str());


}


bool Fluxbox::menuTimestampsChanged() const {
    std::list<MenuTimestamp *>::const_iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::const_iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {
        struct stat buf;

        if (! stat((*it)->filename.c_str(), &buf)) {
            if ((*it)->timestamp != buf.st_ctime)
                return true;
        } else
            return true;
    }

    // no timestamp changed
    return false;
}

void Fluxbox::checkMenu() {
    if (menuTimestampsChanged())
        rereadMenu();
}


void Fluxbox::rereadMenu() {
    m_reread_menu_wait = true;

    if (! m_timer.isTiming())
        m_timer.start();
}


void Fluxbox::real_rereadMenu() {
    std::list<MenuTimestamp *>::iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it)
        delete *it;

    m_menu_timestamps.erase(m_menu_timestamps.begin(), m_menu_timestamps.end());

    ScreenList::iterator sit = m_screen_list.begin();
    ScreenList::iterator sit_end = m_screen_list.end();
    for (; sit != sit_end; ++sit) {
        (*sit)->rereadMenu();
    }
}

void Fluxbox::saveMenuFilename(const char *filename) {
    if (filename == 0)
        return;

    bool found = false;

    std::list<MenuTimestamp *>::iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {
        if ((*it)->filename == filename) {
            found = true; 
            break; 
        }
    }

    if (! found) {
        struct stat buf;

        if (! stat(filename, &buf)) {
            MenuTimestamp *ts = new MenuTimestamp;

            ts->filename = filename;
            ts->timestamp = buf.st_ctime;

            m_menu_timestamps.push_back(ts);
        }
    }
}

void Fluxbox::clearMenuFilenames() {
    std::list<MenuTimestamp *>::iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it)
         delete *it;

    m_menu_timestamps.erase(m_menu_timestamps.begin(), m_menu_timestamps.end());
}

void Fluxbox::timeout() {
    if (m_reconfigure_wait)
        real_reconfigure();

    if (m_reread_menu_wait)
        real_rereadMenu();

    m_reconfigure_wait = m_reread_menu_wait = false;
}

// set focused window
void Fluxbox::setFocusedWindow(FluxboxWindow *win) {
    // already focused
    if (m_focused_window == win) {
#ifdef DEBUG
        cerr<<"Focused window already win"<<endl;
#endif // DEBUG
        return;
    }
#ifdef DEBUG
    cerr<<"-----------------"<<endl;
    cerr<<"Setting Focused window = "<<win<<endl;
    cerr<<"Current Focused window = "<<m_focused_window<<endl;
    cerr<<"------------------"<<endl;
#endif // DEBUG    
    BScreen *old_screen = 0, *screen = 0;
    FluxboxWindow *old_win = 0;
    Workspace *old_wkspc = 0, *wkspc = 0;

    if (m_focused_window != 0) {
        // check if m_focused_window is valid
        bool found = false;
        std::map<Window, FluxboxWindow *>::iterator it = m_window_search.begin();
        std::map<Window, FluxboxWindow *>::iterator it_end = m_window_search.end();
        for (; it != it_end; ++it) {
            if (it->second == m_focused_window) {
                // we found it, end loop
                found = true;
                break;
            }
        }

        if (!found) {
            m_focused_window = 0;
        } else {
            old_win = m_focused_window;
            old_screen = &old_win->screen();

            old_wkspc = old_screen->getWorkspace(old_win->workspaceNumber());

            old_win->setFocusFlag(false);
        }
    }

    if (win && ! win->isIconic()) {
        // make sure we have a valid win pointer with a valid screen
        ScreenList::iterator winscreen = 
            std::find(m_screen_list.begin(), m_screen_list.end(),
                      &win->screen());
        if (winscreen == m_screen_list.end()) {
            m_focused_window = 0; // the window pointer wasn't valid, mark no window focused
        } else {
            screen = *winscreen;
            wkspc = screen->getWorkspace(win->workspaceNumber());		
            m_focused_window = win;     // update focused window
            win->setFocusFlag(true); // set focus flag
        }
    } else
        m_focused_window = 0;


    if (screen != 0)
        screen->updateNetizenWindowFocus();

    if (old_screen && old_screen != screen)
        old_screen->updateNetizenWindowFocus();

}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 * last_focused is set to something if we want to make use of the
 * previously focused window (it must NOT be set focused now, it 
 *   is probably dying).
 */
void Fluxbox::revertFocus(BScreen &screen) {
    // Relevant resources:
    // resource.focus_last = whether we focus last focused when changing workspace
    // Fluxbox::FocusModel = sloppy, click, whatever
    WinClient *next_focus = screen.getLastFocusedWindow(screen.currentWorkspaceID());

    // if setting focus fails, or isn't possible, fallback correctly
    if (!(next_focus && next_focus->fbwindow() &&
          next_focus->fbwindow()->setCurrentClient(*next_focus, true))) {
        setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
        switch (screen.getFocusModel()) {
        case SLOPPYFOCUS:
        case SEMISLOPPYFOCUS:
            XSetInputFocus(FbTk::App::instance()->display(), 
                           PointerRoot, None, CurrentTime);
            break;
        case CLICKTOFOCUS:
            XSetInputFocus(FbTk::App::instance()->display(), 
                           screen.rootWindow().window(),
                           RevertToPointerRoot, CurrentTime);
            break;
        }
    }
}



void Fluxbox::watchKeyRelease(BScreen &screen, unsigned int mods) {
    if (mods == 0) {
        cerr<<"WARNING: attempt to grab without modifiers!"<<endl;
        return;
    }
    m_watching_screen = &screen;
    m_watch_keyrelease = mods;
    XGrabKeyboard(FbTk::App::instance()->display(),
                  screen.rootWindow().window(), True, 
                  GrabModeAsync, GrabModeAsync, CurrentTime);
}
