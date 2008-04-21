// Parser.hh for FbTk
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef FBTK_PARSER_HH
#define FBTK_PARSER_HH

#include <string>
#include <utility>

namespace FbTk {

// interface class for a generic Parser
class Parser {
public:
    typedef std::pair<std::string, std::string> Item;
    static const Item s_empty_item;

    virtual ~Parser() { }

    virtual bool open(const std::string &filename) = 0;
    virtual void close() = 0;
    virtual bool eof() const = 0;
    virtual bool isLoaded() const = 0;
    virtual int row() const = 0;
    virtual std::string line() const = 0;
    virtual Parser &operator >> (Item &out) = 0;
    virtual Item nextItem() = 0;

};

} // end namespace FbTk

#endif // FBTK_PARSER_HH
