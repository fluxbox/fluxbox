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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Screen.cc,v 1.65 2002/08/30 14:03:31 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "Screen.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Icon.hh"
#include "Image.hh"
#include "StringUtil.hh"

#ifdef		SLIT
#include "Slit.hh"
#endif // SLIT

#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef STDC_HEADERS
#	include <sys/types.h>
#endif // STDC_HEADERS

#ifdef HAVE_CTYPE_H
#	include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef HAVE_DIRENT_H
#	include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef HAVE_LOCALE_H
#	include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef HAVE_UNISTD_H
#	include <sys/types.h>
#	include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef HAVE_STDARG_H
#	include <stdarg.h>
#endif // HAVE_STDARG_H

#ifndef  MAXPATHLEN
#define	 MAXPATHLEN 255
#endif // MAXPATHLEN

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <iostream>
#include <memory>
#include <algorithm>

using namespace std;

static bool running = true;

static int anotherWMRunning(Display *display, XErrorEvent *) {
	fprintf(stderr,
		I18n::instance()->
		getMessage(
			FBNLS::ScreenSet, FBNLS::ScreenAnotherWMRunning,
			"BScreen::BScreen: an error occured while querying the X server.\n"
			"	another window manager already running on display %s.\n"),
			DisplayString(display));

	running = false;

	return(-1);
}

static int dcmp(const void *one, const void *two) {
	return (strcmp((*(char **) one), (*(char **) two)));
}

//---------- resource manipulators ---------
template<>
void Resource<Tab::Alignment>::
setFromString(const char *strval) {	
	m_value = Tab::getTabAlignmentNum(strval);
}

template<>
void Resource<Tab::Placement>::
setFromString(const char *strval) {	
	m_value = Tab::getTabPlacementNum(strval);
}

template<>
void Resource<Toolbar::Placement>::
setFromString(const char *strval) {
	if (strcasecmp(strval, "TopLeft")==0)
		m_value = Toolbar::TOPLEFT;
	else if (strcasecmp(strval, "BottomLeft")==0)
		m_value = Toolbar::BOTTOMLEFT;
	else if (strcasecmp(strval, "TopCenter")==0)
		m_value = Toolbar::TOPCENTER;
	else if (strcasecmp(strval, "BottomCenter")==0)
		m_value = Toolbar::BOTTOMCENTER;
	else if (strcasecmp(strval, "TopRight")==0)
		m_value = Toolbar::TOPRIGHT;
	else if (strcasecmp(strval, "BottomRight")==0)
		m_value = Toolbar::BOTTOMRIGHT;
	else
		setDefaultValue();
}

//--------- resource accessors --------------
template<>
string Resource<Tab::Alignment>::
getString() {
	return Tab::getTabAlignmentString(m_value);
}

template<>
string Resource<Tab::Placement>::
getString() {
	return Tab::getTabPlacementString(m_value);
}

template<>
string Resource<Toolbar::Placement>::
getString() {
	switch (m_value) {
		case Toolbar::TOPLEFT:
			return string("TopLeft");
			break;
		case Toolbar::BOTTOMLEFT:
			return string("BottomLeft");
			break;
		case Toolbar::TOPCENTER:
			return string("TopCenter");
			break;			
		case Toolbar::BOTTOMCENTER:
			return string("BottomCenter");
			break;
		case Toolbar::TOPRIGHT:
			return string("TopRight");
			break;
		case Toolbar::BOTTOMRIGHT:
			return string("BottomRight");
			break;
	}
	//default string
	return string("BottomCenter");
}


BScreen::ScreenResource::ScreenResource(ResourceManager &rm, 
	const std::string &scrname, const std::string &altscrname):
