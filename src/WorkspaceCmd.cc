// WorkspaceCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
#include "FocusControl.hh"

#include "FbTk/KeyUtil.hh"

#ifdef HAVE_CMATH
  #include <cmath>
#else
  #include <math.h>
#endif
#include <algorithm>
#include <functional>

void WindowListCmd::execute() {
    if (m_pat.error()) {
        m_cmd->execute();
        return;
    }

    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusControl::Focusables win_list(screen->focusControl().creationOrderWinList());

        FocusControl::Focusables::iterator it = win_list.begin(),
                                           it_end = win_list.end();
        for (; it != it_end; ++it) {
            if (m_pat.match(**it) && (*it)->fbwindow())
                m_cmd->execute(*(*it)->fbwindow());
        }
    }
}

void AttachCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusControl::Focusables win_list(screen->focusControl().focusedOrderWinList());

        FocusControl::Focusables::iterator it = win_list.begin(),
                                           it_end = win_list.end();
        FluxboxWindow *first = 0;
        for (; it != it_end; ++it) {
            if (m_pat.match(**it) && (*it)->fbwindow()) {
                if (first == 0)
                    first = (*it)->fbwindow();
                else
                    first->attachClient((*it)->fbwindow()->winClient());
            }
        }
                
    }
}

void NextWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->cycleFocus(m_option, &m_pat, false);
}

void PrevWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->cycleFocus(m_option, &m_pat, true);
}

void GoToWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        const FocusControl::Focusables *win_list = 0;
        if (m_option & FocusControl::CYCLEGROUPS) {
            win_list = (m_option & FocusControl::CYCLELINEAR) ?
                &screen->focusControl().creationOrderWinList() :
                &screen->focusControl().focusedOrderWinList();
        } else {
            win_list = (m_option & FocusControl::CYCLELINEAR) ?
                &screen->focusControl().creationOrderList() :
                &screen->focusControl().focusedOrderList();
        }
        screen->focusControl().goToWindowNumber(*win_list, m_num, &m_pat);
    }
}

void DirFocusCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen == 0)
        return;

    FluxboxWindow *win = FocusControl::focusedFbWindow();
    if (win)
        screen->focusControl().dirFocus(*win, m_dir);
}

void AddWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->addWorkspace();
}

void RemoveLastWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->removeLastWorkspace();
}

void NextWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->nextWorkspace(m_option == 0 ? 1 : m_option);
}

void PrevWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->prevWorkspace(m_option == 0 ? 1 : m_option);
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
    if (screen != 0) {
        int num = screen->numberOfWorkspaces();
        int actual = m_workspace_num;
        // we need an extra +1, since it's subtracted in FbCommandFactory
        if (actual < 0) actual += num+1;
        if (actual < 0) actual = 0;
        if (actual >= num) actual = num - 1;
        screen->changeWorkspaceID(actual);
    }
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
    size_t win_count = space->windowList().size();

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
    unsigned int cols = int(sqrt((float)win_count));  // truncate to lower
    unsigned int rows = int(0.99 + float(win_count) / float(cols));
    if (max_width<max_height) {    // rotate
        std::swap(cols, rows);
    }

    unsigned int x_offs = screen->maxLeft(head); // window position offset in x
    unsigned int y_offs = screen->maxTop(head); // window position offset in y
   // unsigned int window = 0; // current window
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
        for (j = 0; j < cols && normal_windows.size() > 0; ++j) {


            int cell_center_x = x_offs + (x_offs + cal_width) / 2;
            int cell_center_y = y_offs + (y_offs + cal_height) / 2;
            unsigned int closest_dist = ~0;

            Workspace::Windows::iterator closest = normal_windows.end();
            for (win = normal_windows.begin(); win != normal_windows.end(); win++) {

                int win_center_x = (*win)->frame().x() + ((*win)->frame().x() + (*win)->frame().width() / 2);
                int win_center_y = (*win)->frame().y() + ((*win)->frame().y() + (*win)->frame().height() / 2);
                unsigned int dist = (win_center_x - cell_center_x) * (win_center_x - cell_center_x) + 
                                    (win_center_y - cell_center_y) * (win_center_y - cell_center_y);

                if (dist < closest_dist) {
                    closest = win;
                    closest_dist = dist;
                }
            }

            if (normal_windows.size() > 1) {
                (*closest)->moveResize(x_offs + (*closest)->xOffset(),
			y_offs + (*closest)->yOffset(),
			cal_width - (*closest)->widthOffset(),
			cal_height - (*closest)->heightOffset());
            } else { // the last window gets everything that is left.
                (*closest)->moveResize(x_offs + (*closest)->xOffset(),
			y_offs + (*closest)->yOffset(),
			screen->maxRight(head) - x_offs - (*closest)->widthOffset(),
			cal_height - (*closest)->heightOffset());
            }

            normal_windows.erase(closest);

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

void CloseAllWindowsCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;
  
    BScreen::Workspaces::iterator workspace_it = screen->getWorkspacesList().begin();
    BScreen::Workspaces::iterator workspace_it_end = screen->getWorkspacesList().end();
       for (; workspace_it != workspace_it_end; ++workspace_it) {
            Workspace::Windows windows((*workspace_it)->windowList());
            std::for_each(windows.begin(),
            windows.end(),
            std::mem_fun(&FluxboxWindow::close));
           }
    BScreen::Icons::iterator icon_it = screen->iconList().begin();
    BScreen::Icons::iterator icon_it_end = screen->iconList().end();
	    for (; icon_it != icon_it_end; ++icon_it ) {
	         (*icon_it)->close();
	      }
				   
}
