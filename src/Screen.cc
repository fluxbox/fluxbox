// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Screen.cc for Blackbox - an X11 Window manager
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

// $id$

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "i18n.hh"
#include "fluxbox.hh"
#include "Clientmenu.hh"
#include "Icon.hh"
#include "Image.hh"
#include "Screen.hh"
#include "StringUtil.hh"

#ifdef		SLIT
#include "Slit.hh"
#endif // SLIT

#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef		STDC_HEADERS
#	include <stdlib.h>
#	include <string.h>
#	include <sys/types.h>
#endif // STDC_HEADERS

#ifdef		HAVE_CTYPE_H
#	include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef		HAVE_DIRENT_H
#	include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef		HAVE_LOCALE_H
#	include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef		HAVE_UNISTD_H
#	include <sys/types.h>
#	include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef		HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef		HAVE_STDARG_H
#	include <stdarg.h>
#endif // HAVE_STDARG_H

#ifndef		HAVE_SNPRINTF
#	include "bsd-snprintf.h"
#endif // !HAVE_SNPRINTF

#ifndef	 MAXPATHLEN
#define	 MAXPATHLEN 255
#endif // MAXPATHLEN

#ifndef	 FONT_ELEMENT_SIZE
#define	 FONT_ELEMENT_SIZE 50
#endif // FONT_ELEMENT_SIZE

#include <iostream>
using namespace std;

static Bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
	fprintf(stderr,
		I18n::instance()->
		getMessage(
#ifdef		NLS
			ScreenSet, ScreenAnotherWMRunning,
#else // !NLS
			0, 0,
#endif // NLS
			"BScreen::BScreen: an error occured while querying the X server.\n"
			"	another window manager already running on display %s.\n"),
			DisplayString(display));

	running = False;

	return(-1);
}

static int dcmp(const void *one, const void *two) {
	return (strcmp((*(char **) one), (*(char **) two)));
}

BScreen::BScreen(Fluxbox *b, int scrn) : ScreenInfo(b, scrn) {
	theme = 0;
	fluxbox = b;

	event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
			SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
			ButtonPressMask | ButtonReleaseMask;

	XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
	XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), event_mask);
	XSync(getBaseDisplay()->getXDisplay(), False);
	XSetErrorHandler((XErrorHandler) old);

	managed = running;
	if (! managed)
		return;
	
	I18n *i18n = I18n::instance();
	
	fprintf(stderr,
		i18n->
		getMessage(
#ifdef		NLS
				ScreenSet, ScreenManagingScreen,
#else // !NLS
				0, 0,
#endif // NLS
				"BScreen::BScreen: managing screen %d "
				"using visual 0x%lx, depth %d\n"),
		getScreenNumber(), XVisualIDFromVisual(getVisual()),
		getDepth());

	rootmenu = 0;
		
#ifdef		HAVE_STRFTIME
	resource.strftime_format = 0;
#endif // HAVE_STRFTIME

#ifdef		HAVE_GETPID
	pid_t bpid = getpid();

	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
			fluxbox->getFluxboxPidAtom(), XA_CARDINAL,
			sizeof(pid_t) * 8, PropModeReplace,
			(unsigned char *) &bpid, 1);
#endif // HAVE_GETPID


	XDefineCursor(getBaseDisplay()->getXDisplay(), getRootWindow(),
			fluxbox->getSessionCursor());

	workspaceNames = new LinkedList<char>;
	workspacesList = new LinkedList<Workspace>;
	rootmenuList = new LinkedList<Rootmenu>;
	netizenList = new LinkedList<Netizen>;
	iconList = new LinkedList<FluxboxWindow>;

	image_control =
		new BImageControl(fluxbox, this, True, fluxbox->getColorsPerChannel(),
				fluxbox->getCacheLife(), fluxbox->getCacheMax());
	image_control->installRootColormap();
	root_colormap_installed = True;

	fluxbox->load_rc(this);

	image_control->setDither(resource.image_dither);
	theme = new Theme(getBaseDisplay()->getXDisplay(), getRootWindow(), getColormap(), getScreenNumber(), 
			image_control, fluxbox->getStyleFilename(), fluxbox->getRootCommand());
	
#ifdef GNOME
	/* create the GNOME window */
  Window gnome_win = XCreateSimpleWindow(getBaseDisplay()->getXDisplay(),
			getRootWindow(), 0, 0, 5, 5, 0, 0, 0);

  /* supported WM check */
  XChangeProperty(getBaseDisplay()->getXDisplay(),
			getRootWindow(), getBaseDisplay()->getGnomeSupportingWMCheckAtom(), 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);
  XChangeProperty(getBaseDisplay()->getXDisplay(), gnome_win, 
		getBaseDisplay()->getGnomeSupportingWMCheckAtom(), 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);
	 
  XChangeProperty(getBaseDisplay()->getXDisplay(),
		getRootWindow(), getBaseDisplay()->getGnomeProtAtom(),
		XA_ATOM, 32, PropModeReplace,
		(unsigned char *)getBaseDisplay()->getGnomeListAtoms(), 10);			
