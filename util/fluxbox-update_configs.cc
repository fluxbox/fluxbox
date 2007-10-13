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

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <list>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;
using std::set;
using std::map;
using std::list;

string read_file(string filename);
void write_file(string filename, string &contents);
void save_all_files();

int run_updates(int old_version, FbTk::ResourceManager rm) {
    int new_version = old_version;

    FbTk::Resource<string> rc_keyfile(rm, "~/.fluxbox/keys",
            "session.keyFile", "Session.KeyFile");
    FbTk::Resource<string> rc_appsfile(rm, "~/.fluxbox/apps",
            "session.appsFile", "Session.AppsFile");

    string appsfilename = FbTk::StringUtil::expandFilename(*rc_appsfile);
    string keyfilename = FbTk::StringUtil::expandFilename(*rc_keyfile);

    if (old_version < 1) { // add mouse events to keys file

        string whole_keyfile = read_file(keyfilename);
        string new_keyfile = "";
        // let's put our new keybindings first, so they're easy to find
        new_keyfile += "!mouse actions added by fluxbox-update_configs\n";
        new_keyfile += "OnDesktop Mouse1 :hideMenus\n";
        new_keyfile += "OnDesktop Mouse2 :workspaceMenu\n";
        new_keyfile += "OnDesktop Mouse3 :rootMenu\n";

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
                new_keyfile += "OnDesktop Mouse4 :prevWorkspace\n";
                new_keyfile += "OnDesktop Mouse5 :nextWorkspace\n";
            } else {
                new_keyfile += "OnDesktop Mouse4 :nextWorkspace\n";
                new_keyfile += "OnDesktop Mouse5 :prevWorkspace\n";
            }
        }
        new_keyfile += "\n"; // just for good looks
        new_keyfile += whole_keyfile; // don't forget user's old keybindings

        write_file(keyfilename, new_keyfile);
        new_version = 1;
    }

    if (old_version < 2) { // move groups entries to apps file
        FbTk::Resource<string> rc_groupfile(rm, "~/.fluxbox/groups",
                "session.groupFile", "Session.GroupFile");
        string groupfilename = FbTk::StringUtil::expandFilename(*rc_groupfile);
        string whole_groupfile = read_file(groupfilename);
        string whole_appsfile = read_file(appsfilename);
        string new_appsfile = "";

        list<string> lines;
        FbTk::StringUtil::stringtok(lines, whole_groupfile, "\n");

        list<string>::iterator line_it = lines.begin();
        list<string>::iterator line_it_end = lines.end();
        for (; line_it != line_it_end; ++line_it) {
            new_appsfile += "[group] (workspace=[current])\n";

            list<string> apps;
            FbTk::StringUtil::stringtok(apps, *line_it);

            list<string>::iterator it = apps.begin();
            list<string>::iterator it_end = apps.end();
            for (; it != it_end; ++it) {
                new_appsfile += " [app] (name=";
                new_appsfile += *it;
                new_appsfile += ")\n";
            }

            new_appsfile += "[end]\n";
        }

        new_appsfile += whole_appsfile;
        write_file(appsfilename, new_appsfile);
        new_version = 2;
    }

    if (old_version < 3) { // move toolbar wheeling to keys file
        string whole_keyfile = read_file(keyfilename);
        string new_keyfile = "";
        // let's put our new keybindings first, so they're easy to find
        new_keyfile += "!mouse actions added by fluxbox-update_configs\n";
        bool keep_changes = false;

        // scrolling on toolbar needs to match user's toolbar wheeling settings
        FbTk::Resource<string> rc_wheeling(rm, "Off",
                                           "session.screen0.iconbar.wheelMode",
                                           "Session.Screen0.Iconbar.WheelMode");
        FbTk::Resource<bool> rc_screen(rm, true,
                                       "session.screen0.desktopwheeling",
                                       "Session.Screen0.DesktopWheeling");
        FbTk::Resource<bool> rc_reverse(rm, false,
                                        "session.screen0.reversewheeling",
                                        "Session.Screen0.ReverseWheeling");
        if (strcasecmp((*rc_wheeling).c_str(), "On") == 0 ||
            (strcasecmp((*rc_wheeling).c_str(), "Screen") && *rc_screen)) {
            keep_changes = true;
            if (*rc_reverse) { // if you ask me, this should have been default
                new_keyfile += "OnToolbar Mouse4 :prevWorkspace\n";
                new_keyfile += "OnToolbar Mouse5 :nextWorkspace\n";
            } else {
                new_keyfile += "OnToolbar Mouse4 :nextWorkspace\n";
                new_keyfile += "OnToolbar Mouse5 :prevWorkspace\n";
            }
        }
        new_keyfile += "\n"; // just for good looks
        new_keyfile += whole_keyfile; // don't forget user's old keybindings

        if (keep_changes)
            write_file(keyfilename, new_keyfile);
        new_version = 3;
    }

    return new_version;
}

int main(int argc, char **argv) {
    string rc_filename;
    set<string> style_filenames;
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
        // configs were updated -- let's save our changes
        config_version = new_version;
        resource_manager.save(rc_filename.c_str(), rc_filename.c_str());
        save_all_files();

#ifdef HAVE_SIGNAL_H
        // if we were given a fluxbox pid, send it a reconfigure signal
        if (fb_pid > 0)
            kill(fb_pid, SIGUSR2);
#endif // HAVE_SIGNAL_H

    }

    return 0;
}

static set<string> modified_files;
// we may want to put a size limit on this cache, so it doesn't grow too big
static map<string,string> file_cache;

// returns the contents of the file given, either from the cache or by reading
// the file from disk
string read_file(string filename) {
    // check if we already have the file in memory
    map<string,string>::iterator it = file_cache.find(filename);
    if (it != file_cache.end())
        return it->second;

    // nope, we'll have to read the file
    ifstream infile(filename.c_str());
    string whole_file = "";

    if (!infile) // failed to open file
        return whole_file;

    while (!infile.eof()) {
        string linebuffer;

        getline(infile, linebuffer);
        whole_file += linebuffer + "\n";
    }
    infile.close();

    file_cache[filename] = whole_file;
    return whole_file;
}

#ifdef NOT_USED
// remove the file from the cache, writing to disk if it's been changed
void forget_file(string filename) {
    map<string,string>::iterator cache_it = file_cache.find(filename);
    // check if we knew about the file to begin with
    if (cache_it == file_cache.end())
        return;

    // check if we've actually modified it
    set<string>::iterator mod_it = modified_files.find(filename);
    if (mod_it == modified_files.end()) {
        file_cache.erase(cache_it);
        return;
    }

    // flush our changes to disk and remove all traces
    ofstream outfile(filename.c_str());
    outfile << cache_it->second;
    file_cache.erase(cache_it);
    modified_files.erase(mod_it);
}
#endif // NOT_USED

// updates the file contents in the cache and marks the file as modified so it
// gets saved later
void write_file(string filename, string &contents) {
    modified_files.insert(filename);
    file_cache[filename] = contents;
}

// actually save all the files we've modified
void save_all_files() {
    set<string>::iterator it = modified_files.begin();
    set<string>::iterator it_end = modified_files.end();
    for (; it != it_end; ++it) {
        ofstream outfile(it->c_str());
        outfile << file_cache[it->c_str()];
    }
    modified_files.clear();
}
