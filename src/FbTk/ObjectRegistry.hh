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

#ifndef OBJECTREGISTRY_HH
#define OBJECTREGISTRY_HH

#include <map>
#include <string>

namespace FbTk {

template <typename Creator>
class ObjectRegistry {
public:
    typedef std::map<std::string, Creator> CreatorMap;

    static ObjectRegistry<Creator> &instance() {
        static ObjectRegistry<Creator> s_instance;
        return s_instance;
    }

    Creator lookup(const std::string &name) {
        typename CreatorMap::const_iterator it = m_creators.find(name);
        if (it == m_creators.end())
            return 0;
        return it->second;
    }

    bool registerObject(const std::string &name, Creator creator) {
        m_creators[name] = creator;
        return true;
    }

    const CreatorMap creatorMap() const { return m_creators; }

private:
    ObjectRegistry() {}
    ~ObjectRegistry() {}

    CreatorMap m_creators;
};

} // end namespace FbTk

#endif // OBJECTREGISTRY_HH
