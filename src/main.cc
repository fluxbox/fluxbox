// main.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen(at)linuxmail.org)
//
// main.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// $Id: main.cc,v 1.15 2003/05/07 22:19:59 fluxgen Exp $



#include "i18n.hh"
#include "fluxbox.hh"

#include "../version.h"

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

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#include <iostream>
#include <stdexcept>
#include <typeinfo>

using namespace std;

int main(int argc, char **argv) {
	
    char *session_display = 0;
    char *rc_file = 0;

    NLSInit("fluxbox.cat");
    I18n *i18n = I18n::instance();
	
    int i;
    for (i = 1; i < argc; ++i) {
        if (! strcmp(argv[i], "-rc")) {
            // look for alternative rc file to use

            if ((++i) >= argc) {
                fprintf(stderr,
                        i18n->getMessage(
                                         FBNLS::mainSet, FBNLS::mainRCRequiresArg,
                                         "error: '-rc' requires and argument\n"));	
                exit(1);
            }

            rc_file = argv[i];
        } else if (! strcmp(argv[i], "-display")) {
            // check for -display option... to run on a display other than the one
            // set by the environment variable DISPLAY

            if ((++i) >= argc) {
                fprintf(stderr,
                        i18n->getMessage(				
                                         FBNLS::mainSet, FBNLS::mainDISPLAYRequiresArg,				
                                         "error: '-display' requires an argument\n"));
                exit(1);
            }

            session_display = argv[i];
            char dtmp[255];
            sprintf(dtmp, "DISPLAY=%s", session_display);

            if (putenv(dtmp)) {
                fprintf(stderr,
                        i18n->
                        getMessage(
                                   FBNLS::mainSet, FBNLS::mainWarnDisplaySet,
                                   "warning: couldn't set environment variable 'DISPLAY'\n"));
                perror("putenv()");
            }
        } else if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
            // print current version string
            printf("Fluxbox %s : (c) 2001-2003 Henrik Kinnunen \n\n",
                   __fluxbox_version);
            exit(0);
        } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
            // print program usage and command line options
            printf(i18n->
                   getMessage(
                              FBNLS::mainSet, FBNLS::mainUsage,
                              "Fluxbox %s : (c) 2001-2003 Henrik Kinnunen\n\n"
                              "	-display <string>\t\tuse display connection.\n"
                              "	-rc <string>\t\t\tuse alternate resource file.\n"
                              "	-version\t\t\tdisplay version and exit.\n"
                              "	-help\t\t\t\tdisplay this help text and exit.\n\n"),
                   __fluxbox_version);

            cout<<"Compiled with: "<<endl<<
#ifdef DEBUG
                "DEBUG"<<endl<<
#endif // DEBUG                
#ifdef SLIT
                "SLIT"<<endl<<
#endif // SLIT
#ifdef HAVE_XPM
                "XPM"<<endl<<
#endif // HAVE_XPM
#ifdef USE_GNOME
                "GNOME"<<endl<<
#endif // USE_GNOME 
#ifdef KDE
                "KDE"<<endl<<
#endif // KDE
#ifdef USE_NEWWMSPEC
                "EWMH"<<endl<<
#endif // USE_NEWWMSPEC
#ifdef REMEMBER
                "REMEMBER"<<endl<<
#endif // REMEMBER
#ifdef SHAPE
                "SHAPE"<<endl<<
#endif // SHAPE
#ifdef USE_XFT
                "XFT"<<endl<<
#endif // USE_XFT
#ifdef USE_XMB
                "XMB"<<endl<<
#endif // USE_XMB
#ifdef XINERAMA
                "XINERAMA"<<endl<<
#endif // XINERAMA
#ifdef HAVE_XRENDER
                "RENDER"<<endl<<
#endif // HAVE_XRENDER
                endl;
            ::exit(0);
        }
    }

#ifdef __EMX__
    _chdir2(getenv("X11ROOT"));
#endif // __EMX__
    Fluxbox *fluxbox=0;
    int exitcode=EXIT_FAILURE;
    try {
		
        fluxbox = new Fluxbox(argc, argv, session_display, rc_file);
        fluxbox->eventLoop();
	exitcode = EXIT_SUCCESS;
    } catch (std::out_of_range &oor) {
        cerr<<"Fluxbox: Out of range: "<<oor.what()<<endl;
    } catch (std::logic_error &le) {
        cerr<<"Fluxbox: Logic error: "<<le.what()<<endl;
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
	
    if (fluxbox)
        delete fluxbox;
    return exitcode;
}
