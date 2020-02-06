// main.cc for FbRun
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "FbRun.hh"
#include "FbTk/App.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Color.hh"

#ifdef XINERAMA
extern  "C" {
#include <X11/extensions/Xinerama.h>
}
#endif // XINERAMA

#include <string>
#include <iostream>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

using std::cerr;
using std::endl;
using std::string;

void showUsage(const char *progname) {
    cerr<<"fbrun 1.5 : (c) 2002-2015 Henrik Kinnunen"<<endl;
    cerr<<"Usage: "<<
        progname<<" [arguments]"<<endl<<
        "Arguments: "<<endl<<
        "   -font [font name]           Text font"<<endl<<
        "   -title [title name]         Set title"<<endl<<
        "   -text [text]                Text input"<<endl<<
        "   -print                      Print result to stdout"<<endl<<
        "   -w [width]                  Window width in pixels"<<endl<<
        "   -h [height]                 Window height in pixels"<<endl<<
        "   -pad [size]                 Padding size in pixels"<<endl<<
        "   -display [display string]   Display name"<<endl<<
        "   -pos [x] [y]                Window position in pixels"<<endl<<
        "   -nearmouse                  Window position near mouse"<<endl<<
        "   -center                     Window position on screen center"<<endl<<
        "   -fg [color name]            Foreground text color"<<endl<<
        "   -bg [color name]            Background color"<<endl<<
        "   -hf [history file]          History file to load (default ~/.fluxbox/fbrun_history)"<<endl<<
        "   -cf [completion file]       Complete contents of this file instead of $PATH binaries"<<endl<<
        "   -autocomplete               Complete on typing"<<endl<<
        "   -preselect                  Select preset text"<<endl<<
        "   -help                       Show this help"<<endl<<endl<<
        "Example: fbrun -fg black -bg white -text xterm -title \"run xterm\""<<endl;
}

