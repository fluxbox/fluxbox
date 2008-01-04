// FbMenuParser.hh for Fluxbox
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FBMENUPARSER_HH
#define FBMENUPARSER_HH

#include "FbTk/Parser.hh"

#include <fstream>

class FbMenuParser: public FbTk::Parser {
public:
    FbMenuParser():m_row(0), m_curr_pos(0), m_curr_token(TYPE) {}
    FbMenuParser(const std::string &filename):m_row(0), m_curr_pos(0),
                                              m_curr_token(TYPE) { open(filename); }
    ~FbMenuParser() { close(); }

    bool open(const std::string &filename);
    void close() { m_file.close(); }
    FbTk::Parser &operator >> (FbTk::Parser::Item &out);
    FbTk::Parser::Item nextItem();

    bool isLoaded() const { return m_file.is_open(); }
    bool eof() const { return m_file.eof(); }
    int row() const { return m_row; }
    std::string line() const { return m_curr_line; }
private:
    bool nextLine();

    mutable std::ifstream m_file;
    int m_row;
    int m_curr_pos;
    std::string m_curr_line;
    enum Object {TYPE, NAME, ARGUMENT, ICON, DONE} m_curr_token;
};

#endif // FBMENUPARSER_HH
