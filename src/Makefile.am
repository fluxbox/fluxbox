# Makefile.am for fluxbox
# Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

fluxbox_SOURCES = AtomHandler.hh ArrowButton.hh ArrowButton.cc BaseDisplay.cc BaseDisplay.hh \
	Basemenu.cc Basemenu.hh \
	Clientmenu.cc Clientmenu.hh Configmenu.cc Configmenu.hh \
	DirHelper.hh DirHelper.cc DrawUtil.cc DrawUtil.hh ${newwmspec_SOURCE} \
	FbAtoms.hh FbAtoms.cc FbWinFrame.hh FbWinFrame.cc FbWinFrameTheme.hh FbWinFrameTheme.cc \
	fluxbox.cc fluxbox.hh ${gnome_SOURCE} \
	IconBar.cc IconBar.hh Iconmenu.cc Iconmenu.hh \
	ImageControl.cc ImageControl.hh i18n.cc i18n.hh \
	Keys.cc Keys.hh main.cc \
	Netizen.cc Netizen.hh nl_types_cygnus.h \
	Rootmenu.cc Rootmenu.hh Resource.hh Resource.cc \
	Screen.cc Screen.hh \
	Slit.cc Slit.hh StringUtil.cc StringUtil.hh Subject.cc Subject.hh \
	Tab.hh Tab.cc TextureRender.hh TextureRender.cc Theme.hh Theme.cc \
	Timer.cc Timer.hh Toolbar.cc Toolbar.hh \
	ToolbarTheme.hh ToolbarTheme.cc Observer.cc Observer.hh \
	WinButton.hh WinButton.cc Window.cc Window.hh Windowmenu.cc Windowmenu.hh \
	Workspace.cc Workspace.hh Workspacemenu.cc Workspacemenu.hh \
	XrmDatabaseHelper.hh

LDADD=FbTk/libFbTk.a