#endif 


	const char *s =	i18n->getMessage(
#ifdef		NLS
						ScreenSet, ScreenPositionLength,
#else // !NLS
						0, 0,
#endif // NLS
						"0: 0000 x 0: 0000");
	int l = strlen(s);

	if (i18n->multibyte()) {
		XRectangle ink, logical;
		XmbTextExtents(theme->getWindowStyle().font.set, s, l, &ink, &logical);
		geom_w = logical.width;

		geom_h = theme->getWindowStyle().font.set_extents->max_ink_extent.height;
	} else {
		geom_h = theme->getWindowStyle().font.fontstruct->ascent +
			 theme->getWindowStyle().font.fontstruct->descent;

		geom_w = XTextWidth(theme->getWindowStyle().font.fontstruct, s, l);
	}

	geom_w += getBevelWidth()*2;
	geom_h += getBevelWidth()*2;

	XSetWindowAttributes attrib;
	unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
	attrib.border_pixel = getBorderColor()->getPixel();
	attrib.colormap = getColormap();
	attrib.save_under = True;

	geom_window =
		XCreateWindow(getBaseDisplay()->getXDisplay(), getRootWindow(),
									0, 0, geom_w, geom_h, theme->getBorderWidth(), getDepth(),
									InputOutput, getVisual(), mask, &attrib);
	geom_visible = False;

	if (theme->getWindowStyle().l_focus.getTexture() & BImage::PARENTRELATIVE) {
		if (theme->getWindowStyle().t_focus.getTexture() ==
			(BImage::FLAT | BImage::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				 theme->getWindowStyle().t_focus.getColor()->getPixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
								 &theme->getWindowStyle().t_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				 geom_window, geom_pixmap);
		}
	} else {
		if (theme->getWindowStyle().l_focus.getTexture() ==
				(BImage::FLAT | BImage::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				 theme->getWindowStyle().l_focus.getColor()->getPixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
								 &theme->getWindowStyle().l_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				 geom_window, geom_pixmap);
		}
	}

	workspacemenu = new Workspacemenu(this);
	iconmenu = new Iconmenu(this);	
	configmenu = new Configmenu(this);

	Workspace *wkspc = (Workspace *) 0;
	if (resource.workspaces != 0) {
		for (int i = 0; i < resource.workspaces; ++i) {
			wkspc = new Workspace(this, workspacesList->count());
			workspacesList->insert(wkspc);
			workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
		}
	} else {
		wkspc = new Workspace(this, workspacesList->count());
		workspacesList->insert(wkspc);
		workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
	}

	workspacemenu->insert(i18n->
			getMessage(
#ifdef		NLS
					 IconSet, IconIcons,
#else // !NLS
					 0, 0,
#endif // NLS
					 "Icons"),
			iconmenu);
	workspacemenu->update();

	current_workspace = workspacesList->first();
	workspacemenu->setItemSelected(2, True);

	toolbar = new Toolbar(this);

#ifdef		SLIT
	slit = new Slit(this);
#endif // SLIT

	InitMenu();

	raiseWindows(0, 0);
	rootmenu->update();

	changeWorkspaceID(0);

	int i;
	unsigned int nchild;
	Window r, p, *children;
	XQueryTree(getBaseDisplay()->getXDisplay(), getRootWindow(), &r, &p,
			 &children, &nchild);

	// preen the window list of all icon windows... for better dockapp support
	for (i = 0; i < (int) nchild; i++) {
		if (children[i] == None) continue;

		XWMHints *wmhints = XGetWMHints(getBaseDisplay()->getXDisplay(),
						children[i]);

		if (wmhints) {
			if ((wmhints->flags & IconWindowHint) &&
		(wmhints->icon_window != children[i]))
				for (int j = 0; j < (int) nchild; j++)
					if (children[j] == wmhints->icon_window) {
						children[j] = None;

						break;
					}

			XFree(wmhints);
		}
	}

	// manage shown windows
	for (i = 0; i < (int) nchild; ++i) {
		if (children[i] == None || (! fluxbox->validateWindow(children[i])))
			continue;

		XWindowAttributes attrib;
		if (XGetWindowAttributes(getBaseDisplay()->getXDisplay(), children[i],
				&attrib)) {
			if (attrib.override_redirect) 
				continue;

			if (attrib.map_state != IsUnmapped) {
				new FluxboxWindow(children[i], this);

				FluxboxWindow *win = fluxbox->searchWindow(children[i]);
				if (win) {
					XMapRequestEvent mre;
					mre.window = children[i];
					win->restoreAttributes();
					win->mapRequestEvent(&mre);
				}
			}
		}
	}

	if (! resource.sloppy_focus)
		XSetInputFocus(getBaseDisplay()->getXDisplay(), toolbar->getWindowID(),
			RevertToParent, CurrentTime);

	XFree(children);
	XFlush(getBaseDisplay()->getXDisplay());
	
}


BScreen::~BScreen(void) {
	if (! managed) return;

	if (geom_pixmap != None)
		image_control->removeImage(geom_pixmap);

	if (geom_window != None)
		XDestroyWindow(getBaseDisplay()->getXDisplay(), geom_window);

	removeWorkspaceNames();

	while (workspacesList->count())
		delete workspacesList->remove(0);

	while (rootmenuList->count())
		rootmenuList->remove(0);

	while (iconList->count())
		delete iconList->remove(0);

	while (netizenList->count())
		delete netizenList->remove(0);

#ifdef		HAVE_STRFTIME
	if (resource.strftime_format)
		delete [] resource.strftime_format;
#endif // HAVE_STRFTIME

	delete rootmenu;
	delete workspacemenu;
	delete iconmenu;
	delete configmenu;

#ifdef		SLIT
	delete slit;
#endif // SLIT

	delete toolbar;
	delete image_control;

	delete workspacesList;
	delete workspaceNames;
	delete rootmenuList;
	delete iconList;
	delete netizenList;
	delete theme;

}

