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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: fluxbox.cc,v 1.16 2002/01/09 14:07:09 fluxgen Exp $

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Basemenu.hh"
#include "Clientmenu.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef    SLIT
#include "Slit.hh"
#endif // SLIT

#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "StringUtil.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef    SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE


#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN

#ifdef    HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H

#ifdef    HAVE_SYS_SIGNAL_H
#  include <sys/signal.h>
#endif // HAVE_SYS_SIGNAL_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/types.h>
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#endif // HAVE_LIBGEN_H

#include <iostream>
#include <string>
#include <strstream>
using namespace std;

#ifndef   HAVE_BASENAME
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
: BaseDisplay(m_argv[0], dpy_name)
{
		
	//singleton pointer
	singleton = this;
  grab();

  if (! XSupportsLocale())
    fprintf(stderr, "X server does not support locale\n");

  if (XSetLocaleModifiers("") == NULL)
    fprintf(stderr, "cannot set locale modifiers\n");

// Set default values to member variables

  argc = m_argc;
  argv = m_argv;
  rc_file = rc;
	key=0;
	
  no_focus = False;
  resource.titlebar_file = resource.menu_file = resource.style_file = resource.keys_file = 
		resource.root_cmd = 0;  
	resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec = 0;
		
  focused_window = masked_window = (FluxboxWindow *) 0;
  masked = None;
	/*resource.tabtype.listtype = TabType::Vertical;
	resource.tabtype.listpos = TabType::Top;
	resource.tabtype.pos = TabType::Right;
	*/
	
  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;

#ifdef    SLIT
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

  XrmInitialize();
  load_rc();

#ifdef    HAVE_GETPID
  fluxbox_pid = XInternAtom(getXDisplay(), "_BLACKBOX_PID", False);
#endif // HAVE_GETPID

  screenList = new LinkedList<BScreen>;
  int i;

  //allocate screens
	for (i = 0; i < getNumberOfScreens(); i++) {
    BScreen *screen = new BScreen(this, i);

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
#ifdef    NLS
					blackboxSet, blackboxNoManagableScreens,
#else // !NLS
					 0, 0,
#endif // NLS
					"Fluxbox::Fluxbox: no managable screens found, aborting.\n"));

		throw static_cast<int>(3);
	}

	XSynchronize(getXDisplay(), False);
	XSync(getXDisplay(), False);

	reconfigure_wait = reread_menu_wait = False;

	timer = new BTimer(this, this);
	timer->setTimeout(0);
	timer->fireOnce(True);

	//create keybindings handler and load keys file
	key = new Keys(getXDisplay(), resource.keys_file);

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
	
	if (resource.root_cmd)
		delete [] resource.root_cmd;
	
	if (resource.menu_file)
		delete [] resource.menu_file;
	
	if (resource.style_file)
		delete [] resource.style_file;
	
	if (resource.titlebar_file)
		delete resource.titlebar_file;
		
	if (resource.keys_file)
		delete resource.keys_file;
	
	delete timer;

	delete screenList;
	delete menuTimestamps;

	delete windowSearchList;
	delete menuSearchList;
	delete toolbarSearchList;
	delete tabSearchList;
	delete groupSearchList;

