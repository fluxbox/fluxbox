// ScreenInfo.hh for fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen<at>users.sourceforge.net)
//
// from BaseDisplay.hh in Blackbox 0.61.1
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

// $Id: ScreenInfo.hh,v 1.2 2003/05/10 23:01:00 fluxgen Exp $

#ifndef SCREENINFO_HH
#define SCREENINFO_HH

#include <X11/Xlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef XINERAMA
extern	"C" {
#include <X11/extensions/Xinerama.h>
}
#endif // XINERAMA

/// holds information about a screen
class ScreenInfo {
public:
    explicit ScreenInfo(int screen_num);
    ~ScreenInfo();

    inline int getScreenNumber() const { return screen_number; }

#ifdef XINERAMA
    inline bool hasXinerama() const { return m_hasXinerama; }
    inline int getNumHeads() const { return xineramaNumHeads; }
    unsigned int getHead(int x, int y) const;
    unsigned int getCurrHead() const;
    unsigned int getHeadWidth(unsigned int head) const;
    unsigned int getHeadHeight(unsigned int head) const;
    int getHeadX(unsigned int head) const;
    int getHeadY(unsigned int head) const;
#endif // XINERAMA

private:
    Visual *visual;
    Window root_window;
    Colormap m_colormap;

    int depth, screen_number;
    unsigned int width, height;
#ifdef XINERAMA
    bool m_hasXinerama;
    int xineramaMajor, xineramaMinor, xineramaNumHeads, xineramaLastHead;
    XineramaScreenInfo *xineramaInfos;
#endif // XINERAMA

};

#endif // SCREENINFO_HH
