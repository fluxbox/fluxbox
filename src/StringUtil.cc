// StringUtil.cc for fluxbox 
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: StringUtil.cc,v 1.12 2002/09/15 09:40:51 fluxgen Exp $

#include "StringUtil.hh"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <memory>

using namespace std;

namespace StringUtil
{

//------- strdup ------------------------
// Takes a pointer to string *s as an argument,
// creates a new string n, copies s to n and
// returns a pointer to n.
//----------------------------------------
char *strdup(const char *s) {
  int l = strlen(s) + 1;
  char *n = new char[l];
  strncpy(n, s, l);
  return n;
}

//------- strcasestr --------------
// Tries to find a string in another and
// ignoring the case of the characters
// Returns 0 on success else pointer to str.
//---------------------------------
const char *strcasestr(const char *str, const char *ptn) {
	const char *s2, *p2;
	for( ; *str; str++) {
		for(s2=str, p2=ptn; ; s2++,p2++) {	
			if (!*p2) return str; // check if we reached the end of ptn, if so, return str
			if (toupper(*s2) != toupper(*p2)) break; // check if the chars match(ignoring case)
		}
	}
	return 0;
}

//------------- expandFilename ----------------------
// if ~ then expand it to home of user
// returns expanded filename 
//---------------------------------------------------
string expandFilename(const std::string &filename) {
  
	string retval;
	size_t pos = filename.find_first_not_of(" \t");
	if (pos != std::string::npos && filename[pos] == '~') {  	
    	retval = getenv("HOME");
		if (pos != filename.size()) {
			// copy from the character after '~'
			retval += static_cast<const char *>(filename.c_str() + pos + 1);
		}
	} else
		return filename; //return unmodified value
  
	return retval;
}

//------------- getStringBetween -----------
// Parses a string between "first" and "last" characters
// and ignoring ok_chars as whitespaces. The value is
// returned in "out".
// Returns negative value on error and this value is the position 
// in the in-string where the error occured.
// Returns positive value on success and this value is
// for the position + 1 in the in-string where the "last"-char value
// was found.
//------------------------------------------
int getStringBetween(std::string& out, const char *instr, const char first, const char last,
			const char *ok_chars) {
	assert(first);
	assert(last);
	assert(instr);
	
	std::string::size_type i = 0, 
		total_add=0; //used to add extra if there is a \last to skip
	std::string in(instr);
	
	// eat leading whitespace
	i = in.find_first_not_of(ok_chars);
	if (i == std::string::npos)
		return -in.size();   // nothing left but whitespace

	if (in[i]!=first)		
		return -i; //return position to error	

	// find the end of the token
	std::string::size_type j = i;
	while (1) {
		j = in.find_first_of(last, j+1);
		if (j==std::string::npos)
			return -in.size(); //send negative size

		//we found the last char, check so it doesn't have a '\' before
		if (j>1 && in[j-1] != '\\')	
			break;
		else if (j>1) {
			in.erase(j-1, 1); //remove the '\'
			j--;
			total_add++; //save numchars removed so we can calculate totalpos
		}
	}

	out = in.substr(i+1, j-i-1); //copy the string between first and last		
	//return value to last character
	return (j+1+total_add);
}

void toLower(char * const conv) {
	for (int byte_pos = 0; byte_pos < strlen(conv); ++byte_pos)
		conv[byte_pos] = tolower(conv[byte_pos]);
}

}; //end namespace StringUtil