void BScreen::reconfigure(void) {
	if (Fluxbox::instance()->getRootCommand())
		theme->setRootCommand(Fluxbox::instance()->getRootCommand());
	else
		theme->setRootCommand("");

	theme->load(fluxbox->getStyleFilename());
	theme->reconfigure();
	I18n *i18n = I18n::instance();

	const char *s = i18n->getMessage(
#ifdef		NLS
			ScreenSet, ScreenPositionLength,
#else // !NLS
			0, 0,
#endif // NLS
			"0: 0000 x 0: 0000");
	int l = strlen(s);

	if (i18n->multibyte()) {
		XRectangle ink, logical;
		XmbTextExtents(theme->getWindowStyle().font.set, s, l, &ink, &logical);
		geom_w = logical.width;

		geom_h = theme->getWindowStyle().font.set_extents->max_ink_extent.height;
	} else {
		geom_w = XTextWidth(theme->getWindowStyle().font.fontstruct, s, l);

		geom_h = theme->getWindowStyle().font.fontstruct->ascent +
			theme->getWindowStyle().font.fontstruct->descent; 
	}

	geom_w += getBevelWidth()*2;
	geom_h += getBevelWidth()*2;

	Pixmap tmp = geom_pixmap;
	if (theme->getWindowStyle().l_focus.getTexture() & BImage::PARENTRELATIVE) {
		if (theme->getWindowStyle().t_focus.getTexture() ==
				(BImage::FLAT | BImage::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				theme->getWindowStyle().t_focus.getColor()->getPixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
				&theme->getWindowStyle().t_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				geom_window, geom_pixmap);
		}
	} else {
		if (theme->getWindowStyle().l_focus.getTexture() ==
				(BImage::FLAT | BImage::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				theme->getWindowStyle().l_focus.getColor()->getPixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
				&theme->getWindowStyle().l_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				geom_window, geom_pixmap);
		}
	}
	if (tmp) image_control->removeImage(tmp);

	XSetWindowBorderWidth(getBaseDisplay()->getXDisplay(), geom_window,
												theme->getBorderWidth());
	XSetWindowBorder(getBaseDisplay()->getXDisplay(), geom_window,
									 theme->getBorderColor().getPixel());

	workspacemenu->reconfigure();
	iconmenu->reconfigure();

	{
		int remember_sub = rootmenu->getCurrentSubmenu();
		InitMenu();
		raiseWindows(0, 0);
		rootmenu->reconfigure();
		rootmenu->drawSubmenu(remember_sub);
	}

	configmenu->reconfigure();

	toolbar->reconfigure();

#ifdef		SLIT
	slit->reconfigure();
#endif // SLIT

	LinkedListIterator<Workspace> wit(workspacesList);
	for (; wit.current(); wit++)
		wit.current()->reconfigure();

	LinkedListIterator<FluxboxWindow> iit(iconList);
	for (; iit.current(); iit++)
		if (iit.current()->validateClient())
			iit.current()->reconfigure();

	image_control->timeout();
}


void BScreen::rereadMenu(void) {
	InitMenu();
	raiseWindows(0, 0);

	rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames(void) {
	while (workspaceNames->count())
		delete [] workspaceNames->remove(0);
}

void BScreen::updateWorkspaceNamesAtom(void) {

#ifdef GNOME	
	XTextProperty	text;
	int number_of_desks = workspaceNames->count();
	
	char s[1024];
	char *names[number_of_desks];		
	
	for (int i = 0; i < number_of_desks; i++) {		
		sprintf(s, "Desktop %i", i);
		names[i] = new char[strlen(s) + 1];
		strcpy(names[i], s);
	}
	
	if (XStringListToTextProperty(names, number_of_desks, &text)) {
		XSetTextProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
			 &text, getBaseDisplay()->getGnomeWorkspaceNamesAtom());
		XFree(text.value);
	}
	
	for (int i = 0; i < number_of_desks; i++)
		delete names[i];			
  	
#endif

}

void BScreen::addIcon(FluxboxWindow *w) {
	if (! w) return;

	w->setWorkspace(-1);
	w->setWindowNumber(iconList->count());

	iconList->insert(w);

	iconmenu->insert((const char **) w->getIconTitle());	
	iconmenu->update();
	toolbar->addIcon(w);
}


void BScreen::removeIcon(FluxboxWindow *w) {
	if (! w) return;

	iconList->remove(w->getWindowNumber());

	iconmenu->remove(w->getWindowNumber());
	iconmenu->update();
	toolbar->delIcon(w);
	
	LinkedListIterator<FluxboxWindow> it(iconList);
	for (int i = 0; it.current(); it++, i++)
		it.current()->setWindowNumber(i);
}


FluxboxWindow *BScreen::getIcon(int index) {
	if (index >= 0 && index < iconList->count())
		return iconList->find(index);

	return (FluxboxWindow *) 0;
}


int BScreen::addWorkspace(void) {
	Workspace *wkspc = new Workspace(this, workspacesList->count());
	workspacesList->insert(wkspc);

	workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
			wkspc->getWorkspaceID() + 1);
	workspacemenu->update();

	toolbar->reconfigure();

	updateNetizenWorkspaceCount();	
	
	
	return workspacesList->count();
	
}


