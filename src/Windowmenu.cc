// Windowmenu.cc for Fluxbox
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// Windowmenu.cc for Blackbox - an X11 Window manager
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

// $Id: Windowmenu.cc,v 1.17 2002/08/31 10:42:25 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"

#ifdef STDC_HEADERS
#include <string.h>
#endif // STDC_HEADERS


Windowmenu::Windowmenu(FluxboxWindow *win) : Basemenu(win->getScreen()),
window(win),
screen(window->getScreen()){

	setTitleVisibility(False);
	setMovable(False);
	setInternalMenu();
	
	I18n *i18n = I18n::instance();
	
	sendToMenu = new SendtoWorkspacemenu(this);
	sendGroupToMenu = new SendGroupToWorkspacemenu(this);
	using namespace FBNLS;	
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuSendTo,
		"Send To ..."),
	 sendToMenu);
	 
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuSendGroupTo,
		"Send Group To ..."),
	sendGroupToMenu);
	 
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuShade,
		"Shade"),
	 BScreen::WINDOWSHADE);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuIconify,
		"Iconify"),
	 BScreen::WINDOWICONIFY);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuMaximize,
		"Maximize"),
	 BScreen::WINDOWMAXIMIZE);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuRaise,
		"Raise"),
	 BScreen::WINDOWRAISE);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuLower,
		"Lower"),
	 BScreen::WINDOWLOWER);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuStick,
		"Stick"),
	 BScreen::WINDOWSTICK);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuKillClient,
		"Kill Client"),
	 BScreen::WINDOWKILL);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuClose,
		"Close"),
	 BScreen::WINDOWCLOSE);
	insert(i18n->getMessage(
		WindowmenuSet, WindowmenuTab,
		"Tab"),
	 BScreen::WINDOWTAB);

	update();

	setItemEnabled(2, window->hasTitlebar());
	setItemEnabled(3, window->isIconifiable());
	setItemEnabled(4, window->isMaximizable());
	setItemEnabled(9, window->isClosable());
	setItemEnabled(10, true); //we should always be able to enable the tab

}


Windowmenu::~Windowmenu(void) {
	delete sendToMenu;
	delete sendGroupToMenu;
}


void Windowmenu::show(void) {
	if (isItemEnabled(2)) setItemSelected(2, window->isShaded());
	if (isItemEnabled(4)) setItemSelected(4, window->isMaximized());
	if (isItemEnabled(7)) setItemSelected(7, window->isStuck());

	Basemenu::show();
}


void Windowmenu::itemSelected(int button, unsigned int index) {
	BasemenuItem *item = find(index);
	hide();
	switch (item->function()) {
	case BScreen::WINDOWSHADE:
		if (window->isIconic())
			break;
			
		window->shade();
		if (window->hasTab())
			window->getTab()->shade();
	break;

	case BScreen::WINDOWICONIFY:
		if (!window->isIconic())
			window->iconify();
		else
			window->deiconify(); // restore window
			
	break;

	case BScreen::WINDOWMAXIMIZE:
		window->maximize((unsigned int) button);
	break;

	case BScreen::WINDOWCLOSE:
		window->close();
	break;

	case BScreen::WINDOWRAISE:
		if (window->isIconic())
			break;

		if (window->hasTab())
			window->getTab()->raise(); //raise tabs
		screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
	break;

	case BScreen::WINDOWLOWER:
	 	if (window->isIconic())
			break;

		screen->getWorkspace(window->getWorkspaceNumber())->lowerWindow(window);
		if (window->hasTab())
			window->getTab()->lower(); //lower tabs AND all it's windows
		break;

	case BScreen::WINDOWSTICK:
		window->stick();
		break;

	case BScreen::WINDOWKILL:
		XKillClient(screen->getBaseDisplay()->getXDisplay(),
								window->getClientWindow());
		break;
	case BScreen::WINDOWTAB:
		window->setTab(!window->hasTab());
		break;
	}
}


void Windowmenu::reconfigure(void) {
	setItemEnabled(1, window->hasTitlebar());
	setItemEnabled(2, window->isIconifiable());
	setItemEnabled(3, window->isMaximizable());
	setItemEnabled(8, window->isClosable());

	sendToMenu->reconfigure();
	sendGroupToMenu->reconfigure();
	
	Basemenu::reconfigure();
}


Windowmenu::SendtoWorkspacemenu::SendtoWorkspacemenu(Windowmenu *w)
	: Basemenu(w->screen)
{
	windowmenu = w;

	setTitleVisibility(False);
	setMovable(False);
	setInternalMenu();
	update();
}


void Windowmenu::SendtoWorkspacemenu::itemSelected(int button, unsigned int index) {
	if (button > 2) return;

	if (index <= windowmenu->screen->getCount()) {
		
		// no need to send it to a workspace it already exist on
		if (index == windowmenu->screen->getCurrentWorkspaceID())
			return;

		// if the window is stuck then unstick it
		if (windowmenu->window->isStuck())
			windowmenu->window->stick();

		if (button == 1) { // send to workspace without changing workspace
			windowmenu->screen->sendToWorkspace(index,
				windowmenu->window, false);
		} else if (button == 2) { // send to workspace and change workspace
			windowmenu->screen->sendToWorkspace(index,
				windowmenu->window, true);
		}
	}

	hide();
}


void Windowmenu::SendtoWorkspacemenu::update(void) {
	unsigned int i, r = numberOfItems();

	if (numberOfItems() != 0) {
		for (i = 0; i < r; ++i)
			remove(0);
	}
	for (i = 0; i < windowmenu->screen->getCount(); ++i)
		insert(windowmenu->screen->getWorkspace(i)->name().c_str());

	Basemenu::update();
}


void Windowmenu::SendtoWorkspacemenu::show(void) {
	update();

	Basemenu::show();
}

void Windowmenu::SendGroupToWorkspacemenu::itemSelected(int button, unsigned int index) {
	if (button > 2)
		return;

	if (index <= getWindowMenu()->screen->getCount()) {
		if (index == getWindowMenu()->screen->getCurrentWorkspaceID())
			return;
		if (getWindowMenu()->window->isStuck())
			getWindowMenu()->window->stick();

		if (button == 1) {
			// TODO: use reassociateGroup from BScreen instead
			if (getWindowMenu()->window->hasTab()) {
				for (Tab *first = Tab::getFirst(getWindowMenu()->window->getTab());
						first!=0; first=first->next()) {
					first->withdraw();
					first->getWindow()->withdraw();
					getWindowMenu()->screen->reassociateWindow(first->getWindow(), index, True);
					
				}
			} else	{
				getWindowMenu()->window->withdraw();
				getWindowMenu()->screen->reassociateWindow(getWindowMenu()->window, index, True);
			}
					
		}
		
		if (button == 2)
			getWindowMenu()->screen->changeWorkspaceID(index);
	}
	hide();
}

Windowmenu::SendGroupToWorkspacemenu::SendGroupToWorkspacemenu(Windowmenu *w):SendtoWorkspacemenu(w)
{

}


