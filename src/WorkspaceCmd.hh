// WorkspaceCmd.hh for Fluxbox - an X11 Window manager
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

// $Id: WorkspaceCmd.hh,v 1.1 2003/06/30 14:38:42 fluxgen Exp $

#ifndef WORKSPACECMD_HH
#define WORKSPACECMD_HH
#include "Command.hh"

class NextWindowCmd: public FbTk::Command {
public:
    explicit NextWindowCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class PrevWindowCmd: public FbTk::Command {
public:
    explicit PrevWindowCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class NextWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class PrevWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class JumpToWorkspaceCmd: public FbTk::Command {
public:
    explicit JumpToWorkspaceCmd(int workspace_num);
    void execute();
private:
    const int m_workspace_num;
};

/// arranges windows in current workspace to rows and columns
class ArrangeWindowsCmd: public FbTk::Command {
public:
    void execute();
};

class ShowDesktopCmd: public FbTk::Command {
public:
    void execute();
};

#endif // WORKSPACECMD_HH
