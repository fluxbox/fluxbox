// Screen.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// 
// Screen.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// $Id: Screen.hh,v 1.48 2002/10/25 20:56:12 fluxgen Exp $

#ifndef	 SCREEN_HH
#define	 SCREEN_HH

#include "Theme.hh"
#include "BaseDisplay.hh"
#include "Netizen.hh"
#include "Timer.hh"
#include "Workspace.hh"
#include "Tab.hh"
#include "Resource.hh"
#include "Toolbar.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#endif // SLIT

#include <X11/Xlib.h>
#include <X11/Xresource.h>

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

#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <memory>

class Configmenu;
class Workspacemenu;
class Iconmenu;
class Rootmenu;

class BScreen : public ScreenInfo {
public:
	typedef std::vector<Workspace *> Workspaces;
	typedef std::vector<std::string> WorkspaceNames;
	
	BScreen(ResourceManager &rm,
		const std::string &screenname, const std::string &altscreenname,
		int scrn);
	~BScreen();

	inline bool isToolbarOnTop() const { return *resource.toolbar_on_top; }
	inline bool doToolbarAutoHide() const { return *resource.toolbar_auto_hide; }
	inline bool isSloppyFocus() const { return resource.sloppy_focus; }
	inline bool isSemiSloppyFocus() const { return resource.semi_sloppy_focus; }
	inline bool isRootColormapInstalled() const { return root_colormap_installed; }
	inline bool isScreenManaged() const { return managed; }
	inline bool isTabRotateVertical() const { return *resource.tab_rotate_vertical; }
	inline bool isSloppyWindowGrouping() const { return *resource.sloppy_window_grouping; }
	inline bool isWorkspaceWarping() const { return *resource.workspace_warping; }
	inline bool isDesktopWheeling() const { return *resource.desktop_wheeling; }
	inline bool doAutoRaise() const { return resource.auto_raise; }
	inline bool doImageDither() const { return *resource.image_dither; }
	inline bool doMaxOverSlit() const { return *resource.max_over_slit; }
	inline bool doOpaqueMove() const { return *resource.opaque_move; }
	inline bool doFullMax() const { return *resource.full_max; }
	inline bool doFocusNew() const { return *resource.focus_new; }
	inline bool doFocusLast() const { return *resource.focus_last; }
	inline bool doShowWindowPos() const { return *resource.show_window_pos; }
	bool antialias() const { return *resource.antialias; }
	inline GC getOpGC() const { return theme->getOpGC(); }
	
	inline const FbTk::Color *getBorderColor() const { return &theme->getBorderColor(); }
	inline BImageControl *getImageControl() { return image_control; }
	const Rootmenu * const getRootmenu() const { return rootmenu; }
	Rootmenu * const getRootmenu() { return rootmenu; }
	
	inline const std::string &getRootCommand() const { return *resource.rootcommand; }
#ifdef	 SLIT
	inline bool isSlitOnTop() const { return resource.slit_on_top; }
	inline bool doSlitAutoHide() const { return resource.slit_auto_hide; }
	inline Slit *getSlit() { return slit; }
	inline const Slit *getSlit() const { return slit; }
	inline int getSlitPlacement() const { return resource.slit_placement; }
	inline int getSlitDirection() const { return resource.slit_direction; }
	inline void saveSlitPlacement(int p) { resource.slit_placement = p;  }
	inline void saveSlitDirection(int d) { resource.slit_direction = d;  }
	inline void saveSlitOnTop(bool t) { resource.slit_on_top = t;  }
	inline void saveSlitAutoHide(bool t) { resource.slit_auto_hide = t;  }
#ifdef XINERAMA
	inline unsigned int getSlitOnHead() const { return resource.slit_on_head; }
	inline void saveSlitOnHead(unsigned int h) { resource.slit_on_head = h;  }
#endif // XINERAMA

#endif // SLIT

	inline const Toolbar * const getToolbar() const { return m_toolbar.get(); }
	inline Toolbar * const getToolbar() { return m_toolbar.get(); }

	inline Workspace *getWorkspace(unsigned int w) { return ( w < workspacesList.size() ? workspacesList[w] : 0); }
	inline Workspace *getCurrentWorkspace() { return current_workspace; }

