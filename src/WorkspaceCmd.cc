// WorkspaceCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: WorkspaceCmd.cc,v 1.3 2003/07/19 13:51:24 rathnor Exp $

#include "WorkspaceCmd.hh"
#include "Workspace.hh"
#include "Window.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "Keys.hh"
#include <algorithm>
#include <functional>

void NextWindowCmd::execute() {
    
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        Fluxbox *fb = Fluxbox::instance();
        // special case for commands from key events
        if (fb->lastEvent().type == KeyPress) {
            unsigned int mods = Keys::cleanMods(fb->lastEvent().xkey.state);
            if (mods == 0) // can't stacked cycle unless there is a mod to grab
                screen->nextFocus(m_option | BScreen::CYCLELINEAR);
            else {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                if (!fb->watchingScreen() && !(m_option & BScreen::CYCLELINEAR))
                    Fluxbox::instance()->watchKeyRelease(*screen, mods);
                screen->nextFocus(m_option);
            }
        }

    }
}

void PrevWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        Fluxbox *fb = Fluxbox::instance();
        // special case for commands from key events
        if (fb->lastEvent().type == KeyPress) {
            unsigned int mods = Keys::cleanMods(fb->lastEvent().xkey.state);
            if (mods == 0) // can't stacked cycle unless there is a mod to grab
                screen->prevFocus(m_option | BScreen::CYCLELINEAR);
            else {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                if (!fb->watchingScreen() && !(m_option & BScreen::CYCLELINEAR))
                    Fluxbox::instance()->watchKeyRelease(*screen, mods);
                screen->prevFocus(m_option);
            }
        }
    }
}

void NextWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->nextWorkspace();
}

void PrevWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->prevWorkspace();
}

void LeftWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->leftWorkspace(m_param);
}

void RightWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->rightWorkspace(m_param);
}

JumpToWorkspaceCmd::JumpToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }

void JumpToWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0 && m_workspace_num >= 0 && m_workspace_num < screen->getNumberOfWorkspaces())
        screen->changeWorkspaceID(m_workspace_num);
}


void ArrangeWindowsCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace *space = screen->currentWorkspace();
    const unsigned int win_count = space->windowList().size();
  
    if (win_count == 0) 
        return;

    const unsigned int max_weigth = screen->width();
    const unsigned int max_heigth = screen->height();


    unsigned int cols = 1; // columns
    unsigned int rows = 1; // rows

    // Calculates the "best" width / heigth ratio ( basically it
    // chooses the two divisors of win_count which are closest to
    // each other)
    int rt = win_count; // holds last t value
    for (unsigned int i = 1; i <= win_count; ++i) {
        int t = (win_count / i) - i;
      
        if (t < rt && t >= 0 && (win_count % i) == 0) {
            rt = t;
            rows = i;
            cols = win_count / i;
        }
    }
  
    const unsigned int cal_width = max_weigth/cols; // calculated width ratio (width of every window)
    const unsigned int cal_heigth = max_heigth/rows; // heigth ratio (heigth of every window)

    // Resizes and sets windows positions in columns and rows.
    unsigned int x_offs = 0; // window position offset in x
    unsigned int y_offs = 0; // window position offset in y
    unsigned int window = 0; // current window 
    Workspace::Windows &windowlist = space->windowList();
    for (unsigned int i = 0; i < rows; ++i) {
        x_offs = 0;
        for (unsigned int j = 0; j < cols && window < win_count; ++j, ++window) {
            windowlist[window]->moveResize(x_offs, y_offs, cal_width, cal_heigth);
            // next x offset
            x_offs += cal_width;
        }
        // next y offset
        y_offs += cal_heigth;
    }
}

void ShowDesktopCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace *space = screen->currentWorkspace();
    std::for_each(space->windowList().begin(),
                  space->windowList().end(),
                  std::mem_fun(&FluxboxWindow::iconify));
}
