// main.cc for FbRun
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen(at)linuxmail.org)
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

// $Id: main.cc,v 1.10 2003/08/25 01:30:46 fluxgen Exp $

#include "FbRun.hh"
#include "App.hh"
#include "StringUtil.hh"
#include "Color.hh"

#include <string>
#include <iostream>

using namespace std;

void showUsage(const char *progname) {
    cerr<<"fbrun 1.1.3 : (c) 2002-2003 Henrik Kinnunen"<<endl;
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
        "   -a                          Antialias"<<endl<<
        "   -hf [history file]          History file to load (default ~/.fluxbox/fbrun_history)"<<endl<<
        "   -help                       Show this help"<<endl<<endl<<
        "Example: fbrun -fg black -bg white -text xterm -title \"run xterm\""<<endl;
}

int main(int argc, char **argv) {
    int x = 0, y = 0; // default pos of window
    size_t width = 200, height = 32; // default size of window
    bool set_height = false, set_width=false; // use height/width of font by default
    bool set_pos = false; // set position
    bool antialias = false; // antialias text
    string fontname; // font name
    string title("Run program"); // default title
    string text;         // default input text
    string foreground("black");   // text color
    string background("white");   // text background color
    string display_name; // name of the display connection
    string history_file("~/.fluxbox/fbrun_history"); // command history file
    // parse arguments
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-font") == 0 && i+1 < argc) {
            fontname = argv[++i];
        } else if (strcmp(argv[i], "-title") == 0 && i+1 < argc) {
            title = argv[++i];
        } else if (strcmp(argv[i], "-text") == 0 && i+1 < argc) {
            text = argv[++i];
        } else if (strcmp(argv[i], "-w") == 0 && i+1 < argc) {
            width = atoi(argv[++i]);			
            set_width = true;
        } else if (strcmp(argv[i], "-h") == 0 && i+1 < argc) {
            height = atoi(argv[++i]);
            set_height = true; // mark true else the height of font will be used
        } else if (strcmp(argv[i], "-display") == 0 && i+1 < argc) {
            display_name = argv[++i];
        } else if (strcmp(argv[i], "-pos") == 0 && i+2 < argc) {
            x = atoi(argv[++i]);
            y = atoi(argv[++i]);
            set_pos = true;
        } else if (strcmp(argv[i], "-fg") == 0 && i+1 < argc) {
            foreground = argv[++i];
        } else if (strcmp(argv[i], "-bg") == 0 && i+1 < argc) {
            background = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0) {
            antialias = true;
        } else if (strcmp(argv[i], "-hf") == 0 && i+1 < argc) {
            history_file = argv[++i];
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
		
        FbTk::App application(display_name.c_str());
        FbRun fbrun;

        if (fontname.size() != 0) {
            if (!fbrun.loadFont(fontname.c_str())) {
                cerr<<"Failed to load font: "<<fontname<<endl;
                cerr<<"Falling back to \"fixed\""<<endl;
            }
        }

        // get color
        XColor xc_foreground, xc_background;
        FbTk::Color fg_color(foreground.c_str(), 0);
        FbTk::Color bg_color(background.c_str(), 0);
		
        fbrun.setForegroundColor(fg_color);
        fbrun.setBackgroundColor(bg_color);

        if (set_height)
            fbrun.resize(fbrun.width(), height);
        if (set_width)
            fbrun.resize(width, fbrun.height());
        if (antialias)
            fbrun.setAntialias(antialias);
        // expand and load command history
        string expanded_filename = FbTk::StringUtil::expandFilename(history_file);
        if (!fbrun.loadHistory(expanded_filename.c_str()))
            cerr<<"FbRun Warning: Failed to load history file: "<<expanded_filename<<endl;

        fbrun.setTitle(title);
        fbrun.setText(text);
        fbrun.show();
		
        if (set_pos)
            fbrun.move(x, y);
		
        application.eventLoop();

    } catch (string errstr) {
        cerr<<"Error: "<<errstr<<endl;
    }
}
