// fluxbox.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// blackbox.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: fluxbox.hh,v 1.6 2002/01/11 09:18:58 fluxgen Exp $

#ifndef	 _FLUXBOX_HH_
#define	 _FLUXBOX_HH_

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef		HAVE_STDIO_H
# include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef		TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h>
#else // !TIME_WITH_SYS_TIME
#	ifdef		HAVE_SYS_TIME_H
#		include <sys/time.h>
#	else // !HAVE_SYS_TIME_H
#		include <time.h>
#	endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

//forward declaration
class Fluxbox;

#ifndef _BASEDISPLAY_HH_
#include "BaseDisplay.hh"
#endif

#ifndef _IMAGE_HH_
#include "Image.hh"
#endif

#ifndef _LINKEDLIST_HH_
#include "LinkedList.hh"
#endif

#ifndef _TIMER_HH_
#include "Timer.hh"
#endif 

#ifndef _WINDOW_HH_
#include "Window.hh"
#endif 

#ifndef _TAB_HH_
#include "Tab.hh"
#endif

#ifndef _TOOLBAR_HH_
#include "Toolbar.hh"
#endif

#ifndef _KEYS_HH_
#include "Keys.hh"
#endif

#ifdef		SLIT
#	include "Slit.hh"
#endif // SLIT

#include <string>
#include <vector>

class Fluxbox : public BaseDisplay, public TimeoutHandler {	
public:
	
	
	static Fluxbox *instance(int m_argc=0, char **m_argv=0, char *dpy_name=0, char *rc=0);
	
	inline bool useTabs() const { return resource.tabs; }
	inline bool useIconBar() const { return resource.iconbar; }
	inline void saveTabs(bool value) { resource.tabs = value; }
	inline void saveIconBar(bool value) { resource.iconbar = value; }
#ifdef		HAVE_GETPID
	inline const Atom &getFluxboxPidAtom(void) const { return fluxbox_pid; }
	#ifdef KDE
	//For KDE dock applets
	inline const Atom &getKWM1DockwindowAtom(void) const { return kwm1_dockwindow; } //KDE v1.x
	inline const Atom &getKWM2DockwindowAtom(void) const { return kwm2_dockwindow; } //KDE v2.x
	#endif
#endif // HAVE_GETPID

	Basemenu *searchMenu(Window);

	FluxboxWindow *searchGroup(Window, FluxboxWindow *);
	FluxboxWindow *searchWindow(Window);
	inline FluxboxWindow *getFocusedWindow(void) { return focused_window; }

		
	BScreen *searchScreen(Window);

	inline const Time &getDoubleClickInterval(void) const
		{ return resource.double_click_interval; }
	inline const Time &getLastTime(void) const { return last_time; }

	Toolbar *searchToolbar(Window);
	Tab *searchTab(Window);
	
	enum Titlebar{SHADE=0, MINIMIZE, MAXIMIZE, CLOSE, STICK, MENU, EMPTY};		
	
	inline const std::vector<Fluxbox::Titlebar>& getTitlebarRight() { return titlebar.right; }
	inline const std::vector<Fluxbox::Titlebar>& getTitlebarLeft() { return titlebar.left; }
	inline const char *getStyleFilename(void) const
		{ return resource.style_file; }

	inline const char *getMenuFilename(void) const
		{ return resource.menu_file; }

	inline const int &getColorsPerChannel(void) const
		{ return resource.colors_per_channel; }

	inline const timeval &getAutoRaiseDelay(void) const
		{ return resource.auto_raise_delay; }

	inline const unsigned long &getCacheLife(void) const
		{ return resource.cache_life; }
	inline const unsigned long &getCacheMax(void) const
		{ return resource.cache_max; }

	inline void maskWindowEvents(Window w, FluxboxWindow *bw)
		{ masked = w; masked_window = bw; }
	inline void setNoFocus(Bool f) { no_focus = f; }

