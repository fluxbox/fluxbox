// StringUtil.cc for fluxbox 
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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

#include "StringUtil.hh"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <memory>

using namespace std;

//------- strdup ------------------------
//TODO: comment this
//----------------------------------------
char *StringUtil::strdup(const char *s) {
  int l = strlen(s) + 1;
  char *n = new char[l];
  strncpy(n, s, l);
  return n;
}

//------- strcasestr --------------
// Tries to find a string in another and
// ignoring the case of the characters
// Returns 0 on success else pointer to str.
// TODO: comment this
//---------------------------------
const char * StringUtil::strcasestr(const char *str, const char *ptn) {
	const char *s2, *p2;
	for( ; *str; str++) {
		for(s2=str, p2=ptn; ; s2++,p2++) {	
			if (!*p2) return str;
			if (toupper(*s2) != toupper(*p2)) break;
		}
	}
	return 0;
}

//------------- expandFilename ----------------------
// if ~ then expand it to home of user
// returns expanded filename 
// (note: the function creates new memory for the string)
//---------------------------------------------------
char *StringUtil::expandFilename(const char *filename) {
  
	auto_ptr<char> retval( new char[strlen(filename)+strlen(getenv("HOME"))+2]);
  if (filename[0]=='~') {
    strcat(retval.get(), getenv("HOME"));
    strcat(retval.get(), &filename[1]);
  } else
    return StringUtil::strdup(filename);	//return unmodified value
  
  return StringUtil::strdup(retval.get());	//return modified value
}