int BScreen::removeLastWorkspace(void) {
	if (workspacesList->count() > 1) {
		Workspace *wkspc = workspacesList->last();

		if (current_workspace->getWorkspaceID() == wkspc->getWorkspaceID())
			changeWorkspaceID(current_workspace->getWorkspaceID() - 1);

		wkspc->removeAll();

		workspacemenu->remove(wkspc->getWorkspaceID() + 2);
		workspacemenu->update();

		workspacesList->remove(wkspc);
		delete wkspc;

		toolbar->reconfigure();

		updateNetizenWorkspaceCount();
	
		return workspacesList->count();
	}

	return 0;
}


void BScreen::changeWorkspaceID(int id) {
	if (! current_workspace || id >= workspacesList->count() || id < 0)
		return;
	
	if (id != current_workspace->getWorkspaceID()) {
		XSync(fluxbox->getXDisplay(), True);
		
		current_workspace->hideAll();

		workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
					 False);

		if (fluxbox->getFocusedWindow() &&
				fluxbox->getFocusedWindow()->getScreen() == this &&
				(! fluxbox->getFocusedWindow()->isStuck())) {
				
			current_workspace->setLastFocusedWindow(fluxbox->getFocusedWindow());
			fluxbox->setFocusedWindow((FluxboxWindow *) 0);
			
		}

		current_workspace = getWorkspace(id);

		workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
					 True);
		toolbar->redrawWorkspaceLabel(True);

		current_workspace->showAll();

		if (resource.focus_last && current_workspace->getLastFocusedWindow())
			current_workspace->getLastFocusedWindow()->setInputFocus();		
			
	}

	updateNetizenCurrentWorkspace();
}


void BScreen::addNetizen(Netizen *n) {
	netizenList->insert(n);

	n->sendWorkspaceCount();
	n->sendCurrentWorkspace();

	LinkedListIterator<Workspace> it(workspacesList);
	for (; it.current(); it++) {
		int i;
		for (i = 0; i < it.current()->getCount(); i++)
			n->sendWindowAdd(it.current()->getWindow(i)->getClientWindow(),
											 it.current()->getWorkspaceID());
	}

	Window f = ((fluxbox->getFocusedWindow()) ?
							fluxbox->getFocusedWindow()->getClientWindow() : None);
	n->sendWindowFocus(f);
}


void BScreen::removeNetizen(Window w) {
	LinkedListIterator<Netizen> it(netizenList);
	int i = 0;

	for (; it.current(); it++, i++)
		if (it.current()->getWindowID() == w) {
			Netizen *n = netizenList->remove(i);
			delete n;

			break;
		}
}


void BScreen::updateNetizenCurrentWorkspace(void) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendCurrentWorkspace();
}


void BScreen::updateNetizenWorkspaceCount(void) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendWorkspaceCount();
}


void BScreen::updateNetizenWindowFocus(void) {
	LinkedListIterator<Netizen> it(netizenList);
	Window f = ((fluxbox->getFocusedWindow()) ?
							fluxbox->getFocusedWindow()->getClientWindow() : None);
	for (; it.current(); it++)
		it.current()->sendWindowFocus(f);
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendWindowAdd(w, p);
}


void BScreen::updateNetizenWindowDel(Window w) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendWindowDel(w);
}


void BScreen::updateNetizenWindowRaise(Window w) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendWindowRaise(w);
}


void BScreen::updateNetizenWindowLower(Window w) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendWindowLower(w);
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
	LinkedListIterator<Netizen> it(netizenList);
	for (; it.current(); it++)
		it.current()->sendConfigNotify(e);
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {
	Window session_stack[(num + workspacesList->count() + rootmenuList->count() + 13)];
	int i = 0;

	XRaiseWindow(getBaseDisplay()->getXDisplay(), iconmenu->getWindowID());
	session_stack[i++] = iconmenu->getWindowID();

	LinkedListIterator<Workspace> wit(workspacesList);
	for (; wit.current(); wit++)
		session_stack[i++] = wit.current()->getMenu()->getWindowID();

	session_stack[i++] = workspacemenu->getWindowID();

	session_stack[i++] = configmenu->getFocusmenu()->getWindowID();
	session_stack[i++] = configmenu->getPlacementmenu()->getWindowID();
	session_stack[i++] = configmenu->getTabmenu()->getWindowID();
	session_stack[i++] = configmenu->getWindowID();

#ifdef		SLIT
	session_stack[i++] = slit->getMenu()->getDirectionmenu()->getWindowID();
	session_stack[i++] = slit->getMenu()->getPlacementmenu()->getWindowID();
	session_stack[i++] = slit->getMenu()->getWindowID();
#endif // SLIT

	session_stack[i++] =
		toolbar->getMenu()->getPlacementmenu()->getWindowID();
	session_stack[i++] = toolbar->getMenu()->getWindowID();

	LinkedListIterator<Rootmenu> rit(rootmenuList);
	for (; rit.current(); rit++)
		session_stack[i++] = rit.current()->getWindowID();
	session_stack[i++] = rootmenu->getWindowID();

	if (toolbar->isOnTop())
		session_stack[i++] = toolbar->getWindowID();

#ifdef		SLIT
	if (slit->isOnTop())
		session_stack[i++] = slit->getWindowID();
#endif // SLIT
	
	int k=num;
	while (k--)
		session_stack[i++] = *(workspace_stack + k);

	XRestackWindows(getBaseDisplay()->getXDisplay(), session_stack, i);

//	delete session_stack;
}


#ifdef		HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
	if (resource.strftime_format)
		delete [] resource.strftime_format;

	resource.strftime_format = StringUtil::strdup(format);
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(char *name) {
	workspaceNames->insert(StringUtil::strdup(name));
	
}


