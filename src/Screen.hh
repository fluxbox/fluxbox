// Screen.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Screen.hh,v 1.109 2003/06/23 14:16:04 rathnor Exp $

#ifndef	 SCREEN_HH
#define	 SCREEN_HH

#include "Resource.hh"
#include "Subject.hh"
#include "MultLayers.hh"
#include "ToolbarHandler.hh"
#include "FbRootWindow.hh"
#include "NotCopyable.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <memory>

class Netizen;
class Toolbar;
class FbWinFrameTheme;
class RootTheme;
class WinButtonTheme;
class WinClient;
class Workspace;
class Strut;
class Slit;

namespace FbTk {
class MenuTheme;
class Menu;
class ImageControl;
class XLayerItem;
class FbWindow;
};

/// Handles screen connection, screen clients and workspaces
/**
 Create a toolbar and workspaces, handles switching between workspaces and windows
 */
class BScreen : private FbTk::NotCopyable {
public:
    typedef std::vector<Workspace *> Workspaces;
    typedef std::vector<std::string> WorkspaceNames;

    BScreen(FbTk::ResourceManager &rm,
            const std::string &screenname, const std::string &altscreenname,
            int scrn, int number_of_layers);
    ~BScreen();

    inline bool isSloppyFocus() const { return (*resource.focus_model == Fluxbox::SLOPPYFOCUS); }
    inline bool isSemiSloppyFocus() const { return (*resource.focus_model == Fluxbox::SEMISLOPPYFOCUS); }
    inline bool isRootColormapInstalled() const { return root_colormap_installed; }
    inline bool isScreenManaged() const { return managed; }
    inline bool isSloppyWindowGrouping() const { return *resource.sloppy_window_grouping; }
    inline bool isWorkspaceWarping() const { return *resource.workspace_warping; }
    inline bool isDesktopWheeling() const { return *resource.desktop_wheeling; }
    inline bool doAutoRaise() const { return *resource.auto_raise; }
    inline bool clickRaises() const { return *resource.click_raises; }
    inline bool doImageDither() const { return *resource.image_dither; }
    inline bool doOpaqueMove() const { return *resource.opaque_move; }
    inline bool doFullMax() const { return *resource.full_max; }
    inline bool doFocusNew() const { return *resource.focus_new; }
    inline bool doFocusLast() const { return *resource.focus_last; }
    inline bool doShowWindowPos() const { return *resource.show_window_pos; }
    bool antialias() const { return *resource.antialias; }

    inline FbTk::ImageControl &imageControl() { return *m_image_control.get(); }
    const FbTk::Menu * const getRootmenu() const { return m_rootmenu.get(); }
    FbTk::Menu * const getRootmenu() { return m_rootmenu.get(); }
    const FbTk::Menu &toolbarModemenu() const;
    FbTk::Menu &toolbarModemenu();
	
    inline const std::string &getRootCommand() const { return *resource.rootcommand; }
    inline Fluxbox::FocusModel getFocusModel() const { return *resource.focus_model; }

    inline Slit *slit() { return m_slit.get(); }
    inline const Slit *slit() const { return m_slit.get(); }

    inline const Toolbar *toolbar() const { return m_toolbarhandler->getToolbar(); }
    inline Toolbar *toolbar() { return m_toolbarhandler->getToolbar(); }

    inline const ToolbarHandler &toolbarHandler() const { return *m_toolbarhandler; }
    inline ToolbarHandler &toolbarHandler() { return *m_toolbarhandler; }

    inline Workspace *getWorkspace(unsigned int w) { return ( w < m_workspaces_list.size() ? m_workspaces_list[w] : 0); }
    inline Workspace *currentWorkspace() { return m_current_workspace; }

    const FbTk::Menu *getWorkspacemenu() const { return workspacemenu.get(); }
    FbTk::Menu *getWorkspacemenu() { return workspacemenu.get(); }

