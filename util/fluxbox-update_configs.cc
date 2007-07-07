// fluxbox-update_configs.cc
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "../src/FbTk/I18n.hh"
#include "../src/FbTk/Resource.hh"
#include "../src/FbTk/StringUtil.hh"

#include "../src/defaults.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif // HAVE_SIGNAL_H

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#include <iostream>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;

#define VERSION 1

int run_updates(int old_version, FbTk::ResourceManager rm) {
    int new_version = old_version;

    if (old_version < 1) { // add mouse events to keys file
        FbTk::Resource<string> rc_keyfile(rm, DEFAULTKEYSFILE,
                "session.keyFile", "Session.KeyFile");
        string keyfilename = FbTk::StringUtil::expandFilename(*rc_keyfile);

        // ok, I don't know anything about file handling in c++
        // what's it to you?!?!
        // I assume there should be some error handling in here, but I sure
        // don't know how, and I don't have documentation

        ifstream in_keyfile(keyfilename.c_str());
        string whole_keyfile = "";

        while (!in_keyfile.eof()) {
            string linebuffer;

            getline(in_keyfile, linebuffer);
            whole_keyfile += linebuffer + "\n";
        }
        in_keyfile.close();

        ofstream out_keyfile(keyfilename.c_str());
        // let's put our new keybindings first, so they're easy to find
        out_keyfile << "!mouse actions added by fluxbox-update_configs" << endl
                    << "OnDesktop Mouse1 :hideMenus" << endl
                    << "OnDesktop Mouse2 :workspaceMenu" << endl
                    << "OnDesktop Mouse3 :rootMenu" << endl;

        // scrolling on desktop needs to match user's desktop wheeling settings
        // hmmm, what are the odds that somebody wants this to be different on
        // different screens? the ability is going away until we make per-screen
        // keys files, anyway, so let's just use the first screen's setting
        FbTk::Resource<bool> rc_wheeling(rm, true,
                                         "session.screen0.desktopwheeling",
                                         "Session.Screen0.DesktopWheeling");
        FbTk::Resource<bool> rc_reverse(rm, false,
                                        "session.screen0.reversewheeling",
                                        "Session.Screen0.ReverseWheeling");
        if (*rc_wheeling) {
            if (*rc_reverse) { // if you ask me, this should have been default
                out_keyfile << "OnDesktop Mouse4 :prevWorkspace" << endl
                            << "OnDesktop Mouse5 :nextWorkspace" << endl;
            } else {
                out_keyfile << "OnDesktop Mouse4 :nextWorkspace" << endl
                            << "OnDesktop Mouse5 :prevWorkspace" << endl;
            }
        }
        out_keyfile << endl; // just for good looks

        // now, restore user's old keybindings
        out_keyfile << whole_keyfile;
        new_version = 1;
    }

    return new_version;
}

int main(int argc, char **argv) {
    string rc_filename;
    int i = 1;
    pid_t fb_pid = 0;

    FbTk::NLSInit("fluxbox.cat");
    _FB_USES_NLS;

    for (; i < argc; i++) {
        if (strcmp(argv[i], "-rc") == 0) {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, RCRequiresArg,
                              "error: '-rc' requires an argument", "the -rc option requires a file argument")<<endl;
                exit(1);
            }

            rc_filename = argv[i];
        } else if (strcmp(argv[i], "-pid") == 0) {
            if ((++i) >= argc) {
                // need translations for this, too
                cerr<<"the -pid option requires a numeric argument"<<endl;
            } else
                fb_pid = atoi(argv[i]);
        } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
            // no NLS translations yet -- we'll just have to use English for now
            cout << "  -rc <string>\t\t\tuse alternate resource file.\n"
                 << "  -pid <int>\t\t\ttell fluxbox to reload configuration.\n"
                 << "  -help\t\t\t\tdisplay this help text and exit.\n\n"
                 << endl;
            exit(0);
        }
    }

    if (rc_filename.empty())
        rc_filename = getenv("HOME") + string("/.fluxbox/init");

    FbTk::ResourceManager resource_manager(rc_filename.c_str(),false);
    if (!resource_manager.load(rc_filename.c_str())) {
        // couldn't load rc file
        cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "Failed trying to read rc file")<<":"<<rc_filename<<endl;
        cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFileTrying, "Retrying with", "Retrying rc file loading with (the following file)")<<": "<<DEFAULT_INITFILE<<endl;

        // couldn't load default rc file, either
        if (!resource_manager.load(DEFAULT_INITFILE)) {
            cerr<<_FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
            exit(1); // this is a fatal error for us
        }
    }

    // run updates here
    // I feel like putting this in a separate function for no apparent reason

    FbTk::Resource<int> config_version(resource_manager, 0,
            "session.configVersion", "Session.ConfigVersion");
    int old_version = *config_version;
    int new_version = run_updates(old_version, resource_manager);
    if (new_version > old_version) {
        config_version = new_version;
        resource_manager.save(rc_filename.c_str(), rc_filename.c_str());

#ifdef HAVE_SIGNAL_H
        // if we were given a fluxbox pid, send it a reconfigure signal
        if (fb_pid > 0)
            kill(fb_pid, SIGUSR2);
#endif // HAVE_SIGNAL_H

    }

    return 0;
}