toolbar_on_top(rm, false, scrname+".toolbar.onTop", altscrname+".Toolbar.OnTop"),
toolbar_auto_hide(rm, false, scrname+".toolbar.autoHide", altscrname+".Toolbar.AutoHide"),
image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
opaque_move(rm, false, "session.opaqueMove", "Session.OpaqueMove"),
full_max(rm, true, scrname+".fullMaximization", altscrname+".FullMaximization"),
max_over_slit(rm, true, scrname+".maxOverSlit",altscrname+".MaxOverSlit"),
tab_rotate_vertical(rm, true, scrname+".tab.rotatevertical", altscrname+".Tab.RotateVertical"),
sloppy_window_grouping(rm, true, scrname+".sloppywindowgrouping", altscrname+".SloppyWindowGrouping"),
workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
desktop_wheeling(rm, true, scrname+".desktopwheeling", altscrname+".DesktopWheeling"),
show_window_pos(rm, true, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
focus_last(rm, true, scrname+".focusLastWindow", altscrname+".FocusLastWindow"),
focus_new(rm, true, scrname+".focusNewWindows", altscrname+".FocusNewWindows"),
rootcommand(rm, "", scrname+".rootCommand", altscrname+".RootCommand"),
workspaces(rm, 1, scrname+".workspaces", altscrname+".Workspaces"),
toolbar_width_percent(rm, 65, scrname+".toolbar.widthPercent", altscrname+".Toolbar.WidthPercent"),
edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
tab_width(rm, 64, scrname+".tab.width", altscrname+".Tab.Width"),
tab_height(rm, 16, scrname+".tab.height", altscrname+".Tab.Height"),
tab_placement(rm, Tab::PTOP, scrname+".tab.placement", altscrname+".Tab.Placement"),
tab_alignment(rm, Tab::ALEFT, scrname+".tab.alignment", altscrname+".Tab.Alignment"),
#ifdef XINERAMA
toolbar_on_head(rm, 0, scrname+".toolbar.onhead", altscrname+".Toolbar.onHead"),
#endif // XINERAMA
toolbar_placement(rm, Toolbar::BOTTOMCENTER, scrname+".toolbar.placement", altscrname+".Toolbar.Placement")
{

};

BScreen::BScreen(ResourceManager &rm, Fluxbox *b, 
	const string &screenname, const string &altscreenname,
	int scrn) : ScreenInfo(b, scrn),
#ifdef GNOME
gnome_win(None),
#endif
theme(0),
resource(rm, screenname, altscreenname)
{
	fluxbox = b;

	event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
		SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask;

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
			FBNLS::ScreenSet, FBNLS::ScreenManagingScreen,
			"BScreen::BScreen: managing screen %d "
			"using visual 0x%lx, depth %d\n"),
			getScreenNumber(), XVisualIDFromVisual(getVisual()),
			getDepth());

	rootmenu = 0;
		
	#ifdef HAVE_GETPID
	pid_t bpid = getpid();

	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		fluxbox->getFluxboxPidAtom(), XA_CARDINAL,
		sizeof(pid_t) * 8, PropModeReplace,
		(unsigned char *) &bpid, 1);
	#endif // HAVE_GETPID


	XDefineCursor(getBaseDisplay()->getXDisplay(), getRootWindow(),
		fluxbox->getSessionCursor());

	image_control =
		new BImageControl(fluxbox, this, true, fluxbox->colorsPerChannel(),
			fluxbox->getCacheLife(), fluxbox->getCacheMax());
	image_control->installRootColormap();
	root_colormap_installed = true;

	fluxbox->load_rc(this);

	image_control->setDither(*resource.image_dither);
	theme = new Theme(getBaseDisplay()->getXDisplay(), getRootWindow(), colormap(), getScreenNumber(), 
			image_control, fluxbox->getStyleFilename(), getRootCommand().c_str());

	#ifdef GNOME
	initGnomeAtoms();
	#endif 

	#ifdef NEWWMSPEC
	Atom netwmsupported[] = {
//		getBaseDisplay()->getNETWMStateAtom(),
		getBaseDisplay()->getNETNumberOfDesktopsAtom(),
		getBaseDisplay()->getNETCurrentDesktopAtom(),
		getBaseDisplay()->getNETSupportingWMCheckAtom(),		
	};	

	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		getBaseDisplay()->getNETSupportedAtom(), XA_ATOM, 32, PropModeReplace,
		(unsigned char *)netwmsupported, (sizeof netwmsupported)/sizeof netwmsupported[0]);
	#endif //!NEWWMSPEC

	const char *s =	i18n->getMessage(
		FBNLS::ScreenSet, FBNLS::ScreenPositionLength,
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
	attrib.border_pixel = getBorderColor()->pixel();
	attrib.colormap = colormap();
	attrib.save_under = true;

	geom_window =
		XCreateWindow(getBaseDisplay()->getXDisplay(), getRootWindow(),
			0, 0, geom_w, geom_h, theme->getBorderWidth(), getDepth(),
			InputOutput, getVisual(), mask, &attrib);
	geom_visible = false;

	if (theme->getWindowStyle().l_focus.type() & FbTk::Texture::PARENTRELATIVE) {
		if (theme->getWindowStyle().t_focus.type() ==
			(FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				 theme->getWindowStyle().t_focus.color().pixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
				 &theme->getWindowStyle().t_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				 geom_window, geom_pixmap);
		}
	} else {
		if (theme->getWindowStyle().l_focus.type() ==
				(FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				 theme->getWindowStyle().l_focus.color().pixel());
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
	if (*resource.workspaces != 0) {
		for (int i = 0; i < *resource.workspaces; ++i) {
			wkspc = new Workspace(this, workspacesList.size());
			workspacesList.push_back(wkspc);
			workspacemenu->insert(wkspc->name().c_str(), wkspc->menu());
		}
	} else {
		wkspc = new Workspace(this, workspacesList.size());
		workspacesList.push_back(wkspc);
		workspacemenu->insert(wkspc->name().c_str(), wkspc->menu());
	}

	workspacemenu->insert(i18n->
		getMessage(
			FBNLS::IconSet, FBNLS::IconIcons,
			 "Icons"),
			iconmenu);
	workspacemenu->update();

	current_workspace = workspacesList.front();
	workspacemenu->setItemSelected(2, true);

	toolbar = new Toolbar(this);

	#ifdef SLIT
	slit = new Slit(this);
	#endif // SLIT

	initMenu();

	raiseWindows(0, 0);

	//update menus
	rootmenu->update();
	#ifdef SLIT
	slit->reconfigure();
	#endif
	
	changeWorkspaceID(0);
	updateNetizenWorkspaceCount();
	
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
				for (int j = 0; j < (int) nchild; j++) {
					if (children[j] == wmhints->icon_window) {
						children[j] = None;
						break;
					}
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
				
				FluxboxWindow *win = new FluxboxWindow(children[i], this);
				if (!win->isManaged()) {
					delete win;
					win = 0;
				}
				
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

namespace {
	template<typename T>
	void delete_obj(T * obj) {
		delete obj;
	}
}

BScreen::~BScreen() {
	#ifdef GNOME
	XDestroyWindow(getBaseDisplay()->getXDisplay(), gnome_win);
	#endif
	if (! managed) return;

	if (geom_pixmap != None)
		image_control->removeImage(geom_pixmap);

	if (geom_window != None)
		XDestroyWindow(getBaseDisplay()->getXDisplay(), geom_window);

	removeWorkspaceNames();

#ifdef __INTEL_COMPILER
	//Didn't got icc to work with std::for_each
	//so we do this by hand.
	
	Workspaces::iterator w_it = workspacesList.begin();
	Workspaces::iterator w_it_end = workspacesList.end();
	for(; w_it != w_it_end; ++w_it) {
		delete (*w_it);
	}
	workspacesList.clear();
	
	Icons::iterator i_it = iconList.begin();
	Icons::iterator i_it_end = iconList.end();
	for(; i_it != i_it_end; ++i_it) {
		delete (*i_it);
	}
	iconList.clear();
	
	Netizens::iterator n_it = netizenList.begin();
	Netizens::iterator n_it_end = netizenList.end();
	for(; n_it != n_it_end; ++n_it) {
		delete (*n_it);
	}
	netizenList.clear();
		
#else //__INTEL_COMPILER
	std::for_each(
		workspacesList.begin(),
		workspacesList.end(),
		delete_obj<Workspace>);

	// don't delete items in the rootmenuList?

	std::for_each(
		iconList.begin(),
		iconList.end(),
		delete_obj<FluxboxWindow>);

	std::for_each(
		netizenList.begin(),
		netizenList.end(),
		delete_obj<Netizen>);
#endif //!__INTEL_COMPILER

	delete rootmenu;
	delete workspacemenu;
	delete iconmenu;
	delete configmenu;

#ifdef		SLIT
	delete slit;
#endif // SLIT

	delete toolbar;
	delete image_control;

	delete theme;

}

void BScreen::reconfigure() {
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): BScreen::reconfigure"<<endl;
	#endif
	Fluxbox::instance()->loadRootCommand(this);
	theme->setRootCommand(getRootCommand());

	theme->load(fluxbox->getStyleFilename());
	theme->reconfigure();
	I18n *i18n = I18n::instance();

	const char *s = i18n->getMessage(
		FBNLS::ScreenSet, FBNLS::ScreenPositionLength,
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
	if (theme->getWindowStyle().l_focus.type() & FbTk::Texture::PARENTRELATIVE) {
		if (theme->getWindowStyle().t_focus.type() ==
				(FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				theme->getWindowStyle().t_focus.color().pixel());
		} else {
			geom_pixmap = image_control->renderImage(geom_w, geom_h,
				&theme->getWindowStyle().t_focus);
			XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(),
				geom_window, geom_pixmap);
		}
	} else {
		if (theme->getWindowStyle().l_focus.type() ==
				(FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			geom_pixmap = None;
			XSetWindowBackground(getBaseDisplay()->getXDisplay(), geom_window,
				theme->getWindowStyle().l_focus.color().pixel());
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
		theme->getBorderColor().pixel());

	//reconfigure menus
	workspacemenu->reconfigure();
	iconmenu->reconfigure();

	configmenu->reconfigure();
	
	{
		int remember_sub = rootmenu->currentSubmenu();
		initMenu();
		raiseWindows(0, 0);
		rootmenu->reconfigure();		
		rootmenu->drawSubmenu(remember_sub);
	}


	toolbar->reconfigure();

#ifdef		SLIT
	slit->reconfigure();
#endif // SLIT
	
	//reconfigure workspaces
	Workspaces::iterator wit = workspacesList.begin();
	Workspaces::iterator wit_end = workspacesList.end();
	for (; wit != wit_end; ++wit) {
		(*wit)->reconfigure();
	}

	//reconfigure Icons
	Icons::iterator iit = iconList.begin();
	Icons::iterator iit_end = iconList.end();
	for (; iit != iit_end; ++iit) {
		if ((*iit)->validateClient())
			(*iit)->reconfigure();
	}

	image_control->timeout();
}


void BScreen::rereadMenu() {
	initMenu();
	raiseWindows(0, 0);

	rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames() {
	workspaceNames.erase(workspaceNames.begin(), workspaceNames.end());
}

void BScreen::updateWorkspaceNamesAtom() {

#ifdef GNOME	
	XTextProperty	text;
	int number_of_desks = workspaceNames.size();
	
	char s[1024];
	char *names[number_of_desks];		
	
	for (int i = 0; i < number_of_desks; i++) {		
		sprintf(s, "Desktop %i", i);
		names[i] = new char[strlen(s) + 1];
		strcpy(names[i], s);
	}
	
	if (XStringListToTextProperty(names, number_of_desks, &text)) {
		XSetTextProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
			 &text, FbAtoms::instance()->getGnomeWorkspaceNamesAtom());
		XFree(text.value);
	}
	
	for (int i = 0; i < number_of_desks; i++)
		delete names[i];			

#endif

}

void BScreen::addIcon(FluxboxWindow *w) {
	if (! w) return;

	w->setWorkspace(-1);
	w->setWindowNumber(iconList.size());

	iconList.push_back(w);

	iconmenu->insert(w->getIconTitle().c_str());
	iconmenu->update();
	toolbar->addIcon(w);
}


void BScreen::removeIcon(FluxboxWindow *w) {
	if (! w) return;
	
	{
	Icons::iterator it = iconList.begin();
	Icons::iterator it_end = iconList.end();
	for (; it != it_end; ++it) {
		if (*it == w) {
			iconList.erase(it);
			break;
		}
	}
	}
		
	iconmenu->remove(w->getWindowNumber());
	iconmenu->update();
	toolbar->delIcon(w);
	
	Icons::iterator it = iconList.begin();
	Icons::iterator it_end = iconList.end();
	for (int i = 0; it != it_end; ++it, ++i) {
		(*it)->setWindowNumber(i);
	}
}


FluxboxWindow *BScreen::getIcon(unsigned int index) {
	if (index < iconList.size())
		return iconList[index];

	return 0;
}


int BScreen::addWorkspace() {
	Workspace *wkspc = new Workspace(this, workspacesList.size());
	workspacesList.push_back(wkspc);
	addWorkspaceName(wkspc->name().c_str()); // update names
	//add workspace to workspacemenu
	workspacemenu->insert(wkspc->name().c_str(), wkspc->menu(),
		wkspc->workspaceID() + 2); //+2 so we add it after "remove last"
		
	workspacemenu->update();
	saveWorkspaces(workspacesList.size());
	toolbar->reconfigure();

	updateNetizenWorkspaceCount();	
	
	
	return workspacesList.size();
	
}


int BScreen::removeLastWorkspace() {
	if (workspacesList.size() > 1) {
		Workspace *wkspc = workspacesList.back();

		if (current_workspace->workspaceID() == wkspc->workspaceID())
			changeWorkspaceID(current_workspace->workspaceID() - 1);

		wkspc->removeAll();

		workspacemenu->remove(wkspc->workspaceID()+2); // + 2 is where workspaces starts
		workspacemenu->update();
		
		//remove last workspace
		workspacesList.pop_back();		
		delete wkspc;

		toolbar->reconfigure();

		updateNetizenWorkspaceCount();
		saveWorkspaces(workspacesList.size());
		return workspacesList.size();
	}

	return 0;
}


void BScreen::changeWorkspaceID(unsigned int id) {
	if (! current_workspace || id >= workspacesList.size())
		return;
	
	if (id != current_workspace->workspaceID()) {
		XSync(fluxbox->getXDisplay(), true);
		FluxboxWindow *focused = fluxbox->getFocusedWindow();

		if (focused && focused->isMoving()) {
			reassociateGroup(focused, id, true);
			focused->pauseMoving();
		}

		current_workspace->hideAll();

		workspacemenu->setItemSelected(current_workspace->workspaceID() + 2, false);

		if (focused && focused->getScreen() == this &&
				(! focused->isStuck()) && (!focused->isMoving())) {
			current_workspace->setLastFocusedWindow(focused);
			fluxbox->setFocusedWindow((FluxboxWindow *) 0);
			
		}

		current_workspace = getWorkspace(id);

		workspacemenu->setItemSelected(current_workspace->workspaceID() + 2, true);
		toolbar->redrawWorkspaceLabel(true);

		current_workspace->showAll();

		if (*resource.focus_last && current_workspace->getLastFocusedWindow() &&
				!(focused && focused->isMoving())) {
			current_workspace->getLastFocusedWindow()->setInputFocus();		
		}

		if (focused && focused->isMoving()) {
			focused->resumeMoving();
		}
	}

	updateNetizenCurrentWorkspace();
}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
	if (! current_workspace || id >= workspacesList.size())
		return;

	if (!win)
		win = fluxbox->getFocusedWindow();

	if (id != current_workspace->workspaceID()) {
		XSync(fluxbox->getXDisplay(), True);

		if (win && win->getScreen() == this &&
				(! win->isStuck())) {

					if ( win->getTab() ) {
						Tab *tab = win->getTab();
						tab->disconnect();
						tab->setPosition();
					}

					win->withdraw();
					BScreen::reassociateWindow(win, id, true);
					if (changeWS) {
						BScreen::changeWorkspaceID(id);
						win->setInputFocus();
					}
				}

		}
}


void BScreen::addNetizen(Netizen *n) {
	netizenList.push_back(n);

	n->sendWorkspaceCount();
	n->sendCurrentWorkspace();

	Workspaces::iterator it = workspacesList.begin();
	Workspaces::iterator it_end = workspacesList.end();
	for (; it != it_end; ++it) {
		for (int i = 0; i < (*it)->getCount(); ++i) {
			n->sendWindowAdd((*it)->getWindow(i)->getClientWindow(),
				(*it)->workspaceID());
		}
	}

	Window f = ((fluxbox->getFocusedWindow()) ?
		fluxbox->getFocusedWindow()->getClientWindow() : None);
	n->sendWindowFocus(f);
}

void BScreen::removeNetizen(Window w) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		if ((*it)->getWindowID() == w) {
			Netizen *n = *it;
			delete n;
			netizenList.erase(it);			
			break;
		}
	}
}


void BScreen::updateNetizenCurrentWorkspace() {
	#ifdef NEWWMSPEC
	//update _NET_WM_CURRENT_DESKTOP
	int workspace = getCurrentWorkspaceID();
	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		getBaseDisplay()->getNETCurrentDesktopAtom(), XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&workspace, 1);
	#endif
	#ifdef GNOME
	//update _WIN_WORKSPACE
	int gnome_workspace = getCurrentWorkspaceID();
	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		FbAtoms::instance()->getGnomeWorkspaceAtom(), XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&gnome_workspace, 1);
	updateGnomeClientList();
	#endif
	
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendCurrentWorkspace();
	}

}


