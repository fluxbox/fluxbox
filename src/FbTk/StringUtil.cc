// StringUtil.cc for fluxbox
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

#include "StringUtil.hh"

#include <cstdio>
#include <cctype>
#include <cassert>
#include <cstring>
#include <cerrno>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include <memory>
#include <algorithm>
#include <string>
#include <iostream>

using std::string;
using std::transform;

namespace {

const size_t DIGITS10_ULONGLONGINT = 20; // ULLONG_MAX = 18446744073709551615
const size_t DIGITS16_ULONGLONGINT = 16; // ULLONG_MAX = ffffffffffffffff

template <typename T>
int extractBigNumber(const char* in, T (*extractFunc)(const char*, char**, int), T& out) {

    errno = 0;

    int ret = 0;
    char* end = 0;
    T result = extractFunc(in, &end, 0);

    if (errno == 0 && end != in) {
        out = result;
        ret = 1;
    }

    return ret;
}

template<typename T>
int extractSignedNumber(const std::string& in, T& out) {

    long long int result = 0;

    if (::extractBigNumber(in.c_str(), strtoll, result)) {
        out = static_cast<T>(result);
        return 1;
    }

    return 0;
}

template<typename T>
int extractUnsignedNumber(const std::string& in, T& out) {

    unsigned long long int result = 0;

    if (::extractBigNumber(in.c_str(), strtoull, result)) {
        out = static_cast<T>(result);
        return 1;
    }

    return 0;
}


std::string getHomePath() {

    std::string home;
    const char* h = NULL;
#ifdef _WIN32
    h = getenv("USERPROFILE");
#else
    h = getenv("HOME");
#endif
    if (h) {
        home.assign(h);
    } else {
#ifndef _WIN32
        uid_t uid = geteuid();
        struct passwd* pw = getpwuid(uid);
        if (pw) {
            home.assign(pw->pw_dir);
        }
#endif
    }
    return home;
}

}


namespace FbTk {

namespace StringUtil {

int extractNumber(const std::string& in, int& out) {
    return ::extractSignedNumber<int>(in, out);
}

int extractNumber(const std::string& in, unsigned int& out) {
    return ::extractUnsignedNumber<unsigned int>(in, out);
}

int extractNumber(const std::string& in, long& out) {
    return ::extractSignedNumber<long>(in, out);
}

int extractNumber(const std::string& in, unsigned long& out) {
    return ::extractUnsignedNumber<unsigned long>(in, out);
}

int extractNumber(const std::string& in, long long& out) {
    return ::extractSignedNumber<long long>(in, out);
}

int extractNumber(const std::string& in, unsigned long long& out) {
    return ::extractUnsignedNumber<unsigned long long>(in, out);
}



std::string number2String(long long num) {
    char s[DIGITS10_ULONGLONGINT+1];
    int n = snprintf(s, sizeof(s), "%lld", num);
    return std::string(s, n);
}

std::string number2HexString(long long num) {
    char s[DIGITS16_ULONGLONGINT+1];
    int n = snprintf(s, sizeof(s), "%llx", num);
    return std::string(s, n);
}


/**
   Tries to find a string in another and
   ignoring the case of the characters
   Returns 0 on failure else pointer to str.
*/
const char *strcasestr(const char *str, const char *ptn) {
    const char *s2, *p2;
    for( ; *str; str++) {
        for(s2=str, p2=ptn; ; s2++,p2++) {
            // check if we reached the end of ptn, if so, return str
            if (!*p2)
                return str;
            // check if the chars match(ignoring case)
            if (toupper(*s2) != toupper(*p2))
                break;
        }
    }
    return 0;
}


#ifdef _WIN32

#include <string>
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>

static void removeTrailingPathSeparators(std::string & path) {
        // Remove any trailing path separators
        size_t beforeLastPathSep = path.find_last_not_of("/\\");
        if (beforeLastPathSep != path.size() - 1) {
            path.erase(beforeLastPathSep + 1);
        }
}

static std::string getFluxboxPrefix() {
    static std::string ret;
    static bool init = false;
    if (!init) {
        char buffer[1024];
        HMODULE module = GetModuleHandle(NULL);
        DWORD size = GetModuleFileName(module, buffer, sizeof(buffer));
        if (sizeof(buffer) > 0)
        {
          buffer[sizeof(buffer) - 1] = 0;
        }
        static const char slash = '/';
        static const char backslash = '\\';
        char * lastslash = std::find_end(buffer, buffer+size, &slash, &slash + 1);
        char * lastbackslash = std::find_end(buffer, buffer+size, &backslash, &backslash + 1);
        ret.assign(buffer);

        // Remove the filename
        size_t lastPathSep = ret.find_last_of("/\\");
        if (lastPathSep != std::string::npos) {
            ret.erase(lastPathSep);
        }

        removeTrailingPathSeparators(ret);

        // If the last directory is bin, remove that too.
        lastPathSep = ret.find_last_of("/\\");
        if (lastPathSep != std::string::npos && ret.substr(lastPathSep + 1) == "bin") {
            ret.erase(lastPathSep);
        }

        removeTrailingPathSeparators(ret);
    }
    return ret;
}

#endif // _WIN32

/**
   if ~ then expand it to home of user
   if /DUMMYPREFIX on Windows then expand it to the prefix relative to the
   executable on Windows.
   returns expanded filename
*/
string expandFilename(const string &filename) {
    string retval;
    size_t pos = filename.find_first_not_of(" \t");
    if (pos != string::npos && filename[pos] == '~') {
        retval = getHomePath();
        if (pos + 1 < filename.size()) {
            // copy from the character after '~'
            retval += static_cast<const char *>(filename.c_str() + pos + 1);
        }
    } else {
        retval = filename; //return unmodified value
    }

#if defined(_WIN32) && defined(DUMMYPREFIX)
    if (retval.find(DUMMYPREFIX) == 0) {
      static const std::string dummyPrefix = DUMMYPREFIX;
      retval.replace(0, dummyPrefix.size(), getFluxboxPrefix());
    }
#endif

    return retval;
}

/**
   @return string from last "." to end of string
*/
string findExtension(const string &filename) {
    //get start of extension
    string::size_type start_pos = filename.find_last_of(".");
    if (start_pos == string::npos && start_pos != filename.size())
        return "";
    // return from last . to end of string
    return filename.substr(start_pos + 1);
}

