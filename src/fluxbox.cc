// fluxbox.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// blackbox.cc for blackbox - an X11 Window manager
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

// $Id: fluxbox.cc,v 1.29 2002/02/07 14:23:01 fluxgen Exp $

//Use some GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Basemenu.hh"
#include "Clientmenu.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef SLIT
#include "Slit.hh"
#endif // SLIT

#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "StringUtil.hh"
#include "Resource.hh"
#include "XrmDatabaseHelper.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef		SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE


#ifdef		HAVE_STDIO_H
#	include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef		STDC_HEADERS
#	include <stdlib.h>
#	include <string.h>
#endif // STDC_HEADERS

#ifdef		HAVE_UNISTD_H
#	include <sys/types.h>
#	include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef		HAVE_SYS_PARAM_H
#	include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef	 MAXPATHLEN
#define	 MAXPATHLEN 255
#endif // MAXPATHLEN

#ifdef		HAVE_SYS_SELECT_H
#	include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef		HAVE_SIGNAL_H
#	include <signal.h>
#endif // HAVE_SIGNAL_H

#ifdef		HAVE_SYS_SIGNAL_H
#	include <sys/signal.h>
#endif // HAVE_SYS_SIGNAL_H

#ifdef		HAVE_SYS_STAT_H
#	include <sys/types.h>
#	include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

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

#ifdef		HAVE_LIBGEN_H
#	include <libgen.h>
#endif // HAVE_LIBGEN_H

#include <iostream>
#include <string>
#include <strstream>
#include <memory>

using namespace std;

#ifndef	 HAVE_BASENAME
static inline char *basename (char *);
static inline char *basename (char *s) {
	char *save = s;

	while (*s) if (*s++ == '/') save = s;

	return save;
}
#endif // HAVE_BASENAME

#define RC_PATH "fluxbox"
#define RC_INIT_FILE "init"


// X event scanner for enter/leave notifies - adapted from twm
typedef struct scanargs {
	Window w;
	Bool leave, inferior, enter;
} scanargs;

static Bool queueScanner(Display *, XEvent *e, char *args) {
	if ((e->type == LeaveNotify) &&
			(e->xcrossing.window == ((scanargs *) args)->w) &&
			(e->xcrossing.mode == NotifyNormal)) {
		((scanargs *) args)->leave = True;
		((scanargs *) args)->inferior = (e->xcrossing.detail == NotifyInferior);
	} else if ((e->type == EnterNotify) &&
			(e->xcrossing.mode == NotifyUngrab))
		((scanargs *) args)->enter = True;

	return False;
}
//-----------------------------------------------------------------
//---- accessors for int, bool, and some enums with Resource ------
//-----------------------------------------------------------------
template<>
void Resource<int>::
setFromString(const char* strval) {
	int val;
	if (sscanf(strval, "%d", &val)==1)
		*this = val;
}

template<>
void Resource<std::string>::
setFromString(const char *strval) {
	*this = strval;
}

template<>
void Resource<bool>::
setFromString(char const *strval) {
	if (strcasecmp(strval, "true")==0)
		*this = true;
	else
		*this = false;
}

template<>
void Resource<Fluxbox::TitlebarList>::
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
void Resource<unsigned int>::
setFromString(const char *strval) {	
	if (sscanf(strval, "%ul", &m_value) != 1)
		setDefaultValue();
}

//-----------------------------------------------------------------
//---- manipulators for int, bool, and some enums with Resource ---
//-----------------------------------------------------------------
template<>
std::string Resource<bool>::
getString() {				
	return std::string(**this == true ? "true" : "false");
}

template<>
std::string Resource<int>::
getString() {
	char strval[256];
	sprintf(strval, "%d", **this);
	return std::string(strval);
}

template<>
std::string Resource<std::string>::
getString() { return **this; }

template<>
std::string Resource<Fluxbox::TitlebarList>::
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
string Resource<unsigned int>::
getString() {
	char tmpstr[128];
	sprintf(tmpstr, "%ul", m_value);
	return string(tmpstr);
}

//static singleton var
Fluxbox *Fluxbox::singleton=0;

//------------ instance ---------------------
//returns singleton object of blackbox class
//since we only need to create one instance of Fluxbox
//-------------------------------------------
Fluxbox *Fluxbox::instance(int m_argc, char **m_argv, char *dpy_name, char *rc) {
	return singleton;
}