void BScreen::updateNetizenWorkspaceCount() {

	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendWorkspaceCount();
	}
	#ifdef NEWWMSPEC
	//update _NET_WM_NUMBER_OF_DESKTOPS
	int numworkspaces = getCount()-1;
	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		getBaseDisplay()->getNETNumberOfDesktopsAtom(), XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&numworkspaces, 1);
	#endif
	
	#ifdef GNOME 
	{
	int numworkspaces = getCount();
	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
		FbAtoms::instance()->getGnomeWorkspaceCountAtom(), XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&numworkspaces, 1);
	}	
	#endif
	
}


void BScreen::updateNetizenWindowFocus() {

	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	Window f = ((fluxbox->getFocusedWindow()) ?
			fluxbox->getFocusedWindow()->getClientWindow() : None);
	for (; it != it_end; ++it) {
		(*it)->sendWindowFocus(f);
	}
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendWindowAdd(w, p);
	}
	#ifdef GNOME
	updateGnomeClientList();
	#endif
	
}


void BScreen::updateNetizenWindowDel(Window w) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendWindowDel(w);
	}
	#ifdef GNOME
	updateGnomeClientList();
	#endif
}


void BScreen::updateNetizenWindowRaise(Window w) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendWindowRaise(w);
	}
}


void BScreen::updateNetizenWindowLower(Window w) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendWindowLower(w);
	}
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
	Netizens::iterator it = netizenList.begin();
	Netizens::iterator it_end = netizenList.end();
	for (; it != it_end; ++it) {
		(*it)->sendConfigNotify(e);
	}
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {

	Window session_stack[(num + workspacesList.size() + rootmenuList.size() + 30)];
	int i = 0;	
	XRaiseWindow(getBaseDisplay()->getXDisplay(), iconmenu->windowID());
	session_stack[i++] = iconmenu->windowID();

	Workspaces::iterator wit = workspacesList.begin();
	Workspaces::iterator wit_end = workspacesList.end();
	for (; wit != wit_end; ++wit) {
		session_stack[i++] = (*wit)->menu()->windowID();
	}

	session_stack[i++] = workspacemenu->windowID();

	session_stack[i++] = configmenu->getFocusmenu()->windowID();
	session_stack[i++] = configmenu->getPlacementmenu()->windowID();
	session_stack[i++] = configmenu->getTabmenu()->windowID();
	session_stack[i++] = configmenu->windowID();

	#ifdef		SLIT
	session_stack[i++] = slit->menu().getDirectionmenu()->windowID();
	session_stack[i++] = slit->menu().getPlacementmenu()->windowID();
	#ifdef XINERAMA
	if (hasXinerama()) {
		session_stack[i++] = slit->menu().getHeadmenu()->windowID();
	}
	#endif // XINERAMA
	session_stack[i++] = slit->menu().windowID();
	#endif // SLIT

	session_stack[i++] =
		toolbar->getMenu()->getPlacementmenu()->windowID();
	#ifdef XINERAMA
	if (hasXinerama()) {
		session_stack[i++] = toolbar->getMenu()->getHeadmenu()->windowID();
	}
	#endif // XINERAMA
	session_stack[i++] = toolbar->getMenu()->windowID();

	Rootmenus::iterator rit = rootmenuList.begin();
	Rootmenus::iterator rit_end = rootmenuList.end();
	for (; rit != rit_end; ++rit) {
		session_stack[i++] = (*rit)->windowID();
	}
	session_stack[i++] = rootmenu->windowID();

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

}

