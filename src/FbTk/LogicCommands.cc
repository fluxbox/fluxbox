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

// $Id: $

#include "LogicCommands.hh"

#include "ObjectRegistry.hh"
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
    RefCount<BoolCommand> cmd(0);

    std::vector<std::string>::iterator it = cmds.begin(), it_end = cmds.end();
    for (; it != it_end; ++it) {
        cmd = ObjectRegistry<BoolCommand>::instance().parse(*it, trusted);
        if (*cmd)
            macro->add(cmd);
    }

    if (macro->size() > 0)
        return macro;
    delete macro;
    return 0;
}

BoolCommand *parseLogicCommand(const string &command, const string &args,
                               bool trusted) {
    if (command == "not") {
        BoolCommand *boolcmd =
                ObjectRegistry<BoolCommand>::instance().parse(args, trusted);
        if (!boolcmd)
            return 0;
        RefCount<BoolCommand> ref(boolcmd);
        return new NotCommand(ref);
    } else if (command == "and")
        return addCommands<AndCommand>(new AndCommand(), args, trusted);
    else if (command == "or")
        return addCommands<OrCommand>(new OrCommand(), args, trusted);
    else if (command == "xor")
        return addCommands<XorCommand>(new XorCommand(), args, trusted);
    return 0;
}

REGISTER_OBJECT_PARSER(not, parseLogicCommand, BoolCommand);
REGISTER_OBJECT_PARSER(and, parseLogicCommand, BoolCommand);
REGISTER_OBJECT_PARSER(or, parseLogicCommand, BoolCommand);
REGISTER_OBJECT_PARSER(xor, parseLogicCommand, BoolCommand);

}; // end anonymous namespace

Command *IfCommand::parse(const std::string &command, const std::string &args,
                          bool trusted) {
    std::string blah;
    std::vector<std::string> cmds;
    RefCount<BoolCommand> cond(0);
    RefCount<Command> t(0), f(0);

    StringUtil::stringTokensBetween(cmds, args, blah, '{', '}');
    if (cmds.size() < 3)
        return 0;

    cond = ObjectRegistry<BoolCommand>::instance().parse(cmds[0], trusted);
    if (*cond == 0)
        return 0;

    t = ObjectRegistry<Command>::instance().parse(cmds[1], trusted);
    if (cmds.size() >= 3)
        f = ObjectRegistry<Command>::instance().parse(cmds[2], trusted);
    if (*t == 0 && *f == 0)
        return 0;

    return new IfCommand(cond, t, f);
}

REGISTER_OBJECT_PARSER(if, IfCommand::parse, Command);
REGISTER_OBJECT_PARSER(cond, IfCommand::parse, Command);

void OrCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t OrCommand::size() const {
    return m_commandlist.size();
}

bool OrCommand::bool_execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (m_commandlist[i]->bool_execute())
            return true;
    }
    return false;
}

void AndCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t AndCommand::size() const {
    return m_commandlist.size();
}

bool AndCommand::bool_execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (!m_commandlist[i]->bool_execute())
            return false;
    }
    return true;
}

void XorCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t XorCommand::size() const {
    return m_commandlist.size();
}

bool XorCommand::bool_execute() {
    bool ret = false;
    for (size_t i=0; i < m_commandlist.size(); ++i)
        ret ^= m_commandlist[i]->bool_execute();
    return ret;
}

}; // end namespace FbTk
