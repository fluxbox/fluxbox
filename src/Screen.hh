// Screen.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Screen.hh for Blackbox - an X11 Window manager
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

#ifndef	 SCREEN_HH
#define	 SCREEN_HH


#include "FbRootWindow.hh"
#include "MenuTheme.hh"
#include "PlacementStrategy.hh"

#include "FbTk/Resource.hh"
#include "FbTk/Subject.hh"
#include "FbTk/MultLayers.hh"
#include "FbTk/NotCopyable.hh"
#include "FbTk/Observer.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <memory>
#include <map>

class FluxboxWindow;
class Netizen;
class FbWinFrameTheme;
class RootTheme;
class WinButtonTheme;
class WinClient;
class Workspace;
class Strut;
class Slit;
class HeadArea;
class FocusControl;
class PlacementStrategy;

namespace FbTk {
class Menu;
class ImageControl;
class XLayerItem;
class FbWindow;
class Subject;
}

/// Handles screen connection, screen clients and workspaces
/**
 Create workspaces, handles switching between workspaces and windows
 */
class BScreen : public FbTk::Observer, private FbTk::NotCopyable {
public:
    /// a window becomes active / focussed on a different workspace
    enum FollowModel { 
        IGNORE_OTHER_WORKSPACES = 0, ///< who cares?
        FOLLOW_ACTIVE_WINDOW, ///< go to that workspace
        FETCH_ACTIVE_WINDOW ///< put that window to the current workspace 
    };

    enum ResizeModel { 
        BOTTOMRESIZE = 0, 
        QUADRANTRESIZE, 
        CENTERRESIZE,
        DEFAULTRESIZE = BOTTOMRESIZE };


    typedef std::vector<FluxboxWindow *> Icons;

    typedef std::vector<Workspace *> Workspaces;
    typedef std::vector<std::string> WorkspaceNames;
    typedef std::list<std::pair<const char *, FbTk::Menu *> > ExtraMenus;

    BScreen(FbTk::ResourceManager &rm,
            const std::string &screenname, const std::string &altscreenname,
            int scrn, int number_of_layers);
    ~BScreen();

    void initWindows();
    void initMenus();

    bool isRootColormapInstalled() const { return root_colormap_installed; }
    bool isScreenManaged() const { return managed; }
    bool isSloppyWindowGrouping() const { return *resource.sloppy_window_grouping; }
    bool isWorkspaceWarping() const { return *resource.workspace_warping; }
    bool isDesktopWheeling() const { return *resource.desktop_wheeling; }
    bool doAutoRaise() const { return *resource.auto_raise; }
    bool clickRaises() const { return *resource.click_raises; }
    bool doOpaqueMove() const { return *resource.opaque_move; }
    bool doFullMax() const { return *resource.full_max; }
    bool doShowWindowPos() const { return *resource.show_window_pos; }
    bool antialias() const { return *resource.antialias; }
    bool decorateTransient() const { return *resource.decorate_transient; }
    const std::string &windowMenuFilename() const { return *resource.windowmenufile; }
    FbTk::ImageControl &imageControl() { return *m_image_control.get(); }
    // menus
    const FbTk::Menu &rootMenu() const { return *m_rootmenu.get(); }
    FbTk::Menu &rootMenu() { return *m_rootmenu.get(); }
    const FbTk::Menu &configMenu() const { return *m_configmenu.get(); }
    FbTk::Menu &configMenu() { return *m_configmenu.get(); }
    const FbTk::Menu &windowMenu() const { return *m_windowmenu.get(); }
    FbTk::Menu &windowMenu() { return *m_windowmenu.get(); }
    ExtraMenus &extraWindowMenus() { return m_extramenus; }
    const ExtraMenus &extraWindowMenus() const { return m_extramenus; }
    
    ResizeModel getResizeModel() const { return *resource.resize_model; }

    inline FollowModel getFollowModel() const { return *resource.follow_model; }

    inline const std::string &getScrollAction() const { return *resource.scroll_action; }
    inline const bool getScrollReverse() const { return *resource.scroll_reverse; }

    inline Slit *slit() { return m_slit.get(); }
    inline const Slit *slit() const { return m_slit.get(); }

    inline Workspace *getWorkspace(unsigned int w) { return ( w < m_workspaces_list.size() ? m_workspaces_list[w] : 0); }
    inline Workspace *currentWorkspace() { return m_current_workspace; }
    inline const Workspace *currentWorkspace() const { return m_current_workspace; }