Fluxbox::Fluxbox(int m_argc, char **m_argv, char *dpy_name, char *rc)
: BaseDisplay(m_argv[0], dpy_name),
m_resourcemanager(), m_screen_rm(),
m_rc_tabs(m_resourcemanager, true, "session.tabs", "Session.Tabs"),
m_rc_iconbar(m_resourcemanager, true, "session.iconbar", "Session.Iconbar"),
m_rc_colors_per_channel(m_resourcemanager, 4, "session.colorsPerChannel", "Session.ColorsPerChannel"),
m_rc_stylefile(m_resourcemanager, "", "session.styleFile", "Session.StyleFile"),
m_rc_menufile(m_resourcemanager, DEFAULTMENU, "session.menuFile", "Session.MenuFile"),
m_rc_keyfile(m_resourcemanager, DEFAULTKEYSFILE, "session.keyFile", "Session.KeyFile"),
m_rc_titlebar_left(m_resourcemanager, TitlebarList(0), "session.titlebar.left", "Session.Titlebar.Left"),
m_rc_titlebar_right(m_resourcemanager, TitlebarList(0), "session.titlebar.right", "Session.Titlebar.Right"),
m_rc_cache_life(m_resourcemanager, 5, "session.cacheLife", "Session.CacheLife"),
m_rc_cache_max(m_resourcemanager, 200, "session.cacheMax", "Session.CacheMax"),
focused_window(0),
masked_window(0),
no_focus(false),
rc_file(rc),
argv(m_argv), argc(m_argc), 
key(0)
{

	//singleton pointer
	singleton = this;
	BaseDisplay::GrabGuard gg(*this);
	gg.grab();

	if (! XSupportsLocale())
		fprintf(stderr, "X server does not support locale\n");

	if (XSetLocaleModifiers("") == NULL)
		fprintf(stderr, "cannot set locale modifiers\n");

// Set default values to member variables

	resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec = 0;
	
	masked = None;
	
	windowSearchList = new LinkedList<WindowSearch>;
	menuSearchList = new LinkedList<MenuSearch>;

#ifdef SLIT
	slitSearchList = new LinkedList<SlitSearch>;
	#ifdef KDE
	//For KDE dock applets
	kwm1_dockwindow = XInternAtom(getXDisplay(), "KWM_DOCKWINDOW", False); //KDE v1.x
	kwm2_dockwindow = XInternAtom(getXDisplay(), "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False); //KDE v2.x
	#endif //KDE

#endif // SLIT

	toolbarSearchList = new LinkedList<ToolbarSearch>;
	tabSearchList = new LinkedList<TabSearch>;
	groupSearchList = new LinkedList<WindowSearch>;

	menuTimestamps = new LinkedList<MenuTimestamp>;


	#ifdef HAVE_GETPID
	fluxbox_pid = XInternAtom(getXDisplay(), "_BLACKBOX_PID", False);
	#endif // HAVE_GETPID

	screenList = new LinkedList<BScreen>;
	int i;
	load_rc();
	//allocate screens
	for (i = 0; i < getNumberOfScreens(); i++) {
		char scrname[128], altscrname[128];
		sprintf(scrname, "session.screen%d", i);
		sprintf(altscrname, "session.Screen%d", i);
		BScreen *screen = new BScreen(m_screen_rm, this, scrname, altscrname, i);

		if (! screen->isScreenManaged()) {
			delete screen;
			continue;
		}

		screenList->insert(screen);
	}
	
	I18n *i18n = I18n::instance();
	if (! screenList->count()) {
		fprintf(stderr,
			i18n->
				getMessage(
#ifdef		NLS
				blackboxSet, blackboxNoManagableScreens,
#else // !NLS
				 0, 0,
#endif // NLS
				"Fluxbox::Fluxbox: no managable screens found, aborting.\n"));

		throw static_cast<int>(3);
	}

	XSynchronize(getXDisplay(), False);
	XSync(getXDisplay(), False);

	reconfigure_wait = reread_menu_wait = false;

	timer = new BTimer(this, this);
	timer->setTimeout(0);
	timer->fireOnce(True);

	//create keybindings handler and load keys file
	key = new Keys(getXDisplay(), const_cast<char *>((*m_rc_keyfile).c_str()));

	ungrab();
}


Fluxbox::~Fluxbox(void) {
	
	while (screenList->count())
		delete screenList->remove(0);

	while (menuTimestamps->count()) {
		MenuTimestamp *ts = menuTimestamps->remove(0);

		if (ts->filename)
			delete [] ts->filename;

		delete ts;
	}
	
	delete key;
	key = 0;

	delete timer;

	delete screenList;
	delete menuTimestamps;

	delete windowSearchList;
	delete menuSearchList;
	delete toolbarSearchList;
	delete tabSearchList;
	delete groupSearchList;

#ifdef		SLIT
	delete slitSearchList;
#endif // SLIT
}


void Fluxbox::process_event(XEvent *e) {

	if ((masked == e->xany.window) && masked_window &&
			(e->type == MotionNotify)) {
		last_time = e->xmotion.time;
		masked_window->motionNotifyEvent(&e->xmotion);

		return;
	}
	
	switch (e->type) {
	case ButtonRelease:
	case ButtonPress:
		handleButtonEvent(e->xbutton);
	break;	
	case ConfigureRequest:
	{
		FluxboxWindow *win = (FluxboxWindow *) 0;

		#ifdef SLIT
		Slit *slit = (Slit *) 0;
		#endif // SLIT
		
		if ((win = searchWindow(e->xconfigurerequest.window))) {
			win->configureRequestEvent(&e->xconfigurerequest);

		#ifdef SLIT
		} else if ((slit = searchSlit(e->xconfigurerequest.window))) {
			slit->configureRequestEvent(&e->xconfigurerequest);
		#endif // SLIT

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

				XConfigureWindow(getXDisplay(), e->xconfigurerequest.window,
							e->xconfigurerequest.value_mask, &xwc);
			}

			ungrab();
		}

	}
	break;
	case MapRequest:
	{
		#ifdef		DEBUG
		fprintf(stderr,
			I18n::instance()->
				getMessage(
				#ifdef NLS
				 blackboxSet, blackboxMapRequest,
				#else // !NLS
				 0, 0,
				#endif // NLS
				 "Fluxbox::process_event(): MapRequest for 0x%lx\n"),
							e->xmaprequest.window);
		#endif // DEBUG
		
		#ifdef SLIT
		#ifdef KDE
		//Check and see if client is KDE dock applet.
		//If so add to Slit
		bool iskdedockapp = False;
		Atom ajunk;
		int ijunk;
		unsigned long *data = (unsigned long *) 0, uljunk;

		// Check if KDE v2.x dock applet
		if (XGetWindowProperty(getXDisplay(), e->xmaprequest.window,
				getKWM2DockwindowAtom(), 0l, 1l, False,
				XA_WINDOW, &ajunk, &ijunk, &uljunk,
				&uljunk, (unsigned char **) &data) == Success) {
					
			if (data)
				iskdedockapp = True;
			XFree((char *) data);
	
		}

		// Check if KDE v1.x dock applet
		if (!iskdedockapp) {
			if (XGetWindowProperty(getXDisplay(), e->xmaprequest.window,
					getKWM1DockwindowAtom(), 0l, 1l, False,
					getKWM1DockwindowAtom(), &ajunk, &ijunk, &uljunk,
					&uljunk, (unsigned char **) &data) == Success) {
				iskdedockapp = (data && data[0] != 0);
				XFree((char *) data);
			}
		}

		if (iskdedockapp) {
			XSelectInput(getXDisplay(), e->xmaprequest.window, StructureNotifyMask);
			LinkedListIterator<BScreen> it(screenList);
			for (; it.current() == screenList->last(); it++) {
				BScreen *screen = it.current();
				screen->getSlit()->addClient(e->xmaprequest.window);
			}
			return;
		}
		#endif //KDE
		#endif // SLIT
			
		FluxboxWindow *win = searchWindow(e->xmaprequest.window);

		if (! win) {
			try {
				win = new FluxboxWindow(e->xmaprequest.window);
			} catch (FluxboxWindow::Error error) {
				FluxboxWindow::showError(error);
				delete win;
				win = 0;
			}
		}
			
		if ((win = searchWindow(e->xmaprequest.window)))
			win->mapRequestEvent(&e->xmaprequest);

	}
	break;
	case MapNotify:
	{
		FluxboxWindow *win = searchWindow(e->xmap.window);
		XMapWindow(getXDisplay(), e->xmap.window);
		if (win!=0)
			win->mapNotifyEvent(&e->xmap);

	}
	break;
	

	case UnmapNotify:
	{

		FluxboxWindow *win = 0;
		#ifdef DEBUG 
		cerr<<__FILE__<<"("<<__LINE__<<"): Unmapnotify 0x"<<hex<<
			e->xunmap.window<<dec<<endl;
		#endif
		#ifdef SLIT
		Slit *slit = (Slit *) 0;
		#endif // SLIT

		if ((win = searchWindow(e->xunmap.window))!=0 ) {
			// only process windows with StructureNotify selected 
	     	// (ignore SubstructureNotify)				
			if (win->getClientWindow() != e->xunmap.window ||
					win->isTransient()) {
				win->unmapNotifyEvent(&e->xunmap);
			}
		#ifdef SLIT
		} else if ((slit = searchSlit(e->xunmap.window))!=0) {
			slit->removeClient(e->xunmap.window);
			#ifdef DEBUG
			cerr<<__FILE__<<"("<<__LINE__<<"): Here"<<endl;
			#endif 
		#endif // SLIT
		}
	}
	break;	
	case CreateNotify:
		break;
	case DestroyNotify:
		{
			FluxboxWindow *win = (FluxboxWindow *) 0;

			#ifdef SLIT
			Slit *slit = 0;
			#endif // SLIT

			if ((win = searchWindow(e->xdestroywindow.window))) {
				if (win->destroyNotifyEvent(&e->xdestroywindow)) {
					delete win;
					win = 0;
				}
			#ifdef SLIT
			} else if ((slit = searchSlit(e->xdestroywindow.window))) {
				slit->removeClient(e->xdestroywindow.window, False);
			#endif // SLIT
			}

		}
	break;
	case MotionNotify:
		{
			last_time = e->xmotion.time;

			FluxboxWindow *win = 0;
			Basemenu *menu = 0;
			Tab *tab = 0;
			
			if ((win = searchWindow(e->xmotion.window)) !=0)
				win->motionNotifyEvent(&e->xmotion);
			else if ((menu = searchMenu(e->xmotion.window)) !=0)
				menu->motionNotifyEvent(&e->xmotion);
			else if ((tab = searchTab(e->xmotion.window)) !=0)
				tab->motionNotifyEvent(&e->xmotion);
				
			
		}
	break;
	case PropertyNotify:
		{
			last_time = e->xproperty.time;

			if (e->xproperty.state != PropertyDelete) {
				FluxboxWindow *win = searchWindow(e->xproperty.window);

				if (win)
					win->propertyNotifyEvent(e->xproperty.atom);
			}
			
		}
	break;
	case EnterNotify:
		{
			last_time = e->xcrossing.time;

			BScreen *screen = (BScreen *) 0;
			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;
			Toolbar *tbar = (Toolbar *) 0;
			Tab *tab = (Tab *) 0;
			#ifdef		SLIT
			Slit *slit = (Slit *) 0;
			#endif // SLIT

			if (e->xcrossing.mode == NotifyGrab)
				break;

			XEvent dummy;
			scanargs sa;
			sa.w = e->xcrossing.window;
			sa.enter = sa.leave = False;
			XCheckIfEvent(getXDisplay(), &dummy, queueScanner, (char *) &sa);

			if ((e->xcrossing.window == e->xcrossing.root) &&
					(screen = searchScreen(e->xcrossing.window))) {
				screen->getImageControl()->installRootColormap();
			} else if ((win = searchWindow(e->xcrossing.window))) {
				if ((win->getScreen()->isSloppyFocus() ||
						win->getScreen()->isSemiSloppyFocus()) &&
						(! win->isFocused()) && (! no_focus)) {

					grab();

					if (((! sa.leave) || sa.inferior) && win->isVisible() &&
							win->setInputFocus())
						win->installColormap(True);

					ungrab();
				}
			} else if ((menu = searchMenu(e->xcrossing.window)))
				menu->enterNotifyEvent(&e->xcrossing);
			else if ((tbar = searchToolbar(e->xcrossing.window)))
				tbar->enterNotifyEvent(&e->xcrossing);
			else if ((tab = searchTab(e->xcrossing.window))) {
				win = tab->getWindow();
				if (win->getScreen()->isSloppyFocus() && (! win->isFocused()) &&
						(! no_focus)) {
					win->getScreen()->getWorkspace(win->getWorkspaceNumber())->raiseWindow(win);
					
					grab();

					if (((! sa.leave) || sa.inferior) && win->isVisible() &&
							win->setInputFocus())
						win->installColormap(True);

					ungrab();
				}
			}
				
#ifdef		SLIT
			else if ((slit = searchSlit(e->xcrossing.window)))
				slit->enterNotifyEvent(&e->xcrossing);
#endif // SLIT

			
		}
	break;
	case LeaveNotify:
		{
			last_time = e->xcrossing.time;

			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;
			Toolbar *tbar = (Toolbar *) 0;
			
			#ifdef SLIT
			Slit *slit = (Slit *) 0;
			#endif // SLIT

			if ((menu = searchMenu(e->xcrossing.window)))
				menu->leaveNotifyEvent(&e->xcrossing);
			else if ((win = searchWindow(e->xcrossing.window)))
				win->installColormap(False);
			else if ((tbar = searchToolbar(e->xcrossing.window)))
				tbar->leaveNotifyEvent(&e->xcrossing);
			#ifdef SLIT
			else if ((slit = searchSlit(e->xcrossing.window)))
				slit->leaveNotifyEvent(&e->xcrossing);
			#endif // SLIT
			
			
		}
	break;
	case Expose:
		{
			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;
			Toolbar *tbar = (Toolbar *) 0;
			Tab *tab = 0;
			
			if ((win = searchWindow(e->xexpose.window)))
				win->exposeEvent(&e->xexpose);
			else if ((menu = searchMenu(e->xexpose.window)))
				menu->exposeEvent(&e->xexpose);
			else if ((tbar = searchToolbar(e->xexpose.window)))
				tbar->exposeEvent(&e->xexpose);
			else if ((tab = searchTab(e->xexpose.window)))
				tab->exposeEvent(&e->xexpose);
			
		}
	break;
	case KeyPress:
		handleKeyEvent(e->xkey);
	break;
	case ColormapNotify:
	{
		BScreen *screen = searchScreen(e->xcolormap.window);

		if (screen)
		screen->setRootColormapInstalled((e->xcolormap.state ==
				ColormapInstalled) ? True : False);

	
	}
	break;
	case FocusIn:
	{
		if (e->xfocus.mode == NotifyUngrab ||
				e->xfocus.detail == NotifyPointer)
			break;

		FluxboxWindow *win = searchWindow(e->xfocus.window);
		if (win && ! win->isFocused())
			setFocusedWindow(win);
	
	}
	break;
	case FocusOut:
	break;

	case ClientMessage:
		handleClientMessage(e->xclient);
	break;
	default:
		{

		#ifdef SHAPE
		if (e->type == getShapeEventBase()) {
			XShapeEvent *shape_event = (XShapeEvent *) e;
			FluxboxWindow *win = (FluxboxWindow *) 0;

			if ((win = searchWindow(e->xany.window)) ||
					(shape_event->kind != ShapeBounding))
				win->shapeEvent(shape_event);
			}
		#endif // SHAPE

		}
	}
}

