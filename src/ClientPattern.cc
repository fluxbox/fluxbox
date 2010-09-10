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

#include "ClientPattern.hh"

#include "fluxbox.hh"
#include "FocusControl.hh"
#include "Layer.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Workspace.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/stringstream.hh"
#include "FbTk/STLUtil.hh"

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

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

// needed as well for index on some systems (e.g. solaris)
#include <strings.h>

using std::string;

namespace {

struct Name2WinProperty {
    const char* name;
    ClientPattern::WinProperty prop;
};

Name2WinProperty name_2_winproperties[] = { // sorted for 'bsearch'
    { "class", ClientPattern::CLASS },
    { "focushidden", ClientPattern::FOCUSHIDDEN },
    { "head", ClientPattern::HEAD },
    { "iconhidden", ClientPattern::ICONHIDDEN },
    { "layer", ClientPattern::LAYER },
    { "maximized", ClientPattern::MAXIMIZED },
    { "minimized", ClientPattern::MINIMIZED },
    { "name", ClientPattern::NAME },
    { "role", ClientPattern::ROLE },
    { "screen", ClientPattern::SCREEN },
    { "shaded", ClientPattern::SHADED },
    { "stuck", ClientPattern::STUCK },
    { "title", ClientPattern::TITLE },
    { "transient", ClientPattern::TRANSIENT },
    { "urgent", ClientPattern::URGENT },
    { "workspace", ClientPattern::WORKSPACE },
    { "workspacename", ClientPattern::WORKSPACENAME }
};

int name_2_winproperty_cmp(const void* a, const void* b) {
    return strcmp(
            reinterpret_cast<const Name2WinProperty*>(a)->name,
            reinterpret_cast<const Name2WinProperty*>(b)->name);
}

struct Prop2String {
    ClientPattern::WinProperty prop;
    const char* str;
};

Prop2String property_2_strings[] = { // sorted by 'prop'
    { ClientPattern::TITLE, "title=" },
    { ClientPattern::CLASS, "class=" },
    { ClientPattern::NAME, "name=" },
    { ClientPattern::ROLE, "role=" },
    { ClientPattern::TRANSIENT, "transient=" },
    { ClientPattern::MAXIMIZED,  "maximized=" },
    { ClientPattern::MINIMIZED, "minimized=" }, 
    { ClientPattern::SHADED, "shaded=" },
    { ClientPattern::STUCK, "stuck=" },
    { ClientPattern::FOCUSHIDDEN, "focushidden=" },
    { ClientPattern::ICONHIDDEN, "iconhidden=" },
    { ClientPattern::WORKSPACE, "workspace=" },
    { ClientPattern::WORKSPACENAME, "workspacename=" },
    { ClientPattern::HEAD, "head=" },
    { ClientPattern::LAYER, "layer=" },
    { ClientPattern::URGENT, "urgent=" },
    { ClientPattern::SCREEN, "screen=" }
};


} // end of anonymous namespace


/**
 * This is the type of the actual pattern we want to match against
 * We have a "term" in the whole expression which is the full pattern
 * we also need to keep track of the uncompiled regular expression
 * for final output
 */
struct ClientPattern::Term {

    Term(const FbTk::FbString& _regstr, WinProperty _prop, bool _negate) :
        orig(_regstr),
        regexp(_regstr, true),
        prop(_prop),
        negate(_negate) {

    }

