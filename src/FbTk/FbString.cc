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

// $Id$

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

#include "FbString.hh"
#include "config.h"

#include <stdio.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV
#include <langinfo.h>
#include <locale.h>

#include <iostream>

using std::string;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

namespace FbTk {

namespace FbStringUtil {

enum ConvType { FB2X = 0, X2FB, LOCALE2FB, FB2LOCALE, CONVSIZE };

#ifdef HAVE_ICONV
static iconv_t *iconv_convs = 0;
#else
static int iconv_convs[CONVSIZE];
#endif // HAVE_ICONV

static string locale_codeset;

/// Initialise all of the iconv conversion descriptors
void init() {
    setlocale(LC_CTYPE, "");

#ifdef HAVE_ICONV
    if (iconv_convs != 0)
        return;

    iconv_convs = new iconv_t[CONVSIZE];

#ifdef CODESET
    locale_codeset = nl_langinfo(CODESET);
#else // openbsd doesnt have this (yet?)
    locale_codeset = "";
    string locale = setlocale(LC_CTYPE, NULL);
    size_t pos = locale.find('.');
    if (pos != string::npos)
        locale_codeset = locale.substr(pos+1);
#endif // CODESET

#ifdef DEBUG
    cerr<<"FbTk::FbString: setup converts for local codeset = "<<locale_codeset<<endl;
#endif // DEBUG

    iconv_convs[FB2X] = iconv_open("ISO8859-1", "UTF-8");
    iconv_convs[X2FB] = iconv_open("UTF-8", "ISO8859-1");
    iconv_convs[FB2LOCALE] = iconv_open(locale_codeset.c_str(), "UTF-8");
    iconv_convs[LOCALE2FB] = iconv_open("UTF-8", locale_codeset.c_str());
#else
    for (int i=0; i < CONVSIZE; ++i)
        iconv_convs[i] = 0;
#endif // HAVE_ICONV

}

void shutdown() {
#ifdef HAVE_ICONV
    if (iconv_convs == 0)
        return;

    for (int i=0; i < CONVSIZE; ++i)
        if (iconv_convs[i] != (iconv_t)(-1))
            iconv_close(iconv_convs[i]);

    delete[] iconv_convs;
    iconv_convs = 0;
#endif // HAVE_ICONV
}




#ifdef HAVE_ICONV
/**
   Recodes the text from one encoding to another
   assuming cd is correct
   @param cd the iconv type
   @param msg text to be converted, **NOT** necessarily NULL terminated
   @param size number of BYTES to convert
   @return the recoded string, or 0 on failure
*/

/**
  --NOTE--
  In the "C" locale, this will strip any high-bit characters
  because C means 7-bit ASCII charset. If you don't want this
  then you need to set your locale to something UTF-8, OR something
  ISO8859-1.
*/
string recode(iconv_t cd,
             const string &in) {

    // If empty message, yes this can happen, return
    if (in.empty())
        return "";

    if (cd == ((iconv_t)(-1)))
        return in; // can't convert

    size_t insize = in.size();
    size_t outsize = insize;
    char * out = (char *) malloc(outsize * sizeof(char)); // need realloc
    char * out_ptr = out;

    size_t inbytesleft = insize;
    size_t outbytesleft = outsize;

    char * in_ptr = const_cast<char *>(in.data());
    size_t result = (size_t)(-1);
    bool again = true;

    while (again) {
        again = false;

#ifdef HAVE_CONST_ICONV
        result = iconv(cd, (const char**)(&in_ptr), &inbytesleft, &out_ptr, &outbytesleft);
#else
        result = iconv(cd, &in_ptr, &inbytesleft, &out_ptr, &outbytesleft);
#endif  // HAVE_CONST_ICONV

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
                out = (char *) realloc(out, outsize*sizeof(char));
                if (out != NULL)
                    again = true;
                outbytesleft += insize;
                out_ptr = out + outsize - outbytesleft;
                break;
            default:
                // something else broke
                perror("iconv");
                break;
            }
        }
    }

    // copy to our return string
    string ret;
    ret.append(out, outsize - outbytesleft);

    // reset the conversion descriptor
    iconv(cd, NULL, NULL, NULL, NULL);

    if (out)
        free(out);

    return ret;
}
#else
string recode(int cd,
             const string &str) {
    return str;
}
#endif // HAVE_ICONV

FbString XStrToFb(const string &src) {
    return recode(iconv_convs[X2FB], src);
}

string FbStrToX(const FbString &src) {
    return recode(iconv_convs[FB2X], src);
}


/// Handle thislocale string encodings (strings coming from userspace)
FbString LocaleStrToFb(const string &src) {
    return recode(iconv_convs[LOCALE2FB], src);
}

string FbStrToLocale(const FbString &src) {
    return recode(iconv_convs[FB2LOCALE], src);
}

bool haveUTF8() {
#ifdef HAVE_ICONV
    if (iconv_convs[LOCALE2FB] != ((iconv_t)(-1)))
        return true;
#endif // HAVE_ICONV

    return false;
}


}; // end namespace StringUtil

StringConvertor::StringConvertor(EncodingTarget target):
#ifdef HAVE_ICONV
    m_iconv((iconv_t)(-1)) {
    if (target == ToLocaleStr)
        m_destencoding = FbStringUtil::locale_codeset;
    else
        m_destencoding = "UTF-8";
}
#else
     m_iconv(-1) {}
#endif


StringConvertor::~StringConvertor() {
#ifdef HAVE_ICONV
    if (m_iconv != ((iconv_t)-1))
        iconv_close(m_iconv);
#endif
}

bool StringConvertor::setSource(const string &encoding) {
#ifdef HAVE_ICONV
    string tempenc = encoding;
    if (encoding == "")
        tempenc = FbStringUtil::locale_codeset;

    iconv_t newiconv = iconv_open(m_destencoding.c_str(), tempenc.c_str());
    if (newiconv == ((iconv_t)(-1)))
        return false;
    else {
        if (m_iconv != ((iconv_t)-1))
            iconv_close(m_iconv);
        m_iconv = newiconv;
        return true;
    }
#else
    return false;
#endif
}

string StringConvertor::recode(const string &src) {
#ifdef HAVE_ICONV
    return FbStringUtil::recode(m_iconv, src);
#else
    return src;
#endif
}

}; // end namespace FbTk
