// WorkspaceCmd.hh for Fluxbox - an X11 Window manager
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

#ifndef WORKSPACECMD_HH
#define WORKSPACECMD_HH
#include "Command.hh"

#include "ClientPattern.hh"
#include "CurrentWindowCmd.hh"
#include "FocusControl.hh"

#include "FbTk/RefCount.hh"

class WindowHelperCmd;

class WindowListCmd: public FbTk::Command {
public:
    WindowListCmd(FbTk::RefCount<WindowHelperCmd> cmd, const std::string &pat):
            m_cmd(cmd), m_pat(pat.c_str()) { }

    void execute();

private:
    FbTk::RefCount<WindowHelperCmd> m_cmd;
    ClientPattern m_pat;
};

class AttachCmd: public FbTk::Command {
public:
    explicit AttachCmd(const std::string &pat): m_pat(pat.c_str()) { }
    void execute();
private:
    const ClientPattern m_pat;
};

class NextWindowCmd: public FbTk::Command {
public:
    explicit NextWindowCmd(int option, std::string &pat):
            m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_option;
    const ClientPattern m_pat;
};

class PrevWindowCmd: public FbTk::Command {
public:
    explicit PrevWindowCmd(int option, std::string &pat): 
            m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_option;
    const ClientPattern m_pat;
};

class TypeAheadFocusCmd: public FbTk::Command {
public:
    explicit TypeAheadFocusCmd(int option, std::string &pat):
            m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_option;
    const ClientPattern m_pat;
};

class GoToWindowCmd: public FbTk::Command {
public:
    GoToWindowCmd(int num, int option, std::string &pat):
            m_num(num), m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_num;
    const int m_option;
    const ClientPattern m_pat;
};

class DirFocusCmd: public FbTk::Command {
public:
    explicit DirFocusCmd(const FocusControl::FocusDir dir): m_dir(dir) { }
    void execute();
private:
    const FocusControl::FocusDir m_dir;
};

class AddWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class RemoveLastWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class NextWorkspaceCmd: public FbTk::Command {
public:
    explicit NextWorkspaceCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class PrevWorkspaceCmd: public FbTk::Command {
public:
    explicit PrevWorkspaceCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class LeftWorkspaceCmd: public FbTk::Command {
public:
    explicit LeftWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
};

class RightWorkspaceCmd: public FbTk::Command {
public:
    explicit RightWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
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

class CloseAllWindowsCmd: public FbTk::Command {
public:
    void execute();
};

#endif // WORKSPACECMD_HH
