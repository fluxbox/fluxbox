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
DEFAULT_MENU=$(pkgdatadir)/menu
DEFAULT_STYLE=$(pkgdatadir)/styles/Clean
DEFAULT_KEYSFILE=$(pkgdatadir)/keys
DEFAULT_INITFILE=

AM_CPPFLAGS=@CPPFLAGS@ -I$(srcdir)/FbTk

bin_PROGRAMS= fluxbox
BUILT_SOURCES = defaults.hh
defaults.hh: Makefile
	echo '// This file is generated from makefile. Do not edit!' > defaults.hh
	echo '#define DEFAULTMENU "$(DEFAULT_MENU)"' >> defaults.hh
	echo '#define DEFAULTSTYLE "$(DEFAULT_STYLE)"' >> defaults.hh
	echo '#define DEFAULTKEYSFILE "$(DEFAULT_KEYSFILE)"' >> defaults.hh
	echo '#define DEFAULT_INITFILE "$(pkgdatadir)/init"' >> defaults.hh
	echo '#define LOCALEPATH "$(pkgdatadir)/nls"' >> defaults.hh

# some extra deps for I18n and fluxbox

I18n.$(OBJEXT): defaults.hh
fluxbox.$(OBJEXT): defaults.hh

if NEWWMSPEC
newwmspec_SOURCE= Ewmh.hh Ewmh.cc
endif
if GNOME
gnome_SOURCE= Gnome.hh Gnome.cc
endif
if REMEMBER_SRC
REMEMBER_SOURCE= Remember.hh Remember.cc
# For now we only want regexp if we have remember
if REGEXP_SRC
REGEXP_SOURCE = RegExp.hh RegExp.cc ClientPattern.hh ClientPattern.cc
endif
endif
if TOOLBAR_SRC
TOOLBAR_SOURCE = Toolbar.hh Toolbar.cc \
	ToolbarTheme.hh ToolbarTheme.cc ToolbarItem.hh ToolbarItem.cc \
	ClockTool.hh ClockTool.cc \
	WorkspaceNameTool.hh WorkspaceNameTool.cc WorkspaceNameTheme.hh \
	IconbarTool.hh IconbarTool.cc IconbarTheme.hh IconbarTheme.cc \
	ToolTheme.hh ToolTheme.cc \
	IconButton.hh IconButton.cc \
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
	I18n.cc I18n.hh \
	Keys.cc Keys.hh main.cc \
	Netizen.cc Netizen.hh \
	RootTheme.hh RootTheme.cc \
	FbRootWindow.hh FbRootWindow.cc \
	Screen.cc Screen.hh ScreenResources.cc \
	Slit.cc Slit.hh SlitTheme.hh SlitTheme.cc SlitClient.hh SlitClient.cc \
	WinButton.hh WinButton.cc \
	WinButtonTheme.hh WinButtonTheme.cc \
	Window.cc Window.hh \
	Workspace.cc Workspace.hh \
	FbCommands.hh FbCommands.cc LayerMenu.hh \
	IntResMenuItem.hh IntResMenuItem.cc FbMenu.hh FbMenu.cc \
	WinClient.hh WinClient.cc \
	Strut.hh \
	Xinerama.hh \
	IconMenuItem.hh \
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
	CompareWindow.hh \
	${newwmspec_SOURCE} ${gnome_SOURCE} \
	${REMEMBER_SOURCE} ${REGEXP_SOURCE} ${TOOLBAR_SOURCE}


LDADD=FbTk/libFbTk.a
