// Configmenu.cc for Blackbox - An X11 Window Manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef	 _GNU_SOURCE
#	define _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
# include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "Configmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"

enum {CMENU_USE_TABS=9, CMENU_USE_ICONS, CMENU_SLOPPY_WIN_GROUP, CMENU_TAB_ROTATE=21};


Configmenu::Configmenu(BScreen *scr) : Basemenu(scr) {
	screen = scr;
	
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuConfigOptions,
#else // !NLS
				0, 0,
#endif // NLS
				"Config options"));
	
	setInternalMenu();

	focusmenu = new Focusmenu(this);
	placementmenu = new Placementmenu(this);
	tabmenu = new Tabmenu(this);

	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuFocusModel,
#else // !NLS
				0, 0,
#endif // NLS
				"Focus Model"), focusmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuWindowPlacement,
#else //! NLS
				0, 0,
#endif // NLS
				"Window Placement"), placementmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuTabPlacement,
#else // !NLS
				0, 0,
#endif	// NLS
				"Tab Placement"), tabmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuImageDithering,
#else // !NLS
				0, 0,
#endif // NLS
				"Image Dithering"), 1);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuOpaqueMove,
#else // !NLS
				0, 0,
#endif // NLS
				"Opaque Window Moving"), 2);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuFullMax,
#else // !NLS
				0, 0,
#endif // NLS
				"Full Maximization"), 3);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuFocusNew,
#else // !NLS
				0, 0,
#endif // NLS
				"Focus New Windows"), 4);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuFocusLast,
#else // !NLS
				0, 0,
#endif // NLS
				"Focus Last Window on Workspace"), 5);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuMaxOverSlit,
#else // !NLS
				0, 0,
#endif // NLS
				"Maxmize Over Slit"), 6);


    insert(i18n->getMessage(
#ifdef   NLS
       ConfigmenuSet, ConfigmenuTabs,
#else // !NLS
       0, 0,
#endif // NLS
       "Use Tabs"), CMENU_USE_TABS);
 insert(i18n->getMessage(
#ifdef   NLS
       ConfigmenuSet, ConfigmenuIcons,
#else // !NLS
       0, 0,
#endif // NLS
       "Use Icons"), CMENU_USE_ICONS);
 insert(i18n->getMessage(
#ifdef   NLS
       ConfigmenuSet, ConfigmenuSloppyWindowGrouping,
#else // !NLS
       0, 0,
#endif // NLS
       "Sloppy Window Grouping"), CMENU_SLOPPY_WIN_GROUP);

	update();
	setItemSelected(8, screen->doMaxOverSlit());

	setItemSelected(3, screen->getImageControl()->doDither());
	setItemSelected(4, screen->doOpaqueMove());
	setItemSelected(5, screen->doFullMax());
	setItemSelected(6, screen->doFocusNew());
	setItemSelected(7, screen->doFocusLast());
	setItemSelected(CMENU_USE_TABS, Fluxbox::instance()->useTabs());
	setItemSelected(CMENU_USE_ICONS, Fluxbox::instance()->useIconBar());
	setItemSelected(CMENU_SLOPPY_WIN_GROUP, screen->isSloppyWindowGrouping());
}


Configmenu::~Configmenu(void) {
	delete focusmenu;
	delete placementmenu;
	delete tabmenu;
}


void Configmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function())
			switch(item->function()) {
			case 1:  // dither
				screen->getImageControl()->
				setDither((! screen->getImageControl()->doDither()));

				setItemSelected(index, screen->getImageControl()->doDither());

				break;

			case 2:  // opaque move
				screen->saveOpaqueMove((! screen->doOpaqueMove()));

				setItemSelected(index, screen->doOpaqueMove());

				break;

			case 3:  // full maximization
				screen->saveFullMax((! screen->doFullMax()));

				setItemSelected(index, screen->doFullMax());

				break;
			case 4:  // focus new windows
				screen->saveFocusNew((! screen->doFocusNew()));

				
			case 6: // maximize over slit
				screen->saveMaxOverSlit((! screen->doMaxOverSlit()));
				setItemSelected(index, screen->doMaxOverSlit());
				break;	
			
				setItemSelected(index, screen->doFocusNew());
				break;

			case 5:  // focus last window on workspace
				screen->saveFocusLast((! screen->doFocusLast()));
				setItemSelected(index, screen->doFocusLast());
				break;
			case CMENU_USE_TABS: 
				{
					Fluxbox *fluxbox = Fluxbox::instance();
					fluxbox->saveTabs(!fluxbox->useTabs());
					setItemSelected(index, fluxbox->useTabs());
					screen->reconfigure();
				}
				break;
			case CMENU_USE_ICONS:
				{
					Fluxbox *fluxbox = Fluxbox::instance();
					fluxbox->saveIconBar(!fluxbox->useIconBar());
					setItemSelected(index, fluxbox->useIconBar());
					screen->reconfigure();
				}
				break;
			case CMENU_SLOPPY_WIN_GROUP:
				{
					screen->saveSloppyWindowGrouping(!screen->isSloppyWindowGrouping());
					setItemSelected(index, screen->isSloppyWindowGrouping());
					screen->reconfigure();
				}
				break;

			}
		}
}


void Configmenu::reconfigure(void) {
	focusmenu->reconfigure();
	placementmenu->reconfigure();
	tabmenu->reconfigure();

	Basemenu::reconfigure();
}


Configmenu::Focusmenu::Focusmenu(Configmenu *cm) : Basemenu(cm->screen) {
	configmenu = cm;
	I18n *i18n = I18n::instance();
	setLabel(i18n->getMessage(
#ifdef		NLS
					ConfigmenuSet, ConfigmenuFocusModel,
#else // !NLS
					0, 0,
#endif // NLS
					"Focus Model"));
	setInternalMenu();

	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuClickToFocus,
#else // !NLS
				0, 0,
#endif // NLS
				"Click To Focus"), 1);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuSloppyFocus,
#else // !NLS
				0, 0,
#endif // NLS
				"Sloppy Focus"), 2);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuSemiSloppyFocus,
#else // !NLS
				0, 0,
#endif // NLS
				"Semi Sloppy Focus"), 3);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuAutoRaise,
#else // !NLS
				0, 0,
#endif // NLS
				"Auto Raise"), 4);

	update();

	setItemSelected(0, !(configmenu->screen->isSloppyFocus() || 
							configmenu->screen->isSemiSloppyFocus()));
	setItemSelected(1, configmenu->screen->isSloppyFocus());
	setItemSelected(2, configmenu->screen->isSemiSloppyFocus());
	setItemEnabled(3, (configmenu->screen->isSloppyFocus() ||
						configmenu->screen->isSemiSloppyFocus()));
	setItemSelected(4, configmenu->screen->doAutoRaise());
}


void Configmenu::Focusmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function()) {
			switch (item->function()) {
			case 1: // click to focus
				configmenu->screen->saveSloppyFocus(False);
				configmenu->screen->saveSemiSloppyFocus(False);
				configmenu->screen->saveAutoRaise(False);

				if (! Fluxbox::instance()->getFocusedWindow()) {
					XSetInputFocus(Fluxbox::instance()->getXDisplay(),
						configmenu->screen->getToolbar()->getWindowID(),
						RevertToParent, CurrentTime);
				} else {
					XSetInputFocus(Fluxbox::instance()->getXDisplay(),
			 			Fluxbox::instance()->getFocusedWindow()->getClientWindow(),
			 			RevertToParent, CurrentTime);
				}

				configmenu->screen->reconfigure();

			break;

			case 2: // sloppy focus
				configmenu->screen->saveSemiSloppyFocus(False);
				configmenu->screen->saveSloppyFocus(True);

				configmenu->screen->reconfigure();

			break;

			case 3: // semi sloppy focus
				configmenu->screen->saveSloppyFocus(False);
				configmenu->screen->saveSemiSloppyFocus(True);

				configmenu->screen->reconfigure();

			break;

			case 4: // auto raise with sloppy focus
				Bool change = ((configmenu->screen->doAutoRaise()) ? False : True);
				configmenu->screen->saveAutoRaise(change);

			break;
			}

			setItemSelected(0, !(configmenu->screen->isSloppyFocus() || 
							configmenu->screen->isSemiSloppyFocus()));
			setItemSelected(1, configmenu->screen->isSloppyFocus());
			setItemSelected(2, configmenu->screen->isSemiSloppyFocus());
			setItemEnabled(3, (configmenu->screen->isSloppyFocus() ||
						configmenu->screen->isSemiSloppyFocus()));
			setItemSelected(3, configmenu->screen->doAutoRaise());
		}
	}
}