void Fluxbox::handleButtonEvent(XButtonEvent &be) {
	switch (be.type) {
	case ButtonPress:
	{
		last_time = be.time;

		FluxboxWindow *win = (FluxboxWindow *) 0;
		Basemenu *menu = (Basemenu *) 0;

		#ifdef		SLIT
		Slit *slit = (Slit *) 0;
		#endif // SLIT

		Toolbar *tbar = (Toolbar *) 0;			
		Tab *tab = 0;

		if ((win = searchWindow(be.window))) {

			win->buttonPressEvent(&be);
				
			if (be.button == 1)
				win->installColormap(True);
					
		} else if ((menu = searchMenu(be.window))) {
			menu->buttonPressEvent(&be);

		#ifdef		SLIT
		} else if ((slit = searchSlit(be.window))) {
			slit->buttonPressEvent(&be);
		#endif // SLIT

		} else if ((tbar = searchToolbar(be.window))) {
			tbar->buttonPressEvent(&be);
		} else if ((tab = searchTab(be.window))) {
			tab->buttonPressEvent(&be);
		} else {
			LinkedListIterator<BScreen> it(screenList);

			for (; it.current(); it++) {

				BScreen *screen = it.current();
				if (be.window != screen->getRootWindow())
					continue;
				
				
				if (be.button == 1) {
					if (! screen->isRootColormapInstalled())
						screen->getImageControl()->installRootColormap();

					if (screen->getWorkspacemenu()->isVisible())
						screen->getWorkspacemenu()->hide();
					if (screen->getRootmenu()->isVisible())
						screen->getRootmenu()->hide();
						
				} else if (be.button == 2) {
					int mx = be.x_root -
						(screen->getWorkspacemenu()->getWidth() / 2);
					int my = be.y_root -
						(screen->getWorkspacemenu()->getTitleHeight() / 2);
	
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;

					if (mx + screen->getWorkspacemenu()->getWidth() >
						screen->getWidth()) {
						mx = screen->getWidth() -
							screen->getWorkspacemenu()->getWidth() -						
							screen->getBorderWidth();
					}

					if (my + screen->getWorkspacemenu()->getHeight() >
							screen->getHeight()) {
						my = screen->getHeight() -
							screen->getWorkspacemenu()->getHeight() -
							screen->getBorderWidth();
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
						(screen->getRootmenu()->getWidth() / 2);
					int my = be.y_root -
						(screen->getRootmenu()->getTitleHeight() / 2);

					if (mx < 0) mx = 0;
					if (my < 0) my = 0;

					if (mx + screen->getRootmenu()->getWidth() > screen->getWidth()) {
						mx = screen->getWidth() -
							screen->getRootmenu()->getWidth() -
							screen->getBorderWidth();
					}

					if (my + screen->getRootmenu()->getHeight() >
							screen->getHeight()) {
						my = screen->getHeight() -
							screen->getRootmenu()->getHeight() -
							screen->getBorderWidth();
					}
					screen->getRootmenu()->move(mx, my);

					if (! screen->getRootmenu()->isVisible()) {
						checkMenu();
						screen->getRootmenu()->show();
					}
				} // end button == 3
			}
		}
	}

	break;
	case ButtonRelease:
	{
		last_time = be.time;
		FluxboxWindow *win = (FluxboxWindow *) 0;
		Basemenu *menu = (Basemenu *) 0;
		Toolbar *tbar = (Toolbar *) 0;
		Tab *tab = 0;
		
		if ((win = searchWindow(be.window)))
			win->buttonReleaseEvent(&be);
		else if ((menu = searchMenu(be.window)))
			menu->buttonReleaseEvent(&be);
		else if ((tbar = searchToolbar(be.window)))
			tbar->buttonReleaseEvent(&be);
		else if ((tab = searchTab(be.window)))
			tab->buttonReleaseEvent(&be);
	}
	break;	
	default:
	break;
	}
}

//------------ handleClientMessage --------
// Handles XClientMessageEvent
//-----------------------------------------
void Fluxbox::handleClientMessage(XClientMessageEvent &ce) {
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): ClientMessage. data.l[0]=0x"<<hex<<ce.data.l[0]<<
		"  type=0x"<<ce.message_type<<dec<<endl;
	
	#endif

	if (ce.format != 32)
		return;
	
	if (ce.message_type == getWMChangeStateAtom()) {
		FluxboxWindow *win = searchWindow(ce.window);
		if (! win || ! win->validateClient())
			return;

		if (ce.data.l[0] == IconicState)
			win->iconify();
		if (ce.data.l[0] == NormalState)
			win->deiconify();
	} else if (ce.message_type == getFluxboxChangeWorkspaceAtom()) {
		BScreen *screen = searchScreen(ce.window);

		if (screen && ce.data.l[0] >= 0 &&
				ce.data.l[0] < screen->getCount())
			screen->changeWorkspaceID(ce.data.l[0]);
				
	} else if (ce.message_type == getFluxboxChangeWindowFocusAtom()) {
		FluxboxWindow *win = searchWindow(ce.window);
		if (win && win->isVisible() && win->setInputFocus())
			win->installColormap(True);
	} else if (ce.message_type == getFluxboxCycleWindowFocusAtom()) {
		BScreen *screen = searchScreen(ce.window);

		if (screen) {
			if (! ce.data.l[0])
				screen->prevFocus();
			else
				screen->nextFocus();
		}		
	} else if (ce.message_type == getFluxboxChangeAttributesAtom()) {
		
		FluxboxWindow *win = searchWindow(ce.window);

		if (win && win->validateClient()) {
			BlackboxHints net;
			net.flags = ce.data.l[0];
			net.attrib = ce.data.l[1];
			net.workspace = ce.data.l[2];
			net.stack = ce.data.l[3];
			net.decoration = static_cast<BaseDisplay::Decor>(ce.data.l[4]);
			win->changeBlackboxHints(&net);
		}
	} else {
		bool val  = false;
		#ifdef GNOME
		val  = checkGnomeAtoms(ce);
		#endif //!GNOME
	
		#ifdef NEWWMSPEC
		if (!val)
			val = checkNETWMAtoms(ce);
		#endif //!NEWWMSPEC
	}
}
//----------- handleKeyEvent ---------------
// Handles KeyRelease and KeyPress events
//------------------------------------------
void Fluxbox::handleKeyEvent(XKeyEvent &ke) {
	switch (ke.type) {
	case KeyPress:
	{
		Toolbar *tbar = searchToolbar(ke.window);
		BScreen *screen = searchScreen(ke.window);
			
		if (tbar && tbar->isEditing())
			tbar->keyPressEvent(&ke);
		else if (screen)	{
			#ifdef DEBUG
			cerr<<"KeyEvent"<<endl;
			#endif
			//find action
			Keys::KeyAction action = key->getAction(&ke);
			#ifdef DEBUG
			const char *actionstr = key->getActionStr(action);
			if (actionstr)
				cerr<<"KeyAction("<<actionstr<<")"<<endl;				
			#endif
			if (action==Keys::LASTKEYGRAB) //if action not found end case
				break;

			switch (action) {					
			case Keys::WORKSPACE1:
				screen->changeWorkspaceID(0);
			break;
			case Keys::WORKSPACE2:
				screen->changeWorkspaceID(1);
			break;
			case Keys::WORKSPACE3:
				screen->changeWorkspaceID(2);
			break;
			case Keys::WORKSPACE4:
				screen->changeWorkspaceID(3);
			break;
			case Keys::WORKSPACE5:
				screen->changeWorkspaceID(4);
			break;
			case Keys::WORKSPACE6:
				screen->changeWorkspaceID(5);
			break;
			case Keys::WORKSPACE7:
				screen->changeWorkspaceID(6);
			break;
			case Keys::WORKSPACE8:
				screen->changeWorkspaceID(7);
			break;
			case Keys::WORKSPACE9:
				screen->changeWorkspaceID(8);
			break;
			case Keys::WORKSPACE10:
				screen->changeWorkspaceID(9);
			break;
			case Keys::WORKSPACE11:
				screen->changeWorkspaceID(10);
			break;
			case Keys::WORKSPACE12:
				screen->changeWorkspaceID(11);
			break;
			case Keys::NEXTWORKSPACE:
				screen->nextWorkspace();
			break;
			case Keys::PREVWORKSPACE:
				screen->prevWorkspace();
			break;
			case Keys::LEFTWORKSPACE:
				screen->leftWorkspace();
			break;
			case Keys::RIGHTWORKSPACE:
				screen->rightWorkspace();
			break;
			case Keys::KILLWINDOW: //kill the current window
				XKillClient(screen->getBaseDisplay()->getXDisplay(),
					focused_window->getClientWindow());
			break;
			case Keys::NEXTWINDOW:	//activate next window
				screen->nextFocus();
			break;
			case Keys::PREVWINDOW:	//activate prev window
				screen->prevFocus();
			break;
			case Keys::NEXTTAB: 
				if (focused_window && focused_window->getTab()) {
					Tab *tab = focused_window->getTab();
					if (tab->next()) {
						screen->getCurrentWorkspace()->raiseWindow(
							tab->next()->getWindow());
						tab->next()->getWindow()->setInputFocus();
					} else {
						screen->getCurrentWorkspace()->raiseWindow(
							tab->first()->getWindow());
						tab->first()->getWindow()->setInputFocus();
					}	
				}
			break;						
			case Keys::PREVTAB: 
				if (focused_window && focused_window->getTab()) {
					Tab *tab = focused_window->getTab();
					if (tab->prev()) {
						screen->getCurrentWorkspace()->raiseWindow(
							tab->prev()->getWindow());
						tab->prev()->getWindow()->setInputFocus();
					} else {
						screen->getCurrentWorkspace()->raiseWindow(
							tab->last()->getWindow());
						tab->last()->getWindow()->setInputFocus();
					}
				}
			break;
			case Keys::EXECUTE: //execute command on keypress
			{
				#ifndef		__EMX__
				char displaystring[MAXPATHLEN];
				sprintf(displaystring, "DISPLAY=%s",
					DisplayString(getXDisplay()));
				sprintf(displaystring + strlen(displaystring) - 1, "%d",
					screen->getScreenNumber());		
				#ifdef DEBUG
				cerr<<__FILE__<<"("<<__LINE__<<"): Executing:"<<key->getExecCommand().c_str()<<endl;
				#endif						
						
				bexec(key->getExecCommand().c_str(), displaystring);
				#else
				spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
				#endif // !__EMX__
						
			
			}
			break;
			default: //try to see if its a window action
				doWindowAction(action);
			}
		}
	break;
	}
	
	default:
	break;
	}
	
	
}
void Fluxbox::doWindowAction(Keys::KeyAction action) {
	if (!focused_window)
		return;

	unsigned int t_placement = focused_window->getScreen()->getTabPlacement();
	unsigned int t_alignment = focused_window->getScreen()->getTabAlignment();
	
	switch (action) {
		case Keys::ICONIFY:
			focused_window->iconify();
		break;
		case Keys::RAISE:
			if (focused_window->hasTab())
				focused_window->getTab()->raise(); //raise the tabs if we have any
			focused_window->getScreen()->getWorkspace(focused_window->getWorkspaceNumber())->raiseWindow(focused_window);
		break;
		case Keys::LOWER:
 	   	focused_window->getScreen()->getWorkspace(focused_window->getWorkspaceNumber())->lowerWindow(focused_window);
			if (focused_window->hasTab())
				focused_window->getTab()->lower(); //lower the tabs AND it's windows

		break;
		case Keys::CLOSE:
			focused_window->close();
		break;
		case Keys::SHADE:		
			if (focused_window->hasTab())
				focused_window->getTab()->shade();
			focused_window->shade();
		break;
		case Keys::MAXIMIZE:
			focused_window->maximize(0);
		break;
		case Keys::STICK:
			focused_window->stick();
		break;								
		case Keys::VERTMAX:
			if (focused_window->isResizable())
				focused_window->maximize(3); // maximize vertically, done with mouse3
		break;
		case Keys::HORIZMAX:
			if (focused_window->isResizable())
				focused_window->maximize(2); // maximize horisontally, done with mouse2
		break;
		case Keys::NUDGERIGHT:	
			focused_window->configure(
				focused_window->getXFrame()+1, focused_window->getYFrame(),
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::NUDGELEFT:			
			focused_window->configure(
				focused_window->getXFrame()-1, focused_window->getYFrame(),
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::NUDGEUP:
			focused_window->configure(
				focused_window->getXFrame(), focused_window->getYFrame()-1,
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::NUDGEDOWN:
			focused_window->configure(
				focused_window->getXFrame(), focused_window->getYFrame()+1,
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::BIGNUDGERIGHT:		
			focused_window->configure(
				focused_window->getXFrame()+10, focused_window->getYFrame(),
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::BIGNUDGELEFT:						
			focused_window->configure(
				focused_window->getXFrame()-10, focused_window->getYFrame(),
				focused_window->getWidth(), focused_window->getHeight());
		break;
		case Keys::BIGNUDGEUP:								
			focused_window->configure(
				focused_window->getXFrame(), focused_window->getYFrame()-10,
				focused_window->getWidth(), focused_window->getHeight());
		break;								
		case Keys::BIGNUDGEDOWN:					
			focused_window->configure(
				focused_window->getXFrame(), focused_window->getYFrame()+10,
				focused_window->getWidth(), focused_window->getHeight());								
		break;												
		case Keys::HORIZINC:
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth()+10, focused_window->getHeight());

				if (focused_window->hasTab() &&
						(t_placement == Tab::PTOP || t_placement == Tab::PBOTTOM)) {
					if (t_alignment == Tab::ARELATIVE)
						focused_window->getTab()->calcIncrease();
					if (t_alignment != Tab::PLEFT)
						focused_window->getTab()->setPosition();
				}
		break;								
		case Keys::VERTINC:
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth(), focused_window->getHeight()+10);

				if (focused_window->hasTab() &&
						(t_placement == Tab::PLEFT || t_placement == Tab::PRIGHT)) {
					if (t_alignment == Tab::ARELATIVE)
						focused_window->getTab()->calcIncrease();
					if (t_alignment != Tab::PRIGHT)
						focused_window->getTab()->setPosition();
				}
		break;
		case Keys::HORIZDEC:				
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth()-10, focused_window->getHeight());

				if (focused_window->hasTab() &&
						(t_placement == Tab::PTOP || t_placement == Tab::PBOTTOM)) {
					if (t_alignment == Tab::ARELATIVE)
						focused_window->getTab()->calcIncrease();
					if (t_alignment != Tab::PLEFT)
						focused_window->getTab()->setPosition();
				}
		break;								
		case Keys::VERTDEC:
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth(), focused_window->getHeight()-10);

				if (focused_window->hasTab() &&
						(t_placement == Tab::PLEFT || t_placement == Tab::PRIGHT)) {
					if (t_alignment == Tab::ARELATIVE)
						focused_window->getTab()->calcIncrease();
					if (t_alignment != Tab::PRIGHT)
						focused_window->getTab()->setPosition();
				}
		break;		
		default:	
		break;							
	}

}
#ifdef GNOME
//---------------- checkGnomeAtoms ---------------
// Tries to find Gnome atoms in message
// Returns true on success else false
//---------------------------------------------
bool Fluxbox::checkGnomeAtoms(XClientMessageEvent &ce) {
	if (ce.message_type == getGnomeWorkspaceAtom()) {
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): Got workspace atom="<<ce.data.l[0]<<endl;
		#endif//!DEBUG
		BScreen *screen = 0;
		FluxboxWindow *win = 0;
					
		if ( (win = searchWindow(ce.window))!=0 && // the message sent to client window?
				win->getScreen() && ce.data.l[0] >= 0 &&
				ce.data.l[0] < win->getScreen()->getCount()) {
			win->getScreen()->changeWorkspaceID(ce.data.l[0]);
					
		} else if ((screen = searchScreen(ce.window))!=0 && //the message sent to root window?
				ce.data.l[0] >= 0 &&
				ce.data.l[0] < screen->getCount())
			screen->changeWorkspaceID(ce.data.l[0]);
		
	} else if (ce.message_type == getGnomeStateAtom()) {
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_STATE"<<endl;
		#endif
		FluxboxWindow *win = 0;

		if ((win = searchWindow(ce.window))!=0) {
			cerr<<__FILE__<<"("<<__LINE__<<"): Mask of members to change:"<<
				hex<<ce.data.l[0]<<dec<<endl; // mask_of_members_to_change
			cerr<<"New members:"<<ce.data.l[1]<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_STICKY) {
				cerr<<"Sticky"<<endl;
				if (!win->isStuck())
					win->stick();
			} else if (win->isStuck())
				win->stick();
			
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_MINIMIZED) {
				cerr<<"Minimized"<<endl;
				if (!win->isIconic())
					win->iconify();
			} else if (win->isIconic())
				win->deiconify(true, true);
			
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_MAXIMIZED_VERT)
				cerr<<"Maximize Vert"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_MAXIMIZED_HORIZ)
				cerr<<"Maximize Horiz"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_HIDDEN)
				cerr<<"Hidden"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_SHADED) {
				cerr<<"Shaded"<<endl;
				if (!win->isShaded()) win->shade();
			}
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_HID_WORKSPACE)
				cerr<<"HID Workspace"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_HID_TRANSIENT)
				cerr<<"HID Transient"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_FIXED_POSITION)
				cerr<<"Fixed Position"<<endl;
			if (ce.data.l[0] & BaseDisplay::WIN_STATE_ARRANGE_IGNORE)
				cerr<<"Arrange Ignore"<<endl;			
		}
	} if (ce.message_type == getGnomeHintsAtom()) {
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_HINTS"<<endl;
		#endif
	} else 
		return false; //no gnome atom

	return true;
}
#endif //!GNOME

