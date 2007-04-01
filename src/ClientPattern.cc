// ClientPattern.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "ClientPattern.hh"
#include "RegExp.hh"
#include "WinClient.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/stringstream.hh"

// use GNU extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <fstream>
#include <string>
#include <memory>
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

// needed as well for index on some systems (e.g. solaris)
#include <strings.h>

using std::string;


ClientPattern::ClientPattern():
    m_matchlimit(0),
    m_nummatches(0) {}

// parse the given pattern (to end of line)
ClientPattern::ClientPattern(const char *str):
    m_matchlimit(0),
    m_nummatches(0)
{
    /* A rough grammar of a pattern is:
       PATTERN ::= MATCH+ LIMIT?
       MATCH ::= '(' word ')'
                 | '(' propertyname '=' word ')'
       LIMIT ::= '{' number '}'

       i.e. one or more match definitions, followed by
            an optional limit on the number of apps to match to

       Match definitions are enclosed in parentheses, and if no
       property name is given, then CLASSNAME is assumed.
       If no limit is specified, no limit is applied (i.e. limit = infinity)
    */

    int had_error = 0;

    int pos = 0;
    string match;
    int err = 1; // for starting first loop
    while (had_error == 0 && err > 0) {
        err = FbTk::StringUtil::getStringBetween(match,
                                                 str + pos,
                                                 '(', ')', " \t\n", true);
        if (err > 0) {
            size_t eq = match.find_first_of('=');
            if (eq == match.npos) {
                if (!addTerm(match, NAME)) {
                    had_error = pos + match.find_first_of('(') + 1;
                    break;
                }
            } else {
                // need to determine the property used
                string memstr, expr;
                WinProperty prop;
                memstr.assign(match, 0, eq); // memstr = our identifier
                expr.assign(match, eq+1, match.length());
                if (strcasecmp(memstr.c_str(), "name") == 0) {
                    prop = NAME;
                } else if (strcasecmp(memstr.c_str(), "class") == 0) {
                    prop = CLASS;
                } else if (strcasecmp(memstr.c_str(), "title") == 0) {
                    prop = TITLE;
                } else if (strcasecmp(memstr.c_str(), "role") == 0) {
                    prop = ROLE;
                } else {
                    had_error = pos + match.find_first_of('(') + 1;
                    break;
                }
                if (!addTerm(expr, prop)) {
                    had_error = pos + ((str+pos) - index(str+pos, '=')) + 1;
                    break;
                }
            }
            pos += err;
        }
    }
    if (pos == 0 && had_error == 0) {
        // no match terms given, this is not allowed
        had_error = 1;
    }

    if (had_error == 0) {
        // otherwise, we check for a number
        string number;
        err = FbTk::StringUtil::getStringBetween(number,
                                             str+pos,
                                             '{', '}');
        if (err > 0) {
            FbTk_istringstream iss(number.c_str());
            iss >> m_matchlimit;
            pos+=err;
        }
        // we don't care if there isn't one

        // there shouldn't be anything else on the line
        match = str + pos;
        size_t uerr;// need a special type here
        uerr = match.find_first_not_of(" \t\n", pos);
        if (uerr != match.npos) {
            // found something, not good
            had_error++;
        }
    }

    if (had_error > 0) {
        m_matchlimit = had_error;
        // delete all the terms
        while (!m_terms.empty()) {
            Term * term = m_terms.back();
            delete term;
            m_terms.pop_back();
        }
    }
}

ClientPattern::~ClientPattern() {
    // delete all the terms
    while (!m_terms.empty()) {
        delete m_terms.back();
        m_terms.pop_back();
    }
}

// return a string representation of this pattern
string ClientPattern::toString() const {
    string pat;
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {

        pat.append(" (");

        switch ((*it)->prop) {
        case NAME:
            pat.append("name=");
            break;
        case CLASS:
            pat.append("class=");
            break;
        case TITLE:
            pat.append("title=");
            break;
        case ROLE:
            pat.append("role=");
        }

        pat.append((*it)->orig);
        pat.append(")");
    }

    if (m_matchlimit > 0) {
        char num[20];
        sprintf(num, " {%d}", m_matchlimit);
        pat.append(num);
    }
    return pat;
}

// does this client match this pattern?
bool ClientPattern::match(const WinClient &win) const {
    if (m_matchlimit != 0 && m_nummatches >= m_matchlimit ||
        m_terms.empty())
        return false; // already matched out

    // regmatch everything
    // currently, we use an "AND" policy for multiple terms
    // changing to OR would require minor modifications in this function only
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {
        if (!(*it)->regexp.match(getProperty((*it)->prop, win)))
            return false;
    }
    return true;
}

// add an expression to match against
// The first argument is a regular expression, the second is the member
// function that we wish to match against.
bool ClientPattern::addTerm(const string &str, WinProperty prop) {

    Term *term = new Term(str, true);
    term->orig = str;
    term->prop = prop;

    if (term->regexp.error()) {
        delete term;
        return false;
    }
    m_terms.push_back(term);
    return true;
}

string ClientPattern::getProperty(WinProperty prop, const WinClient &client) const {
    switch (prop) {
    case TITLE:
        return client.title();
        break;
    case CLASS:
        return client.getWMClassClass();
        break;
    case NAME:
        return client.getWMClassName();
        break;
    case ROLE:
        Atom wm_role = XInternAtom(FbTk::App::instance()->display(), "WM_WINDOW_ROLE", False);
        return client.textProperty(wm_role);
        break;
    }
    return client.getWMClassName();
}

bool ClientPattern::equals(const ClientPattern &pat) const {
    // we require the terms to be identical (order too)
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    Terms::const_iterator other_it = pat.m_terms.begin();
    Terms::const_iterator other_it_end = pat.m_terms.end();
    for (; it != it_end && other_it != other_it_end; ++it, ++other_it) {
        if ((*it)->orig != (*other_it)->orig)
            return false;
    }
    if (it != it_end || other_it != other_it_end)
        return false;

    return true;
}
