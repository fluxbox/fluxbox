// CommandParser.cc for Fluxbox - an X11 Window manager
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

// $Id$

#include "CommandParser.hh"
#include "FbTk/StringUtil.hh"

#include <vector>

using std::string;
using std::vector;
using FbTk::StringUtil::removeFirstWhitespace;
using FbTk::StringUtil::toLower;


CommandParser *CommandParser::s_singleton = 0;

CommandFactory::CommandFactory() {

}

CommandFactory::~CommandFactory() {
    // remove all associations with this factory
    CommandParser::instance().removeAssociation(*this);

}

void CommandFactory::addCommand(const std::string &command_name) {
    CommandParser::instance().associateCommand(command_name, *this);
}

// ensure it is singleton
CommandParser::CommandParser() {
    if (s_singleton != 0)
        throw std::string("CommandParser currently meant ot be singleton");
}

CommandParser &CommandParser::instance() {
    if (s_singleton == 0)
        s_singleton = new CommandParser();

    return *s_singleton;
}

FbTk::Command *CommandParser::parseLine(const std::string &line, bool trusted) {

    // parse arguments and command
    string command = line;
    string arguments;
    string::size_type first_pos = removeFirstWhitespace(command);
    FbTk::StringUtil::removeTrailingWhitespace(command);
    string::size_type second_pos = command.find_first_of(" \t", first_pos);
    if (second_pos != string::npos) {
        // ok we have arguments, parsing them here
        arguments = command.substr(second_pos);
        removeFirstWhitespace(arguments);
        command.erase(second_pos); // remove argument from command
    }

    // now we have parsed command and arguments
    command = toLower(command);

    // we didn't find any matching command in default commands,
    // so we search in the command creators modules for a 
    // matching command string
    return toCommand(command, arguments, trusted);

}

FbTk::Command *CommandParser::toCommand(const std::string &command_str,
        const std::string &arguments, bool trusted) {
    if (m_commandfactorys[command_str] != 0)
        return m_commandfactorys[command_str]->stringToCommand(command_str, arguments, trusted);

    return 0;
}

void CommandParser::associateCommand(const std::string &command, CommandFactory &factory) {
    // we shouldnt override other commands
    if (m_commandfactorys[command] != 0)
        return;

    m_commandfactorys[command] = &factory;
}

void CommandParser::removeAssociation(CommandFactory &factory) {
    // commands that are associated with the factory
    vector<string> commands; 
    // find associations
    CommandFactoryMap::iterator factory_it = m_commandfactorys.begin();
    const CommandFactoryMap::iterator factory_it_end = m_commandfactorys.end();
    for (; factory_it != factory_it_end; ++factory_it) {
        if ((*factory_it).second == &factory)
            commands.push_back((*factory_it).first);
    }
    // remove all associations
    while (!commands.empty()) {
        m_commandfactorys.erase(commands.back());
        commands.pop_back();
    }

    if (m_commandfactorys.empty())
        delete s_singleton;
}