Configmenu::Placementmenu::Placementmenu(Configmenu *cm) : Basemenu(cm->screen) {
	configmenu = cm;
	I18n *i18n = I18n::instance();

	setLabel(i18n->getMessage(
#ifdef		NLS
					ConfigmenuSet, ConfigmenuWindowPlacement,
#else // !NLS
					0, 0,
#endif // NLS
					"Window Placement"));
	setInternalMenu();

	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuSmartRows,
#else // !NLS
				0, 0,
#endif // NLS
				"Smart Placement (Rows)"), BScreen::RowSmartPlacement);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuSmartCols,
#else // !NLS
				0, 0,
#endif // NLS
			"Smart Placement (Columns)"), BScreen::ColSmartPlacement);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuCascade,
#else // !NLS
				0, 0,
#endif // NLS
				"Cascade Placement"), BScreen::CascadePlacement);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuLeftRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Left to Right"), BScreen::LeftRight);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuRightLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Right to Left"), BScreen::RightLeft);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuTopBottom,
#else // !NLS
				0, 0,
#endif // NLS
				"Top to Bottom"), BScreen::TopBottom);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuBottomTop,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom to Top"), BScreen::BottomTop);

	update();

	switch (configmenu->screen->getPlacementPolicy()) {
	case BScreen::RowSmartPlacement:
		setItemSelected(0, True);
		break;

	case BScreen::ColSmartPlacement:
		setItemSelected(1, True);
		break;

	case BScreen::CascadePlacement:
		setItemSelected(2, True);
		break;
	}

	Bool rl = (configmenu->screen->getRowPlacementDirection() ==
				BScreen::LeftRight),
				tb = (configmenu->screen->getColPlacementDirection() ==
				BScreen::TopBottom);

	setItemSelected(3, rl);
	setItemSelected(4, ! rl);

	setItemSelected(5, tb);
	setItemSelected(6, ! tb);
}


void Configmenu::Placementmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function()) {
			switch (item->function()) {
			case BScreen::RowSmartPlacement:
				configmenu->screen->savePlacementPolicy(item->function());

				setItemSelected(0, True);
				setItemSelected(1, False);
				setItemSelected(2, False);
				
				break;

			case BScreen::ColSmartPlacement:
				configmenu->screen->savePlacementPolicy(item->function());

				setItemSelected(0, False);
				setItemSelected(1, True);
				setItemSelected(2, False);

				break;

			case BScreen::CascadePlacement:
				configmenu->screen->savePlacementPolicy(item->function());

				setItemSelected(0, False);
				setItemSelected(1, False);
				setItemSelected(2, True);

				break;

			case BScreen::LeftRight:
				configmenu->screen->saveRowPlacementDirection(BScreen::LeftRight);

				setItemSelected(3, True);
				setItemSelected(4, False);

				break;

			case BScreen::RightLeft:
				configmenu->screen->saveRowPlacementDirection(BScreen::RightLeft);

				setItemSelected(3, False);
				setItemSelected(4, True);

	break;

			case BScreen::TopBottom:
	configmenu->screen->saveColPlacementDirection(BScreen::TopBottom);

	setItemSelected(5, True);
	setItemSelected(6, False);

	break;

			case BScreen::BottomTop:
	configmenu->screen->saveColPlacementDirection(BScreen::BottomTop);

	setItemSelected(5, False);
	setItemSelected(6, True);

	break;
			}
		}
	}
}

Configmenu::Tabmenu::Tabmenu(Configmenu *cm) : Basemenu(cm->screen) {
	configmenu = cm;
	I18n *i18n = I18n::instance();

	setLabel(i18n->getMessage(
#ifdef		NLS
					ConfigmenuSet, ConfigmenuTabPlacement,
#else // !NLS
					0, 0,
#endif // NLS
					"Tab Placement"));
	setInternalMenu();

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Left"), Tab::PTop + Tab::ALeft);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Center"), Tab::PTop + Tab::ACenter);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Right"), Tab::PTop + Tab::ARight);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopRelative,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Relative"), Tab::PTop + Tab::ARelative);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Left"), Tab::PBottom + Tab::ALeft);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Center"), Tab::PBottom + Tab::ACenter);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Right"), Tab::PBottom + Tab::ARight);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomRelative,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Relative"), Tab::PBottom + Tab::ARelative);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementLeftTop,
#else // !NLS
				0, 0,