void BScreen::getNameOfWorkspace(int id, char **name) {
	if (id >= 0 && id < workspaceNames->count()) {
		char *wkspc_name = workspaceNames->find(id);

		if (wkspc_name)
			*name = StringUtil::strdup(wkspc_name);
	} else
		*name = 0;
}


void BScreen::reassociateWindow(FluxboxWindow *w, int wkspc_id, Bool ignore_sticky) {
	if (! w) return;

	if (wkspc_id == -1)
		wkspc_id = current_workspace->getWorkspaceID();

	if (w->getWorkspaceNumber() == wkspc_id)
		return;

	if (w->isIconic()) {
		removeIcon(w);
		getWorkspace(wkspc_id)->addWindow(w);
	} else if (ignore_sticky || ! w->isStuck()) {
		getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
		getWorkspace(wkspc_id)->addWindow(w);
	}
}


void BScreen::nextFocus(void) {
	Bool have_focused = False;
	int focused_window_number = -1;
	FluxboxWindow *next;

	if (fluxbox->getFocusedWindow())
		if (fluxbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
			getScreenNumber()) {
			have_focused = True;
			focused_window_number = fluxbox->getFocusedWindow()->getWindowNumber();
		}

	if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
		int next_window_number = focused_window_number;
		do {
			if ((++next_window_number) >= getCurrentWorkspace()->getCount())
				next_window_number = 0;

			next = getCurrentWorkspace()->getWindow(next_window_number);
		} while ((! next->setInputFocus()) && (next_window_number !=
						 focused_window_number));

		if (next_window_number != focused_window_number)
			getCurrentWorkspace()->raiseWindow(next);
	} else if (getCurrentWorkspace()->getCount() >= 1) {
		next = current_workspace->getWindow(0);

		current_workspace->raiseWindow(next);
		next->setInputFocus();
	}
}


void BScreen::prevFocus(void) {
	Bool have_focused = False;
	int focused_window_number = -1;
	FluxboxWindow *prev;

	if (fluxbox->getFocusedWindow())
		if (fluxbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	getScreenNumber()) {
			have_focused = True;
			focused_window_number = fluxbox->getFocusedWindow()->getWindowNumber();
		}

	if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
		int prev_window_number = focused_window_number;
		do {
			if ((--prev_window_number) < 0)
	prev_window_number = getCurrentWorkspace()->getCount() - 1;

			prev = getCurrentWorkspace()->getWindow(prev_window_number);
		} while ((! prev->setInputFocus()) && (prev_window_number !=
						 focused_window_number));

		if (prev_window_number != focused_window_number)
			getCurrentWorkspace()->raiseWindow(prev);
	} else if (getCurrentWorkspace()->getCount() >= 1) {
		prev = current_workspace->getWindow(0);

		current_workspace->raiseWindow(prev);
		prev->setInputFocus();
	}
}

//--------- raiseFocus -----------
// Raise the current focused window
//--------------------------------
void BScreen::raiseFocus(void) {
	Bool have_focused = False;
	int focused_window_number = -1;

	if (fluxbox->getFocusedWindow())
		if (fluxbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	getScreenNumber()) {
			have_focused = True;
			focused_window_number = fluxbox->getFocusedWindow()->getWindowNumber();
		}

	if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
		getWorkspace(fluxbox->getFocusedWindow()->getWorkspaceNumber())->
			raiseWindow(fluxbox->getFocusedWindow());
}


void BScreen::InitMenu(void) {
	I18n *i18n = I18n::instance();
	
	if (rootmenu) {
		while (rootmenuList->count())
			rootmenuList->remove(0);

		while (rootmenu->getCount())
			rootmenu->remove(0);
	} else
		rootmenu = new Rootmenu(this);

	Bool defaultMenu = True;

	if (fluxbox->getMenuFilename()) {
		FILE *menu_file = fopen(fluxbox->getMenuFilename(), "r");

		if (menu_file) {
			if (! feof(menu_file)) {
				const int buff_size = 1024;
				char line[buff_size], label[buff_size];
				memset(line, 0, buff_size);
				memset(label, 0, buff_size);

				while (fgets(line, buff_size, menu_file) && ! feof(menu_file)) {
					if (line[0] != '#') {
						int i, key = 0, index = -1, len = strlen(line);
						key = 0;
						for (i = 0; i < len; i++) {
							if (line[i] == '[')
								index = 0;
							else if (line[i] == ']')
								break;
							else if (line[i] != ' ')
								if (index++ >= 0)
									key += tolower(line[i]);
						}
						if (key == 517) {
							index = -1;
							for (i = index; i < len; i++) {
								if (line[i] == '(') index = 0;
								else if (line[i] == ')') break;
								else if (index++ >= 0) {
									if (line[i] == '\\' && i < len - 1) i++;
										label[index - 1] = line[i];
								}
							}

							if (index == -1)
								index = 0;
							label[index] = '\0';

							rootmenu->setLabel(label);
							defaultMenu = parseMenuFile(menu_file, rootmenu);
							break;
						}
					}
				}
			} else {
				fprintf(stderr,
					i18n->getMessage(
#ifdef		NLS
						ScreenSet, ScreenEmptyMenuFile,
#else // !NLS
						0, 0,
#endif // NLS
						"%s: Empty menu file"),
					fluxbox->getMenuFilename());
			}
			
			fclose(menu_file);
		} else
			perror(fluxbox->getMenuFilename());
	}

	if (defaultMenu) {
		rootmenu->setInternalMenu();
		rootmenu->insert(i18n->getMessage(
#ifdef		NLS
						ScreenSet, Screenxterm,
#else // !NLS
						0, 0,
#endif // NLS
						"xterm"),
						BScreen::Execute,
						i18n->getMessage(
#ifdef		NLS
							ScreenSet, Screenxterm,
#else // !NLS
							0, 0,
#endif // NLS
							"xterm"));
		rootmenu->insert(i18n->getMessage(
#ifdef		NLS
						ScreenSet, ScreenRestart,
#else // !NLS
						0, 0,
#endif // NLS
						"Restart"),
					BScreen::Restart);
		rootmenu->insert(i18n->getMessage(
#ifdef		NLS
						ScreenSet, ScreenExit,
#else // !NLS
						0, 0,
#endif // NLS
						"Exit"),
				BScreen::Exit);
	} else
		fluxbox->saveMenuFilename(fluxbox->getMenuFilename());
}


