// Rootmenu.cc for Blackbox - an X11 Window manager
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

//Use GNU extensions
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "fluxbox.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN


Rootmenu::Rootmenu(BScreen *scrn) : Basemenu(scrn) {
  screen = scrn;  
}


void Rootmenu::itemSelected(int button, unsigned int index) {
	
	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (button == 1) {
		BasemenuItem *item = find(index);

		if (item->function()) {
			switch (item->function()) {
			case BScreen::EXECUTE:
				if (item->exec().size()) {
					#ifndef    __EMX__
					char displaystring[MAXPATHLEN];
					sprintf(displaystring, "DISPLAY=%s",
							DisplayString(screen->getBaseDisplay()->getXDisplay()));
					sprintf(displaystring + strlen(displaystring) - 1, "%d",
						screen->getScreenNumber());

					bexec(item->exec().c_str(), displaystring);
					#else //   __EMX__
					spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec().c_str(), NULL);
					#endif // !__EMX__
				}
				break;

			case BScreen::RESTART:
				fluxbox->restart();
				break;

			case BScreen::RESTARTOTHER:
				if (item->exec().size())
					fluxbox->restart(item->exec().c_str());
				break;

			case BScreen::EXIT:
				fluxbox->shutdown();
			break;

			case BScreen::SETSTYLE:
				if (item->exec().size()) {
					fluxbox->saveStyleFilename(item->exec().c_str());
					fluxbox->reconfigureTabs();
				}
				fluxbox->reconfigure();
				fluxbox->save_rc();
			break;
			case BScreen::RECONFIGURE:
				fluxbox->reconfigure();
				return;
			}
			if (! (screen->getRootmenu()->isTorn() || isTorn()) &&
					item->function() != BScreen::RECONFIGURE &&
					item->function() != BScreen::SETSTYLE)
				hide();
		}
	}
}

