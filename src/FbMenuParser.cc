// FbMenuParser.cc for Fluxbox
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

#include "FbMenuParser.hh"

#include "FbTk/StringUtil.hh"

bool FbMenuParser::open(const std::string &filename) {
    m_file.open(filename.c_str());
    m_curr_pos = 0;
    m_row = 0;
    m_curr_token = DONE;
    return isLoaded();
}

FbTk::Parser &FbMenuParser::operator >> (FbTk::Parser::Item &out) {
    if (eof()) {        
        out = FbTk::Parser::s_empty_item;
        return *this; 
    }

    if (m_curr_line.empty())
        m_curr_token = DONE; // try next line

    char first = '[';
    char second = ']';

    switch (m_curr_token) {
    case TYPE:
        first = '[';
        second = ']';
        break;
    case NAME:
        first = '(';
        second = ')';
        break;
    case ARGUMENT:
        first = '{';
        second = '}';
        break;
    case ICON:
        first = '<';
        second = '>';
        break;
    case DONE: // get new line and call this again
        if (!nextLine()) {
            out = FbTk::Parser::s_empty_item;
            return *this;
        }
        return (*this)>>out;
        break;
    }
    
    std::string key;
    int err = FbTk::StringUtil::
        getStringBetween(key, m_curr_line.c_str() + m_curr_pos,
                         first, second);
    if (err <= 0) {        
        if (m_curr_token == TYPE)
            m_curr_token = NAME;
        else if (m_curr_token == NAME)
            m_curr_token = ARGUMENT;
        else if (m_curr_token == ARGUMENT)
            m_curr_token = ICON;
        else if (m_curr_token == ICON)
            m_curr_token = DONE;

        out = FbTk::Parser::s_empty_item;
        return *this;
    }

    m_curr_pos += err; // update current position in current line

    // set value
    out.second = key;

    // set type and next token to be read
    switch (m_curr_token) {
    case TYPE:
        out.first = "TYPE";
        m_curr_token = NAME;
        break;
    case NAME:
        out.first = "NAME";
        m_curr_token = ARGUMENT;
        break;
    case ARGUMENT:
        out.first = "ARGUMENT";
        m_curr_token = ICON;
        break;
    case ICON:
        out.first = "ICON";
        m_curr_token = DONE;
        break;
    case DONE:
        break;
    }
    return *this;
}

FbTk::Parser::Item FbMenuParser::nextItem() {
    FbTk::Parser::Item item;
    (*this)>>item;
    return item;
}

bool FbMenuParser::nextLine() {
    if (!std::getline(m_file, m_curr_line))
        return false;

    m_row++;
    m_curr_pos = 0;
    m_curr_token = TYPE;

    return true;
}