#ifdef    SLIT
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
	case ButtonPress:
		{
			last_time = e->xbutton.time;

			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;

			#ifdef    SLIT
			Slit *slit = (Slit *) 0;
			#endif // SLIT

			Toolbar *tbar = (Toolbar *) 0;			
			Tab *tab = 0;

			if ((win = searchWindow(e->xbutton.window))) {

				win->buttonPressEvent(&e->xbutton);
				
				if (e->xbutton.button == 1)
					win->installColormap(True);
					
			} else if ((menu = searchMenu(e->xbutton.window))) {
				menu->buttonPressEvent(&e->xbutton);

#ifdef    SLIT
			} else if ((slit = searchSlit(e->xbutton.window))) {
				slit->buttonPressEvent(&e->xbutton);
#endif // SLIT

			} else if ((tbar = searchToolbar(e->xbutton.window))) {
				tbar->buttonPressEvent(&e->xbutton);
			} else if ((tab = searchTab(e->xbutton.window))) {
				tab->buttonPressEvent(&e->xbutton);
			} else {
				LinkedListIterator<BScreen> it(screenList);

				for (; it.current(); it++) {
					BScreen *screen = it.current();
				

						//if ( e->xbutton.button == 1 && 
						//	(win = screen->getIconBar()->findWindow(e->xbutton.window)) ) {
						//	win->deiconify();
						
				 if (e->xbutton.window == screen->getRootWindow()) {
					if (e->xbutton.button == 1) {
							if (! screen->isRootColormapInstalled())
							screen->getImageControl()->installRootColormap();

						if (screen->getWorkspacemenu()->isVisible())
							screen->getWorkspacemenu()->hide();

						if (screen->getRootmenu()->isVisible())
							screen->getRootmenu()->hide();
						
					} else if (e->xbutton.button == 2) {
						int mx = e->xbutton.x_root -
							(screen->getWorkspacemenu()->getWidth() / 2);
	  				int my = e->xbutton.y_root -
			  	  	(screen->getWorkspacemenu()->getTitleHeight() / 2);
	
	  				if (mx < 0) mx = 0;
					  if (my < 0) my = 0;

					  if (mx + screen->getWorkspacemenu()->getWidth() >
							 screen->getWidth())
								mx = screen->getWidth() -
						screen->getWorkspacemenu()->getWidth() -
						screen->getBorderWidth();

					  if (my + screen->getWorkspacemenu()->getHeight() >
								  screen->getHeight())
							my = screen->getHeight() -
						screen->getWorkspacemenu()->getHeight() -
						screen->getBorderWidth();

					  screen->getWorkspacemenu()->move(mx, my);

	      		if (! screen->getWorkspacemenu()->isVisible()) {
            	screen->getWorkspacemenu()->removeParent();
							screen->getWorkspacemenu()->show();
	      		}
	    		} else if (e->xbutton.button == 3) {
			      int mx = e->xbutton.x_root -
			       (screen->getRootmenu()->getWidth() / 2);
	    		  int my = e->xbutton.y_root -
		      	 (screen->getRootmenu()->getTitleHeight() / 2);

			      if (mx < 0) mx = 0;
			      if (my < 0) my = 0;

	  		    if (mx + screen->getRootmenu()->getWidth() > screen->getWidth())
							mx = screen->getWidth() -
						screen->getRootmenu()->getWidth() -
						screen->getBorderWidth();

						if (my + screen->getRootmenu()->getHeight() >
							screen->getHeight())
							my = screen->getHeight() -
					    	screen->getRootmenu()->getHeight() -
								screen->getBorderWidth();

						screen->getRootmenu()->move(mx, my);

						if (! screen->getRootmenu()->isVisible()) {
							checkMenu();
							screen->getRootmenu()->show();
						}
						
		  		} 
			
				}
			}
		}

      break;
	}

	case ButtonRelease:
	{
			last_time = e->xbutton.time;
			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;
			Toolbar *tbar = (Toolbar *) 0;
			Tab *tab = 0;
			if ((win = searchWindow(e->xbutton.window)))
				win->buttonReleaseEvent(&e->xbutton);
			else if ((menu = searchMenu(e->xbutton.window)))
				menu->buttonReleaseEvent(&e->xbutton);
      else if ((tbar = searchToolbar(e->xbutton.window)))
				tbar->buttonReleaseEvent(&e->xbutton);
			else if ((tab = searchTab(e->xbutton.window)))
				tab->buttonReleaseEvent(&e->xbutton);
      break;
    }

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

		break;
	}

  case MapRequest:
    {
#ifdef    DEBUG
      fprintf(stderr,
	      I18n::instance()->
	      getMessage(
#  ifdef    NLS
			 blackboxSet, blackboxMapRequest,
#  else // !NLS
			 0, 0,
#  endif // NLS
			 "Fluxbox::process_event(): MapRequest for 0x%lx\n"),
              e->xmaprequest.window);
#endif // DEBUG
	#ifdef    SLIT
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

      if (! win)
        win = new FluxboxWindow(e->xmaprequest.window);
				
			
      if ((win = searchWindow(e->xmaprequest.window)))
        win->mapRequestEvent(&e->xmaprequest);

      break;
    }

	case MapNotify:
		{
			FluxboxWindow *win = searchWindow(e->xmap.window);

			if (win)
				win->mapNotifyEvent(&e->xmap);

			break;
		}

	case UnmapNotify:
		{
			FluxboxWindow *win = (FluxboxWindow *) 0;

#ifdef    SLIT
			Slit *slit = (Slit *) 0;
#endif // SLIT

			if ((win = searchWindow(e->xunmap.window))) {
				win->unmapNotifyEvent(&e->xunmap);

#ifdef    SLIT
			} else if ((slit = searchSlit(e->xunmap.window))) {
        slit->removeClient(e->xunmap.window);
#endif // SLIT

      }

			break;
		}

  case DestroyNotify:
    {
      FluxboxWindow *win = (FluxboxWindow *) 0;

#ifdef    SLIT
			Slit *slit = 0;
#endif // SLIT

			if ((win = searchWindow(e->xdestroywindow.window))) {
				win->destroyNotifyEvent(&e->xdestroywindow);
#ifdef    SLIT
			} else if ((slit = searchSlit(e->xdestroywindow.window))) {
				slit->removeClient(e->xdestroywindow.window, False);
#endif // SLIT
			}

			break;
		}

  case MotionNotify:
    {
			last_time = e->xmotion.time;

			FluxboxWindow *win = 0;
			Basemenu *menu = 0;
			Tab *tab = 0;
			
			if ((win = searchWindow(e->xmotion.window)))
				win->motionNotifyEvent(&e->xmotion);
			else if ((menu = searchMenu(e->xmotion.window)))
				menu->motionNotifyEvent(&e->xmotion);
			else if ((tab = searchTab(e->xmotion.window)))
				tab->motionNotifyEvent(&e->xmotion);
				
			break;
		}

	case PropertyNotify:
		{
			last_time = e->xproperty.time;

			if (e->xproperty.state != PropertyDelete) {
				FluxboxWindow *win = searchWindow(e->xproperty.window);

				if (win)
					win->propertyNotifyEvent(e->xproperty.atom);
			}

			break;
    }

  case EnterNotify:
    {
			last_time = e->xcrossing.time;

			BScreen *screen = (BScreen *) 0;
			FluxboxWindow *win = (FluxboxWindow *) 0;
			Basemenu *menu = (Basemenu *) 0;
			Toolbar *tbar = (Toolbar *) 0;
			Tab *tab = (Tab *) 0;
			#ifdef    SLIT
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
				
#ifdef    SLIT
      else if ((slit = searchSlit(e->xcrossing.window)))
        slit->enterNotifyEvent(&e->xcrossing);
#endif // SLIT

      break;
    }

  case LeaveNotify:
    {
      last_time = e->xcrossing.time;

      FluxboxWindow *win = (FluxboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;
      Toolbar *tbar = (Toolbar *) 0;
		//	Tab *tab = 0;
			
#ifdef    SLIT
      Slit *slit = (Slit *) 0;
#endif // SLIT

			if ((menu = searchMenu(e->xcrossing.window)))
				menu->leaveNotifyEvent(&e->xcrossing);
			else if ((win = searchWindow(e->xcrossing.window)))
				win->installColormap(False);
			else if ((tbar = searchToolbar(e->xcrossing.window)))
				tbar->leaveNotifyEvent(&e->xcrossing);
		//	else if ((tab = searchTab(e->xcrossing.window)))
		//		tab->leaveNotifyEvent(&e->xcrossing);
#ifdef    SLIT
      else if ((slit = searchSlit(e->xcrossing.window)))
        slit->leaveNotifyEvent(&e->xcrossing);
#endif // SLIT
			
      break;
    }

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
			break;
		}

  case KeyPress:
    {
			Toolbar *tbar = searchToolbar(e->xkey.window);
			BScreen *screen = searchScreen(e->xkey.window);
			
			if (tbar && tbar->isEditing())
				tbar->keyPressEvent(&e->xkey);
			else if (screen)	{
				#ifdef DEBUG
				cerr<<"KeyEvent"<<endl;
				#endif
				//find action
				Keys::KeyAction action = key->getAction(&e->xkey);
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
					/*case Keys::grabUpWorkspace:
						break;
					case Keys::grabDownWorkspace:
						break;
					case Keys::grabLeftWorkspace:
						break;
					case Keys::grabRightWorkspace:
						break;
					*/
					case Keys::KILLWINDOW: //kill the current window
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
						#ifndef    __EMX__
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
						
						break;
					}
					default: //try to see if its a window action
						doWindowAction(action);
				}
			}
			break;
		}

	case ColormapNotify:
		{
      BScreen *screen = searchScreen(e->xcolormap.window);

			if (screen)
				screen->setRootColormapInstalled((e->xcolormap.state ==
					ColormapInstalled) ? True : False);

			break;
		}

	case FocusIn:
	{
		if (e->xfocus.mode == NotifyUngrab ||
				e->xfocus.detail == NotifyPointer)
			break;

		FluxboxWindow *win = searchWindow(e->xfocus.window);
		if (win && ! win->isFocused())
			setFocusedWindow(win);

		break;
	}

  case FocusOut:
    break;

  case ClientMessage:
    {
			if (e->xclient.format == 32) {
				if (e->xclient.message_type == getWMChangeStateAtom()) {
					FluxboxWindow *win = searchWindow(e->xclient.window);
					if (! win || ! win->validateClient())
						return;

					if (e->xclient.data.l[0] == IconicState)
						win->iconify();
					if (e->xclient.data.l[0] == NormalState)
						win->deiconify();
				} else if (e->xclient.message_type == getFluxboxChangeWorkspaceAtom()) {
					BScreen *screen = searchScreen(e->xclient.window);

					if (screen && e->xclient.data.l[0] >= 0 &&
							e->xclient.data.l[0] < screen->getCount())
						screen->changeWorkspaceID(e->xclient.data.l[0]);
				
				} else if (e->xclient.message_type == getFluxboxChangeWindowFocusAtom()) {
					FluxboxWindow *win = searchWindow(e->xclient.window);

          if (win && win->isVisible() && win->setInputFocus())
						win->installColormap(True);
				} else if (e->xclient.message_type == getFluxboxCycleWindowFocusAtom()) {
					BScreen *screen = searchScreen(e->xclient.window);

          if (screen)
            if (! e->xclient.data.l[0])
							screen->prevFocus();
						else
							screen->nextFocus();
				} else if (e->xclient.message_type == getFluxboxChangeAttributesAtom()) {
		
					FluxboxWindow *win = searchWindow(e->xclient.window);

					if (win && win->validateClient()) {
						BlackboxHints net;
						net.flags = e->xclient.data.l[0];
						net.attrib = e->xclient.data.l[1];
						net.workspace = e->xclient.data.l[2];
						net.stack = e->xclient.data.l[3];
						net.decoration = static_cast<BaseDisplay::Decor>(e->xclient.data.l[4]);

	  	  		win->changeBlackboxHints(&net);
					}
				}
			}

			break;
    }


  default:
    {

#ifdef    SHAPE
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

void Fluxbox::doWindowAction(Keys::KeyAction action) {
	if (!focused_window)
		return;
		
	switch (action) {
		case Keys::ICONIFY:
			focused_window->iconify();
		break;
		case Keys::RAISE:
			focused_window->getScreen()->getWorkspace(focused_window->getWorkspaceNumber())->raiseWindow(focused_window);	
		break;
		case Keys::LOWER:
			XLowerWindow(getXDisplay(), focused_window->getFrameWindow());
		break;
		case Keys::CLOSE:
			focused_window->close();
		break;
		case Keys::SHADE:		
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
		break;								
		case Keys::VERTINC:
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth(), focused_window->getHeight()+10);
		break;
		case Keys::HORIZDEC:				
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth()-10, focused_window->getHeight());
		break;								
		case Keys::VERTDEC:
			if (focused_window->isResizable())
				focused_window->configure(
					focused_window->getXFrame(), focused_window->getYFrame(),
					focused_window->getWidth(), focused_window->getHeight()-10);
		break;		
		default:	
		break;							
	}

}

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
    if (it.current())
      if (it.current()->getRootWindow() == window) {
       screen = it.current();
	return screen;
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
    if (tmp)
      if (tmp->getWindow() == window) {
        w = tmp->getData();
        if (w->getClientWindow() != win->getClientWindow())
          return win;
      }
  }

  return (FluxboxWindow *) 0;
}