	const Workspacemenu * const getWorkspacemenu() const { return workspacemenu; }
	Workspacemenu * const getWorkspacemenu() { return workspacemenu; }

	inline unsigned int getHandleWidth() const { return theme->getHandleWidth(); }
	inline unsigned int getBevelWidth() const { return theme->getBevelWidth(); }
	inline unsigned int getFrameWidth() const { return theme->getFrameWidth(); }
	inline unsigned int getBorderWidth() const { return theme->getBorderWidth(); }
	inline unsigned int getBorderWidth2x() const { return theme->getBorderWidth()*2; }
	inline unsigned int getCurrentWorkspaceID() const { return current_workspace->workspaceID(); }

    typedef std::vector<FluxboxWindow *> Icons;

	/// @return number of workspaces
	inline unsigned int getCount() const { return workspacesList.size(); }
	/// @return number of icons
	inline unsigned int getIconCount() const { return iconList.size(); }
	inline const Icons &getIconList() const { return iconList; }
	inline Icons &getIconList() { return iconList; }
	const Workspaces &getWorkspacesList() const { return workspacesList; }
	const WorkspaceNames &getWorkspaceNames() const { return workspaceNames; }

	/// client list signal
	FbTk::Subject &clientListSig() { return m_clientlist_sig; } 
	/// workspace count signal
	FbTk::Subject &workspaceCountSig() { return m_workspacecount_sig; }
	/// workspace names signal 
	FbTk::Subject &workspaceNamesSig() { return m_workspacenames_sig; }
	/// current workspace signal
	FbTk::Subject &currentWorkspaceSig() { return m_currentworkspace_sig; }
			
	/// @return the resource value of number of workspace
	inline int getNumberOfWorkspaces() const { return *resource.workspaces; }	
	inline Toolbar::Placement getToolbarPlacement() const { return *resource.toolbar_placement; }
#ifdef XINERAMA
	inline int getToolbarOnHead() { return *resource.toolbar_on_head; }
#endif // XINERAMA
	inline int getToolbarWidthPercent() const { return *resource.toolbar_width_percent; }
	inline int getPlacementPolicy() const { return resource.placement_policy; }
	inline int getEdgeSnapThreshold() const { return *resource.edge_snap_threshold; }
	inline int getRowPlacementDirection() const { return resource.row_direction; }
	inline int getColPlacementDirection() const { return resource.col_direction; }
	inline unsigned int getTabWidth() const { return *resource.tab_width; }
	inline unsigned int getTabHeight() const { return *resource.tab_height; }
	inline Tab::Placement getTabPlacement() const { return *resource.tab_placement; }
	inline Tab::Alignment getTabAlignment() const { return *resource.tab_alignment; }

	inline void setRootColormapInstalled(Bool r) { root_colormap_installed = r;  }
	inline void saveRootCommand(std::string rootcmd) { *resource.rootcommand = rootcmd;  }
	inline void saveSloppyFocus(bool s) { resource.sloppy_focus = s;  }
	inline void saveSemiSloppyFocus(bool s) { resource.semi_sloppy_focus = s;  }
	inline void saveAutoRaise(bool a) { resource.auto_raise = a;  }
	inline void saveWorkspaces(int w) { *resource.workspaces = w;  }
	inline void saveToolbarOnTop(bool r) { *resource.toolbar_on_top = r;  }
	inline void saveToolbarAutoHide(bool r) { *resource.toolbar_auto_hide = r;  }
	inline void saveToolbarWidthPercent(int w) { *resource.toolbar_width_percent = w;  }
	inline void saveToolbarPlacement(Toolbar::Placement p) { *resource.toolbar_placement = p;  }
#ifdef XINERAMA
	inline void saveToolbarOnHead(int head) { *resource.toolbar_on_head = head;  }
#endif // XINERAMA

