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

// $Id: FbCommands.cc,v 1.2 2003/02/15 01:58:06 fluxgen Exp $

#include "FbCommands.hh"
#include "fluxbox.hh"

#include <sys/types.h>
#include <unistd.h>

#include <iostream>
using namespace std;

namespace FbCommands {

ExecuteCmd::ExecuteCmd(const std::string &cmd):m_cmd(cmd) {

}

void ExecuteCmd::execute() {
#ifndef    __EMX__
    if (! fork()) {
        setsid();
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

void RestartFluxboxCmd::execute() {
    Fluxbox::instance()->restart();
}

void ReconfigureFluxboxCmd::execute() {
    Fluxbox::instance()->reconfigure();
}

SetStyleCmd::SetStyleCmd(const std::string &filename):m_filename(filename) {

}

void SetStyleCmd::execute() {
#ifdef DEBUG
    cerr<<__FILE__<<":Loading style: "<<m_filename<<endl;
#endif // DEBUG
    Fluxbox::instance()->saveStyleFilename(m_filename.c_str());
    Fluxbox::instance()->save_rc();
    FbTk::ThemeManager::instance().load(m_filename.c_str());

}

}; // end namespace FbCommands
