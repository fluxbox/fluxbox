// RootCmdMenuItem.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Mathias Gumz (akira at fluxbox dot org)
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

#include "RootCmdMenuItem.hh"

#include "defaults.hh"
#include "FbCommands.hh"
#include "fluxbox.hh"

#include "FbTk/StringUtil.hh"

RootCmdMenuItem::RootCmdMenuItem(const FbTk::FbString &label, 
                             const std::string &filename,
                             const std::string &cmd):
    FbTk::MenuItem(label), 
    m_filename(filename) {

    std::string prog = cmd.empty() ? realProgramName("fbsetbg") : cmd;
    FbTk::RefCount<FbTk::Command<void> >
        setwp_cmd(new FbCommands::ExecuteCmd(prog + " \"" + m_filename + "\""));
    setCommand(setwp_cmd);
    setToggleItem(true);
    setCloseOnClick(false);
}


bool RootCmdMenuItem::isSelected() const {
    return Fluxbox::instance()->getStyleFilename() == m_filename;
}

