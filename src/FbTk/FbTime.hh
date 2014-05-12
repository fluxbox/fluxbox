// FbTime.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2012 Mathias Gumz (akira at fluxbox dot org)
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

#ifndef FBTK_FBTIME_HH
#define FBTK_FBTIME_HH

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif // HAVE_INTTYPES_H

namespace FbTk {

// time in micro-seconds
//
// interesting links:
//
// http://www.python.org/dev/peps/pep-0418/#operating-system-time-functions
// http://en.cppreference.com/w/cpp/chrono

namespace FbTime {

    const uint64_t IN_MILLISECONDS =  1000L;
    const uint64_t IN_SECONDS = 1000L * IN_MILLISECONDS;
    const uint64_t IN_MINUTES = 60 * IN_SECONDS;

    uint64_t mono();   // point in time, always monotonic
    uint64_t system(); // system time, might jump (DST, leap seconds)

    // calculates the remaining microseconds from 'now' up to the
    // next full 'unit'
    inline uint64_t remainingNext(uint64_t now, uint64_t unit) {
        return (unit - (now % unit));
    }

} // namespace FbTime

} // namespace FbTk

#endif // FBTK_TIME_HH
