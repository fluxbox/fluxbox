// Resource.hh
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: Resource.hh,v 1.9 2002/12/01 13:41:58 rathnor Exp $

#ifndef RESOURCE_HH
#define RESOURCE_HH

#include "NotCopyable.hh"
#include <string>
#include <list>
/**
	Base class for resources
*/
class Resource_base:private FbTk::NotCopyable
{
public:
    virtual ~Resource_base() { };	
	
    /// set from string value
    virtual void setFromString(char const *strval) = 0;
    /// set default value
    virtual void setDefaultValue() = 0;
    /// get string value
    virtual std::string getString() = 0;
    /// get alternative name of this resource
    inline const std::string& altName() const { return m_altname; }
    /// get name of this resource
    inline const std::string& name() const { return m_name; }

protected:	
    Resource_base(const std::string &name, const std::string &altname):
	m_name(name), m_altname(altname)
	{ }

private:
    std::string m_name; ///< name of this resource
    std::string m_altname; ///< alternative name 
};

class ResourceManager;

/**
	Real resource class
*/
template <typename T>
class Resource:public Resource_base
{
public:	
    Resource(ResourceManager &rm, T val, 
             const std::string &name, const std::string &altname):
	Resource_base(name, altname),
	m_value(val), m_defaultval(val),
	m_rm(rm)
	{
            m_rm.addResource(*this); // add this to resource handler
	}
    virtual ~Resource() {
        m_rm.removeResource(*this); // remove this from resource handler
    }

    inline void setDefaultValue() {  m_value = m_defaultval; }
    void setFromString(const char *strval);
    inline Resource<T>& operator = (const T& newvalue) { m_value = newvalue;  return *this;}
	
    std::string getString();	
    inline T& operator*() { return m_value; }
    inline const T& operator*() const { return m_value; }
    inline T *operator->() { return &m_value; }
    inline const T *operator->() const { return &m_value; }
private:
    T m_value, m_defaultval;
    ResourceManager &m_rm;
};

class ResourceManager
{
public:
    typedef std::list<Resource_base *> ResourceList;

    ResourceManager() { }
    virtual ~ResourceManager() {}
    /**
       load all resouces registered to this class
    */
    virtual bool load(const char *filename);
    /**
       save all resouces registered to this class
    */
    virtual bool save(const char *filename, const char *mergefilename=0);
    /**
       add resource to list
    */
    template <class T>
    void addResource(Resource<T> &r) {
        m_resourcelist.push_back(&r);
        m_resourcelist.unique();
    }
    /**
       Remove a specific resource
    */
    template <class T>
    void removeResource(Resource<T> &r) {
        m_resourcelist.remove(&r);
    }
protected:
    static inline void ensureXrmIsInitialize();
private:

    static bool m_init;
    ResourceList m_resourcelist;
};

#endif //_RESOURCE_HH_
