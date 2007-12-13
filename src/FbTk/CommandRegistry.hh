// CommandRegistry.hh for FbTk
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

#ifndef COMMANDREGISTRY_HH
#define COMMANDREGISTRY_HH

#include <string>
#include <map>

using std::string;

namespace FbTk {
class Command;
class BoolCommand;

#define REGISTER_COMMAND_PARSER(name, parser) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerCommand(#name, parser); \
  }

#define REGISTER_BOOLCOMMAND_PARSER(name, parser) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerBoolCommand(#name, parser); \
  }

/* Include some basic command creators */
template <typename Cmd, typename Ret>
Ret *CommandCreator(const string &name, const string &args, bool trusted) {
    return new Cmd();
}

#define REGISTER_COMMAND(name, classname) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerCommand(#name, FbTk::CommandCreator<classname, FbTk::Command>); \
  }

template <typename Cmd, typename Ret>
Ret *CommandCreatorWithArgs(const string &name, const string &args,
                            bool trusted) {
    return new Cmd(args);
}

#define REGISTER_COMMAND_WITH_ARGS(name, classname) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerCommand(#name, FbTk::CommandCreatorWithArgs<classname, FbTk::Command>); \
  }

#define REGISTER_BOOLCOMMAND_WITH_ARGS(name, classname) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerBoolCommand(#name, FbTk::CommandCreatorWithArgs<classname, FbTk::BoolCommand>); \
  }

template <typename Cmd, typename Ret>
Ret *UntrustedCommandCreator(const string &name, const string &args,
                             bool trusted) {
    if (!trusted) return 0;
    return new Cmd();
}

#define REGISTER_UNTRUSTED_COMMAND(name, classname) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerCommand(#name, FbTk::UntrustedCommandCreator<classname, FbTk::Command>); \
  }

template <typename Cmd, typename Ret>
Ret * UntrustedCommandCreatorWithArgs(const string &name, const string &args,
                                      bool trusted) {
    if (!trusted) return 0;
    return new Cmd(args);
}

#define REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(name, classname) \
  namespace { \
    static const bool p_register_##name = FbTk::CommandRegistry::instance().registerCommand(#name, FbTk::UntrustedCommandCreatorWithArgs<classname, FbTk::Command>); \
  }

/* Not built to be virtual at the moment */
class CommandRegistry {
public:
    typedef Command * CreateFunction(const string &name, const string &args, bool trusted);
    typedef BoolCommand * BoolCreateFunction(const string &name,
                                             const string &args, bool trusted);
    typedef std::map<string, CreateFunction *> CreatorMap;
    typedef std::map<string, BoolCreateFunction *> BoolCreatorMap;

    static CommandRegistry &instance();

    Command *parseLine(const string &line, bool trusted = true) const;
    BoolCommand *parseBoolLine(const string &line, bool trusted = true) const;
    Command *getCommand(const string &name, const string &args, bool trusted) const;
    BoolCommand *getBoolCommand(const string &name, const string &args,
                                bool trusted) const;

    /* Note: the ownership of this object passes to this class */
    bool registerCommand(string name, CreateFunction createFunction);
    bool registerBoolCommand(string name, BoolCreateFunction bcf);

    const CreatorMap commandMap() const { return m_commandCreators; }

private:
    CommandRegistry() {}
    ~CommandRegistry() {}

    CreatorMap m_commandCreators;
    BoolCreatorMap m_boolcommandCreators;
};

}; // end namespace FbTk

#endif // COMMANDREGISTRY_HH
