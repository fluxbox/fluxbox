// Configmenu.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Configmenu.cc,v 1.17 2002/10/15 09:49:51 fluxgen Exp $

#include "Configmenu.hh"

#include "i18n.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Screen.hh"
#include "Tab.hh"

enum {CMENU_USE_TABS=9, CMENU_USE_ICONS, CMENU_SLOPPY_WIN_GROUP,
	CMENU_DESKTOP_WHEELING, CMENU_WORKSPACE_WARPING, CMENU_TAB_ROTATE=21};


Configmenu::Configmenu(BScreen *scr) : Basemenu(scr),
m_focusmenu(scr),
m_placementmenu(scr),
m_tabmenu(scr) {
	
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
		FBNLS::ConfigmenuSet, FBNLS::ConfigmenuConfigOptions,
		"Config options"));
	
	setInternalMenu();

	using namespace FBNLS;
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuFocusModel,
		"Focus Model"), &m_focusmenu);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuWindowPlacement,
		"Window Placement"), &m_placementmenu);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuTabPlacement,
		"Tab Placement"), &m_tabmenu);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuImageDithering,
		"Image Dithering"), 1);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuOpaqueMove,
		"Opaque Window Moving"), 2);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuFullMax,
		"Full Maximization"), 3);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuFocusNew,
		"Focus New Windows"), 4);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuFocusLast,
		"Focus Last Window on Workspace"), 5);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuMaxOverSlit,
		"Maximize Over Slit"), 6);
    insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuTabs,
		"Use Tabs"), CMENU_USE_TABS);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuIcons,
		"Use Icons"), CMENU_USE_ICONS);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuSloppyWindowGrouping,
		"Sloppy Window Grouping"), CMENU_SLOPPY_WIN_GROUP);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuWorkspaceWarping,
		"Workspace Warping"), CMENU_WORKSPACE_WARPING);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuDesktopWheeling,
		"Desktop MouseWheel Switching"), CMENU_DESKTOP_WHEELING);

	update();
	setItemSelected(8, screen()->doMaxOverSlit());

	setItemSelected(3, screen()->getImageControl()->doDither());
	setItemSelected(4, screen()->doOpaqueMove());
	setItemSelected(5, screen()->doFullMax());
	setItemSelected(6, screen()->doFocusNew());
	setItemSelected(7, screen()->doFocusLast());
	setItemSelected(CMENU_USE_TABS, Fluxbox::instance()->useTabs());
	setItemSelected(CMENU_USE_ICONS, Fluxbox::instance()->useIconBar());
	setItemSelected(CMENU_SLOPPY_WIN_GROUP, screen()->isSloppyWindowGrouping());
	setItemSelected(CMENU_WORKSPACE_WARPING, screen()->isWorkspaceWarping());
	setItemSelected(CMENU_DESKTOP_WHEELING, screen()->isDesktopWheeling());
}


Configmenu::~Configmenu() {

}


void Configmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function())
			switch(item->function()) {
			case 1:  // dither
				screen()->getImageControl()->
				setDither((! screen()->getImageControl()->doDither()));

				setItemSelected(index, screen()->getImageControl()->doDither());

				break;

			case 2:  // opaque move
				screen()->saveOpaqueMove((! screen()->doOpaqueMove()));

				setItemSelected(index, screen()->doOpaqueMove());

				break;

			case 3:  // full maximization
				screen()->saveFullMax((! screen()->doFullMax()));

				setItemSelected(index, screen()->doFullMax());

				break;
			case 4:  // focus new windows
				screen()->saveFocusNew((! screen()->doFocusNew()));
			
				setItemSelected(index, screen()->doFocusNew());
				break;
				
			case 6: // maximize over slit
				screen()->saveMaxOverSlit( !screen()->doMaxOverSlit() );
				setItemSelected(index, screen()->doMaxOverSlit());
				break;	


			case 5:  // focus last window on workspace
				screen()->saveFocusLast((! screen()->doFocusLast()));
				setItemSelected(index, screen()->doFocusLast());
				break;
			case CMENU_USE_TABS: 
				{
					Fluxbox *fluxbox = Fluxbox::instance();
					fluxbox->saveTabs(!fluxbox->useTabs());
					setItemSelected(index, fluxbox->useTabs());
					screen()->reconfigure();
				}
				break;
			case CMENU_USE_ICONS:
				{
					Fluxbox *fluxbox = Fluxbox::instance();
					fluxbox->saveIconBar(!fluxbox->useIconBar());
					setItemSelected(index, fluxbox->useIconBar());
					screen()->reconfigure();
				}
				break;
			case CMENU_SLOPPY_WIN_GROUP:
				{
					screen()->saveSloppyWindowGrouping(!screen()->isSloppyWindowGrouping());
					setItemSelected(index, screen()->isSloppyWindowGrouping());
					screen()->reconfigure();
				}
				break;
			case CMENU_WORKSPACE_WARPING:
				{
					screen()->saveWorkspaceWarping(!screen()->isWorkspaceWarping());
					setItemSelected(index, screen()->isWorkspaceWarping());
					screen()->reconfigure();
				}
				break;
			case CMENU_DESKTOP_WHEELING:
				{
					screen()->saveDesktopWheeling(!screen()->isDesktopWheeling());
					setItemSelected(index, screen()->isDesktopWheeling());
					screen()->reconfigure();
				}
				break;

			}
			//save resources
			Fluxbox::instance()->save_rc();
			
		}
}


