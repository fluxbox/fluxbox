# Makefile.am for fluxbox
# Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
#
# src/Makefile.am for Blackbox 0.61.x  - an X11 Window manager
# Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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


DEFAULT_MENU=$(pkgdatadir)/menu
DEFAULT_STYLE=$(pkgdatadir)/styles/Clean
DEFAULT_TITLEBAR=$(pkgdatadir)/titlebar
DEFAULT_KEYSFILE=$(pkgdatadir)/keys
DEFAULT_INITFILE=

CPPFLAGS= -Wall @CPPFLAGS@ \
-DLOCALEPATH=\"$(pkgdatadir)/nls\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-DDEFAULTTITLEBAR=\"$(DEFAULT_TITLEBAR)\"\
-DDEFAULTKEYSFILE=\"$(DEFAULT_KEYSFILE)\"\
-DDEFAULT_INITFILE=\"$(pkgdatadir)/init\"

bin_PROGRAMS= fluxbox

fluxbox_SOURCES= BaseDisplay.cc BaseDisplay.hh Basemenu.cc Basemenu.hh \
	Clientmenu.cc Clientmenu.hh Configmenu.cc Configmenu.hh \
	Icon.cc Icon.hh Image.cc Image.hh  \
	Netizen.cc Netizen.hh Rootmenu.cc Rootmenu.hh Screen.cc Screen.hh\
	Slit.cc Slit.hh Timer.cc Timer.hh Toolbar.cc Toolbar.hh \
	Window.cc Window.hh Windowmenu.cc Windowmenu.hh \
	Workspace.cc Workspace.hh Workspacemenu.cc Workspacemenu.hh \
	fluxbox.cc fluxbox.hh \
	i18n.cc i18n.hh main.cc	Tab.hh Tab.cc Keys.cc Keys.hh\
	IconBar.cc IconBar.hh Theme.hh Theme.cc \
	StringUtil.cc StringUtil.hh DrawUtil.cc DrawUtil.hh nl_types_cygnus.h \
	XrmDatabaseHelper.hh NotCopyable.hh Resource.hh Resource.cc Font.hh Font.cc \
	FbAtoms.hh FbAtoms.cc
MAINTAINERCLEANFILES= Makefile.in