int main(int argc, char **argv) {
    int x = 0, y = 0; // default pos of window
    size_t width = 200, height = 32; // default size of window
    bool set_height = false, set_width=false; // use height/width of font by default
    int padding = 0; // default horizontal padding for text
    bool set_pos = false; // set position
    bool near_mouse = false; // popup near mouse
    bool center = false;
    bool print = false;
    bool preselect = false;
    bool autocomplete = getenv("FBRUN_AUTOCOMPLETE");
    string fontname; // font name
    string title("Run program"); // default title
    string text;         // default input text
    string foreground("black");   // text color
    string background("white");   // text background color
    string display_name; // name of the display connection
    string history_file("~/.fluxbox/fbrun_history"); // command history file
    string completion_file; // command history file
    // parse arguments
    for (int i=1; i<argc; i++) {
        string arg = argv[i];
        if ((arg == "-font" || arg == "--font") && i+1 < argc) {
            fontname = argv[++i];
        } else if (arg == "-print" || arg == "--print") {
            print = true;
        } else if ((arg == "-title" || arg == "--title") && i+1 < argc) {
            title = argv[++i];
        } else if ((arg == "-text" || arg == "--text") && i+1 < argc) {
            text = argv[++i];
        } else if (arg == "-w" && i+1 < argc) {
            width = atoi(argv[++i]);
            set_width = true;
        } else if (arg == "-h" && i+1 < argc) {
            height = atoi(argv[++i]);
            set_height = true; // mark true else the height of font will be used
        } else if (arg == "-pad" && i+1 < argc) {
            padding = atoi(argv[++i]);
        } else if ((arg == "-display" || arg == "--display") && i+1 < argc) {
            display_name = argv[++i];
        } else if ((arg == "-pos" || arg == "--pos") && i+2 < argc) {
            x = atoi(argv[++i]);
            y = atoi(argv[++i]);
            set_pos = true;
        } else if (arg == "-nearmouse" || arg == "--nearmouse") {
            set_pos = true;
            near_mouse = true;
        } else if (arg == "-center" || arg == "--center") {
            set_pos = true;
            center = true;
        } else if (strcmp(argv[i], "-fg") == 0 && i+1 < argc) {
            foreground = argv[++i];
        } else if (strcmp(argv[i], "-bg") == 0 && i+1 < argc) {
            background = argv[++i];
        } else if (strcmp(argv[i], "-hf") == 0 && i+1 < argc) {
            history_file = argv[++i];
        } else if (strcmp(argv[i], "-cf") == 0 && i+1 < argc) {
            completion_file = argv[++i];
        } else if (strcmp(argv[i], "-preselect") == 0) {
            preselect = true;
        } else if (strcmp(argv[i], "-autocomplete") == 0) {
            autocomplete = true;
        } else if (arg == "-h" || arg == "-help" || arg == "--help") {
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

        fbrun.setPrint(print);
        fbrun.setAutocomplete(autocomplete);

        if (fontname.size() != 0) {
            if (!fbrun.loadFont(fontname.c_str())) {
                cerr<<"Failed to load font: "<<fontname<<endl;
                cerr<<"Falling back to \"fixed\""<<endl;
            }
        }

        // get color
        FbTk::Color fg_color(foreground.c_str(), 0);
        FbTk::Color bg_color(background.c_str(), 0);

        fbrun.setForegroundColor(fg_color);
        fbrun.setBackgroundColor(bg_color);

        if (set_height)
            fbrun.resize(fbrun.width(), height);
        if (set_width)
            fbrun.resize(width, fbrun.height());

        // expand and load command history
        string expanded_filename = FbTk::StringUtil::expandFilename(history_file);
        if (!fbrun.loadHistory(expanded_filename.c_str()))
            cerr<<"FbRun Warning: Failed to load history file: "<<expanded_filename<<endl;

        if (!completion_file.empty()) {
            expanded_filename = FbTk::StringUtil::expandFilename(completion_file);
            if (!fbrun.loadCompletion(expanded_filename.c_str()))
                cerr<<"FbRun Warning: Failed to load completion file: "<<expanded_filename<<endl;
        }

        fbrun.setPadding(padding);
        fbrun.setTitle(title);
        fbrun.setText(text);

        if (preselect)
            fbrun.selectAll();

        if (near_mouse || center) {

            int wx, wy;
            unsigned int mask;
            Window ret_win;
            Window child_win;

            Display* dpy = FbTk::App::instance()->display();
            int root_x = 0;
            int root_y = 0;
            unsigned int root_w = WidthOfScreen(DefaultScreenOfDisplay(dpy));
            unsigned int root_h = HeightOfScreen(DefaultScreenOfDisplay(dpy));

            if (XQueryPointer(dpy, DefaultRootWindow(dpy),
                              &ret_win, &child_win,
                              &x, &y, &wx, &wy, &mask)) {

#ifdef XINERAMA
                if(XineramaIsActive(dpy)) {
                    XineramaScreenInfo* screen_info = 0;
                    int number = 0;
                    screen_info = XineramaQueryScreens(dpy, &number);
                    if (screen_info) {
                        for(int i= 0; i < number; i++) {
                            if (x >= screen_info[i].x_org &&
                                x <  screen_info[i].x_org + screen_info[i].width &&
                                y >= screen_info[i].y_org &&
                                y <  screen_info[i].y_org + screen_info[i].height) {
                                root_x = screen_info[i].x_org;
                                root_y = screen_info[i].y_org;
                                root_w = screen_info[i].width;
                                root_h = screen_info[i].height;
                                break;
                            }
                        }

                        XFree(screen_info);
                    }
                }
#endif // XINERAMA
            } else if (!center) {
                set_pos = false;
            }

            if (center) {
                x = root_x + root_w/2;
                y = root_y + root_h/2;
            }

            x-= fbrun.width()/2;
            y-= fbrun.height()/2;

            if (x < root_x)
                x = root_x;
            if (x + fbrun.width() > root_x + root_w)
                x = root_x + root_w - fbrun.width();
            if (y < root_y)
                y = root_y;
            if (y + fbrun.height() > root_y + root_h)
                y = root_y + root_h - fbrun.height();
        }

        if (set_pos)
            fbrun.move(x, y);

        fbrun.lockPosition(set_pos);

        fbrun.show();

        application.eventLoop();

    } catch (string & errstr) {
        cerr<<"Error: "<<errstr<<endl;
    }
}