    const FbTk::Menu &workspaceMenu() const { return *m_workspacemenu.get(); }
    FbTk::Menu &workspaceMenu() { return *m_workspacemenu.get(); }

    const FocusControl &focusControl() const { return *m_focus_control; }
    FocusControl &focusControl() { return *m_focus_control; }

    unsigned int currentWorkspaceID() const;
    /*
      maximum screen bounds for given window
    */
    unsigned int maxLeft(int head) const;
    unsigned int maxRight(int head) const;
    unsigned int maxTop(int head) const;
    unsigned int maxBottom(int head) const;
    /// @return true if window is kde dock app
    bool isKdeDockapp(Window win) const;
    /// @return true if dock app was added, else false
    bool addKdeDockapp(Window win);

    inline unsigned int width() const { return rootWindow().width(); }
    inline unsigned int height() const { return rootWindow().height(); }
    inline int screenNumber() const { return rootWindow().screenNumber(); }

    /// @return number of workspaces
    unsigned int numberOfWorkspaces() const { return m_workspaces_list.size(); }

    inline const Icons &iconList() const { return m_icon_list; }
    inline Icons &iconList() { return m_icon_list; }

    const Workspaces &getWorkspacesList() const { return m_workspaces_list; }
    Workspaces &getWorkspacesList() { return m_workspaces_list; }
    const WorkspaceNames &getWorkspaceNames() const { return m_workspace_names; }
    /**
       @name Screen signals
    */
    //@{
    /// client list signal
    FbTk::Subject &clientListSig() { return m_clientlist_sig; } 
    /// icon list sig
    FbTk::Subject &iconListSig() { return m_iconlist_sig; }
    /// workspace count signal
    FbTk::Subject &workspaceCountSig() { return m_workspacecount_sig; }
    /// workspace names signal 
    FbTk::Subject &workspaceNamesSig() { return m_workspacenames_sig; }
    /// workspace area signal
    FbTk::Subject &workspaceAreaSig() { return m_workspace_area_sig; }
    /// current workspace signal
    FbTk::Subject &currentWorkspaceSig() { return m_currentworkspace_sig; }
    /// reconfigure signal
    FbTk::Subject &reconfigureSig() { return m_reconfigure_sig; }
    FbTk::Subject &resizeSig() { return m_resize_sig; }
    //@}

    void update(FbTk::Subject *subj);

    FbTk::Menu *createMenu(const std::string &label);
    void hideMenus();
    // for extras to add menus.
    // These menus will be marked internal,
    // and deleted when the window dies (as opposed to Screen
    void addExtraWindowMenu(const char *label, FbTk::Menu *menu);
    void removeExtraWindowMenu(FbTk::Menu *menu);

    /// hide all windowmenus except the given one (if given)
    void hideWindowMenus(const FluxboxWindow* except= 0);

    inline int getEdgeSnapThreshold() const { return *resource.edge_snap_threshold; }

    void setRootColormapInstalled(bool r) { root_colormap_installed = r;  }
    void saveRootCommand(std::string rootcmd) { *resource.rootcommand = rootcmd;  }

    void saveWorkspaces(int w) { *resource.workspaces = w;  }

    void saveMenu(FbTk::Menu &menu) { m_rootmenu_list.push_back(&menu); }

    FbWinFrameTheme &winFrameTheme() { return *m_windowtheme.get(); }
    const FbWinFrameTheme &winFrameTheme() const { return *m_windowtheme.get(); }
    MenuTheme &menuTheme() { return *m_menutheme.get(); }
    const MenuTheme &menuTheme() const { return *m_menutheme.get(); }
    const RootTheme &rootTheme() const { return *m_root_theme.get(); }
    WinButtonTheme &winButtonTheme() { return *m_winbutton_theme.get(); }
    const WinButtonTheme &winButtonTheme() const { return *m_winbutton_theme.get(); }

    FbRootWindow &rootWindow() { return m_root_window; }
    const FbRootWindow &rootWindow() const { return m_root_window; }

    FbTk::MultLayers &layerManager() { return m_layermanager; }
    const FbTk::MultLayers &layerManager() const { return m_layermanager; }
    FbTk::ResourceManager &resourceManager() { return m_resource_manager; }
    const FbTk::ResourceManager &resourceManager() const { return m_resource_manager; }
    const std::string &name() const { return m_name; }
    const std::string &altName() const { return m_altname; }
    bool isShuttingdown() const { return m_shutdown; }

