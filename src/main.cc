// main.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//               and 2003-2005 Simon Bowden (rathnor at users.sourceforge.net)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "fluxbox.hh"
#include "version.h"
#include "defaults.hh"

#include "Debug.hh"

#include "FbTk/Theme.hh"
#include "FbTk/I18n.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/FileUtil.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

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

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <fstream>
#include <stdexcept>
#include <typeinfo>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostream;
using std::ofstream;
using std::streambuf;
using std::auto_ptr;
using std::out_of_range;
using std::runtime_error;
using std::bad_cast;
using std::bad_alloc;
using std::exception;

static void showInfo(ostream &ostr) {
    _FB_USES_NLS;
    ostr <<
        _FB_CONSOLETEXT(Common, FluxboxVersion, "Fluxbox version", "Fluxbox version heading")
        << ": " 
        << __fluxbox_version <<endl;

    if (strlen(gitrevision()) > 0)
        ostr << _FB_CONSOLETEXT(Common, SvnRevision, "GIT Revision", "Revision number in GIT repositary") 
            << ": " 
            << gitrevision() << endl;
#if defined(__DATE__) && defined(__TIME__)
    ostr << _FB_CONSOLETEXT(Common, Compiled, "Compiled", "Time fluxbox was compiled")
        << ": " 
        << __DATE__ 
        << " "
        << __TIME__ << endl;
#endif
#ifdef __fluxbox_compiler
    ostr << _FB_CONSOLETEXT(Common, Compiler, "Compiler", "Compiler used to build fluxbox")
        << ": "
        << __fluxbox_compiler << endl;
#endif // __fluxbox_compiler
#ifdef __fluxbox_compiler_version
    ostr << _FB_CONSOLETEXT(Common, CompilerVersion, "Compiler version", "Compiler version used to build fluxbox")
        << ": " 
        << __fluxbox_compiler_version << endl;
#endif // __fluxbox_compiler_version

    ostr << endl 
        <<_FB_CONSOLETEXT(Common, Defaults, "Defaults", "Default values compiled in") 
        << ": " << endl;

    ostr <<_FB_CONSOLETEXT(Common, DefaultMenuFile, "    menu", "default menu file (right aligned - make sure same width as other default values)")
        << ": "
        << DEFAULTMENU << endl;
    ostr << _FB_CONSOLETEXT(Common, DefaultStyle, "   style", "default style (right aligned - make sure same width as other default values)")
        << ": "
        << DEFAULTSTYLE << endl;

    ostr << _FB_CONSOLETEXT(Common, DefaultKeyFile, "    keys", "default key file (right aligned - make sure same width as other default values)")
        << ": "
        << DEFAULTKEYSFILE << endl;
    ostr << _FB_CONSOLETEXT(Common, DefaultInitFile, "    init", "default init file (right aligned - make sure same width as other default values)")
        << ": "
        << DEFAULT_INITFILE << endl;

#ifdef NLS
    ostr << _FB_CONSOLETEXT(Common, DefaultLocalePath, "    nls", "location for localization files (right aligned - make sure same width as other default values)")
        << ": "
        << LOCALEPATH << endl;
#endif

    const char NOT[] = "-";
    ostr << endl
        << _FB_CONSOLETEXT(Common, CompiledOptions, "Compiled options", "Options used when compiled")
        << " (" << NOT << " => " 
        << _FB_CONSOLETEXT(Common, Disabled, "disabled", "option is turned off") << "): " << endl
        <<

/**** NOTE: This list is in alphabetical order! ****/

#ifndef HAVE_FRIBIDI
        NOT <<
#endif
        "BIDI" << endl <<

#ifndef DEBUG
        NOT <<
#endif // DEBUG
        "DEBUG" << endl <<

#ifndef USE_NEWWMSPEC
        NOT <<
#endif // USE_NEWWMSPEC
        "EWMH" << endl <<

#ifndef HAVE_IMLIB2
        NOT<<
#endif // HAVE_IMLIB2
        "IMLIB2" << endl <<

#ifndef NLS
        NOT<<
#endif // NLS
        "NLS" << endl <<

#ifndef REMEMBER
        NOT <<
#endif // REMEMBER
        "REMEMBER" << endl <<

#ifndef HAVE_XRENDER
        NOT <<
#endif // HAVE_XRENDER
        "RENDER" << endl <<

#ifndef SHAPE
        NOT <<
#endif // SHAPE
        "SHAPE" << endl <<

#ifndef SLIT
        NOT <<
#endif // SLIT
        "SLIT" << endl <<

#ifndef USE_TOOLBAR
        NOT <<
#endif // USE_TOOLBAR
        "TOOLBAR" << endl <<

#ifndef USE_XFT
        NOT <<
#endif // USE_XFT
        "XFT" << endl <<

#ifndef XINERAMA
        NOT <<
#endif // XINERAMA
        "XINERAMA" << endl <<

#ifndef USE_XMB
        NOT <<
#endif // USE_XMB
        "XMB" << endl <<

#ifndef HAVE_XPM
        NOT <<
#endif // HAVE_XPM
        "XPM" << endl

        << endl;
}