void Configmenu::reconfigure() {
	m_focusmenu.reconfigure();
	m_placementmenu.reconfigure();
	m_tabmenu.reconfigure();

	Basemenu::reconfigure();
}


Configmenu::Focusmenu::Focusmenu(BScreen *scr) : Basemenu(scr) {
	I18n *i18n = I18n::instance();
	using namespace FBNLS;
	setLabel(i18n->getMessage(
			ConfigmenuSet, ConfigmenuFocusModel,
			"Focus Model"));
	setInternalMenu();

	insert(i18n->getMessage(
			ConfigmenuSet, ConfigmenuClickToFocus,
			"Click To Focus"), 1);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuSloppyFocus,
		"Sloppy Focus"), 2);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuSemiSloppyFocus,
		"Semi Sloppy Focus"), 3);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuAutoRaise,
		"Auto Raise"), 4);

	update();

	setItemSelected(0, !(screen()->isSloppyFocus() || 
		screen()->isSemiSloppyFocus()));
	setItemSelected(1, screen()->isSloppyFocus());
	setItemSelected(2, screen()->isSemiSloppyFocus());
	setItemEnabled(3, (screen()->isSloppyFocus() ||
		screen()->isSemiSloppyFocus()));
	setItemSelected(3, screen()->doAutoRaise());
}


void Configmenu::Focusmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function()) {
			switch (item->function()) {
			case 1: // click to focus
				screen()->saveSloppyFocus(False);
				screen()->saveSemiSloppyFocus(False);
				screen()->saveAutoRaise(False);

				if (! Fluxbox::instance()->getFocusedWindow()) {
					XSetInputFocus(Fluxbox::instance()->getXDisplay(),
						screen()->getToolbar()->getWindowID(),
						RevertToParent, CurrentTime);
				} else {
					XSetInputFocus(Fluxbox::instance()->getXDisplay(),
			 			Fluxbox::instance()->getFocusedWindow()->getClientWindow(),
			 			RevertToParent, CurrentTime);
				}

				screen()->reconfigure();

			break;

			case 2: // sloppy focus
				screen()->saveSemiSloppyFocus(False);
				screen()->saveSloppyFocus(True);

				screen()->reconfigure();

			break;

			case 3: // semi sloppy focus
				screen()->saveSloppyFocus(False);
				screen()->saveSemiSloppyFocus(True);

				screen()->reconfigure();

			break;

			case 4: // auto raise with sloppy focus
				Bool change = ((screen()->doAutoRaise()) ? False : True);
				screen()->saveAutoRaise(change);

			break;
			}
			//save resources
			Fluxbox::instance()->save_rc();
			
			//toggle options
			setItemSelected(0, !(screen()->isSloppyFocus() || 
							screen()->isSemiSloppyFocus()));
			setItemSelected(1, screen()->isSloppyFocus());
			setItemSelected(2, screen()->isSemiSloppyFocus());
			setItemEnabled(3, (screen()->isSloppyFocus() ||
						screen()->isSemiSloppyFocus()));
			setItemSelected(3, screen()->doAutoRaise());
		}
	}
}


