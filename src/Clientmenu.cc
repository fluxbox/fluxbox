// Clientmenu.cc for Fluxbox
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// Clientmenu.cc for Blackbox - an X11 Window manager
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

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "fluxbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"


Clientmenu::Clientmenu(Workspace &ws) : Basemenu(ws.getScreen()),
                                        m_wkspc(ws) {
    setInternalMenu();
}


void Clientmenu::itemSelected(int button, unsigned int index) {
    if (button > 2)
        return;
    //get the window with index of the item we selected
    FluxboxWindow *win = m_wkspc.getWindow(index);
    if (win) {
        if (button == 1) {
            if (! m_wkspc.isCurrent())
                m_wkspc.setCurrent();
        } else if (button == 2) {
            if (! m_wkspc.isCurrent())
                win->deiconify(true, false);
        }
        m_wkspc.raiseWindow(win);
        win->setInputFocus();
    }

    if (! (screen()->getWorkspacemenu()->isTorn() || isTorn()))
        hide();
}
