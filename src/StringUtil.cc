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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//------- strdup ------------------------
//TODO: comment this
//----------------------------------------
char *StringUtil::strdup(const char *s) {
  int l = strlen(s) + 1;
  char *n = new char[l];
  strncpy(n, s, l);
  return n;
}


//------------- expandFilename ----------------------
// if ~ then expand it to home of user
// returns expanded filename 
// (note: the function creates new memory for the string)
//---------------------------------------------------
char *StringUtil::expandFilename(const char *filename) {
  
	char retval[strlen(filename)+strlen(getenv("HOME"))+2];
  retval[0]=0; //mark end
  if (filename[0]=='~') {
    strcat(retval, getenv("HOME"));
    strcat(retval, &filename[1]);
  } else
    return StringUtil::strdup(filename);	//return unmodified value
  
  return StringUtil::strdup(retval);	//return modified value
}
