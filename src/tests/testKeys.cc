// testKeys.cc
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

#include "../Keys.hh"
#include <iostream>
#include <X11/Xlib.h>
#include <uds/init.hh>

#ifdef UDS
#include <uds/uds.hh>
// configure UDS
uds::uds_flags_t uds::flags = uds::leak_check|uds::log_allocs;
#endif

using namespace std;

void testKeys(int argc, char **argv) {
	Display *display = XOpenDisplay(0);
	
	if (display==0) {
		cerr<<"Cant open display."<<endl;
		return;
	}
	
	Keys *keys = new Keys(display);
	const char default_keyfile[] = "keys";
	
	if (argc>1) {
		cerr<<"Loading file: "<<argv[1]<<endl;
		keys->load(const_cast<char *>(argv[1]));
	} else {
		cerr<<"Using default file: "<<default_keyfile<<endl;
		keys->load(const_cast<char *>(default_keyfile));
	}
	
	keys->load(const_cast<char *>(default_keyfile));
	
	delete keys;
		
	XCloseDisplay(display);
}

int main(int argc, char **argv) {
	#ifdef UDS
	uds::Init uds_init;
	#endif
	testKeys(argc, argv);	
}