	void setFocusedWindow(FluxboxWindow *w);
	void shutdown(void);
	void load_rc(BScreen *);
	void loadRootCommand(BScreen *);
	void loadTitlebar();
	void saveStyleFilename(const char *);
	void saveMenuFilename(const char *);
	void saveTitlebarFilename(const char *);
	void saveMenuSearch(Window, Basemenu *);
	void saveWindowSearch(Window, FluxboxWindow *);
	void saveToolbarSearch(Window, Toolbar *);
	void saveTabSearch(Window, Tab *);
	void saveGroupSearch(Window, FluxboxWindow *);	
	void removeMenuSearch(Window);
	void removeWindowSearch(Window);
	void removeToolbarSearch(Window);
	void removeTabSearch(Window);
	void removeGroupSearch(Window);
	void restart(const char * = 0);
	void reconfigure(void);
	void reconfigureTabs(void);
	void rereadMenu(void);
	void checkMenu(void);

	virtual Bool handleSignal(int);

	virtual void timeout(void);

#ifdef		SLIT
	Slit *searchSlit(Window);

	void saveSlitSearch(Window, Slit *);
	void removeSlitSearch(Window);
#endif // SLIT

#ifndef	 HAVE_STRFTIME

	enum { B_AMERICANDATE = 1, B_EUROPEANDATE };
#endif // HAVE_STRFTIME
	
	template <class Z>
	class DataSearch {
	private:
		Window window;
		Z *data;


	public:
		DataSearch(Window w, Z *d) { window = w; data = d; }

		inline const Window &getWindow(void) const { return window; }
		inline Z *getData(void) { return data; }
	};
	
		
private:
	typedef struct MenuTimestamp {
		char *filename;
		time_t timestamp;
	} MenuTimestamp;

	struct resource {
		Time double_click_interval;

		char *menu_file, *style_file, *titlebar_file, *keys_file;
		int colors_per_channel;
		timeval auto_raise_delay;
		unsigned long cache_life, cache_max;
		bool tabs, iconbar;
	} resource;
	
	struct titlebar_t {
		std::vector<Fluxbox::Titlebar> left;
		std::vector<Fluxbox::Titlebar> right;
	};
	
	titlebar_t titlebar;
	std::vector<std::string> parseTitleArgs(const char *arg);
	void setTitlebar(std::vector<Fluxbox::Titlebar>& dir, const char *arg);
	
	typedef DataSearch<FluxboxWindow> WindowSearch;
	LinkedList<WindowSearch> *windowSearchList, *groupSearchList;
	typedef DataSearch<Basemenu> MenuSearch;
	LinkedList<MenuSearch> *menuSearchList;
	typedef DataSearch<Toolbar> ToolbarSearch;
	LinkedList<ToolbarSearch> *toolbarSearchList;
	typedef DataSearch<Tab> TabSearch;
	LinkedList<TabSearch> *tabSearchList;
	
#ifdef		SLIT
	typedef DataSearch<Slit> SlitSearch;
	LinkedList<SlitSearch> *slitSearchList;
  #ifdef KDE
	//For KDE dock applets
	Atom kwm1_dockwindow; //KDE v1.x
	Atom kwm2_dockwindow; //KDE v2.x
	#endif//KDE
#endif // SLIT

	LinkedList<MenuTimestamp> *menuTimestamps;
	LinkedList<BScreen> *screenList;

	FluxboxWindow *focused_window, *masked_window;
	BTimer *timer;

#ifdef		HAVE_GETPID
	Atom fluxbox_pid;
#endif // HAVE_GETPID

	Bool no_focus, reconfigure_wait, reread_menu_wait;
	Time last_time;
	Window masked;
	char *rc_file, **argv;
	int argc;
	Keys *key;
	void doWindowAction(Keys::KeyAction action);
protected:
	Fluxbox(int, char **, char * = 0, char * = 0);
	char *getRcFilename();
	void load_rc(void);
	void save_rc(void);
	void reload_rc(void);
	void real_rereadMenu(void);
	void real_reconfigure(void);

	virtual void process_event(XEvent *);
	//only main should be able to creat new blackbox object
	friend int main(int,char **);
	static Fluxbox *singleton;	//singleton object ( can only be destroyed by main )
	virtual ~Fluxbox(void);

};


#endif // _FLUXBOX_HH_
