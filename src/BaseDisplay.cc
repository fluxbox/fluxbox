// BaseDisplay.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// BaseDisplay.cc for Blackbox - an X11 Window manager
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

// $Id: BaseDisplay.cc,v 1.25 2002/12/02 20:02:56 fluxgen Exp $



#include "BaseDisplay.hh"
#include "i18n.hh"
#include "Timer.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include <X11/Xutil.h>

#ifdef	SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif // HAVE_FCNTL_H

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef	HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef	HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef	HAVE_SIGNAL_H
#include <signal.h>
#endif // HAVE_SIGNAL_H

#ifndef	 SA_NODEFER
#ifdef	 SA_INTERRUPT
#define SA_NODEFER SA_INTERRUPT
#else // !SA_INTERRUPT
#define SA_NODEFER (0)
#endif // SA_INTERRUPT
#endif // SA_NODEFER

#ifdef		HAVE_SYS_WAIT_H
#include <sys/types.h>
#include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#if defined(HAVE_PROCESS_H) && defined(__EMX__)
#include <process.h>
#endif // HAVE_PROCESS_H	 && __EMX__

#include <iostream>
using namespace std;

// X error handler to handle any and all X errors while the application is
// running
static Bool internal_error = False;
static Window last_bad_window = None;

#ifdef DEBUG
static int handleXErrors(Display *d, XErrorEvent *e) {
    char errtxt[128];
	
    XGetErrorText(d, e->error_code, errtxt, 128);
    fprintf(stderr,
            I18n::instance()->
            getMessage(
                FBNLS::BaseDisplaySet, FBNLS::BaseDisplayXError,
                "%s:	X error: %s(%d) opcodes %d/%d\n	resource 0x%lx\n"),
            BaseDisplay::instance()->getApplicationName(), errtxt, e->error_code,
            e->request_code, e->minor_code, e->resourceid);

#else // !DEBUG
    static int handleXErrors(Display *, XErrorEvent *e) {
#endif // DEBUG

	if (e->error_code == BadWindow)
            last_bad_window = e->resourceid;

	if (internal_error)
            abort();

	return(False);
    }

// convenience functions
#ifndef		__EMX__
    void bexec(const char *command, char *displaystring) {
	if (! fork()) {
            setsid();
            putenv(displaystring);
            execl("/bin/sh", "/bin/sh", "-c", command, 0);
            exit(0);
	}
    }
#endif // !__EMX__


    BaseDisplay *BaseDisplay::s_singleton = 0;

    BaseDisplay::BaseDisplay(const char *app_name, const char *dpy_name):FbTk::App(dpy_name),
        m_startup(true), m_shutdown(false), 
        m_display_name(XDisplayName(dpy_name)), m_app_name(app_name),
        m_server_grabs(0)
        {
            if (s_singleton != 0)
		throw string("Can't create more than one instance of BaseDisplay!");
	
            s_singleton = this;
	
            last_bad_window = None;
            I18n *i18n = I18n::instance();

            if (display() == 0) {
		throw string(
			i18n->
			getMessage(
                            FBNLS::BaseDisplaySet, FBNLS::BaseDisplayXConnectFail,
                            "BaseDisplay::BaseDisplay: connection to X server failed."));
	
            } else if (fcntl(ConnectionNumber(display()), F_SETFD, 1) == -1) {
		throw string(
			i18n->
                        getMessage(
                            FBNLS::BaseDisplaySet, FBNLS::BaseDisplayCloseOnExecFail,
                            "BaseDisplay::BaseDisplay: couldn't mark display connection "
                            "as close-on-exec"));
		
            }
	

            number_of_screens = ScreenCount(display());

#ifdef		SHAPE
            shape.extensions = XShapeQueryExtension(display(), &shape.event_basep,
                                                    &shape.error_basep);
#else // !SHAPE
            shape.extensions = False;
#endif // SHAPE

	
            XSetErrorHandler((XErrorHandler) handleXErrors);

            for (int i = 0; i < number_of_screens; i++) {
		ScreenInfo *screeninfo = new ScreenInfo(i);
		screenInfoList.push_back(screeninfo);
            }
        }


    BaseDisplay::~BaseDisplay() {
	
	ScreenInfoList::iterator it = screenInfoList.begin();
	ScreenInfoList::iterator it_end = screenInfoList.end();
	for (; it != it_end; ++it) {
            delete (*it);
	}
		
	s_singleton = 0;
    }

    BaseDisplay *BaseDisplay::instance() {
	if (s_singleton == 0)
            throw string("BaseDisplay not created!");
	
	return s_singleton;
    }

    void BaseDisplay::eventLoop() {
	run();

	while ((! m_shutdown) && (! internal_error)) {
            if (XPending(display())) {
                XEvent e;
                XNextEvent(display(), &e);

                if (last_bad_window != None && e.xany.window == last_bad_window) {
#ifdef DEBUG
                    fprintf(stderr,
                            I18n::instance()->
                            getMessage(
                                FBNLS::BaseDisplaySet, FBNLS::BaseDisplayBadWindowRemove,
                                "BaseDisplay::eventLoop(): removing bad window "
                                "from event queue\n"));
#endif // DEBUG
                } else {
                    last_bad_window = None;
                    handleEvent(&e);
                }
            } else {
                BTimer::updateTimers(ConnectionNumber(display())); //handle all timers
            }
	}
    }


    bool BaseDisplay::validateWindow(Window window) {
	XEvent event;
	if (XCheckTypedWindowEvent(display(), window, DestroyNotify, &event)) {
            XPutBackEvent(display(), &event);
            return false;
	}

	return true;
    }


    void BaseDisplay::grab() {
	if (! m_server_grabs++)
            XGrabServer(display());
    }


    void BaseDisplay::ungrab() {
	if (! --m_server_grabs)
            XUngrabServer(display());
	if (m_server_grabs < 0)
            m_server_grabs = 0;
    }

    ScreenInfo::ScreenInfo(int num) {
	basedisplay = BaseDisplay::instance();
	Display * const disp = basedisplay->getXDisplay();
	screen_number = num;

	root_window = RootWindow(disp, screen_number);
	depth = DefaultDepth(disp, screen_number);

	width =
            WidthOfScreen(ScreenOfDisplay(disp, screen_number));
	height =
            HeightOfScreen(ScreenOfDisplay(disp, screen_number));

	// search for a TrueColor Visual... if we can't find one... we will use the
	// default visual for the screen
	XVisualInfo vinfo_template, *vinfo_return;
	int vinfo_nitems;

	vinfo_template.screen = screen_number;
	vinfo_template.c_class = TrueColor;

	visual = (Visual *) 0;

	if ((vinfo_return = XGetVisualInfo(disp,
                                           VisualScreenMask | VisualClassMask,
                                           &vinfo_template, &vinfo_nitems)) &&
            vinfo_nitems > 0) {
			
            for (int i = 0; i < vinfo_nitems; i++) {
                if (depth < (vinfo_return + i)->depth) {
                    depth = (vinfo_return + i)->depth;
                    visual = (vinfo_return + i)->visual;
                }
            }

            XFree(vinfo_return);
	}

	if (visual) {
            m_colormap = XCreateColormap(disp, root_window,
                                         visual, AllocNone);
	} else {
            visual = DefaultVisual(disp, screen_number);
            m_colormap = DefaultColormap(disp, screen_number);
	}

#ifdef XINERAMA
	// check if we have Xinerama extension enabled
	if (XineramaIsActive(disp)) {
            m_hasXinerama = true;
            xineramaLastHead = 0;
            xineramaInfos =
                XineramaQueryScreens(disp, &xineramaNumHeads);
	} else {
            m_hasXinerama = false;
            xineramaInfos = 0; // make sure we don't point anywhere we shouldn't
	}
#endif // XINERAMA 
    }

    ScreenInfo::~ScreenInfo() {
#ifdef XINERAMA
	if (m_hasXinerama) { // only free if we first had it
            XFree(xineramaInfos);
            xineramaInfos = 0;
	}
#endif // XINERAMA
    }

#ifdef XINERAMA

//---------------- getHead ---------------
// Searches for the head at the coordinates
// x,y. If it fails or Xinerama isn't
// activated it'll return head nr 0
//-----------------------------------------
    unsigned int ScreenInfo::getHead(int x, int y) const {

	// is Xinerama extensions enabled?
	if (hasXinerama()) {
            // check if last head is still active
/*		if ((xineramaInfos[xineramaLastHead].x_org <= x) &&
                ((xineramaInfos[xineramaLastHead].x_org +
                xineramaInfos[xineramaLastHead].width) > x) &&
                (xineramaInfos[xineramaLastHead].y_org <= y) &&
                ((xineramaInfos[xineramaLastHead].y_org +
                xineramaInfos[xineramaLastHead].height) > y)) {
                return xineramaLastHead;
		} else { */
            // go trough all the heads, and search
            for (int i = 0; (signed) i < xineramaNumHeads; i++) {
                if (xineramaInfos[i].x_org <= x &&
                    (xineramaInfos[i].x_org + xineramaInfos[i].width) > x &&
                    xineramaInfos[i].y_org <= y &&
                    (xineramaInfos[i].y_org + xineramaInfos[i].height) > y)
                    // return (xineramaLastHead = i);
                    return i;
            }
            //	}
	}

	return 0;
    }

//------------- getCurrHead --------------
// Searches for the head that the pointer
// currently is on, if it isn't found
// the first one is returned
//----------------------------------------
    unsigned int ScreenInfo::getCurrHead(void) const {

	// is Xinerama extensions enabled?
	if (hasXinerama()) {
            int x, y, wX, wY;
            unsigned int mask;
            Window rRoot, rChild;
            // get pointer cordinates
            if ( (XQueryPointer(basedisplay->getXDisplay(), root_window,
				&rRoot, &rChild, &x, &y, &wX, &wY, &mask)) != 0 )  {			
                return getHead(x, y);
            }
	}

	return 0;
    }

//----------- getHeadWidth ------------
// Returns the width of head
//-------------------------------------
    unsigned int ScreenInfo::getHeadWidth(unsigned int head) const {

	if (hasXinerama()) {
            if ((signed) head >= xineramaNumHeads) {
#ifdef DEBUG
                cerr << __FILE__ << ":" <<__LINE__ << ": " <<
                    "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
                return xineramaInfos[xineramaNumHeads - 1].width;
            } else
                return xineramaInfos[head].width;
	}
	
	return getWidth();

    }

//----------- getHeadHeight ------------
// Returns the heigt of head
//--------------------------------------
    unsigned int ScreenInfo::getHeadHeight(unsigned int head) const {

	if (hasXinerama()) {
            if ((signed) head >= xineramaNumHeads) {
#ifdef DEBUG
                cerr << __FILE__ << ":" <<__LINE__ << ": " <<
                    "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
                return xineramaInfos[xineramaNumHeads - 1].height;
            } else
                return xineramaInfos[head].height;
	}

	return getHeight();
    }


//----------- getHeadX -----------------
// Returns the X start of head nr head
//--------------------------------------
    int ScreenInfo::getHeadX(unsigned int head) const {
	if (hasXinerama()) {
            if ((signed) head >= xineramaNumHeads) {
#ifdef DEBUG
                cerr << __FILE__ << ":" <<__LINE__ << ": " <<
                    "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
                return xineramaInfos[head = xineramaNumHeads - 1].x_org;
            } else 
                return xineramaInfos[head].x_org;
	}

	return 0;
    }

//----------- getHeadY -----------------
// Returns the Y start of head
//--------------------------------------
    int ScreenInfo::getHeadY(unsigned int head) const {
	if (hasXinerama()) {
            if ((signed) head >= xineramaNumHeads) {
#ifdef DEBUG
                cerr << __FILE__ << ":" <<__LINE__ << ": " <<
                    "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
                return xineramaInfos[xineramaNumHeads - 1].y_org;
            } else 
                return xineramaInfos[head].y_org;
	}

	return 0;
    }

#endif // XINERAMA
