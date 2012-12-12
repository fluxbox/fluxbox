// fluxbox.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FLUXBOX_HH
#define FLUXBOX_HH

#include "FbTk/App.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"
#include "FbTk/SignalHandler.hh"
#include "FbTk/Signal.hh"

#include "AttentionNoticeHandler.hh"

#include <X11/Xresource.h>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
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

/// main class for the window manager.
/**
    singleton type
*/
class Fluxbox : public FbTk::App,
                public FbTk::SignalEventHandler,
                private FbTk::SignalTracker {
public:
    Fluxbox(int argc, char **argv,
            const std::string& dpy_name,
            const std::string& rc_path, const std::string& rc_filename,
            bool xsync = false);
    virtual ~Fluxbox();

    static Fluxbox *instance();

    /// main event loop
    void eventLoop();
    bool validateWindow(Window win) const;
    bool validateClient(const WinClient *client) const;

    void grab();
    void ungrab();
    Keys *keys() { return m_key.get(); }
    Atom getFluxboxPidAtom() const { return m_fluxbox_pid; }

    // Not currently implemented until we decide how it'll be used
    //WinClient *searchGroup(Window);
    WinClient *searchWindow(Window);

    void initScreen(BScreen *screen);
    BScreen *searchScreen(Window w);

    unsigned int getDoubleClickInterval() const { return *m_rc_double_click_interval; }
    Time getLastTime() const { return m_last_time; }

    AtomHandler *getAtomHandler(const std::string &name);
    void addAtomHandler(AtomHandler *atomh);
    void removeAtomHandler(AtomHandler *atomh);

    /// obsolete
    enum TabsAttachArea{ATTACH_AREA_WINDOW= 0, ATTACH_AREA_TITLEBAR};


    bool getIgnoreBorder() const { return *m_rc_ignoreborder; }
    bool &getPseudoTrans() { return *m_rc_pseudotrans; }

    Fluxbox::TabsAttachArea getTabsAttachArea() const { return *m_rc_tabs_attach_area; }
    const std::string &getStyleFilename() const { return *m_rc_stylefile; }
    const std::string &getStyleOverlayFilename() const { return *m_rc_styleoverlayfile; }

    const std::string &getMenuFilename() const { return *m_rc_menufile; }
    const std::string &getSlitlistFilename() const { return *m_rc_slitlistfile; }
    const std::string &getAppsFilename() const { return *m_rc_appsfile; }
    const std::string &getKeysFilename() const { return *m_rc_keyfile; }
    int colorsPerChannel() const { return *m_rc_colors_per_channel; }
    int getTabsPadding() const { return *m_rc_tabs_padding; }


    time_t getAutoRaiseDelay() const { return *m_rc_auto_raise_delay; }

    unsigned int getCacheLife() const { return *m_rc_cache_life * 60000; }
    unsigned int getCacheMax() const { return *m_rc_cache_max; }


    void maskWindowEvents(Window w, FluxboxWindow *bw)
        { m_masked = w; m_masked_window = bw; }

    void shutdown();
    void load_rc(BScreen &scr);
    void saveStyleFilename(const char *val) { m_rc_stylefile = (val == 0 ? "" : val); }
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

    /// handle any system signal sent to the application
    void handleSignal(int signum);
    /// todo, remove this. just temporary
    void updateFrameExtents(FluxboxWindow &win);

    void attachSignals(FluxboxWindow &win);
    void attachSignals(WinClient &winclient);

    void timed_reconfigure();
    void revertFocus();
    void setShowingDialog(bool value) {
        m_showing_dialog = value; if (!value) revertFocus();
    }

    bool isStartup() const { return m_starting; }
    bool isRestarting() const { return m_restarting; }

    const std::string &getRestartArgument() const { return m_restart_argument; }

    /// get screen from number
    BScreen *findScreen(int num);

    typedef std::list<BScreen *> ScreenList;
    const ScreenList screenList() const { return m_screen_list; }

    bool haveShape() const;
    int shapeEventbase() const;
    std::string getDefaultDataFilename(const char *name) const;
    // screen mouse was in at last key event
    BScreen *mouseScreen() { return m_mousescreen; }
    // screen of window that last key event (i.e. focused window) went to
    BScreen *keyScreen() { return m_keyscreen; }
    const XEvent &lastEvent() const { return m_last_event; }

    AttentionNoticeHandler &attentionHandler() { return m_attention_handler; }

private:
    std::string getRcFilename();
    void load_rc();

    void real_reconfigure();

    void handleEvent(XEvent *xe);

    void handleUnmapNotify(XUnmapEvent &ue);
    void handleClientMessage(XClientMessageEvent &ce);

    /// Called when workspace count on a specific screen changed.
    void workspaceCountChanged( BScreen& screen );
    /// Called when workspace was switched
    void workspaceChanged(BScreen& screen);
    /// Called when workspace names changed
    void workspaceNamesChanged(BScreen &screen);
    /// Called when the client list changed.
    void clientListChanged(BScreen &screen);
    /// Called when the focused window changed on a screen
    void focusedWindowChanged(BScreen &screen,
                              FluxboxWindow* win,
                              WinClient* client);

    /// Called when the workspace area changed.
    void workspaceAreaChanged(BScreen &screen);
    /// Called when a window (FluxboxWindow) dies
    void windowDied(Focusable &focusable);
    /// Called when a client (WinClient) dies
    void clientDied(Focusable &focusable);
    /// Called when a window changes workspace
    void windowWorkspaceChanged(FluxboxWindow &win);
    /// Called when a window changes state
    void windowStateChanged(FluxboxWindow &win);
    /// Called when a window layer changes
    void windowLayerChanged(FluxboxWindow &win);

    std::auto_ptr<FbAtoms> m_fbatoms;

    FbTk::ResourceManager m_resourcemanager, &m_screen_rm;

    std::string m_RC_PATH;

    //--- Resources

    FbTk::Resource<bool> m_rc_ignoreborder;
    FbTk::Resource<bool> m_rc_pseudotrans;
    FbTk::Resource<int> m_rc_colors_per_channel,
        m_rc_double_click_interval,
        m_rc_tabs_padding;
    FbTk::Resource<std::string> m_rc_stylefile,
        m_rc_styleoverlayfile,
        m_rc_menufile, m_rc_keyfile, m_rc_slitlistfile,
        m_rc_appsfile;


    FbTk::Resource<TabsAttachArea> m_rc_tabs_attach_area;
    FbTk::Resource<unsigned int> m_rc_cache_life, m_rc_cache_max;
    FbTk::Resource<time_t> m_rc_auto_raise_delay;

    typedef std::map<Window, WinClient *> WinClientMap;
    WinClientMap m_window_search;
    typedef std::map<Window, FluxboxWindow *> WindowMap;
    WindowMap m_window_search_group;
    // A window is the group leader, which can map to several
    // WinClients in the group, it is *not* fluxbox's concept of groups
    // See ICCCM section 4.1.11
    // The group leader (which may not be mapped, so may not have a WinClient)
    // will have it's window being the group index
    std::multimap<Window, WinClient *> m_group_search;

    ScreenList m_screen_list;

    FluxboxWindow *m_masked_window;

    BScreen *m_mousescreen, *m_keyscreen;

    Atom m_fluxbox_pid;

    bool m_reconfigure_wait;
    Time m_last_time;
    Window m_masked;
    std::string m_rc_file; ///< resource filename
    char **m_argv;
    int m_argc;

    std::string m_restart_argument; ///< what to restart

    XEvent m_last_event;

    ///< when we execute reconfig command we must wait until next event round
    FbTk::Timer m_reconfig_timer;
    bool m_showing_dialog;

    std::auto_ptr<Keys> m_key;

    typedef std::set<AtomHandler *> AtomHandlerContainer;
    typedef AtomHandlerContainer::iterator AtomHandlerContainerIt;

    AtomHandlerContainer m_atomhandler;

    bool m_starting;
    bool m_restarting;
    bool m_shutdown;
    int m_server_grabs;

    AttentionNoticeHandler m_attention_handler;
};


#endif // FLUXBOX_HH