Basemenu *Fluxbox::searchMenu(Window window) {
  Basemenu *menu = (Basemenu *) 0;
  LinkedListIterator<MenuSearch> it(menuSearchList);

  for (; it.current(); it++) {
    MenuSearch *tmp = it.current();

    if (tmp)
      if (tmp->getWindow() == window) {
        menu = tmp->getData();
        return menu;
      }
  }

  return (Basemenu *) 0;
}


Toolbar *Fluxbox::searchToolbar(Window window) {
  Toolbar *tbar = (Toolbar *) 0;
  LinkedListIterator<ToolbarSearch> it(toolbarSearchList);

  for (; it.current(); it++) {
    ToolbarSearch *tmp = it.current();

    if (tmp)
      if (tmp->getWindow() == window) {
        tbar = tmp->getData();
        return tbar;
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


#ifdef    SLIT
Slit *Fluxbox::searchSlit(Window window) {
	Slit *s = (Slit *) 0;
	LinkedListIterator<SlitSearch> it(slitSearchList);

	for (; it.current(); it++) {
		SlitSearch *tmp = it.current();

		if (tmp)
			if (tmp->getWindow() == window) {
				s = tmp->getData();
				return s;
			}
	}

  return (Slit *) 0;
}
#endif // SLIT


void Fluxbox::saveWindowSearch(Window window, FluxboxWindow *data) {
  windowSearchList->insert(new WindowSearch(window, data));

#ifdef GNOME
	/*
	Window	*wl=0;
	int	num=0;

	num = windowSearchList->count();
	wl = new Window[num];
  // Fill in array of window ID's
	LinkedListIterator<WindowSearch> it(windowSearchList);

	for (unsigned int i=0; it.current(); it++, i++)
		wl[i] = it.current()->getWindow();		
	
	//set property
  XChangeProperty(getXDisplay(), DefaultRootWindow(getXDisplay()), getGnomeClientListAtom(), XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char  *)(wl), num);
	
	delete wl;	
	*/
#endif

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

#ifdef    SLIT
void Fluxbox::saveSlitSearch(Window window, Slit *data) {
  slitSearchList->insert(new SlitSearch(window, data));
}
#endif // SLIT


void Fluxbox::removeWindowSearch(Window window) {
	LinkedListIterator<WindowSearch> it(windowSearchList);
	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();

		if (tmp)
			if (tmp->getWindow() == window) {
				windowSearchList->remove(tmp);
				delete tmp;
				break;
			}
  }
}


void Fluxbox::removeGroupSearch(Window window) {
	LinkedListIterator<WindowSearch> it(groupSearchList);
	for (; it.current(); it++) {
		WindowSearch *tmp = it.current();

		if (tmp)
			if (tmp->getWindow() == window) {
				groupSearchList->remove(tmp);
				delete tmp;
				break;
			}
	}
}


void Fluxbox::removeMenuSearch(Window window) {
	LinkedListIterator<MenuSearch> it(menuSearchList);
	for (; it.current(); it++) {
		MenuSearch *tmp = it.current();

    if (tmp)
      if (tmp->getWindow() == window) {
				menuSearchList->remove(tmp);
				delete tmp;
				break;
			}
	}
}


void Fluxbox::removeToolbarSearch(Window window) {
  LinkedListIterator<ToolbarSearch> it(toolbarSearchList);
  for (; it.current(); it++) {
    ToolbarSearch *tmp = it.current();
		if (tmp)
			if (tmp->getWindow() == window) {
				toolbarSearchList->remove(tmp);	
				delete tmp;
				break;
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

#ifdef    SLIT
void Fluxbox::removeSlitSearch(Window window) {
	LinkedListIterator<SlitSearch> it(slitSearchList);
	for (; it.current(); it++) {
		SlitSearch *tmp = it.current();

		if (tmp)
			if (tmp->getWindow() == window) {
				slitSearchList->remove(tmp);
				delete tmp;
				break;
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

//save_rc
//saves resources
void Fluxbox::save_rc(void) {

	XrmDatabase new_blackboxrc = (XrmDatabase) 0;
	char rc_string[1024];

	auto_ptr<char> dbfile(getRcFilename());

//	load_rc(); This overwrites configs made while running, for example
// usage of iconbar and tabs
  
	sprintf(rc_string, "session.iconbar: %s", resource.iconbar ? "true" : "false");
	XrmPutLineResource(&new_blackboxrc, rc_string);
	
	sprintf(rc_string, "session.tabs: %s", resource.tabs ? "true" : "false");
	XrmPutLineResource(&new_blackboxrc, rc_string);
	
	sprintf(rc_string, "session.menuFile:  %s", resource.menu_file);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.titlebarFile:  %s", resource.titlebar_file);
	XrmPutLineResource(&new_blackboxrc, rc_string);
	
	sprintf(rc_string, "session.keyFile:  %s", resource.keys_file);
	XrmPutLineResource(&new_blackboxrc, rc_string);
	
	sprintf(rc_string, "session.colorsPerChannel:  %d",
					resource.colors_per_channel);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.doubleClickInterval:  %lu",
					resource.double_click_interval);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.autoRaiseDelay:  %lu",
					((resource.auto_raise_delay.tv_sec * 1000) +
					(resource.auto_raise_delay.tv_usec / 1000)));
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.cacheLife: %lu", resource.cache_life / 60000);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	sprintf(rc_string, "session.cacheMax: %lu", resource.cache_max);
	XrmPutLineResource(&new_blackboxrc, rc_string);

	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++) {
		BScreen *screen = it.current();
		int screen_number = screen->getScreenNumber();

#ifdef    SLIT
		char *slit_placement = (char *) 0;

		switch (screen->getSlitPlacement()) {
		case Slit::TopLeft: slit_placement = "TopLeft"; break;
		case Slit::CenterLeft: slit_placement = "CenterLeft"; break;
		case Slit::BottomLeft: slit_placement = "BottomLeft"; break;
		case Slit::TopCenter: slit_placement = "TopCenter"; break;
		case Slit::BottomCenter: slit_placement = "BottomCenter"; break;
		case Slit::TopRight: slit_placement = "TopRight"; break;
		case Slit::BottomRight: slit_placement = "BottomRight"; break;
		case Slit::CenterRight: default: slit_placement = "CenterRight"; break;
		}

		sprintf(rc_string, "session.screen%d.slit.placement: %s", screen_number,
				slit_placement);
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.direction: %s", screen_number,
						((screen->getSlitDirection() == Slit::Horizontal) ? "Horizontal" :
																	"Vertical"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.onTop: %s", screen_number,
						((screen->getSlit()->isOnTop()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.slit.autoHide: %s", screen_number,
						((screen->getSlit()->doAutoHide()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // SLIT

		sprintf(rc_string, "session.opaqueMove: %s",
				((screen->doOpaqueMove()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.imageDither: %s",
				((screen->getImageControl()->doDither()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.fullMaximization: %s", screen_number,
					((screen->doFullMax()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.focusNewWindows: %s", screen_number,
						((screen->doFocusNew()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.focusLastWindow: %s", screen_number,
			((screen->doFocusLast()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.rowPlacementDirection: %s", screen_number,
			((screen->getRowPlacementDirection() == BScreen::LeftRight) ?
				"LeftToRight" : "RightToLeft"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.colPlacementDirection: %s", screen_number,
			((screen->getColPlacementDirection() == BScreen::TopBottom) ?
				"TopToBottom" : "BottomToTop"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		char *placement = (char *) 0;
		
		sprintf(rc_string, "session.screen%d.maxOverSlit: %s", screen_number,
					((screen->doMaxOverSlit()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);
		switch (screen->getPlacementPolicy()) {
		case BScreen::CascadePlacement:
			placement = "CascadePlacement";
			break;

		case BScreen::ColSmartPlacement:
			placement = "ColSmartPlacement";
			break;

		default:
		case BScreen::RowSmartPlacement:
			placement = "RowSmartPlacement";
			break;
		}
		sprintf(rc_string, "session.screen%d.windowPlacement:  %s", screen_number,
					placement);
		XrmPutLineResource(&new_blackboxrc, rc_string);
//TODO: This isn't pretty!
		char *focus_mode = 0;
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
			focus_mode);

		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.workspaces:  %d", screen_number,
					screen->getCount());
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.toolbar.onTop:  %s", screen_number,
				((screen->getToolbar()->isOnTop()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.toolbar.autoHide:  %s", screen_number,
					((screen->getToolbar()->doAutoHide()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		char *toolbar_placement = (char *) 0;

		switch (screen->getToolbarPlacement()) {
		case Toolbar::TOPLEFT: toolbar_placement = "TopLeft"; break;
		case Toolbar::BOTTOMLEFT: toolbar_placement = "BottomLeft"; break;
		case Toolbar::TOPCENTER: toolbar_placement = "TopCenter"; break;
		case Toolbar::TOPRIGHT: toolbar_placement = "TopRight"; break;
		case Toolbar::BOTTOMRIGHT: toolbar_placement = "BottomRight"; break;
		case Toolbar::BOTTOMCENTER: default:
			toolbar_placement = "BottomCenter"; break;
		}

		sprintf(rc_string, "session.screen%d.toolbar.placement: %s", screen_number,
						toolbar_placement);
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.tab.placement: %s", screen_number, 
			Tab::getTabPlacementString(screen->getTabPlacement()));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.tab.alignment: %s", screen_number, 
			Tab::getTabAlignmentString(screen->getTabAlignment()));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.tab.rotatevertical:  %s", screen_number,
					((screen->isTabRotateVertical()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.sloppywindowgrouping:  %s", screen_number,
					((screen->isSloppyWindowGrouping()) ? "True" : "False"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		load_rc(screen);

		// these are static, but may not be saved in the users resource file,
    // writing these resources will allow the user to edit them at a later
		// time... but loading the defaults before saving allows us to rewrite the
		// users changes...

#ifdef    HAVE_STRFTIME
		sprintf(rc_string, "session.screen%d.strftimeFormat: %s", screen_number,
			screen->getStrftimeFormat());
		XrmPutLineResource(&new_blackboxrc, rc_string);
#else // !HAVE_STRFTIME
		sprintf(rc_string, "session.screen%d.dateFormat:  %s", screen_number,
			((screen->getDateFormat() == B_EuropeanDate) ?
				"European" : "American"));
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.clockFormat:  %d", screen_number,
			((screen->isClock24Hour()) ? 24 : 12));
		XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // HAVE_STRFTIME

		sprintf(rc_string, "session.screen%d.edgeSnapThreshold: %d", screen_number,
			screen->getEdgeSnapThreshold());
		XrmPutLineResource(&new_blackboxrc, rc_string);

		sprintf(rc_string, "session.screen%d.toolbar.widthPercent:  %d",
						screen_number, screen->getToolbarWidthPercent());
		XrmPutLineResource(&new_blackboxrc, rc_string);

		// write out the users workspace names
		int i, len = 0;
		for (i = 0; i < screen->getCount(); i++)
			len += strlen((screen->getWorkspace(i)->getName()) ?
						screen->getWorkspace(i)->getName() : "Null") + 1;

		char *resource_string = new char[len + 1024],
		*save_string = new char[len], *save_string_pos = save_string,
		*name_string_pos;
		
		if (save_string) {
			for (i = 0; i < screen->getCount(); i++) {
				len = strlen((screen->getWorkspace(i)->getName()) ?
							screen->getWorkspace(i)->getName() : "Null") + 1;
      	name_string_pos =
					(char *) ((screen->getWorkspace(i)->getName()) ?
					screen->getWorkspace(i)->getName() : "Null");

				while (--len) *(save_string_pos++) = *(name_string_pos++);
				*(save_string_pos++) = ',';
			}
    }

    *(--save_string_pos) = '\0';

    sprintf(resource_string, "session.screen%d.workspaceNames:  %s",
	    screen_number, save_string);
    XrmPutLineResource(&new_blackboxrc, resource_string);

    delete [] resource_string;
    delete [] save_string;
  }

  XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.get());

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);		//merge database together
  XrmPutFileDatabase(old_blackboxrc, dbfile.get());
  XrmDestroyDatabase(old_blackboxrc);
//	XrmDestroyDatabase(new_blackboxrc);

}

//-------- getRcFilename -------------
// Returns filename of resource file
// TODO: possible replacement with strstream?
//------------------------------------
char *Fluxbox::getRcFilename() {
	char *dbfile=0;
 
	if (!rc_file) {
		
		string str(getenv("HOME")+string("/.")+RC_PATH+string("/")+RC_INIT_FILE);		
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): str.size()="<<str.size()<<endl;
		cerr<<__FILE__<<"("<<__LINE__<<"): str="<<str<<endl;
		#endif
		return StringUtil::strdup(str.c_str());
	} else
		dbfile = StringUtil::strdup(rc_file);
 
	return dbfile;
}

void Fluxbox::load_rc(void) {
  XrmDatabase database = (XrmDatabase) 0;
  
  //get resource filename
  auto_ptr<char> dbfile(getRcFilename());

  //load file
  database = XrmGetFileDatabase(dbfile.get());
	if (!database) {
		cerr<<"Fluxbox: Cant open "<<dbfile.get()<<" !"<<endl;
		cerr<<"Using: "<<DEFAULT_INITFILE<<endl;
		database = XrmGetFileDatabase(DEFAULT_INITFILE);
	}

  XrmValue value;
  char *value_type;

	if (resource.menu_file) {
		delete [] resource.menu_file;
		resource.menu_file = 0;
	}
	//get menu filename
	if (XrmGetResource(database, "session.menuFile", "Session.MenuFile",
				&value_type, &value)) {
    
		resource.menu_file = StringUtil::expandFilename(value.addr); // expand ~ to $HOME
	} else
		resource.menu_file = StringUtil::strdup(DEFAULTMENU);

	if (resource.titlebar_file) {
		delete resource.titlebar_file;
		resource.titlebar_file = 0;
	}
	
	//get titlebar filename
	if (XrmGetResource(database, "session.titlebarFile", "Session.TitlebarFile",
				&value_type, &value)) {
		resource.titlebar_file = StringUtil::expandFilename(value.addr); //expand ~ to home 
	} else 
		resource.titlebar_file = StringUtil::strdup(DEFAULTTITLEBAR);
	
	//if already allocated memory for keys_file destroy it
	if (resource.keys_file) {
		delete resource.keys_file;
		resource.keys_file = 0;
	}
	
	//get keys filename
	if (XrmGetResource(database, "session.keyFile", "Session.keyFile",
				&value_type, &value)) {
		resource.keys_file = StringUtil::expandFilename(value.addr); //expand ~ to home		
	} else 
		resource.keys_file = StringUtil::strdup(DEFAULTKEYSFILE);
		
	
	if (XrmGetResource(database, "session.iconbar", "Session.Iconbar",
				&value_type, &value)) {
		if (! strncasecmp("true", value.addr, value.size))
      resource.iconbar = true;
    else
      resource.iconbar = false;
	} else
		resource.iconbar = true;
	
	if (XrmGetResource(database, "session.tabs", "Session.Tabs",
				&value_type, &value)) {
		if (! strncasecmp("true", value.addr, value.size))
      resource.tabs = true;
    else
      resource.tabs = false;
	} else
		resource.tabs = true;

	if (XrmGetResource(database, "session.colorsPerChannel",
			"Session.ColorsPerChannel", &value_type, &value)) {
		if (sscanf(value.addr, "%d", &resource.colors_per_channel) != 1)
			resource.colors_per_channel = 4;
		else {
			if (resource.colors_per_channel < 2) 
				resource.colors_per_channel = 2;
			if (resource.colors_per_channel > 6)	
				resource.colors_per_channel = 6;
		}
	} else
		resource.colors_per_channel = 4;

	if (resource.style_file)
		delete [] resource.style_file;

	if (XrmGetResource(database, "session.styleFile", "Session.StyleFile",
				&value_type, &value))
		resource.style_file = StringUtil::expandFilename(value.addr);
	else
		resource.style_file = StringUtil::strdup(DEFAULTSTYLE);

	if (resource.root_cmd) {
		delete [] resource.root_cmd;
		resource.root_cmd = 0;
	}

	if (XrmGetResource(database, "session.rootCommand", "Session.RootCommand", &value_type, &value))
		resource.root_cmd = StringUtil::expandFilename(value.addr);
	else
		resource.root_cmd = 0;

	if (XrmGetResource(database, "session.doubleClickInterval",
				"Session.DoubleClickInterval", &value_type, &value)) {
		if (sscanf(value.addr, "%lu", &resource.double_click_interval) != 1)
			resource.double_click_interval = 250;
	} else
		resource.double_click_interval = 250;

	if (XrmGetResource(database, "session.autoRaiseDelay",
									"Session.AutoRaiseDelay", &value_type, &value)) {
		if (sscanf(value.addr, "%lu", &resource.auto_raise_delay.tv_usec) != 1)
			resource.auto_raise_delay.tv_usec = 250;
	} else
		resource.auto_raise_delay.tv_usec = 250;

	resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec / 1000;
	resource.auto_raise_delay.tv_usec -=
		(resource.auto_raise_delay.tv_sec * 1000);
	resource.auto_raise_delay.tv_usec *= 1000;

	if (XrmGetResource(database, "session.cacheLife", "Session.CacheLife",
										&value_type, &value)) {
		if (sscanf(value.addr, "%lu", &resource.cache_life) != 1)
			resource.cache_life = 5l;
	} else
		resource.cache_life = 5l;

	resource.cache_life *= 60000;

	if (XrmGetResource(database, "session.cacheMax", "Session.CacheMax",
						&value_type, &value)) {
		if (sscanf(value.addr, "%lu", &resource.cache_max) != 1)
			resource.cache_max = 200;
	} else
		resource.cache_max = 200;

	//XrmDestroyDatabase(database);
	
	loadTitlebar();
}
//parseTitleArgs
//parses the titlearg configline
vector<string> Fluxbox::parseTitleArgs(const char *arg) {
	vector<string> args;
	string tmp;
	unsigned int i=0;	
	while ( i<strlen(arg) ) {
		for (; arg[i] != ' ' && i<strlen(arg); i++)
			tmp+=arg[i];
		i++;
		args.push_back(tmp);
		tmp="";
	}
		
	return args;
}

void Fluxbox::setTitlebar(vector<Fluxbox::Titlebar>& dir, const char *arg) {
	vector<string> argv = parseTitleArgs(arg);
	for (unsigned int i=0; i<argv.size(); i++) {
		if (argv[i]==NAME_STICKY)
			dir.push_back(Stick);
		else if (argv[i]==NAME_MAXIMIZE)
			dir.push_back(Maximize);
		else if (argv[i]==NAME_MINIMIZE)
			dir.push_back(Minimize);
		else if (argv[i]== NAME_CLOSE)
			dir.push_back(Close);
		else if (argv[i]==NAME_SHADE)
			dir.push_back(Shade);
		else if (argv[i]==NAME_MENU)
			dir.push_back(Menu);
		else if(argv[i]==NAME_NONE);//do nothing			
		else
			cerr<<"Fluxbox::Titlebar Unknown type: \""<<argv[i]<<"\""<<endl;	
		
	}
}

void Fluxbox::loadTitlebar() {
	XrmDatabase database;
	database = XrmGetFileDatabase(resource.titlebar_file);
	if (!database)
		cerr<<"Fluxbox: Cant open "<<resource.titlebar_file<<" !"<<endl;
	
	XrmValue value;
	char *value_type;
	
	//clear titlebar
	titlebar.left.clear();
	titlebar.right.clear();
	
	if (XrmGetResource(database, "left", "Left", &value_type, &value)) {		
		setTitlebar(titlebar.left, value.addr);
	} else {
		cerr<<"Fluxbox: \'Left\' not found in "<<resource.titlebar_file<<endl;
		cerr<<"Using default."<<endl;
		//default settings		
		titlebar.left.push_back(Shade);
	}
	
	if (XrmGetResource(database, "right", "Right", &value_type, &value)) {				
		setTitlebar(titlebar.right, value.addr);
	} else {
		cerr<<"Fluxbox: \'Right\' not found in "<<resource.titlebar_file<<endl;
		cerr<<"Using default."<<endl;
		//default settings
		titlebar.right.push_back(Stick);
		titlebar.right.push_back(Minimize);
		titlebar.right.push_back(Maximize);
		titlebar.right.push_back(Close);
	}	
	
//	XrmDestroyDatabase(database);
}

void Fluxbox::load_rc(BScreen *screen) {
  XrmDatabase database = (XrmDatabase) 0;

  auto_ptr<char> dbfile(getRcFilename());

  database = XrmGetFileDatabase(dbfile.get());
	if (!database) 
		database = XrmGetFileDatabase(DEFAULT_INITFILE);

  XrmValue value;
  char *value_type, name_lookup[1024], class_lookup[1024];
  int screen_number = screen->getScreenNumber();

  sprintf(name_lookup,  "session.screen%d.fullMaximization", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FullMaximization", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveFullMax(True);
    else
      screen->saveFullMax(False);
  } else
    screen->saveFullMax(False);

  sprintf(name_lookup,  "session.screen%d.focusNewWindows", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FocusNewWindows", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveFocusNew(True);
    else
      screen->saveFocusNew(False);
  } else
    screen->saveFocusNew(False);

  sprintf(name_lookup,  "session.screen%d.focusLastWindow", screen_number);
  sprintf(class_lookup, "Session.Screen%d.focusLastWindow", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveFocusLast(True);
    else
      screen->saveFocusLast(False);
  } else
    screen->saveFocusLast(False);

  sprintf(name_lookup,  "session.screen%d.rowPlacementDirection", screen_number);
  sprintf(class_lookup, "Session.Screen%d.RowPlacementDirection", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "righttoleft", value.size))
      screen->saveRowPlacementDirection(BScreen::RightLeft);
    else
  
  sprintf(name_lookup,  "session.screen%d.maxOverSlit", screen_number);
  sprintf(class_lookup, "Session.Screen%d.MaxOverSlit", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveMaxOverSlit(True);
    else
      screen->saveMaxOverSlit(False);
  } else
    screen->saveMaxOverSlit(False);
      screen->saveRowPlacementDirection(BScreen::LeftRight);
  } else
    screen->saveRowPlacementDirection(BScreen::LeftRight);

  sprintf(name_lookup,  "session.screen%d.colPlacementDirection", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ColPlacementDirection", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "bottomtotop", value.size))
      screen->saveColPlacementDirection(BScreen::BottomTop);
    else
      screen->saveColPlacementDirection(BScreen::TopBottom);
  } else
    screen->saveColPlacementDirection(BScreen::TopBottom);

  sprintf(name_lookup,  "session.screen%d.workspaces", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Workspaces", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int i;
    if (sscanf(value.addr, "%d", &i) != 1) i = 1;
    screen->saveWorkspaces(i);
  } else
    screen->saveWorkspaces(1);

  sprintf(name_lookup,  "session.screen%d.toolbar.widthPercent",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.WidthPercent",
          screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int i;
    if (sscanf(value.addr, "%d", &i) != 1) i = 66;

    if (i <= 0 || i > 100)
      i = 66;

    screen->saveToolbarWidthPercent(i);
  } else
    screen->saveToolbarWidthPercent(66);

  sprintf(name_lookup, "session.screen%d.toolbar.placement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.Placement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      screen->saveToolbarPlacement(Toolbar::TOPLEFT);
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      screen->saveToolbarPlacement(Toolbar::BOTTOMLEFT);
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      screen->saveToolbarPlacement(Toolbar::TOPCENTER);
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      screen->saveToolbarPlacement(Toolbar::TOPRIGHT);
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      screen->saveToolbarPlacement(Toolbar::BOTTOMRIGHT);
    else
      screen->saveToolbarPlacement(Toolbar::BOTTOMCENTER);
  } else
    screen->saveToolbarPlacement(Toolbar::BOTTOMCENTER);

  screen->removeWorkspaceNames();

  sprintf(name_lookup,  "session.screen%d.workspaceNames", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
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
	//TODO MOVE THIS!!!!!!
	#ifdef GNOME	
	{
	Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;
	XGetWindowProperty (screen->getBaseDisplay()->getXDisplay(), 
		screen->getRootWindow(), 
		screen->getBaseDisplay()->getGnomeWorkspaceCountAtom(), 
		0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop);
		
	long val = screen->getNumberOfWorkspaces();
	fprintf(stderr, "HERE!! %s(%d) num wrkspace(%l)\n", __FILE__, __LINE__, val);
		XChangeProperty(screen->getBaseDisplay()->getXDisplay(), screen->getRootWindow(),
			screen->getBaseDisplay()->getGnomeWorkspaceCountAtom(), XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)&val, 1);
	}
	#endif //GNOME
     
		 
  sprintf(name_lookup,  "session.screen%d.toolbar.onTop", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.OnTop", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveToolbarOnTop(True);
    else
      screen->saveToolbarOnTop(False);
  } else
    screen->saveToolbarOnTop(False);

  sprintf(name_lookup,  "session.screen%d.toolbar.autoHide", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.autoHide", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveToolbarAutoHide(True);
    else
      screen->saveToolbarAutoHide(False);
  } else
    screen->saveToolbarAutoHide(False);

//TODO: make this nicer?
	sprintf(name_lookup,  "session.screen%d.focusModel", screen_number);
	sprintf(class_lookup, "Session.Screen%d.FocusModel", screen_number);
	if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
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

  sprintf(name_lookup,  "session.screen%d.windowPlacement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value))
    if (! strncasecmp(value.addr, "RowSmartPlacement", value.size))
      screen->savePlacementPolicy(BScreen::RowSmartPlacement);
    else if (! strncasecmp(value.addr, "ColSmartPlacement", value.size))
      screen->savePlacementPolicy(BScreen::ColSmartPlacement);
    else
      screen->savePlacementPolicy(BScreen::CascadePlacement);
  else
    screen->savePlacementPolicy(BScreen::RowSmartPlacement);

#ifdef    SLIT
  sprintf(name_lookup, "session.screen%d.slit.placement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.Placement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value))
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      screen->saveSlitPlacement(Slit::TopLeft);
    else if (! strncasecmp(value.addr, "CenterLeft", value.size))
      screen->saveSlitPlacement(Slit::CenterLeft);
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      screen->saveSlitPlacement(Slit::BottomLeft);
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      screen->saveSlitPlacement(Slit::TopCenter);
    else if (! strncasecmp(value.addr, "BottomCenter", value.size))
      screen->saveSlitPlacement(Slit::BottomCenter);
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      screen->saveSlitPlacement(Slit::TopRight);
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      screen->saveSlitPlacement(Slit::BottomRight);
    else
      screen->saveSlitPlacement(Slit::CenterRight);
  else
    screen->saveSlitPlacement(Slit::CenterRight);

  sprintf(name_lookup, "session.screen%d.slit.direction", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.Direction", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value))
    if (! strncasecmp(value.addr, "Horizontal", value.size))
      screen->saveSlitDirection(Slit::Horizontal);
    else
      screen->saveSlitDirection(Slit::Vertical);
  else
    screen->saveSlitDirection(Slit::Vertical);

  sprintf(name_lookup, "session.screen%d.slit.onTop", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.OnTop", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value))
    if (! strncasecmp(value.addr, "True", value.size))
      screen->saveSlitOnTop(True);
    else
      screen->saveSlitOnTop(False);
  else
    screen->saveSlitOnTop(False);

  sprintf(name_lookup, "session.screen%d.slit.autoHide", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.AutoHide", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value))
    if (! strncasecmp(value.addr, "True", value.size))
      screen->saveSlitAutoHide(True);
    else
      screen->saveSlitAutoHide(False);
  else
    screen->saveSlitAutoHide(False);
#endif // SLIT

#ifdef    HAVE_STRFTIME
  sprintf(name_lookup,  "session.screen%d.strftimeFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value))
    screen->saveStrftimeFormat(value.addr);
  else
    screen->saveStrftimeFormat("%I:%M %p");
#else //  HAVE_STRFTIME

  sprintf(name_lookup,  "session.screen%d.dateFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.DateFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (strncasecmp(value.addr, "european", value.size))
      screen->saveDateFormat(B_AmericanDate);
    else
      screen->saveDateFormat(B_EuropeanDate);
  } else
    screen->saveDateFormat(B_AmericanDate);

  sprintf(name_lookup,  "session.screen%d.clockFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ClockFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int clock;
    if (sscanf(value.addr, "%d", &clock) != 1) screen->saveClock24Hour(False);
    else if (clock == 24) screen->saveClock24Hour(True);
    else screen->saveClock24Hour(False);
  } else
    screen->saveClock24Hour(False);
#endif // HAVE_STRFTIME

  sprintf(name_lookup,  "session.screen%d.edgeSnapThreshold", screen_number);
  sprintf(class_lookup, "Session.Screen%d.EdgeSnapThreshold", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int threshold;
    if (sscanf(value.addr, "%d", &threshold) != 1)
      screen->saveEdgeSnapThreshold(0);
    else
      screen->saveEdgeSnapThreshold(threshold);
  } else
    screen->saveEdgeSnapThreshold(0);

  sprintf(name_lookup,  "session.screen%d.imageDither", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ImageDither", screen_number);
  if (XrmGetResource(database, "session.imageDither", "Session.ImageDither",
		     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      screen->saveImageDither(True);
    else
      screen->saveImageDither(False);
  } else
    screen->saveImageDither(True);

  if (XrmGetResource(database, "session.opaqueMove", "Session.OpaqueMove",
                     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      screen->saveOpaqueMove(True);
    else
      screen->saveOpaqueMove(False);
  } else
    screen->saveOpaqueMove(False);

	sprintf(name_lookup,  "session.screen%d.tab.width", screen_number);
	sprintf(class_lookup, "Session.Screen%d.Tab.Width", screen_number);
	if (XrmGetResource(database, name_lookup, class_lookup,
			&value_type, &value)) {
		unsigned int tmp_val;

		if (sscanf(value.addr, "%d", &tmp_val) != 1)
			screen->saveTabWidth(64); // default tab width
		else {	
			//TODO: should we remove checks for max/min?
			if (tmp_val > 512)
				screen->saveTabWidth(512);
			else if (tmp_val < 5)
				screen->saveTabWidth(5);
			else
				screen->saveTabWidth(tmp_val);
		}
	} else
		screen->saveTabWidth(64); // default tab width

  sprintf(name_lookup,  "session.screen%d.tab.height", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Tab.Height", screen_number);
	if (XrmGetResource(database, name_lookup, class_lookup,
				&value_type, &value)) {
		unsigned int tmp_val;

		if (sscanf(value.addr, "%d", &tmp_val) != 1)
			screen->saveTabHeight(16); // default tab height
		else {
			//TODO: should we remove checks for max/min?
			if (tmp_val > 50)
				screen->saveTabHeight(50);
			else if (tmp_val < 5)
				screen->saveTabHeight(5);
			else
				screen->saveTabHeight(tmp_val);
		}
	} else
		screen->saveTabHeight(16); // default tab height

  sprintf(name_lookup,  "session.screen%d.tab.placement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Tab.Placement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup,
  			&value_type, &value)) {		
		screen->saveTabPlacement(Tab::getTabPlacementNum(value.addr));
  } else
		screen->saveTabPlacement(Tab::PTOP);

  sprintf(name_lookup,  "session.screen%d.tab.alignment", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Tab.Alignment", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup,
  			&value_type, &value)) {		
		screen->saveTabAlignment(Tab::getTabAlignmentNum(value.addr));
	} else
		screen->saveTabAlignment(Tab::ALEFT);

  sprintf(name_lookup,  "session.screen%d.tab.rotatevertical", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Tab.RotateVertical", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup,
		     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      screen->saveTabRotateVertical(True);
    else
      screen->saveTabRotateVertical(False);
  } else
    screen->saveTabRotateVertical(False);

  sprintf(name_lookup,  "session.screen%d.sloppywindowgrouping", screen_number);
  sprintf(class_lookup, "Session.Screen%d.SloppyWindowGrouping", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup,
		     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      screen->saveSloppyWindowGrouping(True);
    else
      screen->saveSloppyWindowGrouping(False);
  } else
    screen->saveSloppyWindowGrouping(False);

}


void Fluxbox::reload_rc(void) {
  load_rc();
  reconfigure();
}


void Fluxbox::reconfigure(void) {
  reconfigure_wait = True;

  if (! timer->isTiming()) timer->start();
}


void Fluxbox::real_reconfigure(void) {
	grab();

	XrmDatabase new_blackboxrc = (XrmDatabase) 0;
	char style[MAXPATHLEN + 64];

	auto_ptr<char> dbfile(getRcFilename());
 
	sprintf(style, "session.styleFile: %s", resource.style_file);
	XrmPutLineResource(&new_blackboxrc, style);

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
	key->reconfigure(resource.keys_file);

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


void Fluxbox::saveStyleFilename(const char *filename) {
  if (resource.style_file)
    delete [] resource.style_file;

  resource.style_file = StringUtil::strdup(filename);
}


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

  reconfigure_wait = reread_menu_wait = False;
}


void Fluxbox::setFocusedWindow(FluxboxWindow *win) {
	
	BScreen *old_screen = (BScreen *) 0, *screen = (BScreen *) 0;
  FluxboxWindow *old_win = (FluxboxWindow *) 0;
  Toolbar *old_tbar = (Toolbar *) 0, *tbar = (Toolbar *) 0;
  Workspace *old_wkspc = (Workspace *) 0, *wkspc = (Workspace *) 0;

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
