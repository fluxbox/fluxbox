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

#include "CommandRegistry.hh"
#include "StringUtil.hh"

using FbTk::StringUtil::removeFirstWhitespace;
using FbTk::StringUtil::toLower;
using std::string;

namespace FbTk {

namespace {

template <typename M>
M *addCommands(M *macro, const string &args, bool trusted) {
    int pos = 0, err = 0;
    string cmd;

    while (true) {
        RefCount<BoolCommand> tmp(0);
        err = StringUtil::getStringBetween(cmd, args.c_str() + pos,
                                           '{', '}', " \t\n", true);
        pos += err;
        if (err == 0)
            break;

        tmp = CommandRegistry::instance().parseBoolLine(cmd, trusted);
        if (*tmp)
            macro->add(tmp);
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
                CommandRegistry::instance().parseBoolLine(args, trusted);
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

REGISTER_BOOLCOMMAND_PARSER(not, parseLogicCommand);
REGISTER_BOOLCOMMAND_PARSER(and, parseLogicCommand);
REGISTER_BOOLCOMMAND_PARSER(or, parseLogicCommand);
REGISTER_BOOLCOMMAND_PARSER(xor, parseLogicCommand);

}; // end anonymous namespace

Command *IfCommand::parse(const std::string &command, const std::string &args,
                          bool trusted) {
    std::string cmd;
    int err = 0, pos = 0;
    RefCount<BoolCommand> cond(0);
    RefCount<Command> t(0), f(0);

    err = StringUtil::getStringBetween(cmd, args.c_str(),
                                       '{', '}', " \t\n", true);
    if (err > 0)
        cond = CommandRegistry::instance().parseBoolLine(cmd, trusted);
    if (err == 0 || *cond == 0)
        return 0;

    pos = err;
    err = StringUtil::getStringBetween(cmd, args.c_str() + pos,
                                       '{', '}', " \t\n", true);
    if (err == 0)
        return 0;
    t = CommandRegistry::instance().parseLine(cmd, trusted);

    pos += err;
    err = StringUtil::getStringBetween(cmd, args.c_str() + pos,
                                       '{', '}', " \t\n", true);
    if (err > 0)
        f = CommandRegistry::instance().parseLine(cmd, trusted);
    if (err == 0 || *t == 0 && *f == 0)
        return 0;

    return new IfCommand(cond, t, f);
}

REGISTER_COMMAND_PARSER(if, IfCommand::parse);
REGISTER_COMMAND_PARSER(cond, IfCommand::parse);

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
