// cli_options.cc for Fluxbox Window Manager
// Copyright (c) 2014 - Mathias Gumz <akira at fluxbox.org>
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "cli.hh"
#include "version.h"
#include "defaults.hh"
#include "Debug.hh"

#include "FbTk/App.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Theme.hh"
#include "FbTk/I18n.hh"
#include "FbTk/Command.hh"
#include "FbTk/CommandParser.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

FluxboxCli::Options::Options() : xsync(false) {

    const char* env = getenv("DISPLAY");
    if (env && strlen(env) > 0) {
        session_display.assign(env);
    }

    string fname = std::string("~/.") + realProgramName("fluxbox");
    rc_path = FbTk::StringUtil::expandFilename(fname);

    if (!rc_path.empty()) {
        rc_file = rc_path + "/init";
    }
}

int FluxboxCli::Options::parse(int argc, char** argv) {

    _FB_USES_NLS;

    int i;
    for (i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg == "-rc" || arg == "--rc") {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, RCRequiresArg,
                              "error: '-rc' requires an argument",
                              "the -rc option requires a file argument")<<endl;
                return EXIT_FAILURE;
            }

            this->rc_file = argv[i];

        } else if (arg == "-display" || arg == "--display") {
            // check for -display option... to run on a display other than the one
            // set by the environment variable DISPLAY

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, DISPLAYRequiresArg,
                                      "error: '-display' requires an argument",
                                      "")<<endl;
                return EXIT_FAILURE;
            }

            this->session_display = argv[i];
            if (!FbTk::App::setenv("DISPLAY", argv[i])) {
                cerr<<_FB_CONSOLETEXT(main, WarnDisplayEnv,
                                      "warning: couldn't set environment variable 'DISPLAY'",
                                      "")<<endl;
                perror("putenv()");
            }
        } else if (arg == "-version" || arg == "-v" || arg == "--version") {
            // print current version string
            cout << "Fluxbox " << __fluxbox_version << " : (c) 2001-2015 Fluxbox Team " 
                << endl << endl;
            return EXIT_SUCCESS;
        } else if (arg == "-log" || arg == "--log") {
            if (++i >= argc) {
                cerr<<_FB_CONSOLETEXT(main, LOGRequiresArg, 
                                      "error: '-log' needs an argument", "")<<endl;
                return EXIT_FAILURE;
            }
            this->log_filename = argv[i];
        } else if (arg == "-sync" || arg == "--sync") {
            this->xsync = true;
        } else if (arg == "-help" || arg == "-h" || arg == "--help") {
            // print program usage and command line options
            printf(_FB_CONSOLETEXT(main, Usage,
                           "Fluxbox %s : (c) %s Fluxbox Team\n"
                           "Website: http://www.fluxbox.org/\n\n"
                           "-display <string>\t\tuse display connection.\n"
                           "-screen <all|int,int,int>\trun on specified screens only.\n"
                           "-rc <string>\t\t\tuse alternate resource file.\n"
                           "-no-slit\t\t\tdo not provide a slit.\n"
                           "-no-toolbar\t\t\tdo not provide a toolbar.\n"
                           "-version\t\t\tdisplay version and exit.\n"
                           "-info\t\t\t\tdisplay some useful information.\n"
                           "-list-commands\t\t\tlist all valid key commands.\n"
                           "-sync\t\t\t\tsynchronize with X server for debugging.\n"
                           "-log <filename>\t\t\tlog output to file.\n"
                           "-help\t\t\t\tdisplay this help text and exit.\n\n",

                           "Main usage string. Please lay it out nicely. One %%s gives the version, ther other gives the year").c_str(),
                   __fluxbox_version, "2001-2015");
            return EXIT_SUCCESS;
        } else if (arg == "-info" || arg == "-i" || arg == "--info") {
            FluxboxCli::showInfo(cout);
            return EXIT_SUCCESS;
        } else if (arg == "-list-commands" || arg == "--list-commands") {
            FbTk::CommandParser<void>::CreatorMap cmap = FbTk::CommandParser<void>::instance().creatorMap();
            FbTk::CommandParser<void>::CreatorMap::const_iterator it = cmap.begin();
            const FbTk::CommandParser<void>::CreatorMap::const_iterator it_end = cmap.end();
            for (; it != it_end; ++it)
                cout << it->first << endl;
            return EXIT_SUCCESS;
        } else if (arg == "-verbose" || arg == "--verbose") {
            FbTk::ThemeManager::instance().setVerbose(true);
        }
    }
    return -1;
}

