// CommandParser.hh for Fluxbox - an X11 Window manager
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

// $Id: CommandParser.hh,v 1.3 2004/01/02 13:43:58 fluxgen Exp $

#ifndef COMMANDPARSER_HH
#define COMMANDPARSER_HH

#include <string>
#include <map>

#include "RefCount.hh"

namespace FbTk {
class Command;
};

/// Creates commands from command and argument.
/// Used for modules to add new commands in compile/run time
class CommandFactory {
public:
    CommandFactory();
    virtual ~CommandFactory();
    virtual FbTk::Command *stringToCommand(const std::string &command, 
                                           const std::string &arguments) = 0;
protected:
    void addCommand(const std::string &value);
};

/// Parses text into a command
class CommandParser {
public:
    typedef std::map<std::string, CommandFactory *> CommandFactoryMap;

    /// @return parses and returns a command matching the line
    FbTk::Command *parseLine(const std::string &line);

    /// @return instance of command parser
    static CommandParser &instance();
    /// @return map of factorys
    const CommandFactoryMap &factorys() const { return m_commandfactorys; }
private:
    // so CommandFactory can associate it's commands
    friend class CommandFactory;
    /// associate a command with a factory
    void associateCommand(const std::string &name, CommandFactory &factory);
    /// remove all associations with the factory
    void removeAssociation(CommandFactory &factory);

    /// search for a command in our command factory map
    FbTk::Command *toCommand(const std::string &command,
                             const std::string &arguments);
    
    CommandFactoryMap m_commandfactorys; ///< a string to factory map
    
};

#endif // COMMANDPARSER_HH