struct Options {
    Options() : xsync(false) {

        const char* env;

        env = getenv("DISPLAY");
        if (env) {
            session_display.assign(env);
        }

        env = getenv("HOME");
        if (env) {
            rc_path.assign(std::string(env) + "/." + realProgramName("fluxbox"));
            rc_file = rc_path + "/init";
        }
    }


    std::string session_display;
    std::string rc_path;
    std::string rc_file;
    std::string log_filename;
    bool xsync;
};

static void parseOptions(int argc, char** argv, Options& opts) {

    _FB_USES_NLS;

    int i;
    for (i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg == "-rc" || arg == "--rc") {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, RCRequiresArg,
                              "error: '-rc' requires an argument", "the -rc option requires a file argument")<<endl;
                exit(EXIT_FAILURE);
            }

            opts.rc_file = argv[i];
        } else if (arg == "-display" || arg == "--display") {
            // check for -display option... to run on a display other than the one
            // set by the environment variable DISPLAY

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, DISPLAYRequiresArg,
                              "error: '-display' requires an argument",
                              "")<<endl;
                exit(EXIT_FAILURE);
            }

            opts.session_display = argv[i];
            if (!FbTk::App::setenv("DISPLAY", argv[i])) {
                cerr<<_FB_CONSOLETEXT(main, WarnDisplayEnv,
                                "warning: couldn't set environment variable 'DISPLAY'",
                              "")<<endl;
                perror("putenv()");
            }
        } else if (arg == "-version" || arg == "-v" || arg == "--version") {
            // print current version string
            cout << "Fluxbox " << __fluxbox_version << " : (c) 2001-2011 Fluxbox Team " << endl << endl;
            exit(EXIT_SUCCESS);
        } else if (arg == "-log" || arg == "--log") {
            if (++i >= argc) {
                cerr<<_FB_CONSOLETEXT(main, LOGRequiresArg, "error: '-log' needs an argument", "")<<endl;
                exit(EXIT_FAILURE);
            }
            opts.log_filename = argv[i];
        } else if (arg == "-sync" || arg == "--sync") {
            opts.xsync = true;
        } else if (arg == "-help" || arg == "-h" || arg == "--help") {
            // print program usage and command line options
            printf(_FB_CONSOLETEXT(main, Usage,
                           "Fluxbox %s : (c) %s Fluxbox Team\n"
                           "Website: http://www.fluxbox.org/\n\n"
                           "-display <string>\t\tuse display connection.\n"
                           "-screen <all|int,int,int>\trun on specified screens only.\n"
                           "-rc <string>\t\t\tuse alternate resource file.\n"
                           "-version\t\t\tdisplay version and exit.\n"
                           "-info\t\t\t\tdisplay some useful information.\n"
                           "-list-commands\t\t\tlist all valid key commands.\n"
                           "-sync\t\t\t\tsynchronize with X server for debugging.\n"
                           "-log <filename>\t\t\tlog output to file.\n"
                           "-help\t\t\t\tdisplay this help text and exit.\n\n",

                           "Main usage string. Please lay it out nicely. There is one %s that is given the version").c_str(),
                   __fluxbox_version, "2001-2011");
            exit(EXIT_SUCCESS);
        } else if (arg == "-info" || arg == "-i" || arg == "--info") {
            showInfo(cout);
            exit(EXIT_SUCCESS);
        } else if (arg == "-list-commands" || arg == "--list-commands") {
            FbTk::CommandParser<void>::CreatorMap cmap = FbTk::CommandParser<void>::instance().creatorMap();
            FbTk::CommandParser<void>::CreatorMap::const_iterator it = cmap.begin();
            const FbTk::CommandParser<void>::CreatorMap::const_iterator it_end = cmap.end();
            for (; it != it_end; ++it)
                cout << it->first << endl;
            exit(EXIT_SUCCESS);
        } else if (arg == "-verbose" || arg == "--verbose") {
            FbTk::ThemeManager::instance().setVerbose(true);
        }
    }
}