#ifdef NEWWMSPEC
//----------- checkNETWMAtoms -------------------
// Tries to find NEWWM atom in clientmessage
// Returns true on success else false
//-----------------------------------------------
bool Fluxbox::checkNETWMAtoms(XClientMessageEvent &ce) {
	
	if (ce.message_type == getNETWMDesktopAtom()) { //_NET_WM_DESKTOP
		BScreen *screen = searchScreen(ce.window);

		if (screen && ce.data.l[0] >= 0 &&
			ce.data.l[0] < screen->getCount())
			screen->changeWorkspaceID(ce.data.l[0]);
	} else return false;

	return true;
}
#endif //!NEWWMSPEC

Bool Fluxbox::handleSignal(int sig) {
	switch (sig) {
	case SIGHUP:
		reconfigure();
		break;

	case SIGUSR1:
		reload_rc();
		break;

	case SIGUSR2:
		rereadMenu();
		break;

	case SIGSEGV:
	case SIGFPE:
	case SIGINT:
	case SIGTERM:
		shutdown();

	default:
		return False;
	}

	return True;
}


BScreen *Fluxbox::searchScreen(Window window) {
	BScreen *screen = (BScreen *) 0;
	LinkedListIterator<BScreen> it(screenList);

	for (; it.current(); it++) {
		if (it.current()) {
			if (it.current()->getRootWindow() == window) {
				screen = it.current();
				return screen;
			}
		}
	}

	return (BScreen *) 0;
}


