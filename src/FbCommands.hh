// FbCommands.hh for Fluxbox
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// \file contains basic commands to restart, reconfigure, execute command and exit fluxbox

#ifndef FBCOMMANDS_HH
#define FBCOMMANDS_HH

#include "FbTk/Command.hh"

#include <memory>

#include "ClientMenu.hh"
#include "ClientPattern.hh"
#include "FocusableList.hh"

namespace FbCommands {

/// executes a system command
class ExecuteCmd: public FbTk::Command<void> {
public:
    ExecuteCmd(const std::string &cmd, int screen_num = -1);
    void execute();
    /**
     * same as execute but returns pid
     */
    int run();
private:
    std::string m_cmd;
    const int m_screen_num;
};

/// sets environment
class ExportCmd : public FbTk::Command<void> {
public:
    ExportCmd(const std::string& name, const std::string& value);
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    std::string m_name;
    std::string m_value;
};

/// exit fluxbox
class ExitFluxboxCmd: public FbTk::Command<void> {
public:
    void execute();
};

/// saves resources
class SaveResources: public FbTk::Command<void> {
public:
    void execute();
};

/// restarts fluxbox
class RestartFluxboxCmd: public FbTk::Command<void> {
public:
    RestartFluxboxCmd(const std::string &cmd);
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                      const std::string &args, bool trusted);
private:
    std::string m_cmd;
};

/// reconfigures fluxbox
class ReconfigureFluxboxCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ReloadStyleCmd: public FbTk::Command<void> {
public:
    void execute();
};

class SetStyleCmd: public FbTk::Command<void> {
public:
    explicit SetStyleCmd(const std::string &filename);
    void execute();
private:
    std::string m_filename;
};

class KeyModeCmd: public FbTk::Command<void> {
public:
    explicit KeyModeCmd(const std::string &arguments);
    void execute();
private:
    std::string m_keymode;
    std::string m_end_args;
};

class HideMenuCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ShowClientMenuCmd: public FbTk::Command<void> {
public:
    ShowClientMenuCmd(int option, std::string &pat):
            m_option(option|FocusableList::LIST_GROUPS), m_pat(pat.c_str()) { }
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    const int m_option;
    const ClientPattern m_pat;
    std::list<FluxboxWindow *> m_list;
    std::unique_ptr<ClientMenu> m_menu;
};

class ShowCustomMenuCmd: public FbTk::Command<void> {
public:
    explicit ShowCustomMenuCmd(const std::string &arguments);
    void execute();
    void reload();
private:
   std::string custom_menu_file;
   std::unique_ptr<FbMenu> m_menu;    
};

class ShowRootMenuCmd: public FbTk::Command<void> {
public:
    void execute();
};

class ShowWorkspaceMenuCmd: public FbTk::Command<void> {
public:
    void execute();
};

class SetWorkspaceNameCmd: public FbTk::Command<void> {
public:
    SetWorkspaceNameCmd(const std::string &name, int spaceid = -1);
    void execute();
private:
    std::string m_name;
    int m_workspace;
};

class WorkspaceNameDialogCmd: public FbTk::Command<void> {
public:
    void execute();
};

class CommandDialogCmd: public FbTk::Command<void> {
public:
    void execute();
};


class SetResourceValueCmd: public FbTk::Command<void> {
public:
    SetResourceValueCmd(const std::string &resourcename, const std::string &value);
    void execute();
private:
    const std::string m_resname;
    const std::string m_value;
};

class SetResourceValueDialogCmd: public FbTk::Command<void> {
public:
    void execute();
};

class BindKeyCmd: public FbTk::Command<void> {
public:
    BindKeyCmd(const std::string &keybind);
    void execute();
private:
    const std::string m_keybind;
};

/// deiconifies iconified windows
class DeiconifyCmd: public FbTk::Command<void> {
public:
    enum Mode { 
        LAST,
        LASTWORKSPACE,
        ALL,
        ALLWORKSPACE
    };

    enum Destination {
        CURRENT, /// deiconification on current workspace
        ORIGIN,  /// deiconification on origin workspace, change to that ws
        ORIGINQUIET /// deiconification on origin workspace, dont change ws
    };
    
    DeiconifyCmd(Mode mode= LASTWORKSPACE,
                 Destination dest= CURRENT);
    void execute();
    static FbTk::Command<void> *parse(const std::string &command,
                                const std::string &args, bool trusted);
private:
    Mode m_mode;
    Destination m_dest;
};


/// test client pattern
class ClientPatternTestCmd: public FbTk::Command<void> {
public:
    ClientPatternTestCmd(const std::string& args) : m_args(args) { };
    void execute();
private:
    std::string m_args;
};

} // end namespace FbCommands

#endif // FBCOMMANDS_HH
