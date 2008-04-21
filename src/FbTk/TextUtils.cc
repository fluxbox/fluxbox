// Text.cc for FbTk - text utils
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "TextUtils.hh"

#include "Font.hh"
#include "Theme.hh"

#include <strings.h>

namespace FbTk {

int doAlignment(int max_width, int bevel, FbTk::Justify justify,
                const FbTk::Font &font, const char * const text,
                unsigned int textlen, unsigned int &newlen) {

    if (text == 0 || textlen == 0)
        return 0;

    int l = font.textWidth(text, textlen) + bevel;
    unsigned int dlen = textlen;
    int dx = bevel;
    if (l > max_width) {
        for (; dlen > 0; dlen--) {
            l = font.textWidth(text, dlen) + bevel;
            if (l<=max_width)
                break;
        }
    }

    newlen = dlen;

    switch (justify) {
    case FbTk::RIGHT:
        dx = max_width - l - bevel;
    break;
    case FbTk::CENTER:
        dx = (max_width - l)/2;
    break;
    case FbTk::LEFT:
    break;
    }

    return dx;
}


/// specialization for Justify
template <>
void ThemeItem<FbTk::Justify>::setDefaultValue() {
    m_value = LEFT;
}

template <>
void ThemeItem<FbTk::Justify>::setFromString(const char *value) {
    if (strcasecmp("center", value) == 0)
        m_value = FbTk::CENTER;
    else if (strcasecmp("right", value) == 0)
        m_value = FbTk::RIGHT;
    else // default
        setDefaultValue();
}

// do nothing
template <>
void ThemeItem<FbTk::Justify>::load(const std::string *name, const std::string *altname) {
}

} // end namespace FbTk
