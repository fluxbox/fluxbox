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
#include "Screen.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS


Windowmenu::Windowmenu(FluxboxWindow *win) : Basemenu(win->getScreen()) {
  window = win;
  screen = window->getScreen();

  setTitleVisibility(False);
  setMovable(False);
  setInternalMenu();
	
	I18n *i18n = I18n::instance();
	
  sendToMenu = new SendtoWorkspacemenu(this);
  sendGroupToMenu = new SendGroupToWorkspacemenu(this);
	
	insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuSendTo,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Send To ..."),
	 sendToMenu);
	 
	insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuSendGroupTo,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Send Group To ..."),
	 sendGroupToMenu);
	 
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuShade,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Shade"),
	 BScreen::WINDOWSHADE);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuIconify,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Iconify"),
	 BScreen::WINDOWICONIFY);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuMaximize,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Maximize"),
	 BScreen::WINDOWMAXIMIZE);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuRaise,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Raise"),
	 BScreen::WINDOWRAISE);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuLower,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Lower"),
	 BScreen::WINDOWLOWER);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuStick,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Stick"),
	 BScreen::WINDOWSTICK);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuKillClient,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Kill Client"),
	 BScreen::WINDOWKILL);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuClose,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Close"),
	 BScreen::WINDOWCLOSE);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuTab,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Tab"),
	 BScreen::WINDOWTAB);

  update();

  setItemEnabled(2, window->hasTitlebar());
  setItemEnabled(3, window->isIconifiable());
  setItemEnabled(4, window->isMaximizable());
  setItemEnabled(9, window->isClosable());
  setItemEnabled(10, window->hasTab());

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


void Windowmenu::itemSelected(int button, int index) {
  BasemenuItem *item = find(index);

  switch (item->function()) {
  case BScreen::WINDOWSHADE:
    hide();

    window->shade();
		if (window->hasTab())
			window->getTab()->shade();
    break;

  case BScreen::WINDOWICONIFY:
    hide();
    window->iconify();
    break;

  case BScreen::WINDOWMAXIMIZE:
    hide();
    window->maximize((unsigned int) button);
    break;

  case BScreen::WINDOWCLOSE:
    hide();
    window->close();
    break;

  case BScreen::WINDOWRAISE:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
    break;

  case BScreen::WINDOWLOWER:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->lowerWindow(window);
    break;

  case BScreen::WINDOWSTICK:
    hide();
    window->stick();
    break;

  case BScreen::WINDOWKILL:
    hide();
    XKillClient(screen->getBaseDisplay()->getXDisplay(),
                window->getClientWindow());
    break;
	case BScreen::WINDOWTAB:
		hide();
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


void Windowmenu::SendtoWorkspacemenu::itemSelected(int button, int index) {
  if (button > 2) return;

  if (index <= windowmenu->screen->getCount()) {
    if (index == windowmenu->screen->getCurrentWorkspaceID()) return;
    if (windowmenu->window->isStuck()) windowmenu->window->stick();

    if (button == 1) windowmenu->window->withdraw();
    windowmenu->screen->reassociateWindow(windowmenu->window, index, True);
		if (windowmenu->window->getTab()) {
			windowmenu->window->getTab()->disconnect();
			windowmenu->window->getTab()->setPosition();
		}
    if (button == 2) windowmenu->screen->changeWorkspaceID(index);
  }
  hide();
}


void Windowmenu::SendtoWorkspacemenu::update(void) {
  int i, r = getCount();

  if (getCount() != 0)
    for (i = 0; i < r; ++i)
      remove(0);

  for (i = 0; i < windowmenu->screen->getCount(); ++i)
    insert(windowmenu->screen->getWorkspace(i)->getName());

  Basemenu::update();
}


void Windowmenu::SendtoWorkspacemenu::show(void) {
  update();

  Basemenu::show();
}

void Windowmenu::SendGroupToWorkspacemenu::itemSelected(int button, int index) {
	if (button > 2)
		return;

	if (index <= getWindowMenu()->screen->getCount()) {
		if (index == getWindowMenu()->screen->getCurrentWorkspaceID())
			return;
		if (getWindowMenu()->window->isStuck())
			getWindowMenu()->window->stick();

		if (button == 1) {
			if (getWindowMenu()->window->hasTab()) {
				for (Tab *first = Tab::getFirst(getWindowMenu()->window->getTab());
							first!=0; first=first->next()) {
						first->withdraw();
						first->getWindow()->withdraw();
						getWindowMenu()->screen->reassociateWindow(first->getWindow(), index, True);
					
				}
			} else  {
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