	inline void savePlacementPolicy(int p) { resource.placement_policy = p;  }
	inline void saveRowPlacementDirection(int d) { resource.row_direction = d;  }
	inline void saveColPlacementDirection(int d) { resource.col_direction = d;  }
	inline void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t;  }
	inline void saveImageDither(bool d) { resource.image_dither = d;  }
	inline void saveMaxOverSlit(bool m) { resource.max_over_slit = m;  }
	inline void saveOpaqueMove(bool o) { resource.opaque_move = o;  }
	inline void saveFullMax(bool f) { resource.full_max = f;  }
	inline void saveFocusNew(bool f) { resource.focus_new = f;  }
	inline void saveFocusLast(bool f) { resource.focus_last = f;  }
	inline void saveTabWidth(unsigned int w) { resource.tab_width = w;  }
	inline void saveTabHeight(unsigned int h) { resource.tab_height = h;  }
	inline void saveTabPlacement(Tab::Placement p) { *resource.tab_placement = p;  }
	inline void saveTabAlignment(Tab::Alignment a) { *resource.tab_alignment = a;  }
	inline void saveTabRotateVertical(bool r) { resource.tab_rotate_vertical = r;   }
	inline void saveSloppyWindowGrouping(bool s) { resource.sloppy_window_grouping = s;  }
	inline void saveWorkspaceWarping(bool s) { resource.workspace_warping = s; }
	inline void saveDesktopWheeling(bool s) { resource.desktop_wheeling = s; }
	void iconUpdate();
	inline const Iconmenu *getIconmenu() const { return m_iconmenu; }
	inline void setAutoGroupWindow(Window w = 0) { auto_group_window = w; }
	void setAntialias(bool value);
	
	#ifdef HAVE_STRFTIME
	inline const char *getStrftimeFormat() { return resource.strftime_format.c_str(); }
	void saveStrftimeFormat(const char *format);
	#else // !HAVE_STRFTIME
	inline int getDateFormat() { return resource.date_format; }
	inline void saveDateFormat(int f) { resource.date_format = f; }
	inline bool isClock24Hour() { return resource.clock24hour; }
	inline void saveClock24Hour(bool c) { resource.clock24hour = c; }
	#endif // HAVE_STRFTIME

	inline Theme::WindowStyle *getWindowStyle() { return &theme->getWindowStyle(); } 
	inline Theme::MenuStyle *getMenuStyle() { return &theme->getMenuStyle(); } 
	inline Theme::ToolbarStyle *getToolbarStyle() { return &theme->getToolbarStyle(); } 
	const Theme *getTheme() const { return theme; }
	FluxboxWindow *getIcon(unsigned int index);

	int addWorkspace();
	int removeLastWorkspace();
	//scroll workspaces
	void nextWorkspace(const int delta);
	void prevWorkspace(const int delta);
	void rightWorkspace(const int delta);
	void leftWorkspace(const int delta);

	void removeWorkspaceNames();
	void updateWorkspaceNamesAtom();
	
	void addWorkspaceName(const char *name);
	void addNetizen(Netizen *net);
	void removeNetizen(Window win);
	void addIcon(FluxboxWindow *win);
	void removeIcon(FluxboxWindow *win);
	// remove window
	void removeWindow(FluxboxWindow *win);

	std::string getNameOfWorkspace(unsigned int workspace) const;
	void changeWorkspaceID(unsigned int);
	void sendToWorkspace(unsigned int workspace, FluxboxWindow *win=0, bool changeworkspace=true);
	void raiseWindows(const Workspace::Stack &workspace_stack);
	void reassociateGroup(FluxboxWindow *window, unsigned int workspace_id, bool ignore_sticky);
	void reassociateWindow(FluxboxWindow *window, unsigned int workspace_id, bool ignore_sticky);
	void prevFocus(int = 0);
	void nextFocus(int = 0);
	void raiseFocus();
	void reconfigure();	
	void rereadMenu();
	void shutdown();
	void showPosition(int x, int y);
	void showGeometry(unsigned int, unsigned int);
	void hideGeometry();

	FluxboxWindow* useAutoGroupWindow();

	void updateNetizenCurrentWorkspace();
	void updateNetizenWorkspaceCount();
	void updateNetizenWindowFocus();
	void updateNetizenWindowAdd(Window, unsigned long);
	void updateNetizenWindowDel(Window);
	void updateNetizenConfigNotify(XEvent *);
	void updateNetizenWindowRaise(Window);
	void updateNetizenWindowLower(Window);

	enum { ROWSMARTPLACEMENT = 1, COLSMARTPLACEMENT, CASCADEPLACEMENT, LEFTRIGHT,
				 RIGHTLEFT, TOPBOTTOM, BOTTOMTOP };
	enum { LEFTJUSTIFY = 1, RIGHTJUSTIFY, CENTERJUSTIFY };
	enum { ROUNDBULLET = 1, TRIANGELBULLET, SQUAERBULLET, NOBULLET };
	enum { RESTART = 1, RESTARTOTHER, EXIT, SHUTDOWN, EXECUTE, RECONFIGURE,
				 WINDOWSHADE, WINDOWICONIFY, WINDOWMAXIMIZE, WINDOWCLOSE, WINDOWRAISE,
				 WINDOWLOWER, WINDOWSTICK, WINDOWKILL, SETSTYLE, WINDOWTAB};
	// prevFocus/nextFocus option bits
	enum { CYCLESKIPLOWERTABS = 0x01, CYCLESKIPSTUCK = 0x02, CYCLESKIPSHADED = 0x04,
				CYCLEDEFAULT = 0x00 };

	class ScreenSubject:public FbTk::Subject {
	public:
		ScreenSubject(BScreen &scr):m_scr(scr) { }
		const BScreen &screen() const { return m_scr; }
		BScreen &screen() { return m_scr; }
	private:
		BScreen &m_scr;
	};
	
