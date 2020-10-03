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

#ifndef FBTK_STRINGUTIL_HH
#define FBTK_STRINGUTIL_HH

#include <string>

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#include <stdlib.h>
#endif

namespace FbTk {

namespace StringUtil {

/// \@{
/// @param in - input string, might be 0xab or 0123
/// @param out - result if extraction was ok
/// @return 1 - ok, result stored in 'out'
int extractNumber(const std::string& in, unsigned int& out);
int extractNumber(const std::string& in, int& out);
int extractNumber(const std::string& in, long& out);
int extractNumber(const std::string& in, long long& out);
int extractNumber(const std::string& in, unsigned long& out);
int extractNumber(const std::string& in, unsigned long long& out);
/// \@}

/// creates a number to a string
std::string number2String(long long num);
std::string number2HexString(long long num);

/// Similar to `strstr' but this function ignores the case of both strings
const char *strcasestr(const char *str, const char *ptn);

/// expands ~ to value of ${HOME} enviroment variable
std::string expandFilename(const std::string &filename);

/// @return extension of filename (ex: filename.txt will return txt)
std::string findExtension(const std::string &filename);

/// is the char after a 'trigger' part of an alphabet?
/// @param in - string to analyze
/// @param trigger - check for char after trigger
/// @param alphabet - contains chars to search for
/// @param len_alphabet - length of alphabet
/// @param found - position of found char in alphabet (optional)
/// @return position of trigger if found
/// @return std::string::npos if nothing found
std::string::size_type findCharFromAlphabetAfterTrigger(const std::string& in,
    char trigger,
    const char alphabet[], size_t len_alphabet, size_t* found, const char ignore_after_trigger[] = "", size_t len_ignore_after_trigger = 0);

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

/// splits input at first non-leading whitespace and returns both parts
void getFirstWord(const std::string &in, std::string &first, std::string &rest);

template <typename Container>
static void stringTokensBetween(Container &container, const std::string &in,
        std::string &rest, char first, char last,
        const char *ok_chars = " \t\n", bool allow_nesting = true) {

    std::string token;
    int pos = 0;

    while (true) {
        int err = getStringBetween(token, in.c_str() + pos, first, last, ok_chars,
                               allow_nesting);
        if (err <= 0)
            break;
        container.push_back(token);
        pos += err;
    }

    rest = in.c_str() + pos;

}

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

/// Parse token, which might be in formats as follows: <int>, <int>% or *.
/// @param relative - parsed relative value (percentage suffix)
/// @param ignore - this token should be ignored (asterisk)
/// @return parsed integer value or 0 if not applicable
template <typename Container>
static int
parseSizeToken(Container &container, bool &relative, bool &ignore) {

    if (container.empty())
        return 0;

    relative = false;
    ignore = false;

    if (container[0] == '*') {
        ignore = true;
        return 0;
    }

    if (container[container.size() - 1] == '%')
        relative = true;

    return atoi(container.c_str());
}

} // end namespace StringUtil

} // end namespace FbTk


#endif // FBTK_STRINGUTIL_HH
