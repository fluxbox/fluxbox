// LogicCommands.cc for FbTk
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

#include "LogicCommands.hh"

#include "CommandParser.hh"
#include "StringUtil.hh"

#include <vector>

using FbTk::StringUtil::removeFirstWhitespace;
using FbTk::StringUtil::toLower;
using std::string;

namespace FbTk {

namespace {

template <typename M>
M *addCommands(M *macro, const string &args, bool trusted) {
    std::string blah;
    std::vector<std::string> cmds;
    StringUtil::stringTokensBetween(cmds, args, blah, '{', '}');
    RefCount<Command<bool> > cmd(0);

    std::vector<std::string>::iterator it = cmds.begin(), it_end = cmds.end();
    for (; it != it_end; ++it) {
        cmd.reset( CommandParser<bool>::instance().parse(*it, trusted) );
        if (cmd)
            macro->add(cmd);
    }

    if (macro->size() > 0)
        return macro;
    delete macro;
    return 0;
}

Command<bool> *parseLogicCommand(const string &command, const string &args,
                               bool trusted) {
    if (command == "not") {
        Command<bool> *boolcmd =
                CommandParser<bool>::instance().parse(args, trusted);
        if (!boolcmd)
            return 0;
        RefCount<Command<bool> > ref(boolcmd);
        return new NotCommand(ref);
    } else if (command == "and")
        return addCommands<AndCommand>(new AndCommand(), args, trusted);
    else if (command == "or")
        return addCommands<OrCommand>(new OrCommand(), args, trusted);
    else if (command == "xor")
        return addCommands<XorCommand>(new XorCommand(), args, trusted);
    return 0;
}

REGISTER_COMMAND_PARSER(not, parseLogicCommand, bool);
REGISTER_COMMAND_PARSER(and, parseLogicCommand, bool);
REGISTER_COMMAND_PARSER(or, parseLogicCommand, bool);
REGISTER_COMMAND_PARSER(xor, parseLogicCommand, bool);

} // end anonymous namespace

Command<void> *IfCommand::parse(const std::string &command, const std::string &args,
                          bool trusted) {
    std::string blah;
    std::vector<std::string> cmds;
    RefCount<Command<bool> > cond(0);
    RefCount<Command<void> > t(0), f(0);

    StringUtil::stringTokensBetween(cmds, args, blah, '{', '}');
    if (cmds.size() < 2)
        return 0;

    cond.reset( CommandParser<bool>::instance().parse(cmds[0], trusted) );
    if (cond == 0)
        return 0;

    t.reset( CommandParser<void>::instance().parse(cmds[1], trusted) );
    if (cmds.size() >= 3)
        f.reset( CommandParser<void>::instance().parse(cmds[2], trusted) );
    if (t == 0 && f == 0)
        return 0;

    return new IfCommand(cond, t, f);
}

REGISTER_COMMAND_PARSER(if, IfCommand::parse, void);
REGISTER_COMMAND_PARSER(cond, IfCommand::parse, void);

void OrCommand::add(RefCount<Command<bool> > &com) {
    m_commandlist.push_back(com);
}

size_t OrCommand::size() const {
    return m_commandlist.size();
}

bool OrCommand::execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (m_commandlist[i]->execute())
            return true;
    }
    return false;
}

void AndCommand::add(RefCount<Command<bool> > &com) {
    m_commandlist.push_back(com);
}

size_t AndCommand::size() const {
    return m_commandlist.size();
}

bool AndCommand::execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (!m_commandlist[i]->execute())
            return false;
    }
    return true;
}

void XorCommand::add(RefCount<Command<bool> > &com) {
    m_commandlist.push_back(com);
}

size_t XorCommand::size() const {
    return m_commandlist.size();
}

bool XorCommand::execute() {
    bool ret = false;
    for (size_t i=0; i < m_commandlist.size(); ++i)
        ret ^= m_commandlist[i]->execute();
    return ret;
}

} // end namespace FbTk