FluxboxWindow *Fluxbox::searchWindow(Window window) {
	LinkedListIterator<WindowSearch> it(windowSearchList);

	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();
		if (tmp && tmp->getWindow() == window)
			return tmp->getData(); 			
	}

	return (FluxboxWindow *) 0;
}


FluxboxWindow *Fluxbox::searchGroup(Window window, FluxboxWindow *win) {
	FluxboxWindow *w = (FluxboxWindow *) 0;
	LinkedListIterator<WindowSearch> it(groupSearchList);

	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();
		if (tmp) {
			if (tmp->getWindow() == window) {
				w = tmp->getData();
				if (w->getClientWindow() != win->getClientWindow())
					return win;
			}
		}
	}

	return (FluxboxWindow *) 0;
}


Basemenu *Fluxbox::searchMenu(Window window) {
	Basemenu *menu = (Basemenu *) 0;
	LinkedListIterator<MenuSearch> it(menuSearchList);

	for (; it.current(); it++) {
		MenuSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				menu = tmp->getData();
				return menu;
			}
		}
	}

	return (Basemenu *) 0;
}


Toolbar *Fluxbox::searchToolbar(Window window) {
	Toolbar *tbar = (Toolbar *) 0;
	LinkedListIterator<ToolbarSearch> it(toolbarSearchList);

	for (; it.current(); it++) {
		ToolbarSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				tbar = tmp->getData();
				return tbar;
			}
		}
	}

	return (Toolbar *) 0;
}

Tab *Fluxbox::searchTab(Window window) {
	LinkedListIterator<TabSearch> it(tabSearchList);

	for (; it.current(); it++) {
		TabSearch *tmp = it.current();
		if (tmp && tmp->getWindow() == window)
			return tmp->getData();			
	}

	return 0;
}


#ifdef		SLIT
Slit *Fluxbox::searchSlit(Window window) {
	Slit *s = (Slit *) 0;
	LinkedListIterator<SlitSearch> it(slitSearchList);

	for (; it.current(); it++) {
		SlitSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				s = tmp->getData();
				return s;
			}
		}
	}

	return (Slit *) 0;
}
#endif // SLIT


void Fluxbox::saveWindowSearch(Window window, FluxboxWindow *data) {
	windowSearchList->insert(new WindowSearch(window, data));
}


void Fluxbox::saveGroupSearch(Window window, FluxboxWindow *data) {
	groupSearchList->insert(new WindowSearch(window, data));
}


