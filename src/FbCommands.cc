// FbCommands.cc for Fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbCommands.cc,v 1.16 2003/09/06 14:13:06 fluxgen Exp $

#include "FbCommands.hh"
#include "fluxbox.hh"
#include "FbTk/Theme.hh"
#include "Screen.hh"
#include "Menu.hh"
#include "SetWorkspaceName.hh"

#include <sys/types.h>
#include <unistd.h>

#include <iostream>
using namespace std;

namespace FbCommands {

ExecuteCmd::ExecuteCmd(const std::string &cmd, int screen_num):m_cmd(cmd), m_screen_num(screen_num) {

}

void ExecuteCmd::execute() {
#ifndef    __EMX__
    if (! fork()) {
        std::string displaystring("DISPLAY=");
        displaystring += DisplayString(FbTk::App::instance()->display());
        char intbuff[64];
        int screen_num = m_screen_num;
        if (screen_num < 0) {
            if (Fluxbox::instance()->mouseScreen() == 0)
                screen_num = 0;
            else
                screen_num = Fluxbox::instance()->mouseScreen()->screenNumber();
        }

        sprintf(intbuff, "%d", screen_num);

        // remove last number of display and add screen num
        displaystring.erase(displaystring.size()-1);
        displaystring += intbuff;
        setsid();
        putenv(const_cast<char *>(displaystring.c_str()));
        execl("/bin/sh", "/bin/sh", "-c", m_cmd.c_str(), 0);
        exit(0);
    }
#else //   __EMX__
    spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec().c_str(), 0);
#endif // !__EMX__

}

void ExitFluxboxCmd::execute() {
    Fluxbox::instance()->shutdown();
}

void SaveResources::execute() {
    Fluxbox::instance()->save_rc();
}

RestartFluxboxCmd::RestartFluxboxCmd(const std::string &cmd):m_cmd(cmd){
}

void RestartFluxboxCmd::execute() {
    if (m_cmd.size() == 0) {
        Fluxbox::instance()->restart();
    } else {
        Fluxbox::instance()->restart(m_cmd.c_str());
    }
}

void ReconfigureFluxboxCmd::execute() {
    Fluxbox::instance()->reconfigure();
}

SetStyleCmd::SetStyleCmd(const std::string &filename):m_filename(filename) {

}

void SetStyleCmd::execute() {
    Fluxbox::instance()->saveStyleFilename(m_filename.c_str());
    Fluxbox::instance()->save_rc();
    FbTk::ThemeManager::instance().load(m_filename);
}

void ShowRootMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    if (screen->getRootmenu()) {

      Window root_ret;
      Window window_ret;

      int rx, ry;
      int wx, wy;
      unsigned int mask;

      if ( XQueryPointer(FbTk::App::instance()->display(),
            screen->rootWindow().window(), &root_ret, &window_ret,
            &rx, &ry, &wx, &wy, &mask) ) {

        if ( rx - (screen->getRootmenu()->width()/2) > 0 )
          rx-= screen->getRootmenu()->width()/2;
        screen->getRootmenu()->move(rx, ry);
      }

      screen->getRootmenu()->show();
      screen->getRootmenu()->grabInputFocus();
    }
}

void ShowWorkspaceMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    if (screen->getWorkspacemenu()) {

      Window root_ret;
      Window window_ret;

      int rx, ry;
      int wx, wy;
      unsigned int mask;

      if ( XQueryPointer(FbTk::App::instance()->display(),
            screen->rootWindow().window(), &root_ret, &window_ret,
            &rx, &ry, &wx, &wy, &mask) ) {

        if ( rx - (screen->getWorkspacemenu()->width()/2) > 0 )
          rx-= screen->getWorkspacemenu()->width()/2;
        screen->getWorkspacemenu()->move(rx, ry);
      }
      screen->getWorkspacemenu()->show();
      screen->getWorkspacemenu()->grabInputFocus();
    }
}

void ShowWorkspaceMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    if (screen->getWorkspacemenu()) {
        screen->getWorkspacemenu()->show();
        screen->getWorkspacemenu()->grabInputFocus();
    }
}

void SetWorkspaceNameCmd::execute() {

    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    SetWorkspaceName *win = new SetWorkspaceName(*screen);
    win->show();
}

}; // end namespace FbCommands