    FbTk::FbString orig;
    FbTk::RegExp regexp;
    WinProperty prop;
    bool negate;
};

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

    bool had_error = false;

    int pos = 0;
    string match;
    int err = 1; // for starting first loop
    while (!had_error && err > 0) {
        err = FbTk::StringUtil::getStringBetween(match,
                                                 str + pos,
                                                 '(', ')', " \t\n", true);
        if (err > 0) {
            // need to determine the property used
            string memstr, expr;
            WinProperty prop;
            string::size_type eq = match.find_first_of('=');
            if (eq == match.npos) {
                memstr = match;
                expr = "[current]";
            } else {
                memstr.assign(match, 0, eq); // memstr = our identifier
                expr.assign(match, eq+1, match.length());
            }
            bool negate = false;
            if (!memstr.empty() && memstr[memstr.length()-1] == '!') {
                negate = true;
                memstr.assign(memstr, 0, memstr.length()-1);
            }

            memstr = FbTk::StringUtil::toLower(memstr);

            Name2WinProperty key = { memstr.c_str(), CLASS };
            Name2WinProperty* i = reinterpret_cast<Name2WinProperty*>(
                bsearch(&key, name_2_winproperties,
                    sizeof(name_2_winproperties) / sizeof(Name2WinProperty),
                    sizeof(Name2WinProperty),
                    name_2_winproperty_cmp));

            if (i) {
                prop = i->prop;
            } else {
                prop = NAME;
                expr = match;
            }

            had_error = !addTerm(expr, prop, negate);
            pos += err;
        }
    }
    if (pos == 0 && !had_error) {
        // no match terms given, this is not allowed
        had_error = true;
    }

    if (!had_error) {
        // otherwise, we check for a number
        string number;
        err = FbTk::StringUtil::getStringBetween(number,
                                             str+pos,
                                             '{', '}');
        if (err > 0) {
            FbTk::StringUtil::extractNumber(number, m_matchlimit);
            pos+=err;
        }
        // we don't care if there isn't one

        // there shouldn't be anything else on the line
        match = str + pos;
        size_t uerr;// need a special type here
        uerr = match.find_first_not_of(" \t\n", pos);
        if (uerr != match.npos) {
            // found something, not good
            had_error = true;
        }
    }

    if (had_error) {
        FbTk::STLUtil::destroyAndClear(m_terms);
    }
}

ClientPattern::~ClientPattern() {
    FbTk::STLUtil::destroyAndClear(m_terms);
}

// return a string representation of this pattern
string ClientPattern::toString() const {
    string pat;
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {

        pat.append(" (");
        pat.append(property_2_strings[(*it)->prop].str);
        pat.append((*it)->orig);
        pat.append(")");
    }

    if (m_matchlimit > 0) {
        pat.append(" {");
        pat.append(FbTk::StringUtil::number2String(m_matchlimit));
        pat.append("}");
    }
    return pat;
}

// does this client match this pattern?
bool ClientPattern::match(const Focusable &win) const {
    if (m_matchlimit != 0 && m_nummatches >= m_matchlimit)
        return false; // already matched out

    // regmatch everything
    // currently, we use an "AND" policy for multiple terms
    // changing to OR would require minor modifications in this function only
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {
        const Term& term = *(*it);
        if (term.orig == "[current]") {
            WinClient *focused = FocusControl::focusedWindow();
            if (term.prop == WORKSPACE) {
                if (!term.negate ^ (getProperty(term.prop, win) == FbTk::StringUtil::number2String(win.screen().currentWorkspaceID())))
                    return false;
            } else if (term.prop == WORKSPACENAME) {
                const Workspace *w = win.screen().currentWorkspace();
                if (!w || (!term.negate ^ (getProperty(term.prop, win) == w->name())))
                    return false;
            } else if (!focused || (!term.negate ^ (getProperty(term.prop, win) == getProperty(term.prop, *focused))))
                return false;
        } else if (term.prop == HEAD && term.orig == "[mouse]") {
            if (!term.negate ^ (getProperty(term.prop, win) == FbTk::StringUtil::number2String(win.screen().getCurrHead())))
                return false;

        } else if (!term.negate ^ term.regexp.match(getProperty(term.prop, win)))
            return false;
    }
    return true;
}

