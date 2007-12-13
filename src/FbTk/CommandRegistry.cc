// CommandRegistry.cc for FbTk
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

#include "CommandRegistry.hh"
#include "Command.hh"
#include "StringUtil.hh"

#include <iostream>

namespace FbTk {

CommandRegistry &CommandRegistry::instance() {
    static CommandRegistry s_instance;
    return s_instance;
}


bool CommandRegistry::registerCommand(string name,
                                      CreateFunction createFunction) {
    name = StringUtil::toLower(name);
    m_commandCreators[name] = createFunction;
    return true;
}

bool CommandRegistry::registerBoolCommand(string name,
                                          BoolCreateFunction createFunction) {
    name = StringUtil::toLower(name);
    m_boolcommandCreators[name] = createFunction;
    return true;
}

Command *CommandRegistry::parseLine(const string &line, bool trusted) const {
    // parse args and command
    string command, args;
    StringUtil::getFirstWord(line, command, args);

    // now we have parsed command and args
    command = StringUtil::toLower(command);
    return getCommand(command, args, trusted);
}

BoolCommand *CommandRegistry::parseBoolLine(const string &line, bool trusted) const {
    // parse args and command
    string command, args;
    StringUtil::getFirstWord(line, command, args);

    // now we have parsed command and args
    command = StringUtil::toLower(command);
    return getBoolCommand(command, args, trusted);
}

Command *CommandRegistry::getCommand(const string &name, const string &args,
                                     bool trusted) const {
    string lc = StringUtil::toLower(name);
    CreatorMap::const_iterator it = m_commandCreators.find(lc.c_str());
    if (it == m_commandCreators.end())
        return 0;
    else
        return it->second(name, args, trusted);
}

BoolCommand *CommandRegistry::getBoolCommand(const string &name,
        const string &args, bool trusted) const {
    string lc = StringUtil::toLower(name);
    BoolCreatorMap::const_iterator it = m_boolcommandCreators.find(lc.c_str());
    if (it == m_boolcommandCreators.end())
        return 0;
    else
        return it->second(name, args, trusted);
}

}; // end namespace FbTk
