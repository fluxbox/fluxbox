// main.cc for FbRun
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

// $Id: main.cc,v 1.1 2002/08/20 02:04:34 fluxgen Exp $

#include "FbRun.hh"

#include <string>
#include <iostream>
using namespace std;

void showUsage(const char *progname) {
	cerr<<"fbrun 1.1.0 : (c) 2002 Henrik Kinnunen"<<endl;
	cerr<<"Usage: "<<
		progname<<" [arguments]"<<endl<<
		"Arguments: "<<endl<<
		"   -font [font name]           Text font"<<endl<<
		"   -title [title name]         Set title"<<endl<<
		"   -text [text]                Text input"<<endl<<
		"   -w [width]                  Window width in pixels"<<endl<<
		"   -h [height]                 Window height in pixels"<<endl<<
		"   -display [display string]   Display name"<<endl<<
		"   -pos [x] [y]                Window position in pixels"<<endl<<
		"   -fg [color name]            Foreground text color"<<endl<<
		"   -bg [color name]            Background color"<<endl<<
		"   -help                       Show this help"<<endl<<endl<<
		"Example: fbrun -fg black -bg white -text xterm -title \"run xterm\""<<endl;
}

int main(int argc, char **argv) {
	int x = 0, y = 0; // default pos of window
	size_t width = 200, height = 32; // default size of window
	bool set_height = false; // use height of font by default
	bool set_pos = false; // set position
	string fontname; // font name
	string title("Run program"); // default title
	string text;         // default input text
	string foreground("black");   // text color
	string background("white");   // text background color
	string display_name; // name of the display connection

	// parse arguments
	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "-font") == 0 && i+1 < argc) {
			++i;
			fontname = argv[i];
		} else if (strcmp(argv[i], "-title") == 0 && i+1 < argc) {
			++i;
			title = argv[i];
		} else if (strcmp(argv[i], "-text") == 0 && i+1 < argc) {
			++i;
			text = argv[i];
		} else if (strcmp(argv[i], "-w") == 0 && i+1 < argc) {
			++i;
			width = atoi(argv[i]);			
		} else if (strcmp(argv[i], "-h") == 0 && i+1 < argc) {
			++i;
			height = atoi(argv[i]);
			set_height = true; // mark true else the height of font will be used
		} else if (strcmp(argv[i], "-display") == 0 && i+1 < argc) {
			++i;
			display_name = argv[i];
		} else if (strcmp(argv[i], "-pos") == 0 && i+2 < argc) {
			++i;
			x = atoi(argv[i]);
			++i;
			y = atoi(argv[i]);
			set_pos = true;
		} else if (strcmp(argv[i], "-fg") == 0 && i+1 < argc) {
			++i;
			foreground = argv[i];
		} else if (strcmp(argv[i], "-bg") == 0 && i+1 < argc) {
			++i;
			background = argv[i];
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
			showUsage(argv[0]);
			exit(0);
		} else {
			cerr<<"Invalid argument: "<<argv[i]<<endl;
			showUsage(argv[0]);
			exit(0);
		}

	}

	try {

		Display *disp = 0;

		// establish display connection
		disp = XOpenDisplay(display_name.c_str());
		if (disp == 0)
			throw string("Can't open display: " + display_name);
		
		FbRun fbrun(disp);
		if (fontname.size() != 0) {
			if (!fbrun.loadFont(fontname.c_str())) {
				cerr<<"Failed to load font: "<<fontname<<endl;
				cerr<<"Falling back to \"fixed\""<<endl;
			}
		}

		// get color
		XColor xc_foreground, xc_background;
		if (XParseColor(disp, DefaultColormap(disp, DefaultScreen(disp)),
			foreground.c_str(),
			&xc_foreground) == 0) {
			cerr<<"Faild to lookup color: "<<foreground<<endl;
		}

		if (XParseColor(disp, DefaultColormap(disp, DefaultScreen(disp)),
			background.c_str(),
			&xc_background) == 0) {
			cerr<<"Faild to lookup color: "<<background<<endl;
		}

		XAllocColor(disp, DefaultColormap(disp, DefaultScreen(disp)),
			&xc_foreground);
		XAllocColor(disp, DefaultColormap(disp, DefaultScreen(disp)),
			&xc_background);
		
		fbrun.setForeground(xc_foreground);
		fbrun.setBackground(xc_background);

		if (set_height)
			fbrun.resize(width, height);
		
		fbrun.setTitle(title);
		fbrun.setText(text);
		fbrun.show();
		
		if (set_pos)
			fbrun.move(x, y);

		XEvent event;
		// main loop
		while (!fbrun.end()) {
			XNextEvent(disp, &event);
			fbrun.handleEvent(&event);
		}

	} catch (string errstr) {
		cerr<<"Error: "<<errstr<<endl;
	}
}
