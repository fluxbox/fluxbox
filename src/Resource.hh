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

// $Id: Resource.hh,v 1.5 2002/05/17 10:59:58 fluxgen Exp $

#ifndef RESOURCE_HH
#define RESOURCE_HH

#include "NotCopyable.hh"
#include <string>
#include <list>

class Resource_base:private NotCopyable
{
public:

	virtual void setFromString(char const *strval)=0;
	virtual void setDefaultValue()=0;
	
	virtual std::string getString()=0;
	inline const std::string& getAltName() const { return m_altname; }
	inline const std::string& getName() const { return m_name; }
	
protected:	
	Resource_base(const std::string &name, const std::string &altname):
	m_name(name), m_altname(altname)
	{
	
	}
	virtual ~Resource_base(){	};
private:
	std::string m_name, m_altname;
};

class ResourceManager;

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
		m_rm.addResource(*this);
	}
	~Resource() {
		m_rm.removeResource(*this);
	}

	inline void setDefaultValue() {  m_value = m_defaultval; }
	void setFromString(const char *strval);
	inline Resource<T>& operator = (const T& newvalue) { m_value = newvalue;  return *this;}
	
	std::string getString();	
	inline T& operator*(void) { return m_value; }
	inline const T& operator*(void) const { return m_value; }
	inline T *operator->(void) { return &m_value; }
	inline const T *operator->(void) const { return &m_value; }
private:
	T m_value, m_defaultval;
	ResourceManager &m_rm;
};

class ResourceManager
{
public:
	typedef std::list<Resource_base *> ResourceList;

	ResourceManager(){ }	
	
	bool load(const char *filename);
	bool save(const char *filename, const char *mergefilename=0);
	template <class T>
	void addResource(Resource<T> &r) {
		m_resourcelist.push_back(&r);
		m_resourcelist.unique();
	}
	template <class T>
	void removeResource(Resource<T> &r) {
		m_resourcelist.remove(&r);
	}
private:
	static inline void ensureXrmIsInitialize();
	static bool m_init;
	ResourceList m_resourcelist;

};

#endif //_RESOURCE_HH_