Configmenu::Placementmenu::Placementmenu(BScreen *scr) : Basemenu(scr) {

	I18n *i18n = I18n::instance();
	using namespace FBNLS;
	setLabel(i18n->getMessage(
		ConfigmenuSet, ConfigmenuWindowPlacement,
		"Window Placement"));
	setInternalMenu();

	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuSmartRows,
		"Smart Placement (Rows)"), BScreen::ROWSMARTPLACEMENT);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuSmartCols,
		"Smart Placement (Columns)"), BScreen::COLSMARTPLACEMENT);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuCascade,
		"Cascade Placement"), BScreen::CASCADEPLACEMENT);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuLeftRight,
		"Left to Right"), BScreen::LEFTRIGHT);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuRightLeft,
		"Right to Left"), BScreen::RIGHTLEFT);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuTopBottom,
		"Top to Bottom"), BScreen::TOPBOTTOM);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuBottomTop,
		"Bottom to Top"), BScreen::BOTTOMTOP);

	update();

	switch (screen()->getPlacementPolicy()) {
	case BScreen::ROWSMARTPLACEMENT:
		setItemSelected(0, true);
		break;

	case BScreen::COLSMARTPLACEMENT:
		setItemSelected(1, true);
		break;

	case BScreen::CASCADEPLACEMENT:
		setItemSelected(2, true);
		break;
	}

	bool rl = (screen()->getRowPlacementDirection() ==
				BScreen::LEFTRIGHT),
		tb = (screen()->getColPlacementDirection() ==
				BScreen::TOPBOTTOM);

	setItemSelected(3, rl);
	setItemSelected(4, ! rl);

	setItemSelected(5, tb);
	setItemSelected(6, ! tb);
}


void Configmenu::Placementmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function()) {
			switch (item->function()) {
			case BScreen::ROWSMARTPLACEMENT:
				screen()->savePlacementPolicy(item->function());

				setItemSelected(0, true);
				setItemSelected(1, false);
				setItemSelected(2, false);
				
				break;

			case BScreen::COLSMARTPLACEMENT:
				screen()->savePlacementPolicy(item->function());

				setItemSelected(0, false);
				setItemSelected(1, true);
				setItemSelected(2, false);

				break;

			case BScreen::CASCADEPLACEMENT:
				screen()->savePlacementPolicy(item->function());

				setItemSelected(0, false);
				setItemSelected(1, false);
				setItemSelected(2, true);

				break;

			case BScreen::LEFTRIGHT:
				screen()->saveRowPlacementDirection(BScreen::LEFTRIGHT);

				setItemSelected(3, true);
				setItemSelected(4, false);

				break;

			case BScreen::RIGHTLEFT:
				screen()->saveRowPlacementDirection(BScreen::RIGHTLEFT);

				setItemSelected(3, false);
				setItemSelected(4, true);

			break;

			case BScreen::TOPBOTTOM:
			screen()->saveColPlacementDirection(BScreen::TOPBOTTOM);

			setItemSelected(5, true);
			setItemSelected(6, false);

			break;

			case BScreen::BOTTOMTOP:
			screen()->saveColPlacementDirection(BScreen::BOTTOMTOP);

			setItemSelected(5, false);
			setItemSelected(6, true);

			break;
			}
			//save resources
			Fluxbox::instance()->save_rc();
		}
	}
}

