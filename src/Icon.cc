// Icon.cc for Blackbox - an X11 Window manager
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

//Use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "Icon.hh"
#include "Screen.hh"
#include "Window.hh"


Iconmenu::Iconmenu(BScreen *scrn) : Basemenu(scrn) {
	setInternalMenu();

	screen = scrn;	
	setLabel(I18n::instance()->getMessage(
#ifdef		NLS
					IconSet, IconIcons,
#else // !NLS
					0, 0,
#endif // NLS
					"Icons"));
	update();
}


void Iconmenu::itemSelected(int button, unsigned int index) {
	if (button == 1) {
		if (index < screen->getIconCount()) {
			FluxboxWindow *win = screen->getIcon(index);

			if (win)
				win->deiconify();

		}

		if (! (screen->getWorkspacemenu()->isTorn() || isTorn()))
			hide();
	}
}
