// main.cc for testing menu in fluxbox
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

#include "../StringUtil.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

bool loadMenu(string filename);
bool loadMenu2(string filename);

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

int main(int argc, char **argv) {
    string filename = "menu";
    if (argc>1)
        filename = argv[1];
    if (loadMenu2(filename))
        cout<<"Load successfull"<<endl;
    else 
        cout<<"Load failed"<<endl;

/*
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
*/
    return 0;	
}

			

bool loadMenu2(string filename) {
	
    if (!filename.size())
        return false;
	
    ifstream menufile(filename.c_str());
	
	
    if (menufile) {
        string instr;
        vector<string> args;		
        int line=0;
        while (!menufile.eof()) {
            //read a line
            getline(menufile, instr);			
            line++;
            string arg;			
            int pos = StringUtil::getStringBetween(arg, instr.c_str(), '[', ']');
            if (pos<=0) {
                showError(line, -pos, instr);
                continue;
            }
							
            cerr<<"("<<line<<"):"<<arg<<"::";
            int total_pos = pos;
            pos = StringUtil::getStringBetween(arg, instr.c_str()+pos, '(', ')');			
            if (pos<=0) {
                showError(line, total_pos+(-pos), instr);
                continue;
            }
            cerr<<arg<<"::";
			
            total_pos +=pos;
            pos = StringUtil::getStringBetween(arg, instr.c_str()+total_pos, '{', '}');
            if (pos<=0) {
                total_pos = total_pos+(-pos);
                showError(line, total_pos, instr);
                continue;
            }	
            cerr<<arg<<":"<<endl;
			
        }
		
		
    } else	
        return false;

    return true;
}

