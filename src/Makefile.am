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

AM_CPPFLAGS= -Wall @CPPFLAGS@ \
-DLOCALEPATH=\"$(pkgdatadir)/nls\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-DDEFAULTKEYSFILE=\"$(DEFAULT_KEYSFILE)\"\
-DDEFAULT_INITFILE=\"$(pkgdatadir)/init\"\
-IFbTk

bin_PROGRAMS= fluxbox


if NEWWMSPEC
newwmspec_SOURCE= Ewmh.hh Ewmh.cc
endif
if GNOME
gnome_SOURCE= Gnome.hh Gnome.cc
endif
if REMEMBER_SRC
REMEMBER_SOURCE= Remember.hh Remember.cc
endif

fluxbox_SOURCES = AtomHandler.hh ArrowButton.hh ArrowButton.cc \
	BoolMenuItem.hh \
	${newwmspec_SOURCE} \
	FbAtoms.hh FbAtoms.cc FbWinFrame.hh FbWinFrame.cc \
	FbWinFrameTheme.hh FbWinFrameTheme.cc \
	fluxbox.cc fluxbox.hh ${gnome_SOURCE} \
	IconBar.cc IconBar.hh \
	i18n.cc i18n.hh \
	Keys.cc Keys.hh main.cc \
	Netizen.cc Netizen.hh \
	RootTheme.hh RootTheme.cc \
	FbRootWindow.hh FbRootWindow.cc \
	Screen.cc Screen.hh \
	Slit.cc Slit.hh \
	TextButton.hh TextButton.cc \
	Toolbar.cc Toolbar.hh \
	ToolbarHandler.cc ToolbarHandler.hh \
	ToolbarTheme.hh ToolbarTheme.cc \
	WinButton.hh WinButton.cc \
	WinButtonTheme.hh WinButtonTheme.cc \
	Window.cc Window.hh \
	Workspace.cc Workspace.hh \
	FbCommands.hh FbCommands.cc LayerMenu.hh \
	IntResMenuItem.hh IntResMenuItem.cc FbMenu.hh \
	WinClient.hh WinClient.cc \
	Xinerama.hh \
	${REMEMBER_SOURCE}


LDADD=FbTk/libFbTk.a