bool ClientPattern::dependsOnFocusedWindow() const {
    Terms::const_iterator it = m_terms.begin(), it_end = m_terms.end();
    for (; it != it_end; ++it) {
        if ((*it)->prop != WORKSPACE && (*it)->prop != WORKSPACENAME &&
            (*it)->orig == "[current]")
            return true;
    }
    return false;
}

bool ClientPattern::dependsOnCurrentWorkspace() const {
    Terms::const_iterator it = m_terms.begin(), it_end = m_terms.end();
    for (; it != it_end; ++it) {
        if (((*it)->prop == WORKSPACE || (*it)->prop == WORKSPACENAME) &&
            (*it)->orig == "[current]")
            return true;
    }
    return false;
}

// add an expression to match against
// The first argument is a regular expression, the second is the member
// function that we wish to match against.
bool ClientPattern::addTerm(const FbTk::FbString &str, WinProperty prop, bool negate) {

    bool rc = false;
    Term* term = new Term(str, prop, negate);

    if (!term)
        return rc;

    if (!term->regexp.error()) {
        m_terms.push_back(term);
        rc = true;
    } else {
        delete term;
    }

    return rc;
}

FbTk::FbString ClientPattern::getProperty(WinProperty prop, const Focusable &client) {
    // we need this for some of the window properties
    const FluxboxWindow *fbwin = client.fbwindow();

    switch (prop) {
    case TITLE:
        return client.title().logical();
        break;
    case CLASS:
        return client.getWMClassClass();
        break;
    case NAME:
        return client.getWMClassName();
        break;
    case ROLE:
        return client.getWMRole();
        break;
    case TRANSIENT:
        return client.isTransient() ? "yes" : "no";
        break;
    case MAXIMIZED:
        return (fbwin && fbwin->isMaximized()) ? "yes" : "no";
        break;
    case MINIMIZED:
        return (fbwin && fbwin->isIconic()) ? "yes" : "no";
        break;
    case SHADED:
        return (fbwin && fbwin->isShaded()) ? "yes" : "no";
        break;
    case STUCK:
        return (fbwin && fbwin->isStuck()) ? "yes" : "no";
        break;
    case FOCUSHIDDEN:
        return (fbwin && fbwin->isFocusHidden()) ? "yes" : "no";
        break;
    case ICONHIDDEN:
        return (fbwin && fbwin->isIconHidden()) ? "yes" : "no";
        break;
    case WORKSPACE: {
        unsigned int wsnum = (fbwin ? fbwin->workspaceNumber() : client.screen().currentWorkspaceID());
        return FbTk::StringUtil::number2String(wsnum);
        break;
    }
    case WORKSPACENAME: {
        const Workspace *w = (fbwin ?
                client.screen().getWorkspace(fbwin->workspaceNumber()) :
                client.screen().currentWorkspace());
        return w ? w->name() : "";
        break;
    }
    case HEAD: {
        if (!fbwin)
            return "";
        int head = client.screen().getHead(fbwin->fbWindow());
        return FbTk::StringUtil::number2String(head);
        break;
    }
    case LAYER:
        return fbwin ? ::Layer::getString(fbwin->layerNum()) : "";
        break;
    case URGENT:
        return Fluxbox::instance()->attentionHandler()
                .isDemandingAttention(client) ? "yes" : "no";
        break;
    case SCREEN: {
        int screenId = client.screen().screenNumber();
        return FbTk::StringUtil::number2String(screenId);
        break;
    }
    }
    return client.getWMClassName();
}

bool ClientPattern::operator ==(const ClientPattern &pat) const {
    // we require the terms to be identical (order too)
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    Terms::const_iterator other_it = pat.m_terms.begin();
    Terms::const_iterator other_it_end = pat.m_terms.end();
    for (; it != it_end && other_it != other_it_end; ++it, ++other_it) {
        if ((*it)->orig != (*other_it)->orig ||
            (*it)->negate != (*other_it)->negate)
            return false;
    }
    if (it != it_end || other_it != other_it_end)
        return false;

    return true;
}
