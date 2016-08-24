// FbString.cc for fluxbox
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

#include "FbString.hh"

#ifdef HAVE_CERRNO
  #include <cerrno>
#else
  #include <errno.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#include <langinfo.h>
#include <locale.h>

#include <iostream>
#include <vector>

#ifndef HAVE_ICONV
typedef int iconv_t;
#endif // HAVE_ICONV

#ifdef HAVE_FRIBIDI
  #include <fribidi.h>
#endif

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

namespace {

const iconv_t ICONV_NULL = (iconv_t)(-1);

#ifdef HAVE_FRIBIDI
FbTk::FbString makeVisualFromLogical(const FbTk::FbString& src) {

    FriBidiCharType base = FRIBIDI_TYPE_N;

    // reuse allocated memory for reencoding / reordering
    static std::vector<FriBidiChar> us;
    static std::vector<FriBidiChar> out_us;
    static FbTk::FbString result;

    const size_t S = src.size() + 1;
    const size_t S4 = S * 4;

    if (us.capacity() < S)
        us.reserve(S);
    if (out_us.capacity() < S)
        out_us.reserve(S);
    if (result.capacity() < S4)
        result.reserve(S4);

    us.resize(S);
    FriBidiStrIndex len = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8,
            const_cast<char*>(src.c_str()), S - 1,
            &us[0]);

    out_us.resize(S);
    fribidi_log2vis(&us[0], len, &base, &out_us[0], NULL, NULL, NULL);

    result.resize(S4);
    len = fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, &out_us[0], len, &result[0]);
    result.resize(len); // trim to currently used chars

    return result;
}

#endif

} // end of anonymous namespace