#ifdef		HAVE_STRFTIME
void BScreen::saveStrftimeFormat(const char *format) {
	//make sure std::string don't get 0 string
	resource.strftime_format = (format ? format : "");
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(const char *name) {
	workspaceNames.push_back(name);
}


string BScreen::getNameOfWorkspace(unsigned int workspace) const {
	if (workspace < workspaceNames.size()) {
		return workspaceNames[workspace];
	} else {
		return "";
	}

}

void BScreen::reassociateGroup(FluxboxWindow *w, unsigned int wkspc_id, bool ignore_sticky) {
	if (w->hasTab() && (w->getTab()->next() || w->getTab()->prev())) {
		Tab *tab_it = w->getTab()->first();
		for (; tab_it; tab_it = tab_it->next()) {
			reassociateWindow(tab_it->getWindow(), wkspc_id, ignore_sticky);
		}
	} else {
		// no tab, juts move this one
		reassociateWindow(w, wkspc_id, ignore_sticky);
	}
}


void BScreen::reassociateWindow(FluxboxWindow *w, unsigned int wkspc_id, bool ignore_sticky) {
	if (! w) return;

	if (wkspc_id >= getCount())
		wkspc_id = current_workspace->workspaceID();

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


void BScreen::nextFocus(int opts) {
	bool have_focused = false;
	int focused_window_number = -1;
	FluxboxWindow *focused;
	const int num_windows = getCurrentWorkspace()->getCount();
	
	if ((focused = fluxbox->getFocusedWindow())) {
		if (focused->getScreen()->getScreenNumber() == 
				getScreenNumber()) {
			have_focused = true;
			focused_window_number = focused->getWindowNumber();
		}
	}

	if (num_windows > 1 && have_focused) {
		Workspace *wksp = getCurrentWorkspace();
		Workspace::Windows wins = wksp->getWindowList();
		Workspace::Windows::iterator it = wins.begin();
		for (; *it != focused; ++it); //get focused window iterator

		do {
			++it;
			if (it == wins.end())
				it = wins.begin();
			// see if the window should be skipped
			if (! (doSkipWindow(*it, opts) || !(*it)->setInputFocus()) )
				break;
		} while (*it != focused);

		if (*it != focused)
			wksp->raiseWindow(*it);

	} else if (num_windows >= 1) {
		FluxboxWindow *next = current_workspace->getWindow(0);
		//don't raise next window if input focus fails
		if (next->setInputFocus())
			current_workspace->raiseWindow(next);
	}

}


void BScreen::prevFocus(int opts) {
	bool have_focused = false;
	int focused_window_number = -1;
	FluxboxWindow *focused;
	int num_windows = getCurrentWorkspace()->getCount();
	
	if ((focused = fluxbox->getFocusedWindow())) {
		if (focused->getScreen()->getScreenNumber() ==
				getScreenNumber()) {
			have_focused = true;
			focused_window_number = focused->getWindowNumber();
		}
	}

	if (num_windows > 1 && have_focused) {
		Workspace *wksp = getCurrentWorkspace();
		Workspace::Windows wins = wksp->getWindowList();
		Workspace::Windows::iterator it = wins.begin();
		for (; *it != focused; ++it);
		do {
			if (it == wins.begin())
				it = wins.end();
			--it;
			// see if the window should be skipped
			if (! (doSkipWindow(*it, opts) ||	!(*it)->setInputFocus()) )
				break;
		} while (*it != focused);
		if (*it != focused)
			wksp->raiseWindow(*it);
	} else if (num_windows >= 1) {
		FluxboxWindow *next = current_workspace->getWindow(0);
		//don't raise next window if input focus fails
		if (next->setInputFocus())
			current_workspace->raiseWindow(next);
	}

}

//--------- raiseFocus -----------
// Raise the current focused window
//--------------------------------
void BScreen::raiseFocus() {
	bool have_focused = false;
	int focused_window_number = -1;

	if (fluxbox->getFocusedWindow())
		if (fluxbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
				getScreenNumber()) {
			have_focused = true;
			focused_window_number = fluxbox->getFocusedWindow()->getWindowNumber();
		}

	if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
		getWorkspace(fluxbox->getFocusedWindow()->getWorkspaceNumber())->
			raiseWindow(fluxbox->getFocusedWindow());
}

void BScreen::initMenu() {
	I18n *i18n = I18n::instance();
	
	if (rootmenu) {
		rootmenuList.erase(rootmenuList.begin(), rootmenuList.end());

		while (rootmenu->numberOfItems())
			rootmenu->remove(0);			
	} else
		rootmenu = new Rootmenu(this);

	bool defaultMenu = true;

	if (fluxbox->getMenuFilename()) {
		ifstream menu_file(fluxbox->getMenuFilename());

		if (!menu_file.fail()) {
			if (! menu_file.eof()) {
				string line;
				int row = 0;
				while (getline(menu_file, line) && ! menu_file.eof()) {
					row++;
					if (line[0] != '#') {
						string key;
						int pos=0;
						int err = StringUtil::getStringBetween(key, line.c_str(), '[', ']');
						
						if (key == "begin") {
							pos += err;
							string label;
							err = StringUtil::getStringBetween(label, line.c_str()+pos, '(', ')');
							if (err>0) {
								rootmenu->setLabel(label.c_str());
								defaultMenu = parseMenuFile(menu_file, rootmenu, row);
							} else
								cerr<<"Error in menufile. Line("<<row<<")"<<endl;
							break;
						}
					}
				}
			} else {
				fprintf(stderr,
					i18n->getMessage(
						FBNLS::ScreenSet, FBNLS::ScreenEmptyMenuFile,
						"%s: Empty menu file"),
					fluxbox->getMenuFilename());
			}
			menu_file.close();
		} else
			perror(fluxbox->getMenuFilename());
	}

	if (defaultMenu) {
		rootmenu->setInternalMenu();
		rootmenu->insert(i18n->getMessage(
			FBNLS::ScreenSet, FBNLS::Screenxterm,
			"xterm"),
			BScreen::EXECUTE,
			i18n->getMessage(
			FBNLS::ScreenSet, FBNLS::Screenxterm,
			"xterm"));
		rootmenu->insert(i18n->getMessage(
			FBNLS::ScreenSet, FBNLS::ScreenRestart,
			"Restart"),
			BScreen::RESTART);
		rootmenu->insert(i18n->getMessage(
			FBNLS::ScreenSet, FBNLS::ScreenExit,
			"Exit"),
			BScreen::EXIT);
	} else
		fluxbox->saveMenuFilename(fluxbox->getMenuFilename());
}

// looks through a menufile and adds correct items to the root-menu.
bool BScreen::parseMenuFile(ifstream &file, Rootmenu *menu, int &row) {
	
	string line;

	while (! file.eof()) {

		if (getline(file, line)) {
			row++;
			if (line[0] != '#') { //the line is commented
				int parse_pos = 0, err = 0;


				std::string str_key, str_label, str_cmd;
				
				err = StringUtil::getStringBetween(str_key, line.c_str(), '[', ']');
				if (err > 0 ) {
					parse_pos += err;	
					err = StringUtil::getStringBetween(str_label, line.c_str() + parse_pos, '(', ')');
					if (err>0) {
						parse_pos += err;	
						StringUtil::getStringBetween(str_cmd, line.c_str() + parse_pos, '{', '}');
					}
				} else 
					continue; //read next line
				
				if (!str_key.size()) 
					continue;	//read next line

				I18n *i18n = I18n::instance();
				if (str_key == "end") {
					return ((menu->numberOfItems() == 0) ? true : false);
				} else if (str_key == "nop") { 
					menu->insert(str_label.c_str());
				} else if (str_key == "exec") { // exec
					if (!(str_label.size() && str_cmd.size())) {
						fprintf(stderr,
						i18n->getMessage(
							FBNLS::ScreenSet, FBNLS::ScreenEXECError,
							"BScreen::parseMenuFile: [exec] error, "
							"no menu label and/or command defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else
						menu->insert(str_label.c_str(), BScreen::EXECUTE, str_cmd.c_str());

				} else if (str_key == "exit") { // exit
					if (!str_label.size()) {
						fprintf(stderr,
						i18n->getMessage(
							FBNLS::ScreenSet, FBNLS::ScreenEXITError,
							 "BScreen::parseMenuFile: [exit] error, "
							 "no menu label defined\n"));
						 cerr<<"Row: "<<row<<endl;
					} else
						menu->insert(str_label.c_str(), BScreen::EXIT);
				} // end of exit
				else if (str_key == "style") {	// style
					if (!( str_label.size() && str_cmd.size())) {
						fprintf(stderr,
						i18n->
						getMessage(
						 	FBNLS::ScreenSet, FBNLS::ScreenSTYLEError,
						 	"BScreen::parseMenuFile: [style] error, "
						 	"no menu label and/or filename defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else {
						// perform shell style ~ home directory expansion
						// and insert style
						menu->insert(str_label.c_str(), BScreen::SETSTYLE, 
							StringUtil::expandFilename(str_cmd).c_str());
						
					}
				} // end of style
				
				else if (str_key == "config") {
					if (! str_label.size()) {
						fprintf(stderr,
						i18n->
						getMessage(
							FBNLS::ScreenSet, FBNLS::ScreenCONFIGError,
							 "BScreen::parseMenufile: [config] error, "
							 "no label defined"));
						cerr<<"Row: "<<row<<endl;
					} else
						menu->insert(str_label.c_str(), configmenu);
				} // end of config
				else if ( str_key == "include") { // include
					if (!str_label.size()) {
						fprintf(stderr,
						i18n->
						getMessage(
						 	FBNLS::ScreenSet, FBNLS::ScreenINCLUDEError,
					 		"BScreen::parseMenuFile: [include] error, "
						 	"no filename defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else {	// start of else 'x'
						// perform shell style ~ home directory expansion
						string newfile(StringUtil::expandFilename(str_label));

						if (newfile.size() != 0) {
							FILE *submenufile = fopen(newfile.c_str(), "r");

							if (submenufile) {
								struct stat buf;
								if (fstat(fileno(submenufile), &buf) ||
									(! S_ISREG(buf.st_mode))) {
									fprintf(stderr,
									i18n->
									getMessage(
							 			FBNLS::ScreenSet, FBNLS::ScreenINCLUDEErrorReg,
							 			"BScreen::parseMenuFile: [include] error: "
							 			"'%s' is not a regular file\n"), newfile.c_str());
									cerr<<"Row: "<<row<<endl;
								}
								
								if (! feof(submenufile)) {
									fclose(submenufile);
									ifstream subfile(newfile.c_str());
									if (! parseMenuFile(subfile, menu, row))
										fluxbox->saveMenuFilename(newfile.c_str());
								}
							} else
								perror(newfile.c_str());
						} 
					} // end of else 'x'
				} // end of include
				else if (str_key == "submenu") { // sub
					if (!str_label.size()) {
						fprintf(stderr,
						i18n->
						getMessage(
					 		FBNLS::ScreenSet, FBNLS::ScreenSUBMENUError,
					 		"BScreen::parseMenuFile: [submenu] error, "
					 		"no menu label defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else {
						Rootmenu *submenu = new Rootmenu(this);

						if (str_cmd.size())
							submenu->setLabel(str_cmd.c_str());
						else
							submenu->setLabel(str_label.c_str());

						parseMenuFile(file, submenu, row);						
						submenu->update();
						menu->insert(str_label.c_str(), submenu);
						rootmenuList.push_back(submenu);
						
					}
				} // end of sub
				else if (str_key == "restart") {
					if (!str_label.size()) {
						fprintf(stderr,
						i18n->
						getMessage(
					 		FBNLS::ScreenSet, FBNLS::ScreenRESTARTError,
					 		"BScreen::parseMenuFile: [restart] error, "
					 		"no menu label defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else {
					if (str_cmd.size())
						menu->insert(str_label.c_str(), BScreen::RESTARTOTHER, str_cmd.c_str());
					else
						menu->insert(str_label.c_str(), BScreen::RESTART);
					}
				} // end of restart
				else if (str_key == "reconfig") { // reconf
					if (!str_label.c_str()) {
						fprintf(stderr,
						i18n->
						getMessage(
					 		FBNLS::ScreenSet, FBNLS::ScreenRECONFIGError,
					 		"BScreen::parseMenuFile: [reconfig] error, "
					 		"no menu label defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else 
						menu->insert(str_label.c_str(), BScreen::RECONFIGURE);
				} // end of reconf
				else if (str_key == "stylesdir" || str_key == "stylesmenu") {
					bool newmenu = (str_key == "stylesmenu");
					if (!( str_label.size() && str_cmd.size()) && newmenu) {
						fprintf(stderr,
						i18n->
						getMessage(
							FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRError,
					 		"BScreen::parseMenuFile: [stylesdir/stylesmenu]"
					 		" error, no directory defined\n"));
						cerr<<"Row: "<<row<<endl;
					} else { // else 'y'
						createStyleMenu(menu, newmenu, str_label.c_str(), (newmenu) ? str_cmd.c_str() : str_label.c_str());						
					} // end of else 'y' 
			 	} // end of stylesdir
				else if (str_key == "workspaces") {
					if (!str_label.size()) {
						fprintf(stderr,
						i18n->getMessage(
						 FBNLS::ScreenSet, FBNLS::ScreenWORKSPACESError,
						 "BScreen:parseMenuFile: [workspaces] error, "
						 "no menu label defined\n"));
						 cerr<<"Row: "<<row<<endl;
					} else
						menu->insert(str_label.c_str(), workspacemenu);
				} // end of work
			}
		}
	}

	return ((menu->numberOfItems() == 0) ? true : false);
}

void BScreen::createStyleMenu(Rootmenu *menu, bool newmenu, const char *label, const char *directory) {

	// perform shell style ~ home directory expansion
	string stylesdir(StringUtil::expandFilename(directory ? directory : ""));

	I18n *i18n = I18n::instance();						
	struct stat statbuf;

	if (! stat(stylesdir.c_str(), &statbuf)) { // stat
		if (S_ISDIR(statbuf.st_mode)) { // dir
			Rootmenu *stylesmenu;
			if (newmenu)
				stylesmenu = new Rootmenu(this);
			else
				stylesmenu = menu;

			DIR *d = opendir(stylesdir.c_str());
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

			int n, slen = strlen(stylesdir.c_str());
			for (n = 0; n < entries; n++) { // for
				int nlen = strlen(ls[n]);
				char style[MAXPATHLEN + 1];
				strncpy(style, stylesdir.c_str(), slen);
				*(style + slen) = '/';
				strncpy(style + slen + 1, ls[n], nlen + 1);
				if ((! stat(style, &statbuf)) && S_ISREG(statbuf.st_mode))
						stylesmenu->insert(ls[n], BScreen::SETSTYLE, style);
					delete [] ls[n];
			} 

			delete [] ls;

			stylesmenu->update();
			if (newmenu) {
				stylesmenu->setLabel(label);
				menu->insert(label, stylesmenu);
				rootmenuList.push_back(stylesmenu);
			}

			fluxbox->saveMenuFilename(stylesdir.c_str());
		} else { // dir
			fprintf(stderr,
			i18n->
			getMessage(
				FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRErrorNotDir,
				"BScreen::parseMenuFile:"
				" [stylesdir/stylesmenu] error, %s is not a"
				" directory\n"), stylesdir.c_str());
		} // end of 'dir'
	} else { // stat
		fprintf(stderr,
		i18n->
		getMessage(
			FBNLS::ScreenSet, FBNLS::ScreenSTYLESDIRErrorNoExist,		
			"BScreen::parseMenuFile: [stylesdir/stylesmenu]"
			" error, %s does not exist\n"), stylesdir.c_str());
	} // end of 'stat'

}

void BScreen::shutdown() {
	fluxbox->grab();

	XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), NoEventMask);
	XSync(getBaseDisplay()->getXDisplay(), False);

	{
		Workspaces::iterator it = workspacesList.begin();
		Workspaces::iterator it_end = workspacesList.end();
		for (; it != it_end; ++it) {
			(*it)->shutdown();
		}
	}

	{
		while (!iconList.empty()) {
			iconList.back()->restore(true); // restore with remap
			delete iconList.back(); // the window removes it self from iconlist
		}
	}

#ifdef		SLIT
	slit->shutdown();
#endif // SLIT

	fluxbox->ungrab();
}


void BScreen::showPosition(int x, int y) {
	if (! geom_visible) {
#ifdef XINERAMA
		unsigned int head = hasXinerama() ? getCurrHead() : 0;

		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
			getHeadX(head) + (getHeadWidth(head) - geom_w) / 2,
			getHeadY(head) + (getHeadHeight(head) - geom_h) / 2, geom_w, geom_h);
#else // !XINERMA
		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
			(getWidth() - geom_w) / 2,
			(getHeight() - geom_h) / 2, geom_w, geom_h);
#endif // XINERAMA

		XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

		geom_visible = true;
	}
	const int label_size = 1024;
	char label[label_size];
	
	snprintf(label, label_size,
		I18n::instance()->getMessage(
			 FBNLS::ScreenSet, FBNLS::ScreenPositionFormat,
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
#ifdef XINERAMA
		unsigned int head = hasXinerama() ? getCurrHead() : 0;

		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
			getHeadX(head) + (getHeadWidth(head) - geom_w) / 2,
			getHeadY(head) + (getHeadHeight(head) - geom_h) / 2, geom_w, geom_h);
#else // !XINERMA
		XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
			(getWidth() - geom_w) / 2,
			(getHeight() - geom_h) / 2, geom_w, geom_h);
#endif // XINERAMA
		XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

		geom_visible = true;
	}
	
	char label[1024];

	sprintf(label,
		I18n::instance()->getMessage(
			FBNLS::ScreenSet, FBNLS::ScreenGeometryFormat,
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


void BScreen::hideGeometry() {
	if (geom_visible) {
		XUnmapWindow(getBaseDisplay()->getXDisplay(), geom_window);
		geom_visible = false;
	}
}

//-------------- nextWorkspace ---------------
// Goes to the workspace "right" of the current
//--------------------------------------------
void BScreen::nextWorkspace(const int delta) {
	changeWorkspaceID( (getCurrentWorkspaceID()+delta) % getCount());
}

//------------- prevWorkspace ----------------
// Goes to the workspace "left" of the current
//--------------------------------------------
void BScreen::prevWorkspace(const int delta) {
	changeWorkspaceID( (getCurrentWorkspaceID()-delta+getCount()) % getCount());
}

//-------------- rightWorkspace ---------------
// Goes to the workspace "right" of the current
//--------------------------------------------
void BScreen::rightWorkspace(const int delta) {
	if (getCurrentWorkspaceID()+delta < getCount())
		changeWorkspaceID(getCurrentWorkspaceID()+delta);
}

//------------- leftWorkspace ----------------
// Goes to the workspace "left" of the current
//--------------------------------------------
void BScreen::leftWorkspace(const int delta) {
	if (getCurrentWorkspaceID() >= static_cast<unsigned int>(delta))
		changeWorkspaceID(getCurrentWorkspaceID()-delta);
}

//-------- doSkipWindow
// Returns true if the windows should be skiped
// else false
//----------
bool BScreen::doSkipWindow(const FluxboxWindow *w, int opts) {
	return ((opts & CYCLESKIPSTUCK) != 0 && w->isStuck() || // skip if stuck
			(opts & CYCLESKIPLOWERTABS) != 0 && w->isLowerTab() || // skip if lower tab
			(opts & CYCLESKIPSHADED) != 0 && w->isShaded()); // skip if shaded
}

//----------- useAutoGroupWindow -------------
// Access and clear the auto-group window
//--------------------------------------------
FluxboxWindow* BScreen::useAutoGroupWindow() {
	Window w = auto_group_window;
	auto_group_window = 0;
	return w ? Fluxbox::instance()->searchWindow(w) : 0;
}

#ifdef GNOME
void BScreen::initGnomeAtoms() {

	/* create the GNOME window */
	gnome_win = XCreateSimpleWindow(getBaseDisplay()->getXDisplay(),
		getRootWindow(), 0, 0, 5, 5, 0, 0, 0);
	FbAtoms *fba = FbAtoms::instance();
	/* supported WM check */
	 XChangeProperty(getBaseDisplay()->getXDisplay(),
		getRootWindow(), fba->getGnomeSupportingWMCheckAtom(), 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

	XChangeProperty(getBaseDisplay()->getXDisplay(), gnome_win, 
		fba->getGnomeSupportingWMCheckAtom(), 
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

	Atom gnomeatomlist[] = {
		fba->getGnomeWorkspaceAtom(),
		fba->getGnomeWorkspaceCountAtom(),
		fba->getGnomeStateAtom(),
		fba->getGnomeWorkspaceNamesAtom(),
		fba->getGnomeHintsAtom(),
		fba->getGnomeClientListAtom(),
//		getBaseDisplay()->getGnomeLayerAtom(), // not supported yet
	};

	//list atoms that we support
	XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(), 
		fba->getGnomeProtAtom(), XA_ATOM, 32, PropModeReplace,
		(unsigned char *)gnomeatomlist, (sizeof gnomeatomlist)/sizeof gnomeatomlist[0]);

}

void BScreen::updateGnomeClientList() {
	int num=0;
	Workspaces::iterator workspace_it = workspacesList.begin();
	Workspaces::iterator workspace_it_end = workspacesList.end();
	for (; workspace_it != workspace_it_end; ++workspace_it) {
		num += (*workspace_it)->getWindowList().size();
	}
	//int num = getCurrentWorkspace()->getWindowList().size();
	
	Window *wl = new Window[num];
	//start the iterator from begining
	workspace_it = workspacesList.begin();
	int win=0;
	for (; workspace_it != workspace_it_end; ++workspace_it) {
	
		// Fill in array of window ID's
		Workspace::Windows::const_iterator it = (*workspace_it)->getWindowList().begin();
		Workspace::Windows::const_iterator it_end = (*workspace_it)->getWindowList().end();		
		for (; it != it_end; ++it) {
			//check if the window don't want to be visible in the list
			if (! ( (*it)->getGnomeHints() & FluxboxWindow::WIN_STATE_HIDDEN) ) {
				wl[win++] = (*it)->getClientWindow();
			}
		}
	}
	//number of windows to show in client list
	num = win;
	XChangeProperty(getBaseDisplay()->getXDisplay(), 
		getRootWindow(), FbAtoms::instance()->getGnomeClientListAtom(), XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)wl, num);
	
	if (wl)
		delete wl;
}

#endif //!GNOME

