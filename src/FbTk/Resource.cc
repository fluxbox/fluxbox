// Resource.cc
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: Resource.cc,v 1.4 2003/12/19 18:25:39 fluxgen Exp $

#include "XrmDatabaseHelper.hh"
#include "Resource.hh"

#include <iostream>
#include <cassert>

using namespace std;

namespace FbTk {

ResourceManager::ResourceManager(const char *filename, bool lock_db) :
 m_db_lock(0),
 m_database(0),
 m_filename(filename)
{
    ensureXrmIsInitialize();

    if (lock_db)
        lock();
}

ResourceManager::~ResourceManager() {
    if (m_database)
        delete m_database;
}

bool ResourceManager::m_init = false;

/**
  reloads all resources from resourcefile 
  @return true on success else false
*/
bool ResourceManager::load(const char *filename) {
    m_filename = filename;

    // force reload (lock will ensure it exists)
    if (m_database) {
        delete m_database;
        m_database = 0;
    }

    lock();
    if (!m_database) {
        unlock();
        return false;
    }
        
    XrmValue value;
    char *value_type;
	
    //get list and go throu all the resources and load them
    ResourceList::iterator i = m_resourcelist.begin();
    ResourceList::iterator i_end = m_resourcelist.end();	
    for (; i != i_end; ++i) {
	
        Resource_base *resource = *i;
        if (XrmGetResource(**m_database, resource->name().c_str(),
                           resource->altName().c_str(), &value_type, &value))			
            resource->setFromString(value.addr);
        else {
            cerr<<"Failed to read: "<<resource->name()<<endl;
            cerr<<"Setting default value"<<endl;
            resource->setDefaultValue();
        }
    }

    unlock();

    return true;
}

/**
 Saves all the resource to a file
 @return 0 on success  else negative value representing the error
*/
bool ResourceManager::save(const char *filename, const char *mergefilename) {
    assert(filename);
    
    // empty database
    XrmDatabaseHelper database;

    string rc_string;	
    ResourceList::iterator i = m_resourcelist.begin();
    ResourceList::iterator i_end = m_resourcelist.end();
    //write all resources to database
    for (; i != i_end; ++i) {
        Resource_base *resource = *i;
        rc_string = resource->name() + string(": ") + resource->getString();
        XrmPutLineResource(&*database, rc_string.c_str());
    }

    if (database==0)
        return false;

    //check if we want to merge a database
    if (mergefilename) {
        // force reload of file
        m_filename = mergefilename;
        if (m_database) 
            delete m_database;
        m_database = 0;

        lock();

        if (!m_database) {
            unlock();
            return false;
        }

        XrmMergeDatabases(*database, &**m_database); // merge databases
        XrmPutFileDatabase(**m_database, filename); // save database to file

        // don't try to destroy the database (XrmMergeDatabases destroys it)
        *database = 0;
        unlock();
    } else // save database to file
        XrmPutFileDatabase(*database, filename);

    m_filename = filename;
    return true;
}

Resource_base *ResourceManager::findResource(const std::string &resname) {
   // find resource name
    ResourceList::iterator i = m_resourcelist.begin();
    ResourceList::iterator i_end = m_resourcelist.end();
    for (; i != i_end; ++i) {
        if ((*i)->name() == resname ||
            (*i)->altName() == resname) 
            return *i;
    }
    return 0;
}

string ResourceManager::resourceValue(const std::string &resname) {
    Resource_base *res = findResource(resname);
    if (res != 0)
        return res->getString();

    return "";
}

void ResourceManager::setResourceValue(const std::string &resname, const std::string &value) {
    Resource_base *res = findResource(resname);
    if (res != 0)
        res->setFromString(value.c_str());

}

void ResourceManager::ensureXrmIsInitialize() {
    if (!m_init) {
        XrmInitialize();
        m_init = true;
    }
}

ResourceManager &ResourceManager::lock() {
    ++m_db_lock;
    // if the lock was zero, then load the database
    if ((m_db_lock == 1 || !m_database) &&
        m_filename != "") {
        m_database = new XrmDatabaseHelper(m_filename.c_str());

        // check that the database loaded ok
        if (m_database && *m_database == 0) {
            // didn't work
            delete m_database;
            m_database = 0;
        }
    }

    return *this;
}

void ResourceManager::unlock() {
    if (--m_db_lock == 0 && m_database) {
        delete m_database;
        m_database = 0;
    }
}

}; // end namespace FbTk
