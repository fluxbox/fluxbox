// StringUtiltest.cc
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

#include "../StringUtil.hh"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif //_GNU_SOURCE

#include <vector>
#include <iostream>
#include <memory>

#ifdef UDS
#include <uds/init.hh>
#include <uds/uds.hh>
// configure UDS
uds::uds_flags_t uds::flags = uds::leak_check|uds::log_allocs;
#endif

using namespace std;

void testStringtok() {	
	vector<string> ls;
	StringUtil::stringtok(ls, "   arg1   arg2   \targ3\n  arg4 arg5\t\t\t\targ6\n\n \n\n \t\t\narg7");	
	cerr<<"Size:  "<<ls.size()<<". Should be: 7."<<endl;
	for (vector<string>::const_iterator i = ls.begin();
        i != ls.end(); ++i) {
		cerr << ':' << (*i) << ":\n";
	}
}

void testExpandFilename() {
	auto_ptr<char> filename(StringUtil::expandFilename("~/filename/~filename2/file3~/file4"));
	cerr<<"test ";
	string test = string(getenv("HOME"))+"/filename/~filename2/file3~/file4";
	if (strcmp(test.c_str(), filename.get())==0)
		cerr<<"ok.";
	else
		cerr<<"faild";
	cerr<<endl;
}

void testStrcasestr() {
	cerr<<"test1 ";
	if (StringUtil::strcasestr("Test", "TEST") == strcasestr("Test", "TEST"))
		cerr<<"ok."<<endl;
	else
		cerr<<"faild."<<endl;

	cerr<<"test2 ";
	if (StringUtil::strcasestr("Test", "ESTabc") == strcasestr("Test", "ESTabc"))
		cerr<<"ok."<<endl;
	else
		cerr<<"faild."<<endl;
	
	cerr<<"test3 ";
	if (StringUtil::strcasestr("TeSt", "abcTEStabc") == strcasestr("TeSt", "abcTEStabc"))
		cerr<<"ok."<<endl;
	else
		cerr<<"faild."<<endl;

	cerr<<"test4 ";
	if (StringUtil::strcasestr("TEST", "_TEST;_") == strcasestr("TEST", "_TEST;_"))
		cerr<<"ok."<<endl;
	else
		cerr<<"faild."<<endl;

}

void showError(int line, int pos, string& instr) {
	
	cerr<<"Error on line: "<<line<<endl;
	cerr<<instr<<endl;
	for (int c=0; c<pos; c++) {
		if (instr[c]=='\t')
			cerr<<'\t';
		else
			cerr<<" ";
	}
	cerr<<"^ here"<<endl;	
	
}

void testGetStringBetween() {
	string out;
	vector<string> stringlist;
	stringlist.push_back(" \t\t\t   \t[(in \\)\t haha )]  \t\t ");
	stringlist.push_back("(in\\)) {_  _  my_ _}");
	stringlist.push_back("(in) {_  _  my_ _}");
	stringlist.push_back("(in){_  _  my_ _}");	
	stringlist.push_back("\t      \t \t (    in     )    {haha}");
	stringlist.push_back("\t      \t \t (( 	in  \\) )  {haha}");
	stringlist.push_back("\t      \t \t (( 	in  \\) ){hihi}");
	stringlist.push_back("\t      \t \t (( 	in  \\) )|{hihi}");
	for (unsigned int i=0; i<stringlist.size(); i++) {
		int pos = StringUtil::getStringBetween(out, stringlist[i].c_str(), '(', ')');
		int total_pos = 0;
		if (pos<0) {
			showError(i+1, -pos, stringlist[i]);
			continue;
		}
		cerr<<"string="<<stringlist[i]<<endl;
		cerr<<"pos="<<pos<<" ::"<<out;
		total_pos += pos;
		pos = StringUtil::getStringBetween(out, stringlist[i].c_str()+total_pos, '{', '}');				
		if (pos<=0) {
			pos=-pos;
			showError(i+1, total_pos+pos, stringlist[i]);
			continue;
		} 
		cerr<<"::"<<out<<"::"<<endl;
		total_pos += pos;
	}
}
int main() {	
	#ifdef UDS
	uds::Init uds_init;
	#endif
	cerr<<"Testing stringtok."<<endl;	
	testStringtok();
	cerr<<"Testing expandFilename."<<endl;
	testExpandFilename();
	cerr<<"Testing strcasestr."<<endl;
	testStrcasestr();
}
