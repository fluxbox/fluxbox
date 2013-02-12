// RegExp.hh for FbTk
// Copyright (c) 2002 Xavier Brouckaert
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef FBTK_REGEXP_HH
#define FBTK_REGEXP_HH

#include "NotCopyable.hh"

#include <string>

/*
 * If USE_REGEXP isn't defined, then we match just using simple string equality
 */

#ifdef USE_REGEXP
#include <sys/types.h>
#include <regex.h>
#endif // USE_REGEXP

namespace FbTk {

class RegExp: private NotCopyable {
public:
    RegExp(const std::string &str, bool full_match = true);
    ~RegExp();

    bool match(const std::string &str) const;

    bool error() const;

private:
#ifdef USE_REGEXP
    regex_t* m_regex;
#else // notdef USE_REGEXP
    std::string m_str;
#endif // USE_REGEXP

};

} // end namespace FbTk

#endif // FBTK_REGEXP_HH