Configmenu::Tabmenu::Tabmenu(BScreen *scr) : Basemenu(scr) {
	I18n *i18n = I18n::instance();
	using namespace FBNLS;

	setLabel(i18n->getMessage(
		ConfigmenuSet, ConfigmenuTabPlacement,
		"Tab Placement"));
	setInternalMenu();

	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopLeft,
		"Top Left"), Tab::PTOP + Tab::ALEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopCenter,
		"Top Center"), Tab::PTOP + Tab::ACENTER);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopRight,
		"Top Right"), Tab::PTOP + Tab::ARIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementTopRelative,
		"Top Relative"), Tab::PTOP + Tab::ARELATIVE);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomLeft,
		"Bottom Left"), Tab::PBOTTOM + Tab::ALEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomCenter,
		"Bottom Center"), Tab::PBOTTOM + Tab::ACENTER);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomRight,
		"Bottom Right"), Tab::PBOTTOM + Tab::ARIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementBottomRelative,
		"Bottom Relative"), Tab::PBOTTOM + Tab::ARELATIVE);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementLeftTop,
		"Left Top"), Tab::PLEFT + Tab::ARIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementLeftCenter,
		"Left Center"), Tab::PLEFT + Tab::ACENTER);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementLeftBottom,
		"Left Bottom"), Tab::PLEFT + Tab::ALEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementLeftRelative,
		"Left Relative"), Tab::PLEFT + Tab::ARELATIVE);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementRightTop,
		"Right Top"), Tab::PRIGHT + Tab::ARIGHT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementRightCenter,
		"Right Center"), Tab::PRIGHT + Tab::ACENTER);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementRightBottom,
		"Right Bottom"), Tab::PRIGHT + Tab::ALEFT);
	insert(i18n->getMessage(
		CommonSet, CommonPlacementRightRelative,
		"Right Relative"), Tab::PRIGHT + Tab::ARELATIVE);
	insert(i18n->getMessage(
		ConfigmenuSet, ConfigmenuTabRotateVertical,
		"Rotate Vertical Tabs"), CMENU_TAB_ROTATE);
	update();
	setSelected();
}


void Configmenu::Tabmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		#ifdef DEBUG
		fprintf(stderr, "Item(%d)\n", index);
		#endif
		BasemenuItem *item = find(index);
		
		switch (item->function()) {
			case (Tab::PTOP + Tab::ALEFT):
				screen()->saveTabPlacement(Tab::PTOP);
				screen()->saveTabAlignment(Tab::ALEFT);

				screen()->reconfigure();

				break;

			case (Tab::PTOP + Tab::ACENTER):
				screen()->saveTabPlacement(Tab::PTOP);
				screen()->saveTabAlignment(Tab::ACENTER);

				screen()->reconfigure();

				break;

			case (Tab::PTOP + Tab::ARIGHT):
				screen()->saveTabPlacement(Tab::PTOP);
				screen()->saveTabAlignment(Tab::ARIGHT);

				screen()->reconfigure();

				break;

			case (Tab::PTOP + Tab::ARELATIVE):
				screen()->saveTabPlacement(Tab::PTOP);
				screen()->saveTabAlignment(Tab::ARELATIVE);

				screen()->reconfigure();

				break;

			case (Tab::PBOTTOM + Tab::ALEFT):
				screen()->saveTabPlacement(Tab::PBOTTOM);
				screen()->saveTabAlignment(Tab::ALEFT);

				screen()->reconfigure();

				break;
			case (Tab::PBOTTOM + Tab::ACENTER):
				screen()->saveTabPlacement(Tab::PBOTTOM);
				screen()->saveTabAlignment(Tab::ACENTER);

				screen()->reconfigure();

				break;

			case (Tab::PBOTTOM + Tab::ARIGHT):
				screen()->saveTabPlacement(Tab::PBOTTOM);
				screen()->saveTabAlignment(Tab::ARIGHT);

				screen()->reconfigure();

				break;

			case (Tab::PBOTTOM + Tab::ARELATIVE):
				screen()->saveTabPlacement(Tab::PBOTTOM);
				screen()->saveTabAlignment(Tab::ARELATIVE);

				screen()->reconfigure();

				break;

			case (Tab::PLEFT + Tab::ARIGHT):
				screen()->saveTabPlacement(Tab::PLEFT);
				screen()->saveTabAlignment(Tab::ARIGHT);

				screen()->reconfigure();

				break;

			case (Tab::PLEFT + Tab::ACENTER):
				screen()->saveTabPlacement(Tab::PLEFT);
				screen()->saveTabAlignment(Tab::ACENTER);

				screen()->reconfigure();

				break;

			case (Tab::PLEFT + Tab::ALEFT):
				screen()->saveTabPlacement(Tab::PLEFT);
				screen()->saveTabAlignment(Tab::ALEFT);

				screen()->reconfigure();

				break;

			case (Tab::PLEFT + Tab::ARELATIVE):
				screen()->saveTabPlacement(Tab::PLEFT);
				screen()->saveTabAlignment(Tab::ARELATIVE);

				screen()->reconfigure();

				break;

			case (Tab::PRIGHT + Tab::ARIGHT):
				screen()->saveTabPlacement(Tab::PRIGHT);
				screen()->saveTabAlignment(Tab::ARIGHT);

				screen()->reconfigure();

				break;
			case (Tab::PRIGHT + Tab::ACENTER):
				screen()->saveTabPlacement(Tab::PRIGHT);
				screen()->saveTabAlignment(Tab::ACENTER);

				screen()->reconfigure();

				break;

			case (Tab::PRIGHT + Tab::ALEFT):
				screen()->saveTabPlacement(Tab::PRIGHT);
				screen()->saveTabAlignment(Tab::ALEFT);

				screen()->reconfigure();

				break;

			case (Tab::PRIGHT + Tab::ARELATIVE):
				screen()->saveTabPlacement(Tab::PRIGHT);
				screen()->saveTabAlignment(Tab::ARELATIVE);

				screen()->reconfigure();

				break;

			case CMENU_TAB_ROTATE:
				if (screen()->isTabRotateVertical())
					screen()->saveTabRotateVertical(False);
				else
					screen()->saveTabRotateVertical(True);
				break;
			}
			//save resources
			Fluxbox::instance()->save_rc();
			Fluxbox::instance()->reconfigureTabs(); //TODO
			setSelected();			
	}
}