namespace FbTk {

BiDiString::BiDiString(const FbString& logical) 
#ifdef HAVE_FRIBIDI
    : m_visual_dirty(false)
#endif
{
    if (!logical.empty())
        setLogical(logical);
}

const FbString& BiDiString::setLogical(const FbString& logical) {
    m_logical = logical;
#if HAVE_FRIBIDI
    if (m_logical.empty()) {
        m_visual_dirty = false;
        m_visual.clear();
    } else {
        m_visual_dirty = true;
    }
#endif
    return m_logical;
}

const FbString& BiDiString::visual() const {
#if HAVE_FRIBIDI
    if (m_visual_dirty) {
        m_visual = ::makeVisualFromLogical(logical());
    }
    m_visual_dirty = false;
    return m_visual;
#else
    return m_logical;
#endif
}



namespace FbStringUtil {

enum ConvType {
    FB2X = 0,
    X2FB,
    LOCALE2FB,
    FB2LOCALE,
    CONVSIZE
};

static bool s_inited = false;
static iconv_t s_iconv_convs[CONVSIZE];
static std::string s_locale_codeset;

/// Initialise all of the iconv conversion descriptors
void init() {

    if (s_inited)
        return;

    s_inited = true;
    setlocale(LC_CTYPE, "");

    for (int i = 0; i < CONVSIZE; i++) {
        s_iconv_convs[i] = ICONV_NULL;
    }

#ifdef HAVE_ICONV
#if defined(CODESET) && !defined(_WIN32)
    s_locale_codeset = nl_langinfo(CODESET);
#else // openbsd doesnt have this (yet?)
    std::string locale = setlocale(LC_CTYPE, NULL);
    size_t pos = locale.find('.');
    if (pos != std::string::npos)
        s_locale_codeset = locale.substr(pos+1);
#endif // CODESET

#ifdef DEBUG
    cerr << "FbTk::FbString: setup converts for local codeset = " << s_locale_codeset << endl;
#endif // DEBUG

    s_iconv_convs[FB2X] = iconv_open("ISO8859-1", "UTF-8");
    s_iconv_convs[X2FB] = iconv_open("UTF-8", "ISO8859-1");
    s_iconv_convs[FB2LOCALE] = iconv_open(s_locale_codeset.c_str(), "UTF-8");
    s_iconv_convs[LOCALE2FB] = iconv_open("UTF-8", s_locale_codeset.c_str());
#endif // HAVE_ICONV

}

void shutdown() {
#ifdef HAVE_ICONV
    int i;
    for (i = 0; i < CONVSIZE; ++i) {
        if (s_iconv_convs[i] != ICONV_NULL) {
            iconv_close(s_iconv_convs[i]);
            s_iconv_convs[i] = ICONV_NULL;
        }
    }

    s_inited = false;
#endif // HAVE_ICONV
}




/**
   Recodes the text from one encoding to another
   assuming cd is correct
   @param cd the iconv type
   @param msg text to be converted, **NOT** necessarily NULL terminated
   @param size number of BYTES to convert
   @return the recoded string, or 0 on failure
*/
std::string recode(iconv_t cd, const std::string &in) {

#ifdef HAVE_ICONV
/**
  --NOTE--
  In the "C" locale, this will strip any high-bit characters
  because C means 7-bit ASCII charset. If you don't want this
  then you need to set your locale to something UTF-8, OR something
  ISO8859-1.
*/

    // If empty message, yes this can happen, return
    if (in.empty())
        return "";

    if (cd == ICONV_NULL)
        return in; // can't convert

    size_t insize = in.size();
    size_t outsize = insize;
    std::vector<char> out(outsize);
    char* out_ptr = &out[0];

    size_t inbytesleft = insize;
    size_t outbytesleft = outsize;

#ifdef HAVE_CONST_ICONV
    const char* in_ptr = in.data();
#else
    char* in_ptr = const_cast<char*>(in.data());
#endif
    size_t result = (size_t)(-1);
    bool again = true;

    while (again) {
        again = false;

        result = iconv(cd, &in_ptr, &inbytesleft, &out_ptr, &outbytesleft);

        if (result == (size_t)(-1)) {
            switch(errno) {
            case EILSEQ:
                // Try skipping a byte
                in_ptr++;
                inbytesleft--;
                again = true;
            case EINVAL:
                break;
            case E2BIG:
                // need more space!
                outsize += insize;
                out.resize(outsize);
                if (out.capacity() != outsize)
                    again = true;
                outbytesleft += insize;
                out_ptr = (&out[0] + outsize) - outbytesleft;
                break;
            default:
                // something else broke
                perror("iconv");
                break;
            }
        }
    }

    // copy to our return string
    std::string ret;
    ret.append(&out[0], outsize - outbytesleft);

    // reset the conversion descriptor
    iconv(cd, NULL, NULL, NULL, NULL);

    return ret;
#else
    return in;
#endif // HAVE_ICONV
}

FbString XStrToFb(const std::string &src) {
    return recode(s_iconv_convs[X2FB], src);
}

std::string FbStrToX(const FbString &src) {
    return recode(s_iconv_convs[FB2X], src);
}


/// Handle thislocale string encodings (strings coming from userspace)
FbString LocaleStrToFb(const std::string &src) {
    return recode(s_iconv_convs[LOCALE2FB], src);
}

std::string FbStrToLocale(const FbString &src) {
    return recode(s_iconv_convs[FB2LOCALE], src);
}

bool haveUTF8() {
#ifdef HAVE_ICONV
    if (s_iconv_convs[LOCALE2FB] != ICONV_NULL)
        return true;
#endif // HAVE_ICONV

    return false;
}


} // end namespace StringUtil

#ifdef HAVE_ICONV
StringConvertor::StringConvertor(EncodingTarget target) : m_iconv(ICONV_NULL) {
    if (target == ToLocaleStr)
        m_destencoding = FbStringUtil::s_locale_codeset;
    else
        m_destencoding = "UTF-8";
}
#else
StringConvertor::StringConvertor(EncodingTarget target) { }
#endif

StringConvertor::~StringConvertor() {
    reset();
}

bool StringConvertor::setSource(const std::string &encoding) {
#ifdef HAVE_ICONV
    std::string tempenc = encoding.empty() ? FbStringUtil::s_locale_codeset : encoding;

    if ((tempenc == m_destencoding) && (m_iconv == ICONV_NULL)) {
        return true;
    }

    iconv_t newiconv = iconv_open(m_destencoding.c_str(), tempenc.c_str());
    if (newiconv == ICONV_NULL)
        return false;
    else {
        if (m_iconv != ICONV_NULL)
            iconv_close(m_iconv);
        m_iconv = newiconv;
        return true;
    }
#else
    return false;
#endif
}

FbString StringConvertor::recode(const std::string &src) {
#ifdef HAVE_ICONV
    return FbStringUtil::recode(m_iconv, src);
#else
    return src;
#endif
}

void StringConvertor::reset() {
#ifdef HAVE_ICONV
    if (m_iconv != ICONV_NULL)
        iconv_close(m_iconv);
    m_iconv = ICONV_NULL;
#endif
}


} // end namespace FbTk
