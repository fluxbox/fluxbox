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

#ifndef WORKSPACECMD_HH
#define WORKSPACECMD_HH
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"

#include "ClientPattern.hh"
#include "FocusControl.hh"

class WindowListCmd: public FbTk::Command<void> {
public:
    WindowListCmd(FbTk::RefCount<FbTk::Command<void> > cmd, int opts,
                  FbTk::RefCount<FbTk::Command<bool> > filter):
            m_cmd(cmd), m_opts(opts), m_filter(filter) { }

    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);

private:
    FbTk::RefCount<FbTk::Command<void> > m_cmd;
    int m_opts;
    FbTk::RefCount<FbTk::Command<bool> > m_filter;
};

class SomeCmd: public FbTk::Command<bool> {
public:
    SomeCmd(FbTk::RefCount<FbTk::Command<bool> > cmd): m_cmd(cmd) { }

    bool execute();
    static FbTk::Command<bool> *parse(const std::string &command,
                                    const std::string &args, bool trusted);

private:
    FbTk::RefCount<FbTk::Command<bool> > m_cmd;
};

class EveryCmd: public FbTk::Command<bool> {
public:
    EveryCmd(FbTk::RefCount<FbTk::Command<bool> > cmd): m_cmd(cmd) { }

    bool execute();

private:
    FbTk::RefCount<FbTk::Command<bool> > m_cmd;
};

class AttachCmd: public FbTk::Command<void> {
public:
    explicit AttachCmd(const std::string &pat): m_pat(pat.c_str()) { }
    void execute();
private:
    const ClientPattern m_pat;
};

class NextWindowCmd: public FbTk::Command<void> {
public:
    explicit NextWindowCmd(int option, std::string &pat):
            m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_option;
    const ClientPattern m_pat;
};

class PrevWindowCmd: public FbTk::Command<void> {
public:
    explicit PrevWindowCmd(int option, std::string &pat):
            m_option(option), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_option;
    const ClientPattern m_pat;
};

class GoToWindowCmd: public FbTk::Command<void> {
public:
    GoToWindowCmd(int num, int option, std::string &pat):
            m_num(num), m_option(option), m_pat(pat.c_str()) { }
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    const int m_num;
    const int m_option;
    const ClientPattern m_pat;
};

class DirFocusCmd: public FbTk::Command<void> {
public:
    explicit DirFocusCmd(const FocusControl::FocusDir dir): m_dir(dir) { }
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    const FocusControl::FocusDir m_dir;
};

class AddWorkspaceCmd: public FbTk::Command<void> {
public:
    void execute();
};

class RemoveLastWorkspaceCmd: public FbTk::Command<void> {
public:
    void execute();
};

class NextWorkspaceCmd: public FbTk::Command<void> {
public:
    explicit NextWorkspaceCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class PrevWorkspaceCmd: public FbTk::Command<void> {
public:
    explicit PrevWorkspaceCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class LeftWorkspaceCmd: public FbTk::Command<void> {
public:
    explicit LeftWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
};

class RightWorkspaceCmd: public FbTk::Command<void> {
public:
    explicit RightWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
};

class JumpToWorkspaceCmd: public FbTk::Command<void> {
public:
    explicit JumpToWorkspaceCmd(int workspace_num);
    void execute();
private:
    const int m_workspace_num;
};

/// arranges windows in current workspace to rows and columns
class ArrangeWindowsCmd: public FbTk::Command<void> {
public:
    enum {
      UNSPECIFIED,
      VERTICAL,
      HORIZONTAL,
      STACKLEFT,
      STACKRIGHT,
      STACKTOP,
      STACKBOTTOM
    };
    explicit ArrangeWindowsCmd(int tile_method, std::string &pat):
            m_tile_method( tile_method ), m_pat(pat.c_str()) { }
    void execute();
private:
    const int m_tile_method;
    const ClientPattern m_pat;
};

class UnclutterCmd: public FbTk::Command<void> {
public:
    explicit UnclutterCmd(std::string &pat): m_pat(pat.c_str()) { }
    void execute();
private:
    const ClientPattern m_pat;
};

class ShowDesktopCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ToggleSlitAboveCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ToggleSlitHiddenCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ToggleToolbarAboveCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ToggleToolbarHiddenCmd: public FbTk::Command<void> {
public:
    void execute();
};

class CloseAllWindowsCmd: public FbTk::Command<void> {
public:
    void execute();
};

class RelabelButtonCmd: public FbTk::Command<void> {
public:
    explicit RelabelButtonCmd(std::string button, std::string label):
            m_button(button), m_label(label) {}
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    std::string m_button, m_label;
};

class MarkWindowCmd: public FbTk::Command<void> {
public:
    void execute();
};

class GotoMarkedWindowCmd: public FbTk::Command<void> {
public:
    void execute();
};

#endif // WORKSPACECMD_HH
