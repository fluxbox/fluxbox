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

#include <cstring>

namespace {

// calcs longest substring of 'text', fitting into 'n_pixels'
// 'text_len' is an in-out parameter
// 'text_width' is out parameter
void maxTextLength(int n_pixels, const FbTk::Font& font, const char* const text,
        unsigned int& text_len, int& text_width) {

    text_width = font.textWidth(text, text_len);

    // rendered text exceeds n_pixels. calculate 'len' to cut off 'text'.
    if (text_width > n_pixels) {

        // there is less room for thicker glyphs than for
        // thin glyphs. 'text' contains usually a good mix of both. to decide
        // upon where we cut off glyphs from 'text', we do a binary search
        // over 'text' to find the optimal length that fits into 'n_pixels'.
        //
        // by assuming a text that consists of only thick glyphs ("WW") and
        // a text that consist of only thin glyphs (".") we find a good
        // start to binary search:
        //
        //                +---right
        //  [...........|.R ]
        //  [WWWWWWWWWWL|   ]
        //             +------left
        //              n_pixels

        int right = n_pixels / (font.textWidth(".", 1) + 1);
        int left = n_pixels / (font.textWidth("WW", 2) + 1);
        int middle;

        // binary search for longest substring fitting into 'max_width' pixels
        for ( ; left < (right - 1); ) {

            middle = left + ((right - left) / 2);
            text_width = font.textWidth(text, middle);

            if (text_width < n_pixels) {
                left = middle;
            } else {
                right = middle;
            }
        }

        text_len = left;
    }
}

}

namespace FbTk {

int doAlignment(int n_pixels, int bevel, FbTk::Justify justify,
                const FbTk::Font &font, const char * const text,
                unsigned int textlen, unsigned int &newlen) {

    if (text == 0 || textlen == 0)
        return 0;

    int text_width;

    maxTextLength(n_pixels - bevel, font, text, textlen, text_width);

    newlen = textlen;

    if (justify == FbTk::RIGHT) {
        return n_pixels - text_width;
    } else if (justify == FbTk::CENTER) {
        return (n_pixels - text_width + bevel)/2;
    }

    return bevel;
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
