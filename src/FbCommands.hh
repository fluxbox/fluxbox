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

// $Id: FbCommands.hh,v 1.3 2003/04/16 13:33:18 fluxgen Exp $

// \file contains basic commands to restart, reconfigure, execute command and exit fluxbox

#ifndef FBCOMMANDS_HH
#define FBCOMMANDS_HH

#include "Command.hh"

#include <string>

namespace FbCommands {

/// executes a system command
class ExecuteCmd: public FbTk::Command {
public:
    explicit ExecuteCmd(const std::string &cmd);
    void execute();
private:
    std::string m_cmd;
};

/// exit fluxbox
class ExitFluxboxCmd: public FbTk::Command {
public:
    void execute();
};

/// restarts fluxbox
class RestartFluxboxCmd: public FbTk::Command {
public:
    void execute();
};

/// reconfigures fluxbox
class ReconfigureFluxboxCmd: public FbTk::Command {
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

}; // end namespace FbCommands

#endif // FBCOMMANDS_HH
