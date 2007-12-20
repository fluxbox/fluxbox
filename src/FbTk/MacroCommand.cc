// MacroCommand.cc for FbTk
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#include "MacroCommand.hh"

#include "ObjectRegistry.hh"
#include "StringUtil.hh"

#include <list>
#include <string>

namespace FbTk {

namespace {

template <typename M>
M *addCommands(M *macro, const std::string &args, bool trusted) {

    std::string blah;
    std::list<std::string> cmds;
    StringUtil::stringTokensBetween(cmds, args, blah, '{', '}');
    RefCount<Command> cmd(0);

    std::list<std::string>::iterator it = cmds.begin(), it_end = cmds.end();
    for (; it != it_end; ++it) {
        cmd = ObjectRegistry<Command>::instance().parse(*it, trusted);
        if (*cmd)
            macro->add(cmd);
    }

    if (macro->size() > 0)
        return macro;

    delete macro;
    return 0;
}

Command *parseMacroCmd(const std::string &command, const std::string &args,
                       bool trusted) {
    if (command == "macrocmd")
        return addCommands<MacroCommand>(new MacroCommand, args, trusted);
    else if (command == "togglecmd")
        return addCommands<ToggleCommand>(new ToggleCommand, args, trusted);
    return 0;
}

REGISTER_OBJECT_PARSER(macrocmd, parseMacroCmd, Command);
REGISTER_OBJECT_PARSER(togglecmd, parseMacroCmd, Command);

}; // end anonymous namespace

void MacroCommand::add(RefCount<Command> &com) {
    m_commandlist.push_back(com);
}

size_t MacroCommand::size() const {
    return m_commandlist.size();
}

void MacroCommand::execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i)
        m_commandlist[i]->execute();
}

ToggleCommand::ToggleCommand() {
    m_state = 0;
}

void ToggleCommand::add(RefCount<Command> &com) {
    m_commandlist.push_back(com);
}

size_t ToggleCommand::size() const {
    return m_commandlist.size();
}

void ToggleCommand::execute() {
    m_commandlist[m_state]->execute();
    if (++m_state >= m_commandlist.size())
        m_state = 0;
}

}; // end namespace FbTk


