// fluxbox.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// blackbox.hh for Blackbox - an X11 Window manager
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

// $Id: fluxbox.hh,v 1.74 2003/10/05 06:28:47 rathnor Exp $

#ifndef	 FLUXBOX_HH
#define	 FLUXBOX_HH

#include "App.hh"
#include "Resource.hh"
#include "Timer.hh"
#include "Observer.hh"
#include "SignalHandler.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AtomHandler;
class FluxboxWindow;
class WinClient;
class Keys;
class BScreen;
class FbAtoms;


///	main class for the window manager.
/**
	singleton type
*/
class Fluxbox : public FbTk::App,
                public FbTk::SignalEventHandler,
                public FbTk::Observer {
public:
    Fluxbox(int argc, char **argv, const char * dpy_name= 0, 
            const char *rcfilename = 0);
    virtual ~Fluxbox();
	
    static Fluxbox *instance() { return s_singleton; }
    /// main event loop
    void eventLoop();
    bool validateWindow(Window win) const;
    void grab();
    void ungrab();

    inline Atom getFluxboxPidAtom() const { return m_fluxbox_pid; }

    // Not currently implemented until we decide how it'll be used
    //WinClient *searchGroup(Window);
    WinClient *searchWindow(Window);
    inline WinClient *getFocusedWindow() { return m_focused_window; }

		
    BScreen *searchScreen(Window w);

    inline const Time &getDoubleClickInterval() const { return resource.double_click_interval; }
    inline long getUpdateDelayTime() const { return resource.update_delay_time; }
    inline const Time &getLastTime() const { return m_last_time; }

    void addAtomHandler(AtomHandler *atomh);
    void removeAtomHandler(AtomHandler *atomh);

    /// obsolete
    enum Titlebar{SHADE=0, MINIMIZE, MAXIMIZE, CLOSE, STICK, MENU, EMPTY};		

    enum FocusModel { SLOPPYFOCUS=0, SEMISLOPPYFOCUS, CLICKTOFOCUS };

    inline const Bool getIgnoreBorder() const { return *m_rc_ignoreborder; }

    inline const std::vector<Fluxbox::Titlebar>& getTitlebarRight() const { return *m_rc_titlebar_right; }
    inline const std::vector<Fluxbox::Titlebar>& getTitlebarLeft() const { return *m_rc_titlebar_left; }
    inline const std::string &getStyleFilename() const { return *m_rc_stylefile; }

    inline const std::string &getMenuFilename() const { return *m_rc_menufile; }
    inline const std::string &getSlitlistFilename() const { return *m_rc_slitlistfile; }
    inline int colorsPerChannel() const { return *m_rc_colors_per_channel; }
    inline int getNumberOfLayers() const { return *m_rc_numlayers; }

    // class to store layer numbers (special Resource type)
    // we have a special resource type because we need to be able to name certain layers
    // a Resource<int> wouldn't allow this
    class Layer {
    public:
        explicit Layer(int i) : m_num(i) {};
        inline int getNum() const { return m_num; }

        Layer &operator=(int num) { m_num = num; return *this; }
        
    private:
        int m_num;
    };

    // TODO these probably should be configurable
    inline int getMenuLayer()      const { return 0; }
    inline int getAboveDockLayer() const { return 2; }
    inline int getDockLayer()      const { return 4; }
    inline int getTopLayer()       const { return 6; }
    inline int getNormalLayer()    const { return 8; }
    inline int getBottomLayer()    const { return 10; }
    inline int getDesktopLayer()   const { return 12; }


    inline time_t getAutoRaiseDelay() const { return *m_rc_auto_raise_delay; }

    inline unsigned int getCacheLife() const { return *m_rc_cache_life * 60000; }
    inline unsigned int getCacheMax() const { return *m_rc_cache_max; }

    void watchKeyRelease(BScreen &screen, unsigned int mods);

    void setFocusedWindow(WinClient *w);
    // focus revert gets delayed until the end of the event handle
    void revertFocus(BScreen &screen, bool wait_for_end = true);
    void shutdown();
    void load_rc(BScreen &scr);
    void loadRootCommand(BScreen &scr);
    void loadTitlebar();
    void saveStyleFilename(const char *val) { m_rc_stylefile = (val == 0 ? "" : val); }
    void saveMenuFilename(const char *);
    void clearMenuFilenames();
    void saveTitlebarFilename(const char *);
    void saveSlitlistFilename(const char *val) { m_rc_slitlistfile = (val == 0 ? "" : val); }
    void saveWindowSearch(Window win, WinClient *winclient);
    // some windows relate to the group, not the client, so we record separately
    // searchWindow on these windows will give the active client in the group
    void saveWindowSearchGroup(Window win, FluxboxWindow *fbwin);
    void saveGroupSearch(Window win, WinClient *winclient);
    void save_rc();
    void removeWindowSearch(Window win);
    void removeWindowSearchGroup(Window win);
    void removeGroupSearch(Window win);
    void restart(const char *command = 0);
    void reconfigure();
    void rereadMenu();
    /// reloads the menus if the timestamps changed
    void checkMenu();
	
    /// handle any system signal sent to the application
    void handleSignal(int signum);
    void update(FbTk::Subject *changed);

    void attachSignals(FluxboxWindow &win);
    void attachSignals(WinClient &winclient);
	
    void timed_reconfigure();

    bool isStartup() const { return m_starting; }
    enum { B_AMERICANDATE = 1, B_EUROPEANDATE };
	
    typedef std::vector<Fluxbox::Titlebar> TitlebarList;
    /// @return whether the timestamps on the menu changed
    bool menuTimestampsChanged() const;
    bool haveShape() const { return m_have_shape; }
    int shapeEventbase() const { return m_shape_eventbase; }
    void getDefaultDataFilename(char *, std::string &);
    // screen mouse was in at last key event
    BScreen *mouseScreen() { return m_mousescreen; }
    // screen of window that last key event (i.e. focused window) went to
    BScreen *keyScreen() { return m_keyscreen; }
    // screen we are watching for modifier changes
    BScreen *watchingScreen() { return m_watching_screen; }
    const XEvent &lastEvent() const { return m_last_event; }

    /**
     * Allows people to create special event exclusions/redirects
     * useful for getting around X followup events, or for
     * effectively grabbing things
     * The ignore is automatically removed when it finds the stop_win
     * with an event matching the stop_type
     * ignore None means all windows
     */
    void addRedirectEvent(BScreen *screen, long catch_type, Window catch_win, 
                          long stop_type, Window stop_win, Window redirect_win);

    // So that an object may remove the ignore on its own
    void removeRedirectEvent(long stop_type, Window stop_win);

private:

    typedef struct MenuTimestamp {
        std::string filename;
        time_t timestamp;
    } MenuTimestamp;

    struct resource {
        Time double_click_interval;		
        long update_delay_time;
    } resource;
		

    std::string getRcFilename();
    void load_rc();
	
    void reload_rc();
    void real_rereadMenu();
    void real_reconfigure();

    void handleEvent(XEvent *xe);
	
    void setupConfigFiles();
    void handleButtonEvent(XButtonEvent &be);
    void handleUnmapNotify(XUnmapEvent &ue);
    void handleClientMessage(XClientMessageEvent &ce);
    void handleKeyEvent(XKeyEvent &ke);	
    void setTitlebar(std::vector<Fluxbox::Titlebar>& dir, const char *arg);
    
    std::auto_ptr<FbAtoms> m_fbatoms;

    FbTk::ResourceManager m_resourcemanager, &m_screen_rm;
	
    //--- Resources
    FbTk::Resource<bool> m_rc_tabs, m_rc_ignoreborder;
    FbTk::Resource<int> m_rc_colors_per_channel, m_rc_numlayers;
    FbTk::Resource<std::string> m_rc_stylefile, 
        m_rc_menufile, m_rc_keyfile, m_rc_slitlistfile,
        m_rc_groupfile;

	
    FbTk::Resource<TitlebarList> m_rc_titlebar_left, m_rc_titlebar_right;
    FbTk::Resource<unsigned int> m_rc_cache_life, m_rc_cache_max;
    FbTk::Resource<time_t> m_rc_auto_raise_delay;

	
    std::map<Window, WinClient *> m_window_search;
    std::map<Window, FluxboxWindow *> m_window_search_group;
    // A window is the group leader, which can map to several
    // WinClients in the group, it is *not* fluxbox's concept of groups
    // See ICCCM section 4.1.11
    // The group leader (which may not be mapped, so may not have a WinClient) 
    // will have it's window being the group index
    std::multimap<Window, WinClient *> m_group_search;
	
    std::list<MenuTimestamp *> m_menu_timestamps;
    typedef std::list<BScreen *> ScreenList;
    ScreenList m_screen_list;

    WinClient *m_focused_window;
    FbTk::Timer m_timer;

    typedef struct RedirectEvent {
        BScreen *screen;
        long catch_type;
        Window catch_win;
        long stop_type;
        Window stop_win;
        Window redirect_win;
    } RedirectEvent;

    typedef std::list<RedirectEvent *> RedirectEvents;

    RedirectEvents m_redirect_events;
    BScreen *m_mousescreen, *m_keyscreen;
    BScreen *m_watching_screen;
    unsigned int m_watch_keyrelease;

    Atom m_fluxbox_pid;

    bool m_reconfigure_wait, m_reread_menu_wait;
    Time m_last_time;
    std::string m_rc_file; ///< resource filename
    char **m_argv;
    int m_argc;
    XEvent m_last_event;

    std::auto_ptr<Keys> m_key;

    //default arguments for titlebar left and right
    static Fluxbox::Titlebar s_titlebar_left[], s_titlebar_right[];
    static Fluxbox *s_singleton;
    std::vector<AtomHandler *> m_atomhandler;
    bool m_starting;
    bool m_shutdown;
    int m_server_grabs;
    int m_randr_event_type; ///< the type number of randr event
    int m_shape_eventbase; ///< event base for shape events
    bool m_have_shape; ///< if shape is supported by server
    const char *m_RC_PATH;
    const char *m_RC_INIT_FILE;
    Atom m_kwm1_dockwindow, m_kwm2_dockwindow;

    // each event can only affect one screen (right?)
    BScreen *m_focus_revert_screen;
};


#endif // FLUXBOX_HH
