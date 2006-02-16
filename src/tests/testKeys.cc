// testKeys.cc
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

#include <iostream>
#include "../FbTk/App.hh"
#include "../FbTk/KeyUtil.hh"

using namespace std;

void testKeys(int argc, char **argv) {
    FbTk::App app(0);
    if (app.display() == 0) {
        cerr<<"Cant open display."<<endl;
        return;
    }
    std::string theline;
    cout<<"Type key: ";
    getline(cin, theline);
    unsigned int key = FbTk::KeyUtil::getKey(theline.c_str());
    cerr<<"key = "<<key<<endl;
    if (key == NoSymbol)
        cerr<<"NoSymbol"<<endl;
}

int main(int argc, char **argv) {
#ifdef UDS
    uds::Init uds_init;
#endif
    testKeys(argc, argv);	
}
