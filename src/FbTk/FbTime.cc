// FbTime.cc for FbTk - Fluxbox Toolkit
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.

#include "FbTime.hh"

#include <cstdlib>
#include <sys/time.h>


#ifdef HAVE_CLOCK_GETTIME // linux|*bsd|solaris
#include <time.h>

namespace {

uint64_t _mono() {

    uint64_t t = 0L;
    timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        t = (ts.tv_sec * FbTk::FbTime::IN_SECONDS) + (ts.tv_nsec / 1000L);
    }

    return t;
}

}

#endif // HAVE_CLOCK_GETTIME





#ifdef HAVE_MACH_ABSOLUTE_TIME // macosx

// http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
// https://github.com/ThomasHabets/monotonic_clock/blob/master/src/monotonic_mach.c
// http://shiftedbits.org/2008/10/01/mach_absolute_time-on-the-iphone/


#include <mach/mach_time.h>

namespace {

uint64_t _mono() {

    // mach_absolute_time() * info.numer / info.denom yields
    // nanoseconds.

    static double micro_scale = 0.001;  // 1000ms == 1ns
    static bool initial = true;

    if (initial) {
        initial = false;
        mach_timebase_info_data_t info;
        if (mach_timebase_info(&info) == 0) {
            micro_scale *= static_cast<double>(info.numer) / static_cast<double>(info.denom);
        }
    }

    return static_cast<uint64_t>(mach_absolute_time() * micro_scale);
}

}

#endif // HAVE_MACH_ABSOLUTE_TIME

static uint64_t start = ::_mono();

uint64_t FbTk::FbTime::mono() {
    return ::_mono() - start;
}


uint64_t FbTk::FbTime::system() {
    static timeval v;
    gettimeofday(&v, NULL);
    return (v.tv_sec * FbTk::FbTime::IN_SECONDS) + v.tv_usec;
}