    unsigned int currentWorkspaceID() const;
    Pixmap rootPixmap() const;
    /*
      maximum screen bounds for given window
    */
    unsigned int maxLeft(int head) const;
    unsigned int maxRight(int head) const;
    unsigned int maxTop(int head) const;
    unsigned int maxBottom(int head) const;

    inline unsigned int width() const { return rootWindow().width(); }
    inline unsigned int height() const { return rootWindow().height(); }
    inline unsigned int screenNumber() const { return rootWindow().screenNumber(); }
    typedef std::vector<FluxboxWindow *> Icons;
    typedef std::list<WinClient *> FocusedWindows;

    /// @return number of workspaces
    inline unsigned int getCount() const { return m_workspaces_list.size(); }
    /// @return number of icons
    inline unsigned int getIconCount() const { return m_icon_list.size(); }
    inline const Icons &getIconList() const { return m_icon_list; }
    inline Icons &getIconList() { return m_icon_list; }
    inline const FocusedWindows &getFocusedList() const { return focused_list; }
    inline FocusedWindows &getFocusedList() { return focused_list; }
    WinClient *getLastFocusedWindow(int workspace = -1);
    const Workspaces &getWorkspacesList() const { return m_workspaces_list; }
    const WorkspaceNames &getWorkspaceNames() const { return m_workspace_names; }
    /**
       @name Screen signals
    */
    //@{
    /// client list signal
    FbTk::Subject &clientListSig() { return m_clientlist_sig; } 
    /// workspace count signal
    FbTk::Subject &workspaceCountSig() { return m_workspacecount_sig; }
    /// workspace names signal 
    FbTk::Subject &workspaceNamesSig() { return m_workspacenames_sig; }
    /// current workspace signal
    FbTk::Subject &currentWorkspaceSig() { return m_currentworkspace_sig; }
    //@}
		
    /// @return the resource value of number of workspace
    inline int getNumberOfWorkspaces() const { return *resource.workspaces; }	

    inline ToolbarHandler::ToolbarMode toolbarMode() const { return *resource.toolbar_mode; }
    inline int getPlacementPolicy() const { return resource.placement_policy; }
    inline int getEdgeSnapThreshold() const { return *resource.edge_snap_threshold; }
    inline int getRowPlacementDirection() const { return resource.row_direction; }
    inline int getColPlacementDirection() const { return resource.col_direction; }

    inline void setRootColormapInstalled(bool r) { root_colormap_installed = r;  }
    inline void saveRootCommand(std::string rootcmd) { *resource.rootcommand = rootcmd;  }
    inline void saveFocusModel(Fluxbox::FocusModel model) { resource.focus_model = model; }
    inline void saveWorkspaces(int w) { *resource.workspaces = w;  }


    inline void saveToolbarMode(ToolbarHandler::ToolbarMode m) { *resource.toolbar_mode = m; }

