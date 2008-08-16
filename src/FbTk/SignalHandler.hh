// SignalHandler.hh for FbTk
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_SIGNALHANDLER_HH
#define FBTK_SIGNALHANDLER_HH

#include <signal.h>

namespace FbTk {

/// Base class that SignalHandler calls when it gets a signal
/// Use this to catch system signals
class SignalEventHandler {
public:
    virtual void handleSignal(int signum) = 0;
    virtual ~SignalEventHandler() { }
};

///   Handles system signals, singleton.
/**
   Usage: inherit the class SignalEventHandler and then register 
   it to SignalHandler by calling registerHandler with
   a signal number
*/
class SignalHandler {
public:
    /// get singleton object
    static SignalHandler &instance();
    /** 
        Register an event handler
        @return true on success else false
        @param signum signal number
        @param eh event handler
        @param oldhandler_ret return handler to old sighandler
    */
    bool registerHandler(int signum, SignalEventHandler *eh, SignalEventHandler **oldhandler_ret = 0);

private:
    SignalHandler();

    static void handleSignal(int signum);

    static SignalEventHandler *s_signal_handler[NSIG]; ///< NSIG defined in signal.h
}; 

} // end namespace FbTk

#endif // FBTK_SIGNALHANDLER_HH