#endif // NLS
				"Left Top"), Tab::PLeft + Tab::ARight);
	insert(i18n->getMessage(
#ifdef	NLS
				CommonSet, CommonPlacementLeftCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Left Center"), Tab::PLeft + Tab::ACenter);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementLeftBottom,
#else // !NLS
				0, 0,
#endif // NLS
				"Left Bottom"), Tab::PLeft + Tab::ALeft);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementLeftRelative,
#else // !NLS
				0, 0,
#endif // NLS
				"Left Relative"), Tab::PLeft + Tab::ARelative);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementRightTop,
#else // !NLS
				0, 0,
#endif // NLS
				"Right Top"), Tab::PRight + Tab::ARight);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementRightCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Right Center"), Tab::PRight + Tab::ACenter);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementRightBottom,
#else // !NLS
				0, 0,
#endif // NLS
				"Right Bottom"), Tab::PRight + Tab::ALeft);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementRightRelative,
#else // !NLS
				0, 0,
#endif // NLS
				"Right Relative"), Tab::PRight + Tab::ARelative);
	insert(i18n->getMessage(
#ifdef		NLS
				ConfigmenuSet, ConfigmenuTabRotateVertical,
#else // !NLS
				0, 0,
#endif // NLS
				"Rotate Vertical Tabs"), CMENU_TAB_ROTATE);
	update();
	setSelected();
}


void Configmenu::Tabmenu::itemSelected(int button, int index) {
	if (button == 1) {
		fprintf(stderr, "Item(%d)\n", index);
		BasemenuItem *item = find(index);
		
		switch (item->function()) {
			case (Tab::PTop + Tab::ALeft):
				configmenu->screen->saveTabPlacement(Tab::PTop);
				configmenu->screen->saveTabAlignment(Tab::ALeft);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PTop + Tab::ACenter):
				configmenu->screen->saveTabPlacement(Tab::PTop);
				configmenu->screen->saveTabAlignment(Tab::ACenter);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PTop + Tab::ARight):
				configmenu->screen->saveTabPlacement(Tab::PTop);
				configmenu->screen->saveTabAlignment(Tab::ARight);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PTop + Tab::ARelative):
				configmenu->screen->saveTabPlacement(Tab::PTop);
				configmenu->screen->saveTabAlignment(Tab::ARelative);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PBottom + Tab::ALeft):
				configmenu->screen->saveTabPlacement(Tab::PBottom);
				configmenu->screen->saveTabAlignment(Tab::ALeft);

				configmenu->screen->reconfigure();

				break;
			case (Tab::PBottom + Tab::ACenter):
				configmenu->screen->saveTabPlacement(Tab::PBottom);
				configmenu->screen->saveTabAlignment(Tab::ACenter);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PBottom + Tab::ARight):
				configmenu->screen->saveTabPlacement(Tab::PBottom);
				configmenu->screen->saveTabAlignment(Tab::ARight);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PBottom + Tab::ARelative):
				configmenu->screen->saveTabPlacement(Tab::PBottom);
				configmenu->screen->saveTabAlignment(Tab::ARelative);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PLeft + Tab::ARight):
				configmenu->screen->saveTabPlacement(Tab::PLeft);
				configmenu->screen->saveTabAlignment(Tab::ARight);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PLeft + Tab::ACenter):
				configmenu->screen->saveTabPlacement(Tab::PLeft);
				configmenu->screen->saveTabAlignment(Tab::ACenter);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PLeft + Tab::ALeft):
				configmenu->screen->saveTabPlacement(Tab::PLeft);
				configmenu->screen->saveTabAlignment(Tab::ALeft);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PLeft + Tab::ARelative):
				configmenu->screen->saveTabPlacement(Tab::PLeft);
				configmenu->screen->saveTabAlignment(Tab::ARelative);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PRight + Tab::ARight):
				configmenu->screen->saveTabPlacement(Tab::PRight);
				configmenu->screen->saveTabAlignment(Tab::ARight);

				configmenu->screen->reconfigure();

				break;
			case (Tab::PRight + Tab::ACenter):
				configmenu->screen->saveTabPlacement(Tab::PRight);
				configmenu->screen->saveTabAlignment(Tab::ACenter);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PRight + Tab::ALeft):
				configmenu->screen->saveTabPlacement(Tab::PRight);
				configmenu->screen->saveTabAlignment(Tab::ALeft);

				configmenu->screen->reconfigure();

				break;

			case (Tab::PRight + Tab::ARelative):
				configmenu->screen->saveTabPlacement(Tab::PRight);
				configmenu->screen->saveTabAlignment(Tab::ARelative);

				configmenu->screen->reconfigure();

				break;

			case CMENU_TAB_ROTATE:
				if (configmenu->screen->isTabRotateVertical())
					configmenu->screen->saveTabRotateVertical(False);
				else
					configmenu->screen->saveTabRotateVertical(True);
				break;
			}

			Fluxbox::instance()->reconfigureTabs(); //TODO
			setSelected();			
	}
}