Bool BScreen::parseMenuFile(FILE *file, Rootmenu *menu) {
	const int buff_size = 1024;
	char line[buff_size], label[buff_size], command[buff_size];

	while (! feof(file)) {
		memset(line, 0, buff_size);
		memset(label, 0, buff_size);
		memset(command, 0, buff_size);

		if (fgets(line, buff_size, file)) {
			if (line[0] != '#') {
				register int i, key = 0, parse = 0, 
						index = -1,	
						line_length = strlen(line), label_length = 0, command_length = 0;

				// determine the keyword
				key = 0;
				for (i = 0; i < line_length; i++) {
					if (line[i] == '[')
						parse = 1;
					else if (line[i] == ']') 
						break;
					else if (line[i] != ' ')
						if (parse)
							key += tolower(line[i]);
				}

				// get the label enclosed in ()'s
				parse = 0;

				for (i = 0; i < line_length; i++) {
					if (line[i] == '(') {
						index = 0;
						parse = 1;
					} else if (line[i] == ')') 
						break;
					else if (index++ >= 0) {
						if (line[i] == '\\' && i < line_length - 1)
							i++;
						label[index - 1] = line[i];
					}
				}
				if (parse) {
					label[index] = '\0';
					label_length = index;
				} else {
					label[0] = '\0';
					label_length = 0;
				}

				// get the command enclosed in {}'s
				parse = 0;
				index = -1;
				for (i = 0; i < line_length; i++)
					if (line[i] == '{') {
						index = 0;
						parse = 1;
					} else if (line[i] == '}') break;
				else if (index++ >= 0) {
					if (line[i] == '\\' && i < line_length - 1)
						i++;
					command[index - 1] = line[i];
				}

				if (parse) {
					command[index] = '\0';
					command_length = index;
				} else {
					command[0] = '\0';
					command_length = 0;
				}

				I18n *i18n = I18n::instance();
	
			switch (key) {
				case 311: //end
					return ((menu->getCount() == 0) ? True : False);
					break;

				case 333: // nop
					menu->insert(label);
					break;

				case 421: // exec
				if ((! *label) && (! *command)) {
					fprintf(stderr,
					i18n->getMessage(
#ifdef		NLS
						 ScreenSet, ScreenEXECError,
#else // !NLS
						 0, 0,
#endif // NLS
						 "BScreen::parseMenuFile: [exec] error, "
						 "no menu label and/or command defined\n"));
					continue;
				}

				menu->insert(label, BScreen::Execute, command);		
				break;

			case 442: // exit
				if (! *label) {
					fprintf(stderr,
						i18n->getMessage(
#ifdef		NLS
						 ScreenSet, ScreenEXITError,
#else // !NLS
						 0, 0,
#endif // NLS
						 "BScreen::parseMenuFile: [exit] error, "
						 "no menu label defined\n"));
				continue;
				}

				menu->insert(label, BScreen::Exit);
				break;
	
			case 561: // style
			{
				if ((! *label) || (! *command)) {
					fprintf(stderr,
						i18n->
						getMessage(
#ifdef		NLS
					 ScreenSet, ScreenSTYLEError,
#else // !NLS
						 0, 0,
#endif // NLS
					 "BScreen::parseMenuFile: [style] error, "
					 "no menu label and/or filename defined\n"));
					continue;
				}

				char style[MAXPATHLEN];

				// perform shell style ~ home directory expansion
				char *homedir = 0;
				int homedir_len = 0;
				if (*command == '~' && *(command + 1) == '/') {
					homedir = getenv("HOME");
					homedir_len = strlen(homedir);
				}

				if (homedir && homedir_len != 0) {
					strncpy(style, homedir, homedir_len);

					strncpy(style + homedir_len, command + 1,
						command_length - 1);
					*(style + command_length + homedir_len - 1) = '\0';
				} else {
					strncpy(style, command, command_length);
					*(style + command_length) = '\0';
				}

				menu->insert(label, BScreen::SetStyle, style);
			}

			break;

			case 630: // config
				if (! *label) {
					fprintf(stderr,
						i18n->
						getMessage(
#ifdef		NLS
						 ScreenSet, ScreenCONFIGError,
#else // !NLS
						 0, 0,
#endif // NLS
						 "BScreen::parseMenufile: [config] error, "
						 "no label defined"));
				continue;
			}

			menu->insert(label, configmenu);

			break;

			case 740: // include
			{
				if (! *label) {
					fprintf(stderr,
						i18n->
						getMessage(
#ifdef		NLS
					 ScreenSet, ScreenINCLUDEError,
#else // !NLS
				 0, 0,
#endif // NLS
				 "BScreen::parseMenuFile: [include] error, "
				 "no filename defined\n"));
				continue;
			}

			char newfile[MAXPATHLEN];

			// perform shell style ~ home directory expansion
			char *homedir = 0;
			int homedir_len = 0;
			if (*label == '~' && *(label + 1) == '/') {
				homedir = getenv("HOME");
				homedir_len = strlen(homedir);
			}

			if (homedir && homedir_len != 0) {
				strncpy(newfile, homedir, homedir_len);

				strncpy(newfile + homedir_len, label + 1,
					label_length - 1);
				*(newfile + label_length + homedir_len - 1) = '\0';
			} else {
				strncpy(newfile, label, label_length);
				*(newfile + label_length) = '\0';
			}

			if (newfile) {
				FILE *submenufile = fopen(newfile, "r");

				if (submenufile) {
								struct stat buf;
								if (fstat(fileno(submenufile), &buf) ||
										(! S_ISREG(buf.st_mode))) {
									fprintf(stderr,
				i18n->
				getMessage(
#ifdef		NLS
						 ScreenSet, ScreenINCLUDEErrorReg,
#else // !NLS
						 0, 0,
#endif // NLS
						 "BScreen::parseMenuFile: [include] error: "
						 "'%s' is not a regular file\n"), newfile);
									break;
								}

		if (! feof(submenufile)) {
			if (! parseMenuFile(submenufile, menu))
				fluxbox->saveMenuFilename(newfile);

			fclose(submenufile);
		}
				} else
		perror(newfile);
			}
		}

		break;

	case 767: // submenu
		{
			if (! *label) {
				fprintf(stderr,
					i18n->
					getMessage(
#ifdef		NLS
				 ScreenSet, ScreenSUBMENUError,
#else // !NLS
				 0, 0,
#endif // NLS
				 "BScreen::parseMenuFile: [submenu] error, "
				 "no menu label defined\n"));
				continue;
			}

			Rootmenu *submenu = new Rootmenu(this);

			if (*command)
				submenu->setLabel(command);
			else
				submenu->setLabel(label);

			parseMenuFile(file, submenu);
			submenu->update();
			menu->insert(label, submenu);
			rootmenuList->insert(submenu);
		}

		break;

	case 773: // restart
		{
			if (! *label) {
				fprintf(stderr,
					i18n->
					getMessage(
#ifdef		NLS
				 ScreenSet, ScreenRESTARTError,
#else // !NLS
				 0, 0,
#endif // NLS
				 "BScreen::parseMenuFile: [restart] error, "
				 "no menu label defined\n"));
				continue;
			}

			if (*command)
				menu->insert(label, BScreen::RestartOther, command);
			else
				menu->insert(label, BScreen::Restart);
		}

		break;

	case 845: // reconfig
		{
			if (! *label) {
				fprintf(stderr,
					i18n->
					getMessage(
#ifdef		NLS
				 ScreenSet, ScreenRECONFIGError,
#else // !NLS
				 0, 0,
#endif // NLs
				 "BScreen::parseMenuFile: [reconfig] error, "
				 "no menu label defined\n"));
				continue;
			}

			menu->insert(label, BScreen::Reconfigure);
		}

		break;

				case 995: // stylesdir
				case 1113: // stylesmenu
					{
						Bool newmenu = ((key == 1113) ? True : False);

						if ((! *label) || ((! *command) && newmenu)) {
							fprintf(stderr,
					i18n->
					getMessage(
#ifdef		NLS
				 ScreenSet, ScreenSTYLESDIRError,
#else // !NLS
				 0, 0,
#endif // NLS
				 "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
				 " error, no directory defined\n"));
							continue;
						}

						char stylesdir[MAXPATHLEN];

						char *directory = ((newmenu) ? command : label);
						int directory_length = ((newmenu) ? command_length : label_length);

						// perform shell style ~ home directory expansion
						char *homedir = 0;
						int homedir_len = 0;

						if (*directory == '~' && *(directory + 1) == '/') {
							homedir = getenv("HOME");
							homedir_len = strlen(homedir);
						}

						if (homedir && homedir_len != 0) {
							strncpy(stylesdir, homedir, homedir_len);

							strncpy(stylesdir + homedir_len, directory + 1,
											directory_length - 1);
							*(stylesdir + directory_length + homedir_len - 1) = '\0';
						} else {
							strncpy(stylesdir, directory, directory_length);
							*(stylesdir + directory_length) = '\0';
						}

						struct stat statbuf;

						if (! stat(stylesdir, &statbuf)) {
							if (S_ISDIR(statbuf.st_mode)) {
								Rootmenu *stylesmenu;

								if (newmenu)
									stylesmenu = new Rootmenu(this);
								else
									stylesmenu = menu;

								DIR *d = opendir(stylesdir);
								int entries = 0;
								struct dirent *p;

								// get the total number of directory entries
								while ((p = readdir(d))) entries++;
								rewinddir(d);

								char **ls = new char* [entries];
								int index = 0;
								while ((p = readdir(d)))
									ls[index++] = StringUtil::strdup(p->d_name);

								qsort(ls, entries, sizeof(char *), dcmp);

								int n, slen = strlen(stylesdir);
								for (n = 0; n < entries; n++) {
									int nlen = strlen(ls[n]);
									char style[MAXPATHLEN + 1];

									strncpy(style, stylesdir, slen);
									*(style + slen) = '/';
									strncpy(style + slen + 1, ls[n], nlen + 1);

									if ((! stat(style, &statbuf)) && S_ISREG(statbuf.st_mode))
										stylesmenu->insert(ls[n], BScreen::SetStyle, style);

									delete [] ls[n];
								}

								delete [] ls;

								stylesmenu->update();

								if (newmenu) {
									stylesmenu->setLabel(label);
									menu->insert(label, stylesmenu);
									rootmenuList->insert(stylesmenu);
								}

								fluxbox->saveMenuFilename(stylesdir);
							} else {
								fprintf(stderr,
			i18n->
			getMessage(
#ifdef		NLS
					 ScreenSet, ScreenSTYLESDIRErrorNotDir,
#else // !NLS
					 0, 0,
#endif // NLS
					 "BScreen::parseMenuFile:"
					 " [stylesdir/stylesmenu] error, %s is not a"
					 " directory\n"), stylesdir);
							}
						} else {
							fprintf(stderr,
					i18n->
					getMessage(
#ifdef		NLS
				 ScreenSet, ScreenSTYLESDIRErrorNoExist,
#else // !NLS
				 0, 0,
#endif // NLS
				 "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
				 " error, %s does not exist\n"), stylesdir);
						}

						break;
					}

	case 1090: // workspaces
		{
			if (! *label) {
				fprintf(stderr,
					i18n->getMessage(
#ifdef		NLS
							 ScreenSet, ScreenWORKSPACESError,
#else // !NLS
							 0, 0,
#endif // NLS
							 "BScreen:parseMenuFile: [workspaces] error, "
							 "no menu label defined\n"));
				continue;
			}

			menu->insert(label, workspacemenu);

			break;
		}
	}
			}
		}
	}

	return ((menu->getCount() == 0) ? True : False);
}


