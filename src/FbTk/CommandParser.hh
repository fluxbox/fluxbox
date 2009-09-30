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

template <typename Type>
class CommandParser {
public:
    typedef Command<Type> *(*Creator)(const string &, const string &, bool);
    typedef std::map<std::string, Creator> CreatorMap;

    static CommandParser<Type> &instance() {
        static CommandParser<Type> s_instance;
        return s_instance;
    }

    Command<Type> *parse(const string &name, const string &args,
                                bool trusted = true) const {
        string lc = StringUtil::toLower(name);
        Creator creator = lookup(lc);
        if (creator)
            return creator(lc, args, trusted);
        return 0;
    }

    Command<Type> *parse(const string &line, bool trusted = true) const {
        // separate args and command
        string command, args;
        StringUtil::getFirstWord(line, command, args);
        StringUtil::removeFirstWhitespace(args);
        StringUtil::removeTrailingWhitespace(args);

        // now we parse
        return parse(command, args, trusted);
    }

    bool registerCommand(string name, Creator creator) {
        name = StringUtil::toLower(name);
        m_creators[name] = creator;
        return true;
    }

    Creator lookup(const std::string &name) const {
        typename CreatorMap::const_iterator it = m_creators.find(name);
        if (it == m_creators.end())
            return 0;
        return it->second;
    }

    const CreatorMap &creatorMap() const { return m_creators; }

private:
    CommandParser() {}
    ~CommandParser() {}

    CreatorMap m_creators;
};

} // end namespace FbTk

#endif // COMMANDPARSER_HH
