// Resourcetest.cc
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "Resource.hh"

#include <string>
#include <iostream>
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

using namespace std;

enum TestEnum{TEST1, THIS_IS_TRUE, USE_LOVE};

//----------------
// Manipulators
//-----------------

template<>
void Resource<TestEnum>::
setFromString(const char *str) {
    if (strcasecmp(str, "TEST1")==0)
        *this = TEST1;
    else if (strcasecmp(str, "THIS_IS_TRUE")==0)
        *this = THIS_IS_TRUE;
    else if (strcasecmp(str, "USE_LOVE")==0)
        *this = USE_LOVE;
}

template<>
void Resource<int>::
setFromString(const char* strval) {
    int val;
    if (sscanf(strval, "%d", &val)==1)
        *this = val;
}

template<>
void Resource<std::string>::
setFromString(const char *strval) {
    *this = strval;
}

template<>
void Resource<bool>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "true")==0)
        *this = true;
    else
        *this = false;
}

//-----------------
// accessors 
//-----------------
template<>
std::string Resource<TestEnum>::
getString() {
    switch (m_value) {
    case TEST1:
        return string("TEST1");
    case THIS_IS_TRUE:
        return string("THIS_IS_TRUE");
    case USE_LOVE:
        return string("USE_LOVE");
    }
    return string("");
}

template<>
std::string Resource<bool>::
getString() {				
    return std::string(**this == true ? "true" : "false");
}

template<>
std::string Resource<int>::
getString() {
    char strval[256];
    sprintf(strval, "%d", **this);
    return std::string(strval);
}

template<>
std::string Resource<string>::
getString() { return **this; }

int main(int argc, char **argv) {
	
    ResourceManager rm;
    // resources
    Resource<int> val(rm, 123, "session.test", "Session.Test");
    Resource<bool> boolval(rm, true, "session.bool", "Session.Bool");
    Resource<string> strval(rm, "none", "string", "String");
    Resource<TestEnum> enumval(rm, TEST1, "enumval", "EnumVal");

    const char *defaultfile_open = "res", 
        *defaultfile_save = "res_save";

    if (argc>1) {
        if(!rm.load(argv[1]))
            cerr<<"Faild to load database: "<<argv[1]<<endl;
    } else {
        if (!rm.load(defaultfile_open))
            cerr<<"Faild to load database: "<<defaultfile_open<<endl;
    }
    cerr<<"Value="<<*val<<endl;
    cerr<<"boolValue="<<boolval.getString()<<endl;
    cerr<<"strValue="<<*strval<<endl;
    cerr<<"enumValue="<<enumval.getString()<<endl;
	
    if (!rm.save(defaultfile_save))
        cerr<<"Faild to save database to file:"<<defaultfile_save<<endl;

    if (!rm.save("I dont exist", "Not me either"))
        cerr<<"faild to save and merge database."<<endl;
    return 0;
}
