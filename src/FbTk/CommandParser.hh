// CommandParser.hh for FbTk
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

#ifndef CommandParser_HH
#define CommandParser_HH

#include "Command.hh"
#include "StringUtil.hh"

#include <string>
#include <map>

using std::string;

namespace FbTk {

// helper for registering a function to parse arguments
#define REGISTER_COMMAND_PARSER(name, parser, type) \
    static const bool p_register_command_##type_##name = FbTk::CommandParser<type>::instance().registerCommand(#name, &parser)

// include some basic Command<void> creators
template <typename ClassName, typename Type>
Command<Type> *CommandCreator(const string &name, const string &args,
                              bool trusted) {
    return new ClassName();
}

#define REGISTER_COMMAND(name, classname, type) \
    static const bool p_register_##type_##name = FbTk::CommandParser<type>::instance().registerCommand(#name, &FbTk::CommandCreator<classname, type>)

template <typename ClassName, typename Type>
Command<Type> *CommandCreatorWithArgs(const string &name, const string &args,
                                      bool trusted) {
    return new ClassName(args);
}

#define REGISTER_COMMAND_WITH_ARGS(name, classname, type) \
    static const bool p_register_##type_##name = FbTk::CommandParser<type>::instance().registerCommand(#name, &FbTk::CommandCreatorWithArgs<classname, type>)

template <typename ClassName, typename Type>
Command<Type> *UntrustedCommandCreator(const string &name, const string &args,
                                       bool trusted) {
    if (!trusted) return 0;
    return new ClassName();
}

#define REGISTER_UNTRUSTED_COMMAND(name, classname, type) \
    static const bool p_register_##type_##name = FbTk::CommandParser<type>::instance().registerCommand(#name, &FbTk::UntrustedCommandCreator<classname, type>)

template <typename ClassName, typename Type>
Command<Type> *UntrustedCommandCreatorWithArgs(const string &name,
         const string &args, bool trusted) {
    if (!trusted) return 0;
    return new ClassName(args);
}

#define REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(name, classname, type) \
    static const bool p_register_##type_##name = FbTk::CommandParser<type>::instance().registerCommand(#name, &FbTk::UntrustedCommandCreatorWithArgs<classname, type>)

/// The member functions are defined in CommandParser.cc and compiled there
/// once for each Type below (see the extern templates), instead of in every
/// file that registers or parses a command.
template <typename Type>
class CommandParser {
public:
    typedef Command<Type> *(*Creator)(const string &, const string &, bool);
    typedef std::map<std::string, Creator> CreatorMap;

    static CommandParser<Type> &instance();

    Command<Type> *parse(const string &name, const string &args,
                         bool trusted = true) const;

    Command<Type> *parse(const string &line, bool trusted = true) const;

    /// for the REGISTER_* macros: takes the literal, no std::string temporary
    bool registerCommand(const char *name, Creator creator);
    bool registerCommand(const std::string &name, Creator creator);

    Creator lookup(const std::string &name) const;

    const CreatorMap &creatorMap() const { return m_creators; }

private:
    CommandParser();
    ~CommandParser();

    CreatorMap m_creators;
};

// the only instantiations; add a line here and in CommandParser.cc for a new Type
extern template class CommandParser<void>;
extern template class CommandParser<bool>;

} // end namespace FbTk

#endif // COMMANDPARSER_HH
