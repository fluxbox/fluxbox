// ScreenResources.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// holds screen resource handling

#include "Screen.hh"
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;

namespace FbTk {

template <>
string FbTk::Resource<FbTk::MenuTheme::MenuMode>::getString() const {
    switch (*(*this)) {
    case FbTk::MenuTheme::DELAY_OPEN:
        return string("Delay");
    case FbTk::MenuTheme::CLICK_OPEN:
        return string("Click");
    }
    return string("Delay");
}

template <>
void FbTk::Resource<FbTk::MenuTheme::MenuMode>::setFromString(const char *str) {
    if (strcasecmp(str, "Delay") == 0)
        *(*this) = FbTk::MenuTheme::DELAY_OPEN;
    else if (strcasecmp(str, "Click") == 0)
        *(*this) = FbTk::MenuTheme::CLICK_OPEN;
    else
        setDefaultValue();
}
} // end namespace FbTk
