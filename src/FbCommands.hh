// FbCommands.hh for Fluxbox
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

// $Id$

// \file contains basic commands to restart, reconfigure, execute command and exit fluxbox

#ifndef FBCOMMANDS_HH
#define FBCOMMANDS_HH

#include "Command.hh"

#include <string>

namespace FbCommands {

/// executes a system command
class ExecuteCmd: public FbTk::Command {
public:
    ExecuteCmd(const std::string &cmd, int screen_num = -1);
    void execute();
private:
    std::string m_cmd;
    const int m_screen_num;
};

/// sets environment
class ExportCmd : public FbTk::Command {
public:
    ExportCmd(const std::string& name, const std::string& value);
    void execute();
private:
    std::string m_name;
    std::string m_value;
};

/// exit fluxbox
class ExitFluxboxCmd: public FbTk::Command {
public:
    void execute();
};

/// saves resources
class SaveResources: public FbTk::Command {
public:
    void execute();
};

/// restarts fluxbox
class RestartFluxboxCmd: public FbTk::Command {
public:
    RestartFluxboxCmd(const std::string &cmd);
    void execute();
private:
    std::string m_cmd;
};

/// reconfigures fluxbox
class ReconfigureFluxboxCmd: public FbTk::Command {
public:
    void execute();
};

class ReloadStyleCmd: public FbTk::Command {
public:
    void execute();
};

class SetStyleCmd: public FbTk::Command {
public:
    explicit SetStyleCmd(const std::string &filename);
    void execute();
private:
    std::string m_filename;
};

class ShowRootMenuCmd: public FbTk::Command {
public:
    void execute();
};

class ShowWorkspaceMenuCmd: public FbTk::Command {
public:
    void execute();
};

class SetWorkspaceNameCmd: public FbTk::Command {
public:
    SetWorkspaceNameCmd(const std::string &name, int spaceid = -1);
    void execute();
private:
    std::string m_name;
    int m_workspace;
};

class WorkspaceNameDialogCmd: public FbTk::Command {
public:
    void execute();
};

class CommandDialogCmd: public FbTk::Command {
public:
    void execute();
};


class SetResourceValueCmd: public FbTk::Command {
public:
    SetResourceValueCmd(const std::string &resourcename, const std::string &value);
    void execute();
private:
    const std::string m_resname;
    const std::string m_value;
};

class SetResourceValueDialogCmd: public FbTk::Command {
public:
    void execute();
};

class BindKeyCmd: public FbTk::Command {
public:
    BindKeyCmd(const std::string &keybind);
    void execute();
private:
    const std::string m_keybind;
};

/// deiconifies iconified windows
class DeiconifyCmd: public FbTk::Command {
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
    
    DeiconifyCmd(const Mode mode= LASTWORKSPACE,
                 const Destination dest= CURRENT);
    void execute();
private:
    Mode m_mode;
    Destination m_dest;
};

} // end namespace FbCommands

#endif // FBCOMMANDS_HH
