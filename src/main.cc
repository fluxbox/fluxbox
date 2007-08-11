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

// $Id$

#include "fluxbox.hh"
#include "version.h"
#include "defaults.hh"

#include "FbTk/Theme.hh"
#include "FbTk/I18n.hh"
#include "FbTk/StringUtil.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

#include <iostream>
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
    ostr<<_FB_CONSOLETEXT(Common, FluxboxVersion, "Fluxbox version", "Fluxbox version heading")<<": "<<__fluxbox_version<<endl;

    if (strlen(svnversion()) > 0)
        ostr << _FB_CONSOLETEXT(Common, SvnRevision, "SVN Revision", "Revision number in SVN repositary") << ": " << svnversion() << endl;
#if defined(__DATE__) && defined(__TIME__)
    ostr<<_FB_CONSOLETEXT(Common, Compiled, "Compiled", "Time fluxbox was compiled")<<": "<<__DATE__<<" "<<__TIME__<<endl;
#endif
#ifdef __fluxbox_compiler
    ostr<<_FB_CONSOLETEXT(Common, Compiler, "Compiler", "Compiler used to build fluxbox")<<": "<<__fluxbox_compiler<<endl;
#endif // __fluxbox_compiler
#ifdef __fluxbox_compiler_version
    ostr<<_FB_CONSOLETEXT(Common, CompilerVersion, "Compiler version", "Compiler version used to build fluxbox")<<": "<<__fluxbox_compiler_version<<endl;
#endif // __fluxbox_compiler_version

    ostr<<endl<<_FB_CONSOLETEXT(Common, Defaults, "Defaults", "Default values compiled in")<<":"<<endl;

    ostr<<_FB_CONSOLETEXT(Common, DefaultMenuFile, "    menu", "default menu file (right aligned - make sure same width as other default values)")<<": "<<DEFAULTMENU<<endl;
    ostr<<_FB_CONSOLETEXT(Common, DefaultStyle, "   style", "default style (right aligned - make sure same width as other default values)")<<": "<<DEFAULTSTYLE<<endl;

    ostr<<_FB_CONSOLETEXT(Common, DefaultKeyFile, "    keys", "default key file (right aligned - make sure same width as other default values)")<<": "<<DEFAULTKEYSFILE<<endl;
    ostr<<_FB_CONSOLETEXT(Common, DefaultInitFile, "    init", "default init file (right aligned - make sure same width as other default values)")<<": "<<DEFAULT_INITFILE<<endl;

#ifdef NLS
    ostr<<_FB_CONSOLETEXT(Common, DefaultLocalePath, "    nls", "location for localization files (right aligned - make sure same width as other default values)")<<": "<<LOCALEPATH<<endl;
#endif

    const char NOT[] = "-";
    ostr<<endl<<
        _FB_CONSOLETEXT(Common, CompiledOptions, "Compiled options", "Options used when compiled")
        <<" ("<<NOT<<" => "<<
        _FB_CONSOLETEXT(Common, Disabled, "disabled", "option is turned off")<<"): "<<endl<<

/**** NOTE: This list is in alphabetical order! ****/

#ifndef DEBUG
        NOT<<
#endif // DEBUG
        "DEBUG"<<endl<<

#ifndef USE_NEWWMSPEC
        NOT<<
#endif // USE_NEWWMSPEC
        "EWMH"<<endl<<

#ifndef USE_GNOME
        NOT<<
#endif // USE_GNOME
        "GNOME"<<endl<<

#ifndef HAVE_IMLIB2
        NOT<<
#endif // HAVE_IMLIB2
        "IMLIB2"<<endl<<

#ifndef KDE
        NOT<<
#endif // KDE
        "KDE"<<endl<<

#ifndef NLS
        NOT<<
#endif // NLS
        "NLS"<<endl<<

#ifndef REMEMBER
        NOT<<
#endif // REMEMBER
        "REMEMBER"<<endl<<

#ifndef HAVE_XRENDER
        NOT<<
#endif // HAVE_XRENDER
        "RENDER"<<endl<<

#ifndef SHAPE
        NOT<<
#endif // SHAPE
        "SHAPE"<<endl<<

#ifndef SLIT
        NOT<<
#endif // SLIT
        "SLIT"<<endl<<

#ifndef USE_TOOLBAR
        NOT<<
#endif // USE_TOOLBAR
        "TOOLBAR"<<endl<<

#ifndef USE_XFT
        NOT<<
#endif // USE_XFT
        "XFT"<<endl<<

#ifndef XINERAMA
        NOT<<
#endif // XINERAMA
        "XINERAMA"<<endl<<

#ifndef USE_XMB
        NOT<<
#endif // USE_XMB
        "XMB"<<endl<<

#ifndef HAVE_XPM
        NOT<<
#endif // HAVE_XPM
        "XPM"<<endl<<


        endl;
}

