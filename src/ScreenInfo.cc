// ScreenInfo.cc for fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen<at>users.sourceforge.net)
//
// from BaseDisplay.cc in Blackbox 0.61.1
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: ScreenInfo.cc,v 1.1 2003/05/10 13:54:29 fluxgen Exp $

#include "ScreenInfo.hh"

#include "App.hh"

#include <X11/Xutil.h>

ScreenInfo::ScreenInfo(int num) {
    
    Display * const disp = FbTk::App::instance()->display();
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

/**
   Searches for the head at the coordinates
   x,y. If it fails or Xinerama isn't
   activated it'll return head nr 0
*/
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

/**
   Searches for the head that the pointer
   currently is on, if it isn't found
   the first one is returned
*/
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

/**
 @return the width of head
*/
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

/**
 @return the heigt of head
*/
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


/**
 @return the X start of head nr head
*/
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

/**
 @return the Y start of head
*/
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
