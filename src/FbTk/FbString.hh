// FbString.hh for fluxbox 
// Copyright (c) 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
// Copyright (c) 2006 Simon Bowden    (rathnor at fluxbox dot org)
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

//$Id$

#ifndef FBTK_FBSTRING_HH
#define FBTK_FBSTRING_HH

#include <string>

#include "config.h"
#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

namespace FbTk {

// Use this type for things converted to our internal encoding (UTF-8)
// (or just plain whatever for now if no utf-8 available)
typedef std::string FbString;

namespace FbStringUtil {

void init();
void shutdown();

/// Stuff to handle strings in different encodings
/// Rule: Only hardcode-initialise FbStrings as ascii (7bit) characters

// NOTE: X "STRING" types are defined (ICCCM) as ISO Latin-1 encoding
FbString XStrToFb(const std::string &src);
std::string FbStrToX(const FbString &src);

/// Handle thislocale string encodings (strings coming from userspace)
FbString LocaleStrToFb(const std::string &src);
std::string FbStrToLocale(const FbString &src);

bool haveUTF8();

} // namespace FbStringUtil

class StringConvertor {
public:

    enum EncodingTarget { ToFbString, ToLocaleStr };

    StringConvertor(EncodingTarget target);
    ~StringConvertor();

    bool setSource(const std::string &encoding);
    void reset() {
#ifdef HAVE_ICONV
 m_iconv = ((iconv_t)(-1));
#endif
    }

    std::string recode(const std::string &src);

private:
#ifdef HAVE_ICONV
    iconv_t m_iconv;
#else
    int m_iconv;
#endif

    std::string m_destencoding;
};

} // namespace FbTk

#endif // FBTK_FBSTRING_HH

