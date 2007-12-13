// LogicCommands.hh for FbTk
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id: $

#ifndef FBTK_LOGICCOMMANDS_HH
#define FBTK_LOGICCOMMANDS_HH

#include "Command.hh"
#include "RefCount.hh"

#include <string>
#include <vector>

namespace FbTk {

/// executes a boolcommand and uses the result to decide what to do
class IfCommand: public Command {
public:
    IfCommand(RefCount<BoolCommand> &cond,
              RefCount<Command> &t, RefCount<Command> &f):
        m_cond(cond), m_t(t), m_f(f) { }
    void execute() {
        if (m_cond->bool_execute()) {
            if (*m_t) m_t->execute();
        } else
            if (*m_f) m_f->execute();
    }
    static Command *parse(const std::string &cmd, const std::string &args,
                          bool trusted);
private:
    RefCount<BoolCommand> m_cond;
    RefCount<Command> m_t, m_f;
};

/// executes a list of boolcommands until one is true
class OrCommand: public BoolCommand {
public:
    void add(RefCount<BoolCommand> &com);
    size_t size() const;
    bool bool_execute();

private:
    std::vector<RefCount<BoolCommand> > m_commandlist;
};

/// executes a list of boolcommands until one is false
class AndCommand: public BoolCommand {
public:
    void add(RefCount<BoolCommand> &com);
    size_t size() const;
    bool bool_execute();

private:
    std::vector<RefCount<BoolCommand> > m_commandlist;
};

/// executes a list of boolcommands, returning the parity
class XorCommand: public BoolCommand {
public:
    void add(RefCount<BoolCommand> &com);
    size_t size() const;
    bool bool_execute();

private:
    std::vector<RefCount<BoolCommand> > m_commandlist;
};

/// executes a boolcommand and returns the negation
class NotCommand: public BoolCommand {
public:
    NotCommand(RefCount<BoolCommand> &com): m_command(com) { }
    bool bool_execute() { return !m_command->bool_execute(); }

private:
    RefCount<BoolCommand> m_command;
};

} // end namespace FbTk

#endif // FBTK_LOGICCOMMANDS_HH
