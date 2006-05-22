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

#include "FbString.hh"
#include "config.h"

#include <stdio.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV
#include <langinfo.h>
#include <locale.h>

#include <iostream>
using namespace std;

namespace FbTk {

namespace FbStringUtil {

enum ConvType { FB2X = 0, X2FB, LOCALE2FB, FB2LOCALE, CONVSIZE };

#ifdef HAVE_ICONV
static iconv_t *iconv_convs = 0;
#else
static int iconv_convs[CONVSIZE];
#endif // HAVE_ICONV

/// Initialise all of the iconv conversion descriptors
void init() {
    static bool s_init = false;
    if (s_init)
        return;
    s_init = true;

    iconv_convs = new iconv_t[CONVSIZE];

    setlocale(LC_CTYPE, "");

#ifdef HAVE_ICONV
#ifdef CODESET
    std::string locale_codeset = nl_langinfo(CODESET);
#else // openbsd doesnt have this (yet?)
    std::string locale_codeset = "";
    std::string locale = setlocale(LC_CTYPE, NULL);
    size_t pos = locale.find('.');
    if (pos != std::string::npos) 
        locale_codeset = locale.substr(pos);
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
    for (int i=0; i < CONVSIZE; ++i) 
        if (iconv_convs[i] != (iconv_t)(-1))
            iconv_close(iconv_convs[i]);
#endif // HAVE_ICONV

    delete[] iconv_convs;

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
std::string recode(iconv_t cd,
             const std::string &in) {

    // If empty message, yes this can happen, return
    if (in.empty()) 
        return "";

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
    std::string ret;
    ret.append(out, outsize - outbytesleft);

    // reset the conversion descriptor
    iconv(cd, NULL, NULL, NULL, NULL);

    if (out)
        free(out);

    return ret;
}
#else
std::string recode(int cd,
             const std::string &str) {
    return str;
}
#endif // HAVE_ICONV

FbString XStrToFb(const std::string &src) {
    return recode(iconv_convs[X2FB], src);
}

std::string FbStrToX(const FbString &src) {
    return recode(iconv_convs[FB2X], src);
}


/// Handle thislocale string encodings (strings coming from userspace)
FbString LocaleStrToFb(const std::string &src) {
    return recode(iconv_convs[LOCALE2FB], src);
}

std::string FbStrToLocale(const FbString &src) {
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

}; // end namespace FbTk
