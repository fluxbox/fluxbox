// Netizen.cc for Blackbox - An X11 Window Manager
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
#include "../config.h"
#endif // HAVE_CONFIG_H

#include "Netizen.hh"


Netizen::Netizen(BScreen *scr, Window win) {
  screen = scr;
  basedisplay = screen->getBaseDisplay();
  window = win;

  event.type = ClientMessage;
  event.xclient.message_type = basedisplay->getFluxboxStructureMessagesAtom();
  event.xclient.display = basedisplay->getXDisplay();
  event.xclient.window = window;
  event.xclient.format = 32;
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyStartupAtom();
  event.xclient.data.l[1] = event.xclient.data.l[2] =
    event.xclient.data.l[3] = event.xclient.data.l[4] = 0l;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
}


void Netizen::sendWorkspaceCount(void) {
 
  #ifdef GNOME
/*	long val = screen->getCount();
	XChangeProperty(basedisplay->getXDisplay(), screen->getRootWindow(),
			basedisplay->getGnomeWorkspaceCountAtom(), XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&val, 1);*/
	printf("UPDATE!\n");
	unsigned long data=(unsigned long) screen->getCount();
  Atom                atom_set;
  // CARD32              val;
  
  atom_set = XInternAtom(basedisplay->getXDisplay(), "_WIN_WORKSPACE_COUNT", False);
  // val = mode.numdesktops;
  XChangeProperty(basedisplay->getXDisplay(), window, atom_set, 
		 XA_CARDINAL, 32, PropModeReplace,(unsigned char *)&data, 1);
	#endif //GNOME
	
	event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWorkspaceCountAtom();
  event.xclient.data.l[1] = screen->getCount();

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
	
	
}


void Netizen::sendCurrentWorkspace(void) {
	#ifdef GNOME
	//update atom	to workspace
	long val;
  val = screen->getCurrentWorkspaceID();
  XChangeProperty(basedisplay->getXDisplay(), screen->getRootWindow(), 
				basedisplay->getGnomeWorkspaceAtom(), XA_CARDINAL, 32,
         PropModeReplace, (unsigned char *)&val, 1);

	#endif

	event.xclient.data.l[0] = basedisplay->getFluxboxNotifyCurrentWorkspaceAtom();
	event.xclient.data.l[1] = screen->getCurrentWorkspaceID();

	XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);

}


void Netizen::sendWindowFocus(Window w) {
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWindowFocusAtom();
  event.xclient.data.l[1] = w;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
}


void Netizen::sendWindowAdd(Window w, unsigned long p) {
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWindowAddAtom();
  event.xclient.data.l[1] = w;
  event.xclient.data.l[2] = p;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);

  event.xclient.data.l[2] = 0l;
}


void Netizen::sendWindowDel(Window w) {
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWindowDelAtom();
  event.xclient.data.l[1] = w;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
}


void Netizen::sendWindowRaise(Window w) {
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWindowRaiseAtom();
  event.xclient.data.l[1] = w;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
}


void Netizen::sendWindowLower(Window w) {
  event.xclient.data.l[0] = basedisplay->getFluxboxNotifyWindowLowerAtom();
  event.xclient.data.l[1] = w;

  XSendEvent(basedisplay->getXDisplay(), window, False, NoEventMask, &event);
}


void Netizen::sendConfigNotify(XEvent *e) {
  XSendEvent(basedisplay->getXDisplay(), window, False,
             StructureNotifyMask, e);
}