void BScreen::shutdown(void) {
	fluxbox->grab();

	XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), NoEventMask);
	XSync(getBaseDisplay()->getXDisplay(), False);

	LinkedListIterator<Workspace> it(workspacesList);
	for (; it.current(); it ++)
		it.current()->shutdown();

	while (iconList->count()) {
		iconList->first()->restore();
		delete iconList->first();
	}

#ifdef		SLIT
	slit->shutdown();
#endif // SLIT

	fluxbox->ungrab();
}


void BScreen::showPosition(int x, int y) {
	if (! geom_visible) {
		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
											(getWidth() - geom_w) / 2,
											(getHeight() - geom_h) / 2, geom_w, geom_h);
		XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

		geom_visible = True;
	}
	const int label_size = 1024;
	char label[label_size];
	
	snprintf(label, label_size,
		I18n::instance()->getMessage(
#ifdef		NLS
				 ScreenSet, ScreenPositionFormat,
#else // !NLS
				 0, 0,
#endif // NLS
				 "X: %4d x Y: %4d"), x, y);

	XClearWindow(getBaseDisplay()->getXDisplay(), geom_window);

	if (I18n::instance()->multibyte()) 
		XmbDrawString(getBaseDisplay()->getXDisplay(), geom_window,
			theme->getWindowStyle().font.set, theme->getWindowStyle().l_text_focus_gc,
			theme->getBevelWidth(), theme->getBevelWidth() -
			theme->getWindowStyle().font.set_extents->max_ink_extent.y,
			label, strlen(label));
	else
		XDrawString(getBaseDisplay()->getXDisplay(), geom_window,
			theme->getWindowStyle().l_text_focus_gc,
			theme->getBevelWidth(),
			theme->getWindowStyle().font.fontstruct->ascent +
			theme->getBevelWidth(), label, strlen(label));
	
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
	if (! geom_visible) {
		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
											(getWidth() - geom_w) / 2,
											(getHeight() - geom_h) / 2, geom_w, geom_h);
		XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

		geom_visible = True;
	}
	
	char label[1024];

	sprintf(label,
		I18n::instance()->getMessage(
#ifdef		NLS
				 ScreenSet, ScreenGeometryFormat,
#else // !NLS
				 0, 0,
#endif // NLS
				 "W: %4d x H: %4d"), gx, gy);

	XClearWindow(getBaseDisplay()->getXDisplay(), geom_window);

	if (I18n::instance()->multibyte())
		XmbDrawString(getBaseDisplay()->getXDisplay(), geom_window,
			theme->getWindowStyle().font.set, theme->getWindowStyle().l_text_focus_gc,
			theme->getBevelWidth(), theme->getBevelWidth() -
			theme->getWindowStyle().font.set_extents->max_ink_extent.y,
			label, strlen(label));
	else
		XDrawString(getBaseDisplay()->getXDisplay(), geom_window,
		theme->getWindowStyle().l_text_focus_gc,
		theme->getBevelWidth(),
		theme->getWindowStyle().font.fontstruct->ascent +
		theme->getBevelWidth(), label, strlen(label));
}


void BScreen::hideGeometry(void) {
	if (geom_visible) {
		XUnmapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		geom_visible = False;
	}
}

//-------------- nextWorkspace ---------------
// Goes to the workspace "right" of the current
//--------------------------------------------
void BScreen::nextWorkspace(void) {
	if (getCurrentWorkspaceID()+1 > getCount()-1)
		changeWorkspaceID(0);
	else
		changeWorkspaceID(getCurrentWorkspaceID()+1);
}

//------------- prevWorkspace ----------------
// Goes to the workspace "left" of the current
//--------------------------------------------
void BScreen::prevWorkspace(void) {
	if (getCurrentWorkspaceID()-1 < 0)
		changeWorkspaceID(getCount()-1);
	else
		changeWorkspaceID(getCurrentWorkspaceID()-1);
}