void Configmenu::Tabmenu::setSelected() {
	setItemSelected(0, (screen()->getTabPlacement() == Tab::PTOP &&
		screen()->getTabAlignment() == Tab::ALEFT));
	setItemSelected(1, (screen()->getTabPlacement() == Tab::PTOP &&
		screen()->getTabAlignment() == Tab::ACENTER));
	setItemSelected(2, (screen()->getTabPlacement() == Tab::PTOP &&
		screen()->getTabAlignment() == Tab::ARIGHT));
	setItemSelected(3, (screen()->getTabPlacement() == Tab::PTOP &&
		screen()->getTabAlignment() == Tab::ARELATIVE));
	setItemSelected(4, (screen()->getTabPlacement() == Tab::PBOTTOM &&
		screen()->getTabAlignment() == Tab::ALEFT));
	setItemSelected(5, (screen()->getTabPlacement() == Tab::PBOTTOM &&
		screen()->getTabAlignment() == Tab::ACENTER));
	setItemSelected(6, (screen()->getTabPlacement() == Tab::PBOTTOM &&
		screen()->getTabAlignment() == Tab::ARIGHT));
	setItemSelected(7, (screen()->getTabPlacement() == Tab::PBOTTOM &&
		screen()->getTabAlignment() == Tab::ARELATIVE));
	setItemSelected(8, (screen()->getTabPlacement() == Tab::PLEFT &&
		screen()->getTabAlignment() == Tab::ARIGHT));
	setItemSelected(9, (screen()->getTabPlacement() == Tab::PLEFT &&
		screen()->getTabAlignment() == Tab::ACENTER));
	setItemSelected(10, (screen()->getTabPlacement() == Tab::PLEFT &&
		screen()->getTabAlignment() == Tab::ALEFT));
	setItemSelected(11, (screen()->getTabPlacement() == Tab::PLEFT &&
		screen()->getTabAlignment() == Tab::ARELATIVE));
	setItemSelected(12, (screen()->getTabPlacement() == Tab::PRIGHT &&
		screen()->getTabAlignment() == Tab::ARIGHT));
	setItemSelected(13, (screen()->getTabPlacement() == Tab::PRIGHT &&
		screen()->getTabAlignment() == Tab::ACENTER));
	setItemSelected(14, (screen()->getTabPlacement() == Tab::PRIGHT &&
		screen()->getTabAlignment() == Tab::ALEFT));
	setItemSelected(15, (screen()->getTabPlacement() == Tab::PRIGHT &&
		screen()->getTabAlignment() == Tab::ARELATIVE));
	setItemEnabled(16, (screen()->getTabPlacement() == Tab::PLEFT ||
		screen()->getTabPlacement() == Tab::PRIGHT));
	setItemSelected(16, (screen()->isTabRotateVertical() &&
		(screen()->getTabPlacement() == Tab::PLEFT ||
		screen()->getTabPlacement() == Tab::PRIGHT))); 
}