private:
	void createStyleMenu(Rootmenu *menu, bool newmenu, const char *label, const char *directory);

	bool parseMenuFile(std::ifstream &, Rootmenu *, int&);

	void initMenu();

	bool doSkipWindow(const FluxboxWindow *w, int options);

	ScreenSubject 
		m_clientlist_sig,  ///< client signal
		m_workspacecount_sig, ///< workspace count signal
		m_workspacenames_sig, ///< workspace names signal 
		m_currentworkspace_sig; ///< current workspace signal
		
	
	Theme *theme;
	
	Bool root_colormap_installed, managed, geom_visible;
	GC opGC;
	Pixmap geom_pixmap;
	Window geom_window;

	BImageControl *image_control;
	Configmenu *configmenu;
	Iconmenu *m_iconmenu;

	Rootmenu *rootmenu;

    typedef std::list<Rootmenu *> Rootmenus;
    typedef std::list<Netizen *> Netizens;

    Rootmenus rootmenuList;
    Netizens netizenList;
    Icons iconList;

#ifdef		SLIT
	Slit *slit;
#endif // SLIT

	std::auto_ptr<Toolbar> m_toolbar;
	Workspace *current_workspace;
	Workspacemenu *workspacemenu;

	unsigned int geom_w, geom_h;
	unsigned long event_mask;

    WorkspaceNames workspaceNames;
    Workspaces workspacesList;

	Window auto_group_window;
	
	struct ScreenResource {
		ScreenResource(ResourceManager &rm, const std::string &scrname,
			const std::string &altscrname);

		Resource<bool> toolbar_on_top, toolbar_auto_hide,
			image_dither, opaque_move, full_max,
			max_over_slit, tab_rotate_vertical,
			sloppy_window_grouping, workspace_warping,
			desktop_wheeling, show_window_pos,
			focus_last, focus_new,
			antialias;
		Resource<std::string> rootcommand;		
		bool auto_raise, sloppy_focus, semi_sloppy_focus,
			ordered_dither;
		Resource<int> workspaces, toolbar_width_percent, edge_snap_threshold,
			tab_width, tab_height;
		int placement_policy, row_direction, col_direction;

		Resource<Tab::Placement> tab_placement;
		Resource<Tab::Alignment> tab_alignment;
		#ifdef XINERAMA
		Resource<int> toolbar_on_head;
		#endif // XINERAMA

		Resource<Toolbar::Placement> toolbar_placement;


		#ifdef SLIT
		bool slit_on_top, slit_auto_hide;
		int slit_placement, slit_direction;

		#ifdef XINERAMA
		unsigned int slit_on_head;
		#endif // XINERAMA

		#endif // SLIT


		#ifdef	HAVE_STRFTIME
		std::string strftime_format;
		#else // !HAVE_STRFTIME
		Bool clock24hour;
		int date_format;
		#endif // HAVE_STRFTIME

	} resource;
};


#endif // SCREEN_HH