    PlacementStrategy &placementStrategy() { return *m_placement_strategy; }
    const PlacementStrategy &placementStrategy() const { return *m_placement_strategy; }
    
    int addWorkspace();
    int removeLastWorkspace();
    // scroll workspaces
    void nextWorkspace() { nextWorkspace(1); }
    void prevWorkspace() { prevWorkspace(1); }
    void nextWorkspace(int delta);
    void prevWorkspace(int delta);
    void rightWorkspace(int delta);
    void leftWorkspace(int delta);

    void removeWorkspaceNames();
    void updateWorkspaceNamesAtom();
	
    void addWorkspaceName(const char *name);
    void addNetizen(Window win);
    void removeNetizen(Window win);
    void addIcon(FluxboxWindow *win);
    void removeIcon(FluxboxWindow *win);
    // remove window
    void removeWindow(FluxboxWindow *win);
    void removeClient(WinClient &client);

    std::string getNameOfWorkspace(unsigned int workspace) const;
    void changeWorkspaceID(unsigned int);
    void sendToWorkspace(unsigned int workspace, FluxboxWindow *win=0, 
                         bool changeworkspace=true);
    void reassociateWindow(FluxboxWindow *window, unsigned int workspace_id, 
                           bool ignore_sticky);


    void reconfigure();	
    void rereadMenu();
    void shutdown();
    /// show position window centered on the screen with "X x Y" text
    void showPosition(int x, int y);
    void hidePosition();
    /// show geomentry with "width x height"-text, not size of window
    void showGeometry(unsigned int width, unsigned int height);
    void hideGeometry();

    void notifyReleasedKeys(XKeyEvent &ke);

    void setLayer(FbTk::XLayerItem &item, int layernum);
    // remove? no, items are never removed from their layer until they die

    /// updates root window size and resizes/reconfigures screen clients 
    /// that depends on screen size (slit)
    /// (and maximized windows?)
    void updateSize();

    // Xinerama-related functions
    bool hasXinerama() const { return m_xinerama_avail; }
    int numHeads() const { return m_xinerama_num_heads; }

    void initXinerama();

    int getHead(int x, int y) const;
    int getHead(FbTk::FbWindow &win) const;
    int getCurrHead() const;
    int getHeadX(int head) const;
    int getHeadY(int head) const;
    int getHeadWidth(int head) const;
    int getHeadHeight(int head) const;

    // returns the new (x,y) for a rectangle fitted on a head
    std::pair<int,int> clampToHead(int head, int x, int y, int w, int h) const;

    // magic to allow us to have "on head" placement (menu) without
    // the object really knowing about it.
    template <typename OnHeadObject>
    int getOnHead(OnHeadObject &obj);

    template <typename OnHeadObject>
    void setOnHead(OnHeadObject &obj, int head);

    // grouping - we want ordering, so we can either search for a 
    // group to the left, or to the right (they'll be different if
    // they exist).
    FluxboxWindow *findGroupLeft(WinClient &winclient);
    FluxboxWindow *findGroupRight(WinClient &winclient);

    // notify netizens
    void updateNetizenCurrentWorkspace();
    void updateNetizenWorkspaceCount();
    void updateNetizenWindowFocus();
    void updateNetizenWindowAdd(Window, unsigned long);
    void updateNetizenWindowDel(Window);
    void updateNetizenConfigNotify(XEvent &ev);
    void updateNetizenWindowRaise(Window);
    void updateNetizenWindowLower(Window);

    /// create window frame for client window and attach it
    FluxboxWindow *createWindow(Window clientwin);
    FluxboxWindow *createWindow(WinClient &client);
    void setupWindowActions(FluxboxWindow &win);
    /// request workspace space, i.e "don't maximize over this area"
    Strut *requestStrut(int head, int left, int right, int top, int bottom);
    /// remove requested space and destroy strut
    void clearStrut(Strut *strut); 
    /// updates max avaible area for the workspace
    void updateAvailableWorkspaceArea();

    // for extras to add menus. These menus must be marked
    // internal for their safety, and __the extension__ must
    // delete and remove the menu itself (opposite to Window)
    void addConfigMenu(const char *label, FbTk::Menu &menu);
    void removeConfigMenu(FbTk::Menu &menu);


    /// Adds a resource to managed resource list
    /// This resource is now owned by Screen and will be destroyed
    /// when screen dies
    void addManagedResource(FbTk::Resource_base *resource);