void Fluxbox::saveMenuSearch(Window window, Basemenu *data) {
	menuSearchList->insert(new MenuSearch(window, data));
}


void Fluxbox::saveToolbarSearch(Window window, Toolbar *data) {
	toolbarSearchList->insert(new ToolbarSearch(window, data));
}


void Fluxbox::saveTabSearch(Window window, Tab *data) {
	tabSearchList->insert(new TabSearch(window, data));
}

#ifdef		SLIT
void Fluxbox::saveSlitSearch(Window window, Slit *data) {
	slitSearchList->insert(new SlitSearch(window, data));
}
#endif // SLIT


void Fluxbox::removeWindowSearch(Window window) {
	LinkedListIterator<WindowSearch> it(windowSearchList);
	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				windowSearchList->remove(tmp);
				delete tmp;
				break;
			}
		}
	}
}


void Fluxbox::removeGroupSearch(Window window) {
	LinkedListIterator<WindowSearch> it(groupSearchList);
	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				groupSearchList->remove(tmp);
				delete tmp;
				break;
			}
		}
	}
}


void Fluxbox::removeMenuSearch(Window window) {
	LinkedListIterator<MenuSearch> it(menuSearchList);
	for (; it.current(); it++) {
		MenuSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				menuSearchList->remove(tmp);
				delete tmp;
				break;
			}
		}
	}
}


void Fluxbox::removeToolbarSearch(Window window) {
	LinkedListIterator<ToolbarSearch> it(toolbarSearchList);
	for (; it.current(); it++) {
		ToolbarSearch *tmp = it.current();
		if (tmp) {
			if (tmp->getWindow() == window) {
				toolbarSearchList->remove(tmp);	
				delete tmp;
				break;
			}
		}
	}
}


void Fluxbox::removeTabSearch(Window window) {
	LinkedListIterator<TabSearch> it(tabSearchList);
	for (; it.current(); it++) {
		TabSearch *tmp = it.current();
		if (tmp && tmp->getWindow() == window) {
			tabSearchList->remove(tmp);	
			delete tmp;
			break;
		}
	}
}

#ifdef		SLIT
void Fluxbox::removeSlitSearch(Window window) {
	LinkedListIterator<SlitSearch> it(slitSearchList);
	for (; it.current(); it++) {
		SlitSearch *tmp = it.current();

		if (tmp) {
			if (tmp->getWindow() == window) {
				slitSearchList->remove(tmp);
				delete tmp;
				break;
			}
		}
	}
}
#endif // SLIT


void Fluxbox::restart(const char *prog) {
	shutdown();

	if (prog) {
		execlp(prog, prog, NULL);
		perror(prog);
	}

	// fall back in case the above execlp doesn't work
	execvp(argv[0], argv);
	execvp(basename(argv[0]), argv);
}


void Fluxbox::shutdown(void) {
	BaseDisplay::shutdown();

	XSetInputFocus(getXDisplay(), PointerRoot, None, CurrentTime);

	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++)
		it.current()->shutdown();

	XSync(getXDisplay(), False);

	save_rc();
}

//------ save_rc --------
//saves resources
//----------------------
void Fluxbox::save_rc(void) {
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): Saving resources --------------"<<endl;
	#endif
	
	XrmDatabase new_blackboxrc = 0;
	
	char rc_string[1024];

	auto_ptr<char> dbfile(getRcFilename());

	// load_rc();
	// This overwrites configs made while running, for example
	// usage of iconbar and tabs
	
	
	if (*dbfile) {
		m_resourcemanager.save(dbfile.get(), dbfile.get());
		m_screen_rm.save(dbfile.get(), dbfile.get());
	} else
		cerr<<"database filename is invalid!"<<endl;
	

	sprintf(rc_string, "session.doubleClickInterval:	%lu",
					resource.double_click_interval);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.autoRaiseDelay:	%lu",
					((resource.auto_raise_delay.tv_sec * 1000) +
					(resource.auto_raise_delay.tv_usec / 1000)));
	XrmPutLineResource(&new_blackboxrc, rc_string);

	LinkedListIterator<BScreen> it(screenList);

	//Save screen resources

	for (; it.current(); it++) {
		BScreen *screen = it.current();
		int screen_number = screen->getScreenNumber();

#ifdef		SLIT
		string slit_placement;

		switch (screen->getSlitPlacement()) {
		case Slit::TOPLEFT: slit_placement = "TopLeft"; break;
		case Slit::CENTERLEFT: slit_placement = "CenterLeft"; break;
		case Slit::BOTTOMLEFT: slit_placement = "BottomLeft"; break;
		case Slit::TOPCENTER: slit_placement = "TopCenter"; break;
		case Slit::BOTTOMCENTER: slit_placement = "BottomCenter"; break;
		case Slit::TOPRIGHT: slit_placement = "TopRight"; break;
		case Slit::BOTTOMRIGHT: slit_placement = "BottomRight"; break;
		case Slit::CENTERRIGHT: default: slit_placement = "CenterRight"; break;
		}

		sprintf(rc_string, "session.screen%d.slit.placement: %s", screen_number,
			slit_placement.c_str());
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.direction: %s", screen_number,
			((screen->getSlitDirection() == Slit::HORIZONTAL) ? "Horizontal" :
			"Vertical"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.onTop: %s", screen_number,
			((screen->getSlit()->isOnTop()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.autoHide: %s", screen_number,
			((screen->getSlit()->doAutoHide()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // SLIT

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

		default:
		case BScreen::ROWSMARTPLACEMENT:
			placement = "RowSmartPlacement";
			break;
		}
		
		sprintf(rc_string, "session.screen%d.windowPlacement: %s", screen_number,
			placement.c_str());
		XrmPutLineResource(&new_blackboxrc, rc_string);

		std::string focus_mode;
		if (screen->isSloppyFocus() && screen->doAutoRaise())
			focus_mode = "AutoRaiseSloppyFocus";
		else if (screen->isSloppyFocus())
			focus_mode = "SloppyFocus";
		else if (screen->isSemiSloppyFocus() && screen->doAutoRaise())
			focus_mode = "AutoRaiseSemiSloppyFocus";
		else if (screen->isSemiSloppyFocus())
			focus_mode = "SemiSloppyFocus";	
		else
			focus_mode = "ClickToFocus";

		sprintf(rc_string, "session.screen%d.focusModel: %s", screen_number,
			focus_mode.c_str());
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
			((screen->getDateFormat() == B_EuropeanDate) ?
				"European" : "American"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.clockFormat:	%d", screen_number,
			((screen->isClock24Hour()) ? 24 : 12));
		XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // HAVE_STRFTIME

		// write out the users workspace names
		sprintf(rc_string, "session.screen%d.workspaceNames: ", screen_number);
		string workspaces_string(rc_string);
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): workspaces="<<screen->getCount()<<endl;
		#endif
		for (int workspace=0; workspace < screen->getCount(); workspace++) {
			if (screen->getWorkspace(workspace)->getName()!=0)
				workspaces_string.append(screen->getWorkspace(workspace)->getName());
			else
				workspaces_string.append("Null");
			workspaces_string.append(", ");
		}

		XrmPutLineResource(&new_blackboxrc, workspaces_string.c_str());
	
	}

	XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.get());

	XrmMergeDatabases(new_blackboxrc, &old_blackboxrc); //merge database together
	XrmPutFileDatabase(old_blackboxrc, dbfile.get());
	XrmDestroyDatabase(old_blackboxrc);
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): ------------ SAVING DONE"<<endl;	
	#endif
}

//-------- getRcFilename -------------
// Returns filename of resource file
//------------------------------------
char *Fluxbox::getRcFilename() {
	char *dbfile=0;
 
	if (!rc_file) {		
		string str(getenv("HOME")+string("/.")+RC_PATH+string("/")+RC_INIT_FILE);
		return StringUtil::strdup(str.c_str());

	} else
		dbfile = StringUtil::strdup(rc_file);
 
	return dbfile;
}

