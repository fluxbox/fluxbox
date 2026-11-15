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

#include "fluxbox.hh"
#include "version.h"
#include "defaults.hh"
#include "cli.hh"

#include "FbTk/I18n.hh"
#include "FbTk/StringUtil.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <typeinfo>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostream;
using std::ofstream;
using std::streambuf;
using std::out_of_range;
using std::runtime_error;
using std::bad_cast;
using std::bad_alloc;
using std::exception;

namespace {

std::unique_ptr<Fluxbox> fluxbox;

// a signal can interrupt fluxbox anywhere, also in the middle of an Xlib call
// (which holds the display lock) or of malloc(). so only async-signal-safe
// things happen here: everything else is left to the event loop via
// Fluxbox::postSignal(), and exits use _exit(), never exit(), which would run
// destructors that talk to the X server or use already destroyed statics
void handleSignal(int signum) {

    const int saved_errno = errno;

    switch (signum) {
#ifndef _WIN32
    case SIGCHLD: // we don't want the child process to kill us
        // more than one process may have terminated
        while (waitpid(-1, 0, WNOHANG | WUNTRACED) > 0) {
            ;
        }
        break;
#endif
    case SIGSEGV:
    case SIGFPE: // returning would run the faulting instruction again
        abort();
        break;
    case SIGALRM:
        // last resort for shutting down fluxbox. the alarm() is set in
        // Fluxbox::shutdown() and covers the destruction of fluxbox, too
        if (Fluxbox::shutdownStarted()) {
            static const char msg[] = "fluxbox took longer than expected to shutdown\n";
            ssize_t ignored = write(STDERR_FILENO, msg, sizeof(msg) - 1);
            (void)ignored;
            _exit(13);
        }
        break;
    default:
        Fluxbox::postSignal(signum);
        break;
    }

    errno = saved_errno;
}

void setupSignalHandling() {
    signal(SIGSEGV, handleSignal);
    signal(SIGSEGV, handleSignal);
    signal(SIGFPE, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGINT, handleSignal);
#ifdef HAVE_ALARM
    signal(SIGALRM, handleSignal);
#endif
#ifndef _WIN32
    signal(SIGPIPE, handleSignal); // e.g. output sent to grep
    signal(SIGCHLD, handleSignal);
    signal(SIGHUP, handleSignal);
    signal(SIGUSR1, handleSignal);
    signal(SIGUSR2, handleSignal);
#endif
}

}

int main(int argc, char **argv) {

    FbTk::I18n::init(0);

    FluxboxCli::Options opts;
    int exitcode = opts.parse(argc, argv);

    if (exitcode != -1) {
        exit(exitcode);
    }
    exitcode = EXIT_FAILURE;

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

        FluxboxCli::showInfo(log_file);
        log_file << "------------------------------------------" << endl;
        // setup log to use cout and cerr stream
        outbuf = cout.rdbuf(log_file.rdbuf());
        errbuf = cerr.rdbuf(log_file.rdbuf());
    }

    FluxboxCli::setupConfigFiles(opts.rc_path, opts.rc_file);
    FluxboxCli::updateConfigFilesIfNeeded(opts.rc_file);

    try {

        fluxbox.reset(new Fluxbox(argc, argv,
                    opts.session_display,
                    opts.rc_path,
                    opts.rc_file,
                    opts.xsync));
        setupSignalHandling();
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
    } catch (string & error_str) {
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

#ifdef HAVE_ALARM
    // fluxbox is destroyed in time: no more need for the last resort alarm()
    // of Fluxbox::shutdown()
    alarm(0);
#endif

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

        const std::string basename = FbTk::StringUtil::basename(argv[0]);
        execvp(basename.c_str(), argv);
        perror(basename.c_str());
    }

    return exitcode;
}

