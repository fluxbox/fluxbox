// RegExp.cc for FbTk
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

#include "RegExp.hh"

#include <iostream>

using std::string;

#ifdef USE_REGEXP
using std::cerr;
using std::endl;
#endif // USE_REGEXP

namespace FbTk {

// full_match is to say if we match on this regexp using the full string
// or just a substring. Substrings aren't supported if not HAVE_REGEXP
RegExp::RegExp(const string &str, bool full_match):
#ifdef USE_REGEXP
m_regex(0) {
    string match;
    if (full_match) {
        match = "^";
        match.append(str);
        match.append("$");
    } else {
        match = str;
    }

    m_regex = new regex_t;
    int ret = regcomp(m_regex, match.c_str(), REG_NOSUB | REG_EXTENDED);
    if (ret != 0) {
        delete m_regex; // I don't think I regfree a failed compile?
        m_regex = 0;
    }
}
#else // notdef USE_REGEXP
m_str(str) {}
#endif // USE_REGEXP

RegExp::~RegExp() {
#ifdef USE_REGEXP
    if (m_regex != 0) {
        regfree(m_regex);
        delete m_regex;
    }
#endif // USE_REGEXP
}

bool RegExp::match(const string &str) const {
#ifdef USE_REGEXP
    if (m_regex)
        return regexec(m_regex, str.c_str(), 0, 0, 0) == 0;
    else
        return false;
#else // notdef USE_REGEXP
    return (m_str == str);
#endif // USE_REGEXP
}


bool RegExp::error() const {
#ifdef USE_REGEXP
    return m_regex == 0;
#else
    return m_str == "";
#endif // USE_REGEXP
}

} // end namespace FbTk
