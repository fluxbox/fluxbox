// Resource.cc
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

// $Id: Resource.cc,v 1.3 2002/07/20 09:51:26 fluxgen Exp $

#include "Resource.hh"
#include "XrmDatabaseHelper.hh"

#include <iostream>
#include <cassert>

using namespace std;

bool ResourceManager::m_init = false;

//-------- load -----------
// loads a resourcefile 
// returns true on success
// else false
//-------------------------
bool ResourceManager::load(const char *filename) {
	assert(filename);

	ensureXrmIsInitialize();
	
	XrmDatabaseHelper database;
	database = XrmGetFileDatabase(filename);
	if (database==0)
		return false;
	
	XrmValue value;
	char *value_type;
	
	//get list and go throu all the resources and load them
	ResourceList::iterator i = m_resourcelist.begin();
	ResourceList::iterator i_end = m_resourcelist.end();	
	for (; i != i_end; ++i) {
	
		Resource_base *resource = *i;
		if (XrmGetResource(*database, resource->name().c_str(),
				resource->altName().c_str(), &value_type, &value))			
			resource->setFromString(value.addr);
		else {
			cerr<<"Failed to read: "<<resource->name()<<endl;
			cerr<<"Setting default value"<<endl;
			resource->setDefaultValue();
		}
	}

	return true;
}

//-------------- save -----------------
// Saves all the resource to a file
// returns 0 on success
// else negative value representing
// the error
//-------------------------------------
bool ResourceManager::save(const char *filename, const char *mergefilename) {
	assert(filename);
	
	ensureXrmIsInitialize();

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
		XrmDatabaseHelper olddatabase(mergefilename);
		if (olddatabase == 0) // did we load the file?
			return false;
		
		XrmMergeDatabases(*database, &*olddatabase); // merge databases
		XrmPutFileDatabase(*olddatabase, filename); // save database to file
		
		*database = 0; // don't try to destroy the database
	} else // save database to file
		XrmPutFileDatabase(*database, filename);

	return true;
}

void ResourceManager::ensureXrmIsInitialize() {
	if (!m_init) {
		XrmInitialize();
		m_init = true;
	}
}
	