    inline void savePlacementPolicy(int p) { resource.placement_policy = p;  }
    inline void saveRowPlacementDirection(int d) { resource.row_direction = d;  }
    inline void saveColPlacementDirection(int d) { resource.col_direction = d;  }
    inline void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t;  }
    inline void saveImageDither(bool d) { resource.image_dither = d;  }

    inline void saveOpaqueMove(bool o) { resource.opaque_move = o;  }
    inline void saveFullMax(bool f) { resource.full_max = f;  }
    inline void saveFocusNew(bool f) { resource.focus_new = f;  }
    inline void saveFocusLast(bool f) { resource.focus_last = f;  }
    inline void saveSloppyWindowGrouping(bool s) { resource.sloppy_window_grouping = s;  }
    inline void saveWorkspaceWarping(bool s) { resource.workspace_warping = s; }
    inline void saveDesktopWheeling(bool s) { resource.desktop_wheeling = s; }

    void setAntialias(bool value);
	
    inline const char *getStrftimeFormat() { return resource.strftime_format.c_str(); }
    void saveStrftimeFormat(const char *format);

    inline int getDateFormat() { return resource.date_format; }
    inline void saveDateFormat(int f) { resource.date_format = f; }
    inline bool isClock24Hour() { return resource.clock24hour; }
    inline void saveClock24Hour(bool c) { resource.clock24hour = c; }

    inline FbWinFrameTheme &winFrameTheme() { return *m_windowtheme.get(); }
    inline const FbWinFrameTheme &winFrameTheme() const { return *m_windowtheme.get(); }
    inline FbTk::MenuTheme *menuTheme() { return m_menutheme.get(); }
    inline const FbTk::MenuTheme *menuTheme() const { return m_menutheme.get(); }
    inline const RootTheme &rootTheme() const { return *m_root_theme.get(); }
    FbRootWindow &rootWindow() { return m_root_window; }
    const FbRootWindow &rootWindow() const { return m_root_window; }

    FluxboxWindow *getIcon(unsigned int index);
    FbTk::MultLayers &layerManager() { return m_layermanager; }
    const FbTk::MultLayers &layerManager() const { return m_layermanager; }
    FbTk::ResourceManager &resourceManager() { return m_resource_manager; }
    const FbTk::ResourceManager &resourceManager() const { return m_resource_manager; }
    const std::string &name() const { return m_name; }
    const std::string &altName() const { return m_altname; }
    int addWorkspace();
    int removeLastWorkspace();
    //scroll workspaces
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
    void prevFocus() { prevFocus(0); }
    void nextFocus() { nextFocus(0); }
    void prevFocus(int options);
    void nextFocus(int options);
    void raiseFocus();
    void setFocusedWindow(WinClient &winclient);

    enum FocusDir { FOCUSUP, FOCUSDOWN, FOCUSLEFT, FOCUSRIGHT };
    void dirFocus(FluxboxWindow &win, FocusDir dir);

    void reconfigure();	
    void rereadMenu();
    void shutdown();
    /// show position window centered on the screen with "X x Y" text
    void showPosition(int x, int y);
    /// show geomentry with "width x height"-text, not size of window
    void showGeometry(unsigned int width, unsigned int height);
    void hideGeometry();

    void notifyReleasedKeys(XKeyEvent &ke);

    void setLayer(FbTk::XLayerItem &item, int layernum);
    // remove? no, items are never removed from their layer until they die

    FluxboxWindow* useAutoGroupWindow();

    /// updates root window size and resizes/reconfigures screen clients 
    /// that depends on screen size (toolbar, slit)
    /// (and maximized windows?)
    void updateSize();

    // Xinerama-related functions
    inline bool hasXinerama() const { return m_xinerama_avail; }
    inline int numHeads() const { return m_xinerama_num_heads; }

    void initXinerama();

    int getHead(int x, int y) const;
    int getHead(FbTk::FbWindow &win) const;
    int getCurrHead() const;
    int getHeadX(int head) const;
    int getHeadY(int head) const;
    int getHeadWidth(int head) const;
    int getHeadHeight(int head) const;

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
    void updateNetizenConfigNotify(XEvent *);
    void updateNetizenWindowRaise(Window);
    void updateNetizenWindowLower(Window);
    /// create window frame for client window and attach it
    FluxboxWindow *createWindow(Window clientwin);
    FluxboxWindow *createWindow(WinClient &client);
    void setupWindowActions(FluxboxWindow &win);
    /// request workspace space, i.e "don't maximize over this area"
    Strut *requestStrut(int left, int right, int top, int bottom);
    /// remove requested space and destroy strut
    void clearStrut(Strut *strut); 
    /// updates max avaible area for the workspace
    void updateAvailableWorkspaceArea();

    enum { ROWSMARTPLACEMENT = 1, COLSMARTPLACEMENT, CASCADEPLACEMENT,
           UNDERMOUSEPLACEMENT, LEFTRIGHT, RIGHTLEFT, TOPBOTTOM, BOTTOMTOP };

    // prevFocus/nextFocus option bits
    enum { CYCLEGROUPS = 0x01, CYCLESKIPSTUCK = 0x02, CYCLESKIPSHADED = 0x04,
           CYCLELINEAR = 0x08, CYCLEDEFAULT = 0x00 };

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
    void createStyleMenu(FbTk::Menu &menu, const char *label, const char *directory);

    bool parseMenuFile(std::ifstream &filestream, FbTk::Menu &menu, int &row);

    void initMenu();

    bool doSkipWindow(const WinClient &winclient, int options);

    void renderGeomWindow();

    ScreenSubject 
    m_clientlist_sig,  ///< client signal
        m_workspacecount_sig, ///< workspace count signal
        m_workspacenames_sig, ///< workspace names signal 
        m_currentworkspace_sig; ///< current workspace signal
		
    FbTk::MultLayers m_layermanager;
	
    bool root_colormap_installed, managed, geom_visible, cycling_focus;
    GC opGC;
    Pixmap geom_pixmap;

    FbTk::FbWindow m_geom_window;

    std::auto_ptr<FbTk::ImageControl> m_image_control;
    std::auto_ptr<FbTk::Menu> m_configmenu;

    std::auto_ptr<FbTk::Menu> m_rootmenu;

    typedef std::list<FbTk::Menu *> Rootmenus;
    typedef std::list<Netizen *> Netizens;

    Rootmenus m_rootmenu_list;
    Netizens m_netizen_list;
    Icons m_icon_list;

    // This list keeps the order of window focusing for this screen
    // Screen global so it works for sticky windows too.
    FocusedWindows focused_list;
    FocusedWindows::iterator cycling_window;
    WinClient *cycling_last;

    std::auto_ptr<Slit> m_slit;

    Workspace *m_current_workspace;
    std::auto_ptr<FbTk::Menu> workspacemenu;

    WorkspaceNames m_workspace_names;
    Workspaces m_workspaces_list;

    Window auto_group_window;

    std::auto_ptr<FbWinFrameTheme> m_windowtheme;
    std::auto_ptr<WinButtonTheme> m_winbutton_theme;
    std::auto_ptr<FbTk::MenuTheme> m_menutheme;
    std::auto_ptr<RootTheme> m_root_theme;

    FbRootWindow m_root_window;

    struct ScreenResource {
        ScreenResource(FbTk::ResourceManager &rm, const std::string &scrname,
                       const std::string &altscrname);

        FbTk::Resource<bool> image_dither, opaque_move, full_max,
            sloppy_window_grouping, workspace_warping,
            desktop_wheeling, show_window_pos,
            focus_last, focus_new,
            antialias, auto_raise, click_raises;
        FbTk::Resource<std::string> rootcommand;		
        FbTk::Resource<Fluxbox::FocusModel> focus_model;
        bool ordered_dither;
        FbTk::Resource<int> workspaces, edge_snap_threshold, menu_alpha;

        int placement_policy, row_direction, col_direction;

        FbTk::Resource<ToolbarHandler::ToolbarMode> toolbar_mode;


        std::string strftime_format;

        bool clock24hour;
        int date_format;


    } resource;

    // This is a map of windows to clients for clients that had a left
    // window set, but that window wasn't present at the time
    typedef std::map<Window, WinClient *> Groupables;
    Groupables m_expecting_groups;

    const std::string m_name, m_altname;
    FbTk::ResourceManager &m_resource_manager;

    std::auto_ptr<ToolbarHandler> m_toolbarhandler;

    bool m_xinerama_avail;
    int m_xinerama_num_heads;

    // Xinerama related private data
   
    int m_xinerama_center_x, m_xinerama_center_y;

    std::auto_ptr<Strut> m_available_workspace_area;

    struct XineramaHeadInfo {
        int x, y, width, height;        
    } *m_xinerama_headinfo;

    std::list<Strut *> m_strutlist;
};


#endif // SCREEN_HH
