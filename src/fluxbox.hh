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
#include "FbTk/Signal.hh"
#include "FbTk/MenuSearch.hh"

#include "AttentionNoticeHandler.hh"
#include "ShortcutManager.hh"

#include <X11/Xresource.h>

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
#include <cstdio>

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
                private FbTk::SignalTracker {
public:

    typedef std::list<BScreen *> ScreenList;

    enum {
        OPT_TOOLBAR = 1 << 0,
        OPT_SLIT    = 1 << 1
    };

    /// obsolete
    enum TabsAttachArea{ATTACH_AREA_WINDOW= 0, ATTACH_AREA_TITLEBAR};


    static Fluxbox *instance();


    Fluxbox(int argc, char **argv,
            const std::string& dpy_name,
            const std::string& rc_path, const std::string& rc_filename,
            bool xsync = false);
    virtual ~Fluxbox();


    /// main event loop
    void eventLoop();

    void grab();
    void ungrab();
    Keys *keys() { return m_key.get(); }
    Atom getFluxboxPidAtom() const { return m_fluxbox_pid; }


    void initScreen(BScreen *screen);

    WinClient *searchWindow(Window);
    BScreen *searchScreen(Window w);
    bool validateWindow(Window win) const;
    bool validateClient(const WinClient *client) const;

    // Not currently implemented until we decide how it'll be used
    //WinClient *searchGroup(Window);

    Time getLastTime() const { return m_last_time; }

    AtomHandler *getAtomHandler(const std::string &name);
    void addAtomHandler(AtomHandler *atomh);
    void removeAtomHandler(AtomHandler *atomh);


    std::string getDefaultDataFilename(const char *name) const;

    bool &getPseudoTrans()                             { return *m_config.pseudotrans; }
    bool getIgnoreBorder() const                       { return *m_config.ignore_border; }
    Fluxbox::TabsAttachArea getTabsAttachArea() const  { return *m_config.tabs_attach_area; }
    const std::string &getStyleFilename() const        { return *m_config.style_file; }
    const std::string &getStyleOverlayFilename() const { return *m_config.overlay_file; }
    const std::string &getMenuFilename() const         { return *m_config.menu_file; }
    const std::string &getSlitlistFilename() const     { return *m_config.slit_file; }
    const std::string &getAppsFilename() const         { return *m_config.apps_file; }
    const std::string &getKeysFilename() const         { return *m_config.key_file; }
    int colorsPerChannel() const                       { return *m_config.colors_per_channel; }
    int getTabsPadding() const                         { return *m_config.tabs_padding; }
    unsigned int getDoubleClickInterval() const        { return *m_config.double_click_interval; }
    time_t getAutoRaiseDelay() const                   { return *m_config.auto_raise_delay; }
    unsigned int getCacheLife() const                  { return *m_config.cache_life * 60000; }
    unsigned int getCacheMax() const                   { return *m_config.cache_max; }


    void maskWindowEvents(Window w, FluxboxWindow *bw)
        { m_masked = w; m_masked_window = bw; }

    void shutdown(int x_wants_down = 0);
    void load_rc(BScreen &scr);
    void saveStyleFilename(const char *val) { m_config.style_file = (val == 0 ? "" : val); }
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
    void reconfigThemes();

    /// todo, remove this. just temporary
    void updateFrameExtents(FluxboxWindow &win);

    void attachSignals(FluxboxWindow &win);
    void attachSignals(WinClient &winclient);

    void timed_reconfigure();
    void revertFocus();
    void setShowingDialog(bool value) {
        m_showing_dialog = value; if (!value) revertFocus();
    }

    bool isStartup() const       { return m_state.starting; }
    bool isRestarting() const    { return m_state.restarting; }
    bool isShuttingDown() const  { return m_state.shutdown; }

    const std::string &getRestartArgument() const { return m_restart_argument; }

    /// get screen from number
    BScreen *findScreen(int num);

    const ScreenList screenList() const { return m_screens; }

    bool haveShape() const;
    int shapeEventbase() const;


    BScreen *mouseScreen() { return m_active_screen.mouse; }
    BScreen *keyScreen() { return m_active_screen.key; }
    const XEvent &lastEvent() const { return m_last_event; }

    AttentionNoticeHandler &attentionHandler() { return m_attention_handler; }
    ShortcutManager &shortcutManager() { return *m_shortcut_manager; }

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


    typedef std::map<Window, WinClient *> WinClientMap;
    typedef std::map<Window, FluxboxWindow *> WindowMap;
    typedef std::set<AtomHandler *> AtomHandlerContainer;
    typedef AtomHandlerContainer::iterator AtomHandlerContainerIt;


    //--- Resources

    std::unique_ptr<FbAtoms> m_fbatoms;
    FbTk::ResourceManager  m_resourcemanager;
    FbTk::ResourceManager& m_screen_rm;

    struct Config {
        Config(FbTk::ResourceManager& rm, const std::string& path);

        std::string path;
        std::string file;

        FbTk::Resource<bool> ignore_border;
        FbTk::Resource<bool> pseudotrans;
        FbTk::Resource<int>  colors_per_channel;
        FbTk::Resource<int>  double_click_interval;
        FbTk::Resource<int>  tabs_padding;

        FbTk::Resource<std::string> style_file;
        FbTk::Resource<std::string> overlay_file;
        FbTk::Resource<std::string> menu_file;
        FbTk::Resource<std::string> key_file;
        FbTk::Resource<std::string> slit_file;
        FbTk::Resource<std::string> apps_file;

        FbTk::Resource<TabsAttachArea> tabs_attach_area;
        FbTk::Resource<FbTk::MenuSearch::Mode> menusearch;
        FbTk::Resource<unsigned int>   cache_life;
        FbTk::Resource<unsigned int>   cache_max;
        FbTk::Resource<time_t>         auto_raise_delay;
    } m_config;


    std::unique_ptr<Keys>    m_key;
    AtomHandlerContainer   m_atomhandler;
    AttentionNoticeHandler m_attention_handler;

    ScreenList             m_screens;
    WinClientMap           m_window_search;
    WindowMap              m_window_search_group;

    // A window is the group leader, which can map to several
    // WinClients in the group, it is *not* fluxbox's concept of groups
    // See ICCCM section 4.1.11
    // The group leader (which may not be mapped, so may not have a WinClient)
    // will have it's window being the group index
    std::multimap<Window, WinClient *> m_group_search;


    Time    m_last_time;
    XEvent  m_last_event;

    Window  m_masked;
    FluxboxWindow *m_masked_window;

    struct {
        BScreen* mouse;
        BScreen* key;
    } m_active_screen;

    Atom m_fluxbox_pid;

    bool m_reconfigure_wait;
    char **m_argv;
    int m_argc;
    std::string m_restart_argument; ///< what to restart

    ///< when we execute reconfig command we must wait until next event round
    FbTk::Timer m_reconfig_timer;
    FbTk::Timer m_key_reload_timer;
    bool m_showing_dialog;

    struct {
        bool starting;
        bool restarting;
        bool shutdown;
    } m_state;

    int m_server_grabs;
    std::unique_ptr<ShortcutManager> m_shortcut_manager;
};


#endif // FLUXBOX_HH