    class ScreenSubject:public FbTk::Subject {
    public:
        ScreenSubject(BScreen &scr):m_scr(scr) { }
        const BScreen &screen() const { return m_scr; }
        BScreen &screen() { return m_scr; }
    private:
        BScreen &m_scr;
    };

private:
    void setupConfigmenu(FbTk::Menu &menu);
    void initMenu();
    void renderGeomWindow();
    void renderPosWindow();

    const Strut* availableWorkspaceArea(int head) const;

    ScreenSubject 
    m_clientlist_sig,  ///< client signal
        m_iconlist_sig, ///< notify if a window gets iconified/deiconified
        m_workspacecount_sig, ///< workspace count signal
        m_workspacenames_sig, ///< workspace names signal 
        m_workspace_area_sig, ///< workspace area changed signal
        m_currentworkspace_sig, ///< current workspace signal
        m_reconfigure_sig, ///< reconfigure signal
        m_resize_sig; ///< resize signal
		
    FbTk::MultLayers m_layermanager;
	
    bool root_colormap_installed, managed, geom_visible, pos_visible;

    GC opGC;
    Pixmap geom_pixmap, pos_pixmap;



    std::auto_ptr<FbTk::ImageControl> m_image_control;
    std::auto_ptr<FbTk::Menu> m_configmenu, m_rootmenu, m_workspacemenu, m_windowmenu;

    ExtraMenus m_extramenus;

    typedef std::list<FbTk::Menu *> Rootmenus;
    typedef std::list<Netizen *> Netizens;
    typedef std::list<std::pair<const char *, FbTk::Menu *> > Configmenus;


    Rootmenus m_rootmenu_list;
    Netizens m_netizen_list;
    Configmenus m_configmenu_list;
    Icons m_icon_list;

    std::auto_ptr<Slit> m_slit;

    Workspace *m_current_workspace;

    WorkspaceNames m_workspace_names;
    Workspaces m_workspaces_list;

    std::auto_ptr<FbWinFrameTheme> m_windowtheme;
    std::auto_ptr<WinButtonTheme> m_winbutton_theme;
    std::auto_ptr<MenuTheme> m_menutheme;
    std::auto_ptr<RootTheme> m_root_theme;

    FbRootWindow m_root_window;
    FbTk::FbWindow m_geom_window, m_pos_window;

    struct ScreenResource {
        ScreenResource(FbTk::ResourceManager &rm, const std::string &scrname,
                       const std::string &altscrname);

        FbTk::Resource<bool> image_dither, opaque_move, full_max,
            sloppy_window_grouping, workspace_warping,
            desktop_wheeling, show_window_pos,            
            antialias, auto_raise, click_raises, decorate_transient;
        FbTk::Resource<std::string> rootcommand;		
        FbTk::Resource<ResizeModel> resize_model;
        FbTk::Resource<std::string> windowmenufile;
        FbTk::Resource<FollowModel> follow_model;
        bool ordered_dither;
        FbTk::Resource<int> workspaces, edge_snap_threshold, focused_alpha,
            unfocused_alpha, menu_alpha, menu_delay, menu_delay_close;
        FbTk::Resource<FbTk::MenuTheme::MenuMode> menu_mode;

        FbTk::Resource<int> gc_line_width;
        FbTk::Resource<FbTk::GContext::LineStyle> gc_line_style;
        FbTk::Resource<FbTk::GContext::JoinStyle> gc_join_style;
        FbTk::Resource<FbTk::GContext::CapStyle>  gc_cap_style;
        FbTk::Resource<std::string> scroll_action;
        FbTk::Resource<bool> scroll_reverse;

    } resource;

    /// Holds manage resources that screen destroys
    FbTk::ResourceManager::ResourceList m_managed_resources;

    FbTk::ResourceManager &m_resource_manager;
    const std::string m_name, m_altname;

    FocusControl *m_focus_control;
    PlacementStrategy *m_placement_strategy;

    // This is a map of windows to clients for clients that had a left
    // window set, but that window wasn't present at the time
    typedef std::map<Window, WinClient *> Groupables;
    Groupables m_expecting_groups;

    // Xinerama related private data
    bool m_xinerama_avail;
    int m_xinerama_num_heads;
    int m_xinerama_center_x, m_xinerama_center_y;

    HeadArea *m_head_areas;

    struct XineramaHeadInfo {
        int x, y, width, height;        
    } *m_xinerama_headinfo;

    bool m_shutdown;
};


#endif // SCREEN_HH
