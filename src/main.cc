// main.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//               and 2003 Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: main.cc,v 1.23 2003/08/28 23:18:37 fluxgen Exp $

#include "fluxbox.hh"
#include "I18n.hh"
#include "version.h"
#include "defaults.hh"
#include "FbTk/Theme.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <typeinfo>

using namespace std;
void showInfo(ostream &ostr) {
    ostr<<"Fluxbox version: "<<__fluxbox_version<<endl;
    ostr<<"Compiled: "<<__DATE__<<" "<<__TIME__<<endl;
    ostr<<"Compiler: ";
#ifdef  __GNUG__ 
    ostr<<"GCC";
#else
    ostr<<"Unknown";
#endif
    ostr<<endl<<"Compiler version: "<<__VERSION__<<endl;    

    ostr<<endl<<"Defaults:"<<endl;
    ostr<<"    menu: "<<DEFAULTMENU<<endl;
    ostr<<"   style: "<<DEFAULTSTYLE<<endl;
 
    ostr<<"    keys: "<<DEFAULTKEYSFILE<<endl;
    ostr<<"    init: "<<DEFAULT_INITFILE<<endl;

    const char NOT[] = "-";
    ostr<<endl<<"Compiled options ("<<NOT<<" => disabled): "<<endl<<
#ifndef DEBUG
        NOT<<
#endif // DEBUG                
        "DEBUG"<<endl<<

#ifndef SLIT
        NOT<<
#endif // SLIT
        "SLIT"<<endl<<

#ifndef USE_TOOLBAR
        NOT<<
#endif // USE_TOOLBAR
        "TOOLBAR"<<endl<<

#ifndef HAVE_XPM
        NOT<<
#endif // HAVE_XPM
        "XPM"<<endl<<

#ifndef USE_GNOME
        NOT<<
#endif // USE_GNOME 
        "GNOME"<<endl<<

#ifndef KDE
        NOT<<
#endif // KDE
        "KDE"<<endl<<

#ifndef USE_NEWWMSPEC
        NOT<<
#endif // USE_NEWWMSPEC
        "EWMH"<<endl<<

#ifndef REMEMBER
        NOT<<
#endif // REMEMBER
        "REMEMBER"<<endl<<

#ifndef SHAPE
        NOT<<
#endif // SHAPE
        "SHAPE"<<endl<<

#ifndef USE_XFT
        NOT<<
#endif // USE_XFT
        "XFT"<<endl<<

#ifndef USE_XMB
        NOT<<
#endif // USE_XMB
        "XMB"<<endl<<

#ifndef XINERAMA
        NOT<<
#endif // XINERAMA
        "XINERAMA"<<endl<<

#ifndef HAVE_XRENDER
        NOT<<
#endif // HAVE_XRENDER
        "RENDER"<<endl<<
        endl;
}

int main(int argc, char **argv) {
	
    std::string session_display;
    std::string rc_file;
    std::string log_filename;

    NLSInit("fluxbox.cat");
    I18n &i18n = *I18n::instance();
	
    int i;
    for (i = 1; i < argc; ++i) {
        if (! strcmp(argv[i], "-rc")) {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                fprintf(stderr,
                        i18n.getMessage(FBNLS::mainSet, FBNLS::mainRCRequiresArg,
                                         "error: '-rc' requires and argument\n"));	
                exit(1);
            }

            rc_file = argv[i];
        } else if (! strcmp(argv[i], "-display")) {
            // check for -display option... to run on a display other than the one
            // set by the environment variable DISPLAY

            if ((++i) >= argc) {
                fprintf(stderr,
                        i18n.getMessage(FBNLS::mainSet, FBNLS::mainDISPLAYRequiresArg,
                                         "error: '-display' requires an argument\n"));
                exit(1);
            }

            session_display = argv[i];
            std::string display_env = "DISPLAY=" + session_display;
            if (putenv(const_cast<char *>(display_env.c_str()))) {
                fprintf(stderr,
                        i18n.
                        getMessage(FBNLS::mainSet, FBNLS::mainWarnDisplaySet,
                                   "warning: couldn't set environment variable 'DISPLAY'\n"));
                perror("putenv()");
            }
        } else if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
            // print current version string
            cout<<"Fluxbox "<<__fluxbox_version<<" : (c) 2001-2003 Henrik Kinnunen "<<endl<<endl;
            exit(0);
        } else if (strcmp(argv[i], "-log") == 0 ) {
            if (i + 1 >= argc) {
                cerr<<"error: '-log' need an argument"<<endl;
                exit(1);
            }
            log_filename = argv[++i];
        } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
            // print program usage and command line options
            printf(i18n.
                   getMessage(FBNLS::mainSet, FBNLS::mainUsage,
                              "Fluxbox %s : (c) %s Henrik Kinnunen\n"
                              "Website: http://www.fluxbox.org/ \n\n"
                              "	-display <string>\t\tuse display connection.\n"
                              "	-rc <string>\t\t\tuse alternate resource file.\n"
                              "	-version\t\t\tdisplay version and exit.\n"
                              "	-info\t\t\t\tdisplay some useful information.\n"
                              "\t-log <filename>\t\t\tlog output to file.\n"
                              "	-help\t\t\t\tdisplay this help text and exit.\n\n"),
                   __fluxbox_version, "2001-2003");
            exit(0);
        } else if (strcmp(argv[i], "-info") == 0 || strcmp(argv[i], "-i") == 0) {
            showInfo(cout);
            exit(0);
        } else if (strcmp(argv[i], "-verbose") == 0) {
            FbTk::ThemeManager::instance().setVerbose(true);
        }
    }

#ifdef __EMX__
    _chdir2(getenv("X11ROOT"));
#endif // __EMX__
    std::auto_ptr<Fluxbox> fluxbox;
    int exitcode=EXIT_FAILURE;

    streambuf *outbuf = 0;
    streambuf *errbuf = 0;

    ofstream log_file(log_filename.c_str());

    // setup log file
    if (log_file) {
        cerr<<"Loggin to: "<<log_filename<<endl;
        log_file<<"------------------------------------------"<<endl;
        log_file<<"Logfile: "<<log_filename<<endl;
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

    } catch (std::out_of_range &oor) {
        cerr<<"Fluxbox: Out of range: "<<oor.what()<<endl;
    } catch (std::runtime_error &re) {
        cerr<<"Fluxbox: Runtime error: "<<re.what()<<endl;
    } catch (std::bad_cast &bc) {
        cerr<<"Fluxbox: Bad cast: "<<bc.what()<<endl; 
    } catch (std::bad_alloc &ba) {
        cerr<<"Fluxbox: Bad Alloc: "<<ba.what()<<endl;
    } catch (std::exception &e) {
        cerr<<"Fluxbox: Standard exception: "<<e.what()<<endl;
    } catch (std::string error_str) {
        cerr<<"Error: "<<error_str<<endl;
    } catch (...) {
        cerr<<"Fluxbox: Unknown error."<<endl;
        abort();
    }
 
    // restore cout and cin streams
    if (outbuf != 0)
        cout.rdbuf(outbuf);
    if (errbuf != 0)
        cerr.rdbuf(errbuf);

    return exitcode;
}