void Fluxbox::load_rc(void) {
	XrmDatabaseHelper database;
	
	//get resource filename
	auto_ptr<char> dbfile(getRcFilename());
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): dbfile="<<dbfile.get()<<endl;
	#endif
	if (dbfile.get()) {
		if (!m_resourcemanager.load(dbfile.get())) {
			cerr<<"Faild to load database:"<<dbfile.get()<<endl;
			cerr<<"Trying with: "<<DEFAULT_INITFILE<<endl;
			if (!m_resourcemanager.load(DEFAULT_INITFILE))
				cerr<<"Faild to load database: "<<DEFAULT_INITFILE<<endl;
		}
	} else {
		if (!m_resourcemanager.load(DEFAULT_INITFILE))
			cerr<<"Faild to load database: "<<DEFAULT_INITFILE<<endl;
	}
	
	XrmValue value;
	char *value_type;

	if (m_rc_menufile->size()) {
		char *tmpvar =StringUtil::expandFilename(m_rc_menufile->c_str());
		*m_rc_menufile = (tmpvar==0 ? "" : tmpvar);
		if (!m_rc_menufile->size())
			m_rc_menufile.setDefaultValue();

		delete tmpvar;
	} else
		m_rc_menufile.setDefaultValue();

	if (*m_rc_colors_per_channel < 2)
		*m_rc_colors_per_channel = 2;
	else if (*m_rc_colors_per_channel > 6)
		*m_rc_colors_per_channel = 6;

	if (*m_rc_stylefile=="")
		*m_rc_stylefile = DEFAULTSTYLE;
	else {
		auto_ptr<char> tmpvar(StringUtil::expandFilename(m_rc_stylefile->c_str()));
		*m_rc_stylefile = (tmpvar.get()==0 ? "" : tmpvar.get());
	}

	//load file
	database = XrmGetFileDatabase(dbfile.get());
	if (database==0) {
		cerr<<"Fluxbox: Cant open "<<dbfile.get()<<" !"<<endl;
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

}

void Fluxbox::load_rc(BScreen *screen) {
	#ifdef DEBUG
	cerr<<"Loading BScreen(this="<<screen<<") num="<<screen->getScreenNumber()<<"------------"<<endl;
	#endif
	//get resource filename
	auto_ptr<char> dbfile(getRcFilename());
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): dbfile="<<dbfile.get()<<endl;
	#endif
	if (dbfile.get()) {
		if (!m_screen_rm.load(dbfile.get())) {
			cerr<<"Faild to load database:"<<dbfile.get()<<endl;
			cerr<<"Trying with: "<<DEFAULT_INITFILE<<endl;
			if (!m_screen_rm.load(DEFAULT_INITFILE))
				cerr<<"Faild to load database: "<<DEFAULT_INITFILE<<endl;
		}
	} else {
		if (!m_screen_rm.load(DEFAULT_INITFILE))
			cerr<<"Faild to load database: "<<DEFAULT_INITFILE<<endl;
	}
	
	XrmDatabaseHelper database;

	database = XrmGetFileDatabase(dbfile.get());
	if (database==0)
		database = XrmGetFileDatabase(DEFAULT_INITFILE);
		
	XrmValue value;
	char *value_type, name_lookup[1024], class_lookup[1024];
	int screen_number = screen->getScreenNumber();

	sprintf(name_lookup, "session.screen%d.rowPlacementDirection", screen_number);
	sprintf(class_lookup, "Session.Screen%d.RowPlacementDirection", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		if (! strncasecmp(value.addr, "righttoleft", value.size))
			screen->saveRowPlacementDirection(BScreen::RIGHTLEFT);
		else	
			screen->saveRowPlacementDirection(BScreen::LEFTRIGHT);
	} else
		screen->saveRowPlacementDirection(BScreen::LEFTRIGHT);

	sprintf(name_lookup, "session.screen%d.colPlacementDirection", screen_number);
	sprintf(class_lookup, "Session.Screen%d.ColPlacementDirection", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		if (! strncasecmp(value.addr, "bottomtotop", value.size))
			screen->saveColPlacementDirection(BScreen::BOTTOMTOP);
		else
			screen->saveColPlacementDirection(BScreen::TOPBOTTOM);
	} else
		screen->saveColPlacementDirection(BScreen::TOPBOTTOM);

	screen->removeWorkspaceNames();

	sprintf(name_lookup, "session.screen%d.workspaceNames", screen_number);
	sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): Workspaces="<<screen->getNumberOfWorkspaces()<<endl;
		#endif
		char *search = StringUtil::strdup(value.addr);

		int i;
		for (i = 0; i < screen->getNumberOfWorkspaces(); i++) {
			char *nn;

			if (! i) nn = strtok(search, ",");
			else nn = strtok(NULL, ",");

			if (nn)
				screen->addWorkspaceName(nn);	
			else break;
			
		}

		delete [] search;
	}

	sprintf(name_lookup, "session.screen%d.focusModel", screen_number);
	sprintf(class_lookup, "Session.Screen%d.FocusModel", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		if (! strncasecmp(value.addr, "clicktofocus", value.size)) {
			screen->saveAutoRaise(False);
			screen->saveSloppyFocus(False);
			screen->saveSemiSloppyFocus(False);

		} else if (! strncasecmp(value.addr, "autoraisesloppyfocus", value.size)) {
			screen->saveSemiSloppyFocus(False);
			screen->saveSloppyFocus(True);
			screen->saveAutoRaise(True);
		} else if (! strncasecmp(value.addr, "autoraisesemisloppyfocus", value.size)) {
			screen->saveSloppyFocus(False);
			screen->saveSemiSloppyFocus(True);
			screen->saveAutoRaise(True);

		} else if (! strncasecmp(value.addr, "semisloppyfocus", value.size)) {
			screen->saveSloppyFocus(False);
			screen->saveSemiSloppyFocus(True);
			screen->saveAutoRaise(False);

		} else {

			screen->saveSemiSloppyFocus(False);	
			screen->saveSloppyFocus(True);
			screen->saveAutoRaise(False);
		}
	} else {
		screen->saveSemiSloppyFocus(False);
		screen->saveSloppyFocus(True); //TODO: fluxgen, shouldn't this be false?
		screen->saveAutoRaise(False); //as click should be default, or?
	}

	sprintf(name_lookup, "session.screen%d.windowPlacement", screen_number);
	sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		if (! strncasecmp(value.addr, "RowSmartPlacement", value.size))
			screen->savePlacementPolicy(BScreen::ROWSMARTPLACEMENT);
		else if (! strncasecmp(value.addr, "ColSmartPlacement", value.size))
			screen->savePlacementPolicy(BScreen::COLSMARTPLACEMENT);
		else
			screen->savePlacementPolicy(BScreen::CASCADEPLACEMENT);
	else
		screen->savePlacementPolicy(BScreen::ROWSMARTPLACEMENT);

#ifdef SLIT
	sprintf(name_lookup, "session.screen%d.slit.placement", screen_number);
	sprintf(class_lookup, "Session.Screen%d.Slit.Placement", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		if (! strncasecmp(value.addr, "TopLeft", value.size))
			screen->saveSlitPlacement(Slit::TOPLEFT);
		else if (! strncasecmp(value.addr, "CenterLeft", value.size))
			screen->saveSlitPlacement(Slit::CENTERLEFT);
		else if (! strncasecmp(value.addr, "BottomLeft", value.size))
			screen->saveSlitPlacement(Slit::BOTTOMLEFT);
		else if (! strncasecmp(value.addr, "TopCenter", value.size))
			screen->saveSlitPlacement(Slit::TOPCENTER);
		else if (! strncasecmp(value.addr, "BottomCenter", value.size))
			screen->saveSlitPlacement(Slit::BOTTOMCENTER);
		else if (! strncasecmp(value.addr, "TopRight", value.size))
			screen->saveSlitPlacement(Slit::TOPRIGHT);
		else if (! strncasecmp(value.addr, "BottomRight", value.size))
			screen->saveSlitPlacement(Slit::BOTTOMRIGHT);
		else
			screen->saveSlitPlacement(Slit::CENTERRIGHT);
	else
		screen->saveSlitPlacement(Slit::CENTERRIGHT);

	sprintf(name_lookup, "session.screen%d.slit.direction", screen_number);
	sprintf(class_lookup, "Session.Screen%d.Slit.Direction", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		if (! strncasecmp(value.addr, "Horizontal", value.size))
			screen->saveSlitDirection(Slit::HORIZONTAL);
		else
			screen->saveSlitDirection(Slit::VERTICAL);
	else
		screen->saveSlitDirection(Slit::VERTICAL);

	sprintf(name_lookup, "session.screen%d.slit.onTop", screen_number);
	sprintf(class_lookup, "Session.Screen%d.Slit.OnTop", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		if (! strncasecmp(value.addr, "True", value.size))
			screen->saveSlitOnTop(True);
		else
			screen->saveSlitOnTop(False);
	else
		screen->saveSlitOnTop(False);

	sprintf(name_lookup, "session.screen%d.slit.autoHide", screen_number);
	sprintf(class_lookup, "Session.Screen%d.Slit.AutoHide", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		if (! strncasecmp(value.addr, "True", value.size))
			screen->saveSlitAutoHide(True);
		else
			screen->saveSlitAutoHide(False);
	else
		screen->saveSlitAutoHide(False);
#endif // SLIT

#ifdef HAVE_STRFTIME
	sprintf(name_lookup, "session.screen%d.strftimeFormat", screen_number);
	sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value))
		screen->saveStrftimeFormat(value.addr);
	else
		screen->saveStrftimeFormat("%I:%M %p");
#else //	HAVE_STRFTIME

	sprintf(name_lookup, "session.screen%d.dateFormat", screen_number);
	sprintf(class_lookup, "Session.Screen%d.DateFormat", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		if (strncasecmp(value.addr, "european", value.size))
			screen->saveDateFormat(B_AmericanDate);
		else
			screen->saveDateFormat(B_EuropeanDate);
	} else
		screen->saveDateFormat(B_AmericanDate);

	sprintf(name_lookup, "session.screen%d.clockFormat", screen_number);
	sprintf(class_lookup, "Session.Screen%d.ClockFormat", screen_number);
	if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
			&value)) {
		int clock;
		if (sscanf(value.addr, "%d", &clock) != 1) screen->saveClock24Hour(False);
		else if (clock == 24) screen->saveClock24Hour(True);
		else screen->saveClock24Hour(False);
	} else
		screen->saveClock24Hour(False);