void Configmenu::Tabmenu::setSelected(void) {
	setItemSelected(0, (configmenu->screen->getTabPlacement() == Tab::PTop &&
						configmenu->screen->getTabAlignment() == Tab::ALeft));
	setItemSelected(1, (configmenu->screen->getTabPlacement() == Tab::PTop &&
						configmenu->screen->getTabAlignment() == Tab::ACenter));
	setItemSelected(2, (configmenu->screen->getTabPlacement() == Tab::PTop &&
						configmenu->screen->getTabAlignment() == Tab::ARight));
	setItemSelected(3, (configmenu->screen->getTabPlacement() == Tab::PTop &&
						configmenu->screen->getTabAlignment() == Tab::ARelative));
	setItemSelected(4, (configmenu->screen->getTabPlacement() == Tab::PBottom &&
						configmenu->screen->getTabAlignment() == Tab::ALeft));
	setItemSelected(5, (configmenu->screen->getTabPlacement() == Tab::PBottom &&
						configmenu->screen->getTabAlignment() == Tab::ACenter));
	setItemSelected(6, (configmenu->screen->getTabPlacement() == Tab::PBottom &&
						configmenu->screen->getTabAlignment() == Tab::ARight));
	setItemSelected(7, (configmenu->screen->getTabPlacement() == Tab::PBottom &&
						configmenu->screen->getTabAlignment() == Tab::ARelative));
	setItemSelected(8, (configmenu->screen->getTabPlacement() == Tab::PLeft &&
						configmenu->screen->getTabAlignment() == Tab::ARight));
	setItemSelected(9, (configmenu->screen->getTabPlacement() == Tab::PLeft &&
						configmenu->screen->getTabAlignment() == Tab::ACenter));
	setItemSelected(10, (configmenu->screen->getTabPlacement() == Tab::PLeft &&
						configmenu->screen->getTabAlignment() == Tab::ALeft));
	setItemSelected(11, (configmenu->screen->getTabPlacement() == Tab::PLeft &&
						configmenu->screen->getTabAlignment() == Tab::ARelative));
	setItemSelected(12, (configmenu->screen->getTabPlacement() == Tab::PRight &&
						configmenu->screen->getTabAlignment() == Tab::ARight));
	setItemSelected(13, (configmenu->screen->getTabPlacement() == Tab::PRight &&
						configmenu->screen->getTabAlignment() == Tab::ACenter));
	setItemSelected(14, (configmenu->screen->getTabPlacement() == Tab::PRight &&
						configmenu->screen->getTabAlignment() == Tab::ALeft));
	setItemSelected(15, (configmenu->screen->getTabPlacement() == Tab::PRight &&
						configmenu->screen->getTabAlignment() == Tab::ARelative));
	setItemEnabled(16, (configmenu->screen->getTabPlacement() == Tab::PLeft ||
						configmenu->screen->getTabPlacement() == Tab::PRight));
	setItemSelected(16, (configmenu->screen->isTabRotateVertical() &&
						(configmenu->screen->getTabPlacement() == Tab::PLeft ||
						configmenu->screen->getTabPlacement() == Tab::PRight))); 
}
