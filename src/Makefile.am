# Makefile.am for fluxbox
# Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
#
# src/Makefile.am for Blackbox 0.61.x  - an X11 Window manager
# Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software. 
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.


SUBDIRS= FbTk
DEFAULT_MENU=@DEFAULT_MENU@
DEFAULT_STYLE=$(pkgdatadir)/styles/Clean
DEFAULT_KEYSFILE=@DEFAULT_KEYS@
DEFAULT_INITFILE=@DEFAULT_INIT@
PROGRAM_PREFIX=@program_prefix@
PROGRAM_SUFFIX=@program_suffix@

AM_CPPFLAGS=-I$(srcdir)/FbTk

bin_PROGRAMS= fluxbox
BUILT_SOURCES = defaults.hh
CONFIG_CLEAN_FILES = defaults.hh
defaults.hh: Makefile
	@( \
		echo '// This file is generated from Makefile. Do not edit!'; \
		echo '#include <string>'; \
		echo ''; \
		echo '#define DEFAULTMENU "$(DEFAULT_MENU)"'; \
		echo '#define DEFAULTSTYLE "$(DEFAULT_STYLE)"'; \
		echo '#define DEFAULTKEYSFILE "$(DEFAULT_KEYSFILE)"'; \
		echo '#define DEFAULT_INITFILE "$(DEFAULT_INITFILE)"'; \
		echo '#define PROGRAM_PREFIX "$(PROGRAM_PREFIX:NONE=)"'; \
		echo '#define PROGRAM_SUFFIX "$(PROGRAM_SUFFIX:NONE=)"'; \
		echo 'std::string realProgramName(std::string name);'; \
		echo 'const char* svnversion(void);' ) > defaults.hh

defaults.cc: force
	@( \
		echo '#include "defaults.hh"'; \
		echo ''; \
		echo 'std::string realProgramName(std::string name) {'; \
		echo '  return PROGRAM_PREFIX + name + PROGRAM_SUFFIX;'; \
		echo '}'; \
		echo ''; \
		echo 'const char* svnversion(void) {'; \
		echo '  return "'`(svnversion . | sed "s/[^0-9].*//") 2> /dev/null`'";';\
		echo '}' ) > defaults_tmp.cc
	@if ! cmp defaults_tmp.cc defaults.cc ; then cp defaults_tmp.cc defaults.cc; fi
	@rm defaults_tmp.cc

force: ;

# an extra dep for fluxbox
fluxbox.$(OBJEXT): defaults.hh



if NEWWMSPEC
newwmspec_SOURCE= Ewmh.hh Ewmh.cc
endif
if GNOME
gnome_SOURCE= Gnome.hh Gnome.cc
endif
if REMEMBER_SRC
# For now we only want regexp if we have remember
REMEMBER_SOURCE= Remember.hh Remember.cc \
	RegExp.hh RegExp.cc ClientPattern.hh ClientPattern.cc
endif
if TOOLBAR_SRC
TOOLBAR_SOURCE = Toolbar.hh Toolbar.cc \
	ToolbarTheme.hh ToolbarTheme.cc ToolbarItem.hh ToolbarItem.cc \
	ClockTool.hh ClockTool.cc \
	WorkspaceNameTool.hh WorkspaceNameTool.cc WorkspaceNameTheme.hh \
	IconbarTool.hh IconbarTool.cc \
	ToolTheme.hh ToolTheme.cc \
	SystemTray.hh SystemTray.cc \
	GenericTool.hh GenericTool.cc \
	ButtonTool.hh ButtonTool.cc ButtonTheme.hh ButtonTheme.cc \
	ToolFactory.hh ToolFactory.cc
endif

fluxbox_SOURCES = AtomHandler.hh ArrowButton.hh ArrowButton.cc \
	BoolMenuItem.hh \
	FbAtoms.hh FbAtoms.cc FbWinFrame.hh FbWinFrame.cc \
	FbWinFrameTheme.hh FbWinFrameTheme.cc \
	fluxbox.cc fluxbox.hh \
	Keys.cc Keys.hh main.cc \
	RootTheme.hh RootTheme.cc \
	FbRootWindow.hh FbRootWindow.cc \
	Screen.cc Screen.hh ScreenResources.cc \
	Slit.cc Slit.hh SlitTheme.hh SlitTheme.cc SlitClient.hh SlitClient.cc \
	WinButton.hh WinButton.cc \
	WinButtonTheme.hh WinButtonTheme.cc \
	Window.cc Window.hh \
	Workspace.cc Workspace.hh \
	FbCommands.hh FbCommands.cc LayerMenu.hh LayerMenu.cc \
	Layer.hh \
	IntResMenuItem.hh FbMenu.hh FbMenu.cc \
	WinClient.hh WinClient.cc \
	Strut.hh \
	Xinerama.hh \
	Xutil.hh Xutil.cc \
	CurrentWindowCmd.hh CurrentWindowCmd.cc \
	WorkspaceCmd.hh WorkspaceCmd.cc \
	CommandParser.hh CommandParser.cc \
	FbCommandFactory.hh FbCommandFactory.cc \
	Shape.hh Shape.cc \
	MenuTheme.hh MenuTheme.cc \
	Container.hh Container.cc \
	TextTheme.hh TextTheme.cc \
	BorderTheme.hh BorderTheme.cc \
	CommandDialog.hh CommandDialog.cc SendToMenu.hh SendToMenu.cc \
	AlphaMenu.hh AlphaMenu.cc ObjectResource.hh \
	CompareWindow.hh \
	Parser.hh Parser.cc FbMenuParser.hh FbMenuParser.cc \
	StyleMenuItem.hh StyleMenuItem.cc \
	RootCmdMenuItem.hh RootCmdMenuItem.cc\
	MenuCreator.hh MenuCreator.cc \
	ClientMenu.hh ClientMenu.cc \
	WorkspaceMenu.hh WorkspaceMenu.cc \
	FocusModelMenuItem.hh \
	ToggleMenu.hh \
	HeadArea.hh HeadArea.cc \
	Resources.cc \
	WindowCmd.hh WindowCmd.cc \
	FocusControl.hh FocusControl.cc \
	PlacementStrategy.hh \
	CascadePlacement.hh CascadePlacement.cc \
	ColSmartPlacement.hh ColSmartPlacement.cc \
	MinOverlapPlacement.hh MinOverlapPlacement.cc \
	RowSmartPlacement.hh RowSmartPlacement.cc \
	ScreenPlacement.hh ScreenPlacement.cc \
	UnderMousePlacement.hh UnderMousePlacement.cc \
	AttentionNoticeHandler.hh AttentionNoticeHandler.cc \
	IconButton.hh IconButton.cc \
	IconbarTheme.hh IconbarTheme.cc \
	STLUtil.hh \
	Focusable.hh FocusableList.hh FocusableList.cc \
	${newwmspec_SOURCE} ${gnome_SOURCE} \
	${REMEMBER_SOURCE} ${TOOLBAR_SOURCE}


LDADD=FbTk/libFbTk.a defaults.$(OBJEXT)
