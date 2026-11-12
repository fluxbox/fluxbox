// CommandParser.cc for FbTk
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

#include "CommandParser.hh"

namespace FbTk {

template <typename Type>
CommandParser<Type>::CommandParser() {
}

template <typename Type>
CommandParser<Type>::~CommandParser() {
}

template <typename Type>
CommandParser<Type> &CommandParser<Type>::instance() {
    static CommandParser<Type> s_instance;
    return s_instance;
}

template <typename Type>
Command<Type> *CommandParser<Type>::parse(const string &name, const string &args,
                                          bool trusted) const {
    string lc = StringUtil::toLower(name);
    Creator creator = lookup(lc);
    if (creator)
        return creator(lc, args, trusted);
    return 0;
}

template <typename Type>
Command<Type> *CommandParser<Type>::parse(const string &line, bool trusted) const {
    // separate args and command
    string command, args;
    StringUtil::getFirstWord(line, command, args);
    StringUtil::removeFirstWhitespace(args);
    StringUtil::removeTrailingWhitespace(args);

    // now we parse
    return parse(command, args, trusted);
}

template <typename Type>
bool CommandParser<Type>::registerCommand(const char *name, Creator creator) {
    return registerCommand(string(name), creator);
}

template <typename Type>
bool CommandParser<Type>::registerCommand(const string &name, Creator creator) {
    m_creators[StringUtil::toLower(name)] = creator;
    return true;
}

template <typename Type>
typename CommandParser<Type>::Creator
CommandParser<Type>::lookup(const std::string &name) const {
    typename CreatorMap::const_iterator it = m_creators.find(name);
    if (it == m_creators.end())
        return 0;
    return it->second;
}

// keep in sync with the extern templates in CommandParser.hh
template class CommandParser<void>;
template class CommandParser<bool>;

} // end namespace FbTk
