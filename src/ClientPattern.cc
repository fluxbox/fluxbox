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

            if (memstr == "name") {
                prop = NAME;
            } else if (memstr == "class") {
                prop = CLASS;
            } else if (memstr == "title") {
                prop = TITLE;
            } else if (memstr == "role") {
                prop = ROLE;
            } else if (memstr == "transient") {
                prop = TRANSIENT;
            } else if (memstr == "maximized") {
                prop = MAXIMIZED;
            } else if (memstr == "minimized") {
                prop = MINIMIZED;
            } else if (memstr == "shaded") {
                prop = SHADED;
            } else if (memstr == "stuck") {
                prop = STUCK;
            } else if (memstr == "focushidden") {
                prop = FOCUSHIDDEN;
            } else if (memstr == "iconhidden") {
                prop = ICONHIDDEN;
            } else if (memstr == "workspace") {
                prop = WORKSPACE;
            } else if (memstr == "workspacename") {
                prop = WORKSPACENAME;
            } else if (memstr == "head") {
                prop = HEAD;
            } else if (memstr == "layer") {
                prop = LAYER;
            } else if (memstr == "urgent") {
                prop = URGENT;
            } else if (memstr == "screen") {
                prop = SCREEN;
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
            had_error = true;
        }
    }

    if (had_error) {
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
            break;
        case TRANSIENT:
            pat.append("transient=");
            break;
        case MAXIMIZED:
            pat.append("maximized=");
            break;
        case MINIMIZED:
            pat.append("minimized=");
            break;
        case SHADED:
            pat.append("shaded=");
            break;
        case STUCK:
            pat.append("stuck=");
            break;
        case FOCUSHIDDEN:
            pat.append("focushidden=");
            break;
        case ICONHIDDEN:
            pat.append("iconhidden=");
            break;
        case WORKSPACE:
            pat.append("workspace=");
            break;
        case WORKSPACENAME:
            pat.append("workspacename=");
            break;
        case HEAD:
            pat.append("head=");
            break;
        case LAYER:
            pat.append("layer=");
            break;
        case URGENT:
            pat.append("urgent=");
            break;
        case SCREEN:
            pat.append("screen=");
            break;
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
bool ClientPattern::match(const Focusable &win) const {
    if (m_matchlimit != 0 && m_nummatches >= m_matchlimit)
        return false; // already matched out

    // regmatch everything
    // currently, we use an "AND" policy for multiple terms
    // changing to OR would require minor modifications in this function only
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {
        if ((*it)->orig == "[current]") {
            WinClient *focused = FocusControl::focusedWindow();
            if ((*it)->prop == WORKSPACE) {
                if (!(*it)->negate ^ (getProperty((*it)->prop, win) == FbTk::StringUtil::number2String(win.screen().currentWorkspaceID())))
                    return false;
            } else if ((*it)->prop == WORKSPACENAME) {
                const Workspace *w = win.screen().currentWorkspace();
                if (!w || (!(*it)->negate ^
                           (getProperty((*it)->prop, win) == w->name())))
                    return false;
            } else if (!focused || (!(*it)->negate ^
                                    (getProperty((*it)->prop, win) ==
                                     getProperty((*it)->prop, *focused))))
                return false;
        } else if ((*it)->prop == HEAD && (*it)->orig == "[mouse]") {
            if (!(*it)->negate ^ (getProperty((*it)->prop, win) == FbTk::StringUtil::number2String(win.screen().getCurrHead())))
                return false;

        } else if (!(*it)->negate ^ (*it)->regexp.match(getProperty((*it)->prop, win)))
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
bool ClientPattern::addTerm(const string &str, WinProperty prop, bool negate) {

    Term *term = new Term(str, true);
    term->orig = str;
    term->prop = prop;
    term->negate = negate;

    if (term->regexp.error()) {
        delete term;
        return false;
    }
    m_terms.push_back(term);
    return true;
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