int main(int argc, char **argv) {

    string session_display("");
    string rc_file;
    string log_filename;

    FbTk::NLSInit("fluxbox.cat");
    _FB_USES_NLS;

    int i;
    for (i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg == "-rc") {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, RCRequiresArg,
                              "error: '-rc' requires an argument", "the -rc option requires a file argument")<<endl;
                exit(EXIT_FAILURE);
            }

            rc_file = argv[i];
        } else if (arg == "-display") {
            // check for -display option... to run on a display other than the one
            // set by the environment variable DISPLAY

            if ((++i) >= argc) {
                cerr<<_FB_CONSOLETEXT(main, DISPLAYRequiresArg,
                              "error: '-display' requires an argument",
                              "")<<endl;
                exit(EXIT_FAILURE);
            }

            session_display = argv[i];
            string display_env = "DISPLAY=" + session_display;
            if (putenv(const_cast<char *>(display_env.c_str()))) {
                cerr<<_FB_CONSOLETEXT(main, WarnDisplayEnv,
                                "warning: couldn't set environment variable 'DISPLAY'",
                              "")<<endl;
                perror("putenv()");
            }
        } else if (arg == "-version" || arg == "-v") {
            // print current version string
            cout << "Fluxbox " << __fluxbox_version << " : (c) 2001-2007 Fluxbox Team " << endl << endl;
            exit(EXIT_SUCCESS);
        } else if (arg == "-log") {
            if (++i >= argc) {
                cerr<<_FB_CONSOLETEXT(main, LOGRequiresArg, "error: '-log' needs an argument", "")<<endl;
                exit(EXIT_FAILURE);
            }
            log_filename = argv[i];
        } else if (arg == "-help" || arg == "-h") {
            // print program usage and command line options
            printf(_FB_CONSOLETEXT(main, Usage,
                           "Fluxbox %s : (c) %s Henrik Kinnunen\n"
                           "Website: http://www.fluxbox.org/\n\n"
                           "-display <string>\t\tuse display connection.\n"
                           "-screen <all|int,int,int>\trun on specified screens only.\n"
                           "-rc <string>\t\t\tuse alternate resource file.\n"
                           "-version\t\t\tdisplay version and exit.\n"
                           "-info\t\t\t\tdisplay some useful information.\n"
                           "-log <filename>\t\t\tlog output to file.\n"
                           "-help\t\t\t\tdisplay this help text and exit.\n\n",

                           "Main usage string. Please lay it out nicely. There is one %s that is given the version").c_str(),
                   __fluxbox_version, "2001-2007");
            exit(EXIT_SUCCESS);
        } else if (arg == "-info" || arg == "-i") {
            showInfo(cout);
            exit(EXIT_SUCCESS);
        } else if (arg == "-verbose") {
            FbTk::ThemeManager::instance().setVerbose(true);
        }
    }

#ifdef __EMX__
    _chdir2(getenv("X11ROOT"));
#endif // __EMX__
    auto_ptr<Fluxbox> fluxbox;
    int exitcode=EXIT_FAILURE;

    streambuf *outbuf = 0;
    streambuf *errbuf = 0;

    ofstream log_file(log_filename.c_str());

    // setup log file
    if (log_file) {
        cerr<<_FB_CONSOLETEXT(main, LoggingTo, "Logging to", "Logging to a file")<<": "<<log_filename<<endl;
        log_file<<"------------------------------------------"<<endl;
        log_file<<_FB_CONSOLETEXT(main, LogFile, "Log File", "")<<": "<<log_filename<<endl;
        showInfo(log_file);
        log_file<<"------------------------------------------"<<endl;
        // setup log to use cout and cerr stream
        outbuf = cout.rdbuf(log_file.rdbuf());
        errbuf = cerr.rdbuf(log_file.rdbuf());
    }

    try {

        fluxbox.reset(new Fluxbox(argc, argv, session_display.c_str(), rc_file.c_str()));
        fluxbox->eventLoop();

        exitcode = EXIT_SUCCESS;

    } catch (out_of_range &oor) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorOutOfRange, "Out of range", "Error message")<<": "<<oor.what()<<endl;
    } catch (runtime_error &re) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorRuntime, "Runtime error", "Error message")<<": "<<re.what()<<endl;
    } catch (bad_cast &bc) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorBadCast, "Bad cast", "Error message")<<": "<<bc.what()<<endl;
    } catch (bad_alloc &ba) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorBadAlloc, "Bad Alloc", "Error message")<<": "<<ba.what()<<endl;
    } catch (exception &e) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorStandardException, "Standard Exception", "Error message")<<": "<<e.what()<<endl;
    } catch (string error_str) {
        cerr<<_FB_CONSOLETEXT(Common, Error, "Error", "Error message header")<<": "<<error_str<<endl;
    } catch (...) {
        cerr<<"Fluxbox: "<<_FB_CONSOLETEXT(main, ErrorUnknown, "Unknown error", "Error message")<<"."<<endl;
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

            const char *arg = restart_argument.c_str();
            if (arg) {
                execlp(shell, shell, "-c", arg, (const char *) NULL);
                perror(arg);
            }
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