    string::size_type findCharFromAlphabetAfterTrigger(const std::string& in, char trigger, const char alphabet[], size_t len_alphabet, size_t* found, const char ignore_after_trigger[], size_t len_ignore_after_trigger) {
    for (const char* s = in.c_str(); *s != '\0'; ) {
        if (*s++ == trigger && *s != '\0') {
            auto end = ignore_after_trigger + len_ignore_after_trigger;
            while (std::find(ignore_after_trigger, end, *s) != end)
                s++;

            if (*s == '\0')
                return string::npos;

            for (const char* a = alphabet; (a - alphabet) < static_cast<ssize_t>(len_alphabet); ++a) {
                if (*s == *a) {
                    if (found) {
                        *found = a - alphabet;
                    }
                    return s - in.c_str() - 1;
                }
            }
            s++;
        }
    }
    return string::npos;
}


string replaceString(const string &original,
                     const char *findthis,
                     const char *replace) {
    size_t i = 0;
    const int size_of_replace = strlen(replace);
    const int size_of_find = strlen(findthis);
    string ret_str(original);
    while (i < ret_str.size()) {
        i = ret_str.find(findthis, i);
        if (i == string::npos)
            break;
        // erase old string and insert replacement
        ret_str.erase(i, size_of_find);
        ret_str.insert(i, replace);
        // jump to next position after insert
        i += size_of_replace;
    }

    return ret_str;
}

/**
   Parses a string between "first" and "last" characters
   and ignoring ok_chars as whitespaces. The value is
   returned in "out".
   Returns negative value on error and this value is the position
   in the in-string where the error occured.
   Returns positive value on success and this value is
   for the position + 1 in the in-string where the "last"-char value
   was found.
*/
int getStringBetween(string& out, const char *instr, char first, char last,
                     const char *ok_chars, bool allow_nesting) {
    assert(first);
    assert(last);
    assert(instr);

    string::size_type i = 0;
    string::size_type total_add=0; //used to add extra if there is a \last to skip
    string in(instr);

    // eat leading whitespace
    i = in.find_first_not_of(ok_chars);
    if (i == string::npos)
        return -in.size();   // nothing left but whitespace

    if (in[i]!=first)
        return -i; //return position to error

    // find the end of the token
    string::size_type j = i, k;
    int nesting = 0;
    while (1) {
        k = in.find_first_of(first, j+1);
        j = in.find_first_of(last, j+1);
        if (j==string::npos)
            return -in.size(); //send negative size

        if (allow_nesting && k < j && in[k-1] != '\\') {
            nesting++;
            j = k;
            continue;
        }
        //we found the last char, check so it doesn't have a '\' before
        if (j>1 && in[j-1] != '\\') {
            if (allow_nesting && nesting > 0) nesting--;
            else
                break;
        } else if (j>1 && !allow_nesting) { // we leave escapes if we're allowing nesting
            in.erase(j-1, 1); //remove the '\'
            j--;
            total_add++; //save numchars removed so we can calculate totalpos
        }
    }

    out = in.substr(i+1, j-i-1); //copy the string between first and last
    //return value to last character
    return (j+1+total_add);
}

string toLower(const string &conv) {
    string ret = conv;
    transform(ret.begin(), ret.end(), ret.begin(), tolower);
    return ret;
}

string toUpper(const string &conv) {
    string ret = conv;
    transform(ret.begin(), ret.end(), ret.begin(), toupper);
    return ret;
}

string basename(const string &filename) {
    string::size_type first_pos = filename.find_last_of("/");
    if (first_pos != string::npos)
        return filename.substr(first_pos + 1);
    return filename;
}

string::size_type removeFirstWhitespace(string &str) {
    string::size_type first_pos = str.find_first_not_of(" \t");
    str.erase(0, first_pos);
    return first_pos;
}


string::size_type removeTrailingWhitespace(string &str) {
    // strip trailing whitespace
    string::size_type first_pos = str.find_last_not_of(" \t");
    string::size_type last_pos = str.find_first_of(" \t", first_pos);
    if (last_pos != string::npos)
        str.erase(last_pos);
    return first_pos;
}

void getFirstWord(const std::string &in, std::string &word, std::string &rest) {
    word = in;
    string::size_type first_pos = StringUtil::removeFirstWhitespace(word);
    string::size_type second_pos = word.find_first_of(" \t", first_pos);
    if (second_pos != string::npos) {
        rest = word.substr(second_pos);
        word.erase(second_pos);
    }
}

} // end namespace StringUtil

} // end namespace FbTk