#endif // HAVE_STRFTIME

	//check size on toolbarwidth percent	
	if (screen->getToolbarWidthPercent() <= 0 || 
			screen->getToolbarWidthPercent() > 100)
		screen->saveToolbarWidthPercent(66);

	if (screen->getTabWidth()>512)
		screen->saveTabWidth(512);
	else if (screen->getTabWidth()<0)
		screen->saveTabWidth(64);
	
	if (screen->getTabHeight()>512)
		screen->saveTabHeight(512);
	else if (screen->getTabHeight()<0)
		screen->saveTabHeight(5);
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"---------------------- LOADING DONE"<<endl;
	#endif
}

void Fluxbox::loadRootCommand(BScreen *screen)	{
	XrmDatabase database = (XrmDatabase) 0;
 
	auto_ptr<char> dbfile(getRcFilename());

	database = XrmGetFileDatabase(dbfile.get());
	if (!database) 
		database = XrmGetFileDatabase(DEFAULT_INITFILE);

	XrmValue value;
	char *value_type, name_lookup[1024], class_lookup[1024];
	sprintf(name_lookup, "session.screen%d.rootCommand", screen->getScreenNumber());
	sprintf(class_lookup, "Session.Screen%d.RootCommand", screen->getScreenNumber());
	if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
			&value)) {										 
		screen->saveRootCommand(value.addr==0 ? "": value.addr);
	} else
		screen->saveRootCommand("");		
	
}

void Fluxbox::reload_rc(void) {
	load_rc();
	reconfigure();
}


void Fluxbox::reconfigure(void) {
	reconfigure_wait = true;

	if (! timer->isTiming()) timer->start();
}


void Fluxbox::real_reconfigure(void) {
	BaseDisplay::GrabGuard gg(*this);
	grab();

	XrmDatabase new_blackboxrc = (XrmDatabase) 0;

	auto_ptr<char> dbfile(getRcFilename());
	XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.get());

	XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
	XrmPutFileDatabase(old_blackboxrc, dbfile.get());
	
	if (old_blackboxrc)
		XrmDestroyDatabase(old_blackboxrc);

	for (int i = 0, n = menuTimestamps->count(); i < n; i++) {
		MenuTimestamp *ts = menuTimestamps->remove(0);

		if (ts) {
			if (ts->filename)
				delete [] ts->filename;

			delete ts;
		}
	}

	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++) {
		BScreen *screen = it.current();

		screen->reconfigure();
	}
	
	//reconfigure keys
	key->reconfigure(const_cast<char *>(m_rc_keyfile->c_str()));

	//reconfigure tabs
	reconfigureTabs();

	ungrab();
}

//------------- reconfigureTabs ----------
// Reconfigure all tabs size and increase steps
// ---------------------------------------
void Fluxbox::reconfigureTabs(void) {
	//tab reconfiguring
	LinkedListIterator<TabSearch> it(tabSearchList);
	//setting all to unconfigured
	for (; it.current(); it++) {
		TabSearch *tmp = it.current();
		if (tmp)
			tmp->getData()->setConfigured(false);
	}
	it.reset(); // resetting list and start configure tabs
	//reconfiguring
	for (; it.current(); it++) {
		TabSearch *tmp = it.current();
		Tab *tab = tmp->getData();
		if (!tab->configured()) {
			tab->setConfigured(true);
			tab->resizeGroup(); 
			tab->calcIncrease();
			tab->setPosition();
		}
	}
}

void Fluxbox::checkMenu(void) {
	Bool reread = False;
	LinkedListIterator<MenuTimestamp> it(menuTimestamps);
	for (; it.current() && (! reread); it++) {
		struct stat buf;

		if (! stat(it.current()->filename, &buf)) {
			if (it.current()->timestamp != buf.st_ctime)
				reread = True;
		} else
			reread = True;
	}

	if (reread) rereadMenu();
}


void Fluxbox::rereadMenu(void) {
	reread_menu_wait = True;

	if (! timer->isTiming()) timer->start();
}


void Fluxbox::real_rereadMenu(void) {
	for (int i = 0, n = menuTimestamps->count(); i < n; i++) {
		MenuTimestamp *ts = menuTimestamps->remove(0);

		if (ts) {
			if (ts->filename)
	delete [] ts->filename;

			delete ts;
		}
	}

	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++)
		it.current()->rereadMenu();
}

/*
void Fluxbox::saveStyleFilename(const char *filename) {
	if (resource.style_file)
		delete [] resource.style_file;

	resource.style_file = StringUtil::strdup(filename);
}
*/

void Fluxbox::saveMenuFilename(const char *filename) {
	Bool found = False;

	LinkedListIterator<MenuTimestamp> it(menuTimestamps);
	for (; it.current() && (! found); it++)
		if (! strcmp(it.current()->filename, filename)) found = True;

	if (! found) {
		struct stat buf;

		if (! stat(filename, &buf)) {
			MenuTimestamp *ts = new MenuTimestamp;

			ts->filename = StringUtil::strdup(filename);
			ts->timestamp = buf.st_ctime;

			menuTimestamps->insert(ts);
		}
	}
}


void Fluxbox::timeout(void) {
	if (reconfigure_wait)
		real_reconfigure();

	if (reread_menu_wait)
		real_rereadMenu();

	reconfigure_wait = reread_menu_wait = false;
}


void Fluxbox::setFocusedWindow(FluxboxWindow *win) {
	
	BScreen *old_screen = 0, *screen = 0;
	FluxboxWindow *old_win = 0;
	Toolbar *old_tbar = 0, *tbar = 0;
	Workspace *old_wkspc = 0, *wkspc = 0;

	if (focused_window) {
		old_win = focused_window;
		old_screen = old_win->getScreen();
		
		old_tbar = old_screen->getToolbar();
		old_wkspc = old_screen->getWorkspace(old_win->getWorkspaceNumber());

		old_win->setFocusFlag(False);
		old_wkspc->getMenu()->setItemSelected(old_win->getWindowNumber(), False);
		
	}

	if (win && ! win->isIconic()) {
		
		screen = win->getScreen();
		tbar = screen->getToolbar();
		wkspc = screen->getWorkspace(win->getWorkspaceNumber());		
		focused_window = win;
		win->setFocusFlag(True);
		wkspc->getMenu()->setItemSelected(win->getWindowNumber(), True);
		
	} else
		focused_window = (FluxboxWindow *) 0;

	if (tbar)
		tbar->redrawWindowLabel(True);
	if (screen)
		screen->updateNetizenWindowFocus();

	if (old_tbar && old_tbar != tbar)
		old_tbar->redrawWindowLabel(True);
	if (old_screen && old_screen != screen)
		old_screen->updateNetizenWindowFocus();

}
