// ObjectRegistry.hh for FbTk
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

#ifndef OBJECTREGISTRY_HH
#define OBJECTREGISTRY_HH

#include "StringUtil.hh"

#include <string>
#include <map>

using std::string;

namespace FbTk {

#define REGISTER_OBJECT_PARSER(name, parser, type) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<type>::instance().registerObject(#name, parser); \
  }

#define REGISTER_OBJECT_PARSER_NSBASE(name, parser, space, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<space::base >::instance().registerObject(#name, parser); \
  }

/* Include some basic Object creators */
template <typename Derived, typename Base>
Base *ObjectCreator(const string &name, const string &args, bool trusted) {
    return new Derived();
}

#define REGISTER_OBJECT(name, derived, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<base >::instance().registerObject(#name, FbTk::ObjectCreator<derived, base >); \
  }

#define REGISTER_OBJECT_NSBASE(name, derived, space, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<space::base >::instance().registerObject(#name, FbTk::ObjectCreator<derived, space::base >); \
  }

template <typename Derived, typename Base>
Base *ObjectCreatorWithArgs(const string &name, const string &args,
                            bool trusted) {
    return new Derived(args);
}

#define REGISTER_OBJECT_WITH_ARGS(name, derived, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<base >::instance().registerObject(#name, FbTk::ObjectCreatorWithArgs<derived, base >); \
  }

#define REGISTER_OBJECT_WITH_ARGS_NSBASE(name, derived, space, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<space::base >::instance().registerObject(#name, FbTk::ObjectCreatorWithArgs<derived, space::base >); \
  }

template <typename Derived, typename Base>
Base *UntrustedObjectCreator(const string &name, const string &args,
                             bool trusted) {
    if (!trusted) return 0;
    return new Derived();
}

#define REGISTER_UNTRUSTED_OBJECT(name, derived, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<base >::instance().registerObject(#name, FbTk::UntrustedObjectCreator<derived, base >); \
  }

#define REGISTER_UNTRUSTED_OBJECT_NSBASE(name, derived, space, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<space::base >::instance().registerObject(#name, FbTk::UntrustedObjectCreator<derived, space::base >); \
  }

template <typename Derived, typename Base>
Base * UntrustedObjectCreatorWithArgs(const string &name, const string &args,
                                      bool trusted) {
    if (!trusted) return 0;
    return new Derived(args);
}

#define REGISTER_UNTRUSTED_OBJECT_WITH_ARGS(name, derived, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<base >::instance().registerObject(#name, FbTk::UntrustedObjectCreatorWithArgs<derived, base >); \
  }

#define REGISTER_UNTRUSTED_OBJECT_WITH_ARGS_NSBASE(name, derived, space, base) \
  namespace { \
    static const bool p_register_##type_##name = FbTk::ObjectRegistry<space::base >::instance().registerObject(#name, FbTk::UntrustedObjectCreatorWithArgs<derived, space::base >); \
  }

template <typename Base>
class ObjectRegistry {
public:
    typedef Base * CreateFunction(const string &name, const string &args, bool trusted);
    typedef std::map<string, CreateFunction *> CreatorMap;

    static ObjectRegistry<Base > &instance() {
        static ObjectRegistry<Base > s_instance;
        return s_instance;
    }

    Base *parse(const string &name, const string &args, bool trusted = true) const {
        string lc = StringUtil::toLower(name);
        typename CreatorMap::const_iterator it = m_creators.find(lc.c_str());
        if (it == m_creators.end())
            return 0;
        else
            return it->second(lc, args, trusted);
    }

    Base *parse(const string &line, bool trusted = true) const {
        // parse args and command
        string command, args;
        StringUtil::getFirstWord(line, command, args);
        StringUtil::removeFirstWhitespace(args);
        StringUtil::removeTrailingWhitespace(args);

        // now we have parsed command and args
        return parse(command, args, trusted);
    }

    bool registerObject(string name, CreateFunction createFunction) {
        name = StringUtil::toLower(name);
        m_creators[name] = createFunction;
        return true;
    }

    const CreatorMap creatorMap() const { return m_creators; }

private:
    ObjectRegistry() {}
    ~ObjectRegistry() {}

    CreatorMap m_creators;
};

}; // end namespace FbTk

#endif // OBJECTREGISTRY_HH