/**
 setup the configutation files in
 home directory
*/
void setupConfigFiles(const std::string& dirname, const std::string& rc) {

    _FB_USES_NLS;

    const bool has_dir = FbTk::FileUtil::isDirectory(dirname.c_str());


    struct CFInfo {
        bool create_file;
        const char* default_name;
        const std::string filename;
    } cfiles[] = {
        { !has_dir, DEFAULT_INITFILE, rc },
        { !has_dir, DEFAULTKEYSFILE, dirname + "/keys" },
        { !has_dir, DEFAULTMENU, dirname + "/menu" },
        { !has_dir, DEFAULT_APPSFILE, dirname + "/apps" },
        { !has_dir, DEFAULT_OVERLAY, dirname + "/overlay" },
        { !has_dir, DEFAULT_WINDOWMENU, dirname + "/windowmenu" }
    };
    const size_t nr_of_cfiles = sizeof(cfiles)/sizeof(CFInfo);


    if (has_dir) { // check if anything with these names exists, if not create new
        for (size_t i = 0; i < nr_of_cfiles; ++i) {
            cfiles[i].create_file = access(cfiles[i].filename.c_str(), F_OK);
        }
    } else {

        fbdbg << "Creating dir: " << dirname << endl;
        if (mkdir(dirname.c_str(), 0700)) {
            fprintf(stderr, _FB_CONSOLETEXT(Fluxbox, ErrorCreatingDirectory,
                                    "Can't create %s directory",
                                    "Can't create a directory, one %s for directory name").c_str(),
                    dirname.c_str());
            cerr << endl;
            return;
        }
    }

    bool sync_fs = false;

    // copy default files if needed
    for (size_t i = 0; i < nr_of_cfiles; ++i) {
        if (cfiles[i].create_file) {
            FbTk::FileUtil::copyFile(cfiles[i].default_name, cfiles[i].filename.c_str());
            sync_fs = true;
        }
    }

    if (sync_fs) {
       sync();
    }
}


// configs might be out of date, so run fluxbox-update_configs
// if necassary.
void updateConfigFilesIfNeeded(const std::string& rc_file) {

    const int CONFIG_VERSION = 13; // TODO: move this to 'defaults.hh' or 'config.h'

    FbTk::ResourceManager r_mgr(rc_file.c_str(), false);
    FbTk::Resource<int> c_version(r_mgr, 0, "session.configVersion", "Session.ConfigVersion");

    if (!r_mgr.load(rc_file.c_str())) {
        _FB_USES_NLS;
        cerr << _FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")
            << ": " 
            << rc_file << endl;
        return;
    }

    if (*c_version < CONFIG_VERSION) {

        fbdbg << "updating config files from version " 
            << *c_version
            << " to "
            << CONFIG_VERSION
            << endl;

        string commandargs = realProgramName("fluxbox-update_configs");
        commandargs += " -rc " + rc_file;

        if (system(commandargs.c_str())) {
            fbdbg << "running '" 
                << commandargs
                << "' failed." << endl;
        }
        sync();
    }
}


