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

// $Id: FbCommands.cc,v 1.1 2003/01/09 17:46:10 fluxgen Exp $

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

}; // end namespace FbCommands
