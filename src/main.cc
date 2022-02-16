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

void handleSignal(int signum) {

    _FB_USES_NLS;
    static int re_enter = 0;

    switch (signum) {
#ifndef _WIN32
    case SIGCHLD: // we don't want the child process to kill us
        // more than one process may have terminated
        while (waitpid(-1, 0, WNOHANG | WUNTRACED) > 0);
        break;
    case SIGHUP:
        // xinit sends HUP when it wants to go down. there is no point in
        // restoring anything in the screens / workspaces, the connection
        // to the xserver might drop any moment
        if (fluxbox.get()) { fluxbox->shutdown(1); }
        break;
    case SIGUSR1:
        if (fluxbox.get()) { fluxbox->restart(); }
        break;
    case SIGUSR2:
        if (fluxbox.get()) { fluxbox->reconfigure(); }
        break;
#endif
    case SIGSEGV:
        abort();
        break;
    case SIGALRM:
        // last resort for shutting down fluxbox. the alarm() is set in
        // Fluxbox::shutdown()
        if (fluxbox.get() && fluxbox->isShuttingDown()) {
            cerr << "fluxbox took longer than expected to shutdown\n";
            exit(13);
        }
        break;
    case SIGFPE:
    case SIGINT:
#ifndef _WIN32
    case SIGPIPE:
#endif
    case SIGTERM:
        if (fluxbox.get()) { fluxbox->shutdown(); }
        break;
    default:
        fprintf(stderr,
                _FB_CONSOLETEXT(BaseDisplay, SignalCaught, 
                    "%s:      signal %d caught\n",
                    "signal catch debug message. Include %s for Command<void> and %d for signal number").c_str(),
                "TODO: m_arg[0]", signum);

        if (! fluxbox->isStartup() && ! re_enter) {
            re_enter = 1;
            cerr<<_FB_CONSOLETEXT(BaseDisplay, ShuttingDown, 
                    "Shutting Down\n",
                    "Quitting because of signal, end with newline");
            if (fluxbox.get()) { fluxbox->shutdown(); }
        }

        cerr << _FB_CONSOLETEXT(BaseDisplay, Aborting, 
                    "Aborting... dumping core\n",
                    "Aboring and dumping core, end with newline");
        abort();
        break;
    }
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

