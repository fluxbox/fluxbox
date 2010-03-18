// RelaySignal.hh
// Copyright (c) 2010 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FBTK_RELAY_SIGNAL_HH
#define FBTK_RELAY_SIGNAL_HH

#include "Signal.hh"
#include "MemFun.hh"
#include "Subject.hh"

namespace FbTk {

/**
 * Relays a new signal type to the old subject type signal. When the new signal
 * emits the subject notify() will be called.
 * This function is temporary and just a helper during transition between old
 * and new signal system.
 *
 * @param from The original source of the signal.
 * @param to_subject The destination Subject.
 */
template < typename Signal >
void relaySignal(Signal& from, FbTk::Subject& to_subject) {
    from.connect(MemFunIgnoreArgs(to_subject, &FbTk::Subject::notify));
}

/**
 * Relays a new signal type to the old subject type signal. When the new signal
 * emits the subject notify() will be called.
 * This function is temporary and just a helper during transition between old
 * and new signal system.
 *
 * @param tracker Keeps track of signals
 * @param from The original source of the signal.
 * @param to_subject The destination Subject
 */
template < typename Signal >
void relaySignal(SignalTracker& tracker, Signal& from, FbTk::Subject& to_subject) {
    tracker.join(from, MemFunIgnoreArgs(to_subject, &FbTk::Subject::notify));
}

} // end namespace FbTk

#endif // FBTK_RELAY_SIGNAL_HH
