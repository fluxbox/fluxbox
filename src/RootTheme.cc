// RootTheme.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: RootTheme.cc,v 1.8 2004/03/22 21:01:42 fluxgen Exp $

#include "RootTheme.hh"

#include "FbCommands.hh"
#include "FbTk/App.hh"

RootTheme::RootTheme(int screen_num, std::string &screen_root_command):
    FbTk::Theme(screen_num),
    m_root_command(*this, "rootCommand", "RootCommand"), 
    m_screen_root_command(screen_root_command),
    m_opgc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_lock(false) {
    
    Display *disp = FbTk::App::instance()->display();
    m_opgc.setForeground(WhitePixel(disp, screen_num)^BlackPixel(disp, screen_num));
    m_opgc.setFunction(GXxor);
    m_opgc.setSubwindowMode(IncludeInferiors);
    m_opgc.setLineAttributes(1, LineSolid, CapNotLast, JoinMiter);
}

RootTheme::~RootTheme() {

}

void RootTheme::reconfigTheme() {
    if (m_lock)
        return;

    // override resource root command?
    if (m_screen_root_command == "") { 
        // do root command
        FbCommands::ExecuteCmd cmd(*m_root_command, screenNum());
        cmd.execute();
    } else {
        FbCommands::ExecuteCmd cmd(m_screen_root_command, screenNum());
        cmd.execute();
    }
}
