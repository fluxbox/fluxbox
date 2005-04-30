// WorkspaceCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "WorkspaceCmd.hh"

#include "Workspace.hh"
#include "Window.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "WinClient.hh"

#include "FbTk/KeyUtil.hh"

#ifdef HAVE_CMATH
  #include <cmath>
#else
  #include <math.h>
#endif
#include <algorithm>
#include <functional>
#include <iostream>

void NextWindowCmd::execute() {
    
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        Fluxbox *fb = Fluxbox::instance();
        // special case for commands from key events
        if (fb->lastEvent().type == KeyPress) {
            unsigned int mods = FbTk::KeyUtil::instance().cleanMods(fb->lastEvent().xkey.state);
            if (mods == 0) // can't stacked cycle unless there is a mod to grab
                screen->nextFocus(m_option | BScreen::CYCLELINEAR);
            else {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                if (!fb->watchingScreen() && 
                    !(m_option & BScreen::CYCLELINEAR))
                    Fluxbox::instance()->watchKeyRelease(*screen, mods);
                screen->nextFocus(m_option);
            }
        } else
            screen->nextFocus(m_option);
    }
}

void PrevWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        Fluxbox *fb = Fluxbox::instance();
        // special case for commands from key events
        if (fb->lastEvent().type == KeyPress) {
            unsigned int mods = FbTk::KeyUtil::instance().cleanMods(fb->lastEvent().xkey.state);
            if (mods == 0) // can't stacked cycle unless there is a mod to grab
                screen->prevFocus(m_option | BScreen::CYCLELINEAR);
            else {
                // if stacked cycling, then set a watch for 
                // the release of exactly these modifiers
                if (!fb->watchingScreen() 
                    && !(m_option & BScreen::CYCLELINEAR))
                    Fluxbox::instance()->watchKeyRelease(*screen, mods);
                screen->prevFocus(m_option);
            }
        } else
            screen->nextFocus(m_option);
    }
}

void DirFocusCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen == 0)
        return;

    WinClient *client = Fluxbox::instance()->getFocusedWindow();
    if (client == 0 || client->fbwindow() == 0)
        return;

    screen->dirFocus(*client->fbwindow(), m_dir);
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


/**
  try to arrange the windows on the current workspace in a 'clever' way.
  we take the shaded-windows and put them ontop of the workspace and put the
  normal windows underneath it.
 */
void ArrangeWindowsCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace *space = screen->currentWorkspace();
    unsigned int win_count = space->windowList().size();
  
    if (win_count == 0) 
        return;

    // TODO: choice between 
    //        -  arrange using all windows on all heads
    //        -  arrange for each head
    //        -  only on current head
    const int head = screen->getCurrHead();
    Workspace::Windows::iterator win;
    
    Workspace::Windows normal_windows;
    Workspace::Windows shaded_windows;
    for(win = space->windowList().begin(); win != space->windowList().end(); win++) {
        int winhead = screen->getHead((*win)->fbWindow());
        if (winhead == head || winhead == 0) {
            if (!(*win)->isShaded())
                normal_windows.push_back(*win);
            else
                shaded_windows.push_back(*win);
        }
    }

    // to arrange only shaded windows is a bit pointless imho (mathias)
    if (normal_windows.size() == 0)
        return;

    win_count = normal_windows.size();
    
    const unsigned int max_width = screen->maxRight(head) - screen->maxLeft(head);
    unsigned int max_height = screen->maxBottom(head) - screen->maxTop(head);

	// try to get the same number of rows as columns.
	unsigned int rows = int(sqrt((float)win_count));  // truncate to lower
	unsigned int cols = int(0.99 + float(win_count) / float(rows));
	if (max_width<max_height) {	// rotate
        std::swap(cols, rows);
	}

    unsigned int x_offs = screen->maxLeft(head); // window position offset in x
    unsigned int y_offs = screen->maxTop(head); // window position offset in y
    unsigned int window = 0; // current window 
    const unsigned int cal_width = max_width/cols; // calculated width ratio (width of every window)
    unsigned int i;
    unsigned int j;

    // place the shaded windows
    // TODO: until i resolve the shadedwindow->moveResize() issue to place
    // them in the same columns as the normal windows i just place the shaded
    // windows unchanged ontop of the current head
    for (i = 0, win = shaded_windows.begin(); win != shaded_windows.end(); win++, i++) {
        if (i & 1)
            (*win)->move(x_offs, y_offs);
        else
            (*win)->move(screen->maxRight(head) - (*win)->frame().width(), y_offs);
            
        y_offs += (*win)->frame().height();
    }

    // TODO: what if the number of shaded windows is really big and we end up
    // with really little space left for the normal windows? how to handle
    // this?
    if (!shaded_windows.empty())
        max_height -= i * (*shaded_windows.begin())->frame().height(); 

    const unsigned int cal_height = max_height/rows; // height ratio (height of every window)
    // Resizes and sets windows positions in columns and rows.
    for (i = 0; i < rows; ++i) {
        x_offs = screen->maxLeft(head);
        for (j = 0; j < cols && window < win_count; ++j, ++window) {
			if (window!=(win_count-1)) {
	            normal_windows[window]->moveResize(x_offs, y_offs, cal_width, cal_height);
			} else { // the last window gets everything that is left.
	            normal_windows[window]->moveResize(x_offs, y_offs, screen->maxRight(head)-x_offs, cal_height);
			}
            // next x offset
            x_offs += cal_width;
        }
        // next y offset
        y_offs += cal_height;
    }
}

void ShowDesktopCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace::Windows windows(screen->currentWorkspace()->windowList());
    std::for_each(windows.begin(),
                  windows.end(),
                  std::mem_fun(&FluxboxWindow::iconify));
}