int main(int argc, char **argv) {

    FbTk::NLSInit("fluxbox.cat");

    Options opts;
    parseOptions(argc, argv, opts);

#ifdef __EMX__
    _chdir2(getenv("X11ROOT"));
#endif // __EMX__

    streambuf *outbuf = 0;
    streambuf *errbuf = 0;

    ofstream log_file(opts.log_filename.c_str());

    _FB_USES_NLS;

    // setup log file
    if (log_file.is_open()) {
        cerr << _FB_CONSOLETEXT(main, LoggingTo, "Logging to", "Logging to a file") 
            << ": " 
            << opts.log_filename << endl;

        log_file <<"------------------------------------------" << endl;
        log_file << _FB_CONSOLETEXT(main, LogFile, "Log File", "")
            << ": "
            << opts.log_filename <<endl;

        showInfo(log_file);
        log_file << "------------------------------------------" << endl;
        // setup log to use cout and cerr stream
        outbuf = cout.rdbuf(log_file.rdbuf());
        errbuf = cerr.rdbuf(log_file.rdbuf());
    }

    int exitcode = EXIT_FAILURE;

    setupConfigFiles(opts.rc_path, opts.rc_file);
    updateConfigFilesIfNeeded(opts.rc_file);

    auto_ptr<Fluxbox> fluxbox;
    try {

        fluxbox.reset(new Fluxbox(argc, argv,
                    opts.session_display,
                    opts.rc_path,
                    opts.rc_file,
                    opts.xsync));
        fluxbox->eventLoop();

        exitcode = EXIT_SUCCESS;

    } catch (out_of_range &oor) {
        cerr <<"Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorOutOfRange, "Out of range", "Error message")
            << ": "
            << oor.what() << endl;
    } catch (runtime_error &re) {
        cerr << "Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorRuntime, "Runtime error", "Error message")
            << ": "
            << re.what() << endl;
    } catch (bad_cast &bc) {
        cerr << "Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorBadCast, "Bad cast", "Error message")
            << ": "
            << bc.what() << endl;
    } catch (bad_alloc &ba) {
        cerr << "Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorBadAlloc, "Bad Alloc", "Error message")
            << ": "
            << ba.what() << endl;
    } catch (exception &e) {
        cerr << "Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorStandardException, "Standard Exception", "Error message")
            << ": "
            << e.what() << endl;
    } catch (string error_str) {
        cerr << _FB_CONSOLETEXT(Common, Error, "Error", "Error message header")
            << ": "
            << error_str << endl;
    } catch (...) {
        cerr << "Fluxbox: "
            << _FB_CONSOLETEXT(main, ErrorUnknown, "Unknown error", "Error message")
            << "." << endl;
        abort();
    }

    bool restarting = false;
    string restart_argument;

    if (fluxbox.get()) {
        restarting = fluxbox->isRestarting();
        restart_argument = fluxbox->getRestartArgument();
    }

    // destroy fluxbox
    fluxbox.reset(0);

    // restore cout and cin streams
    if (outbuf != 0)
        cout.rdbuf(outbuf);
    if (errbuf != 0)
        cerr.rdbuf(errbuf);

    FbTk::FbStringUtil::shutdown();

    if (restarting) {
        if (!restart_argument.empty()) {
            const char *shell = getenv("SHELL");
            if (!shell)
                shell = "/bin/sh";

            execlp(shell, shell, "-c", restart_argument.c_str(), (const char *) NULL);
            perror(restart_argument.c_str());
        }

        // fall back in case the above execlp doesn't work
        execvp(argv[0], argv);
        perror(argv[0]);

        const char *basename = FbTk::StringUtil::basename(argv[0]).c_str();
        execvp(basename, argv);
        perror(basename);
    }

    return exitcode;
}

