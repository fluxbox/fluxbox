// CommandParser.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
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

// $Id: CommandParser.cc,v 1.1 2003/06/30 14:44:43 fluxgen Exp $

#include "CommandParser.hh"

#include "StringUtil.hh"

#include <vector>
#include <iostream>
#include <iterator>
using namespace std;
namespace {

string::size_type removeFirstWhitespace(std::string &str) {
    string::size_type first_pos = str.find_first_not_of(" \t");
    if (first_pos != string::npos)
        str.erase(0, first_pos);
    return first_pos;
}

}; // end anonymous namespace

CommandFactory::CommandFactory() {

}

CommandFactory::~CommandFactory() {
    // remove all associations with this factory
    CommandParser::instance().removeAssociation(*this);
}

void CommandFactory::addCommand(const std::string &command_name) {
    CommandParser::instance().associateCommand(command_name, *this);
}

CommandParser &CommandParser::instance() {
    static CommandParser singleton;
    return singleton;
}

FbTk::Command *CommandParser::parseLine(const std::string &line) {
    
    // parse arguments and command
    string command = line;
    string arguments;
    string::size_type first_pos = removeFirstWhitespace(command);
    string::size_type second_pos = command.find_first_of(" \t", first_pos);
    if (second_pos != string::npos) {
        // ok we have arguments, parsing them here
        arguments = command.substr(second_pos);
        removeFirstWhitespace(arguments);
        command.erase(second_pos); // remove argument from command                
    }

    // now we have parsed command and arguments
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): command = ["<<
      command<<"] arguments=["<<arguments<<"]"<<endl;
#endif // DEBUG

    FbTk::StringUtil::toLower(command);

    // we didn't find any matching command in default commands,
    // so we search in the command creators modules for a matching command string
    return toCommand(command, arguments);

}

ostream &operator << (ostream &the_stream, const CommandParser::CommandFactoryMap::value_type &value) {
    the_stream<<value.first;
    return the_stream;
}

void CommandParser::showCommands(std::ostream &the_stream) const {
    // copy command strings to stream
    copy(m_commandfactorys.begin(),
         m_commandfactorys.end(),
         ostream_iterator<CommandFactoryMap::value_type>(the_stream, "\n"));
}

FbTk::Command *CommandParser::toCommand(const std::string &command_str, const std::string &arguments) {
    if (m_commandfactorys[command_str] != 0)
        return m_commandfactorys[command_str]->stringToCommand(command_str, arguments);

    return 0;
}

void CommandParser::associateCommand(const std::string &command, CommandFactory &factory) {
    // we shouldnt override other commands
    if (m_commandfactorys[command] != 0)
        return;

    m_commandfactorys[command] = &factory;
}

void CommandParser::removeAssociation(CommandFactory &factory) {
    std::vector<std::string> commands; // commands that are associated with the factory
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
}
