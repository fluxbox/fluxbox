// StringUtil.hh for fluxbox 
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_STRINGUTIL_HH
#define FBTK_STRINGUTIL_HH

#include <string>

namespace FbTk {

namespace StringUtil {


/// Similar to `strstr' but this function ignores the case of both strings
const char *strcasestr(const char *str, const char *ptn);

/// expands ~ to value of ${HOME} enviroment variable
std::string expandFilename(const std::string &filename);

/// @return extension of filename (ex: filename.txt will return txt)
std::string findExtension(const std::string &filename);

/// @return copy of original with find_string replaced with "replace"
std::string replaceString(const std::string &original, 
                          const char *find_string,
                          const char *replace);

///  returns string between character first and last
int getStringBetween(std::string& out, const char *instr,
                     char first, char last,
                     const char *ok_chars=" \t\n", bool allow_nesting = false);

/// @return lower case letters of conv
std::string toLower(const std::string &conv);
/// @return upper case letters of conv
std::string toUpper(const std::string &conv);
#ifdef basename
#undef basename
#endif // basename
std::string basename(const std::string &basename);


/// removes the first whitespace characters of the string
std::string::size_type removeFirstWhitespace(std::string &str);
std::string::size_type removeTrailingWhitespace(std::string &str);

/// Breaks a string into tokens
template <typename Container>
static void
stringtok (Container &container, std::string const &in,
           const char * const delimiters = " \t\n") {

    const std::string::size_type len = in.length();
    std::string::size_type i = 0;

    while ( i < len ) {
        // eat leading whitespace
        i = in.find_first_not_of(delimiters, i);
        if (i == std::string::npos)
            return;   // nothing left but white space

        // find the end of the token
        std::string::size_type j = in.find_first_of(delimiters, i);

        // push token
        if (j == std::string::npos) {
            container.push_back(in.substr(i));
            return;
        } else
            container.push_back(in.substr(i, j-i));

        // set up for next loop
        i = j + 1;
    }
}

} // end namespace StringUtil

} // end namespace FbTk


#endif // FBTK_STRINGUTIL_HH
