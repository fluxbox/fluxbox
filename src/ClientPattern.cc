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

#include <fstream>
#include <string>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstring>

// needed as well for index on some systems (e.g. solaris)
#include <strings.h>

using std::string;

namespace {

struct Name2WinProperty {
    const char* name;
    ClientPattern::WinProperty prop;
};

// sorted alphabetically for 'bsearch'
const Name2WinProperty name_2_winproperties[] = {
    { "class", ClientPattern::CLASS },
    { "focushidden", ClientPattern::FOCUSHIDDEN },
    { "fullscreen", ClientPattern::FULLSCREEN },
    { "head", ClientPattern::HEAD },
    { "iconhidden", ClientPattern::ICONHIDDEN },
    { "layer", ClientPattern::LAYER },
    { "maximized", ClientPattern::MAXIMIZED },
    { "maximizedhorizontal", ClientPattern::HORZMAX },
    { "maximizedvertical", ClientPattern::VERTMAX },
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

extern "C" {
int name_2_winproperty_cmp(const void* a, const void* b) {
    return strcmp(
            reinterpret_cast<const Name2WinProperty*>(a)->name,
            reinterpret_cast<const Name2WinProperty*>(b)->name);
}
}

const Name2WinProperty* find_winproperty_by_name(const FbTk::FbString& name) {

    const Name2WinProperty key = { name.c_str(), ClientPattern::CLASS };
    const Name2WinProperty* result = reinterpret_cast<Name2WinProperty*>(
            bsearch(&key, name_2_winproperties,
                sizeof(name_2_winproperties) / sizeof(Name2WinProperty),
                sizeof(Name2WinProperty),
                name_2_winproperty_cmp));

    return result;
}


struct Prop2String {
    ClientPattern::WinProperty prop;
    const char* str;
};

Prop2String property_2_strings[] = { // sorted by 'prop'
    { ClientPattern::TITLE, "title" },
    { ClientPattern::CLASS, "class" },
    { ClientPattern::NAME, "name" },
    { ClientPattern::ROLE, "role" },
    { ClientPattern::TRANSIENT, "transient" },
    { ClientPattern::MAXIMIZED,  "maximized" },
    { ClientPattern::MINIMIZED, "minimized" }, 
    { ClientPattern::SHADED, "shaded" },
    { ClientPattern::STUCK, "stuck" },
    { ClientPattern::FOCUSHIDDEN, "focushidden" },
    { ClientPattern::ICONHIDDEN, "iconhidden" },
    { ClientPattern::WORKSPACE, "workspace" },
    { ClientPattern::WORKSPACENAME, "workspacename" },
    { ClientPattern::HEAD, "head" },
    { ClientPattern::LAYER, "layer" },
    { ClientPattern::URGENT, "urgent" },
    { ClientPattern::SCREEN, "screen" },
    { ClientPattern::XPROP, "@" },
    { ClientPattern::FULLSCREEN, "fullscreen" },
    { ClientPattern::VERTMAX, "maximizedvertical" },
    { ClientPattern::HORZMAX, "maximizedhorizontal" },
};


} // end of anonymous namespace


/**
 * This is the type of the actual pattern we want to match against
 * We have a "term" in the whole expression which is the full pattern
 * we also need to keep track of the uncompiled regular expression
 * for final output
 */
struct ClientPattern::Term {

    Term(const FbTk::FbString& _regstr, WinProperty _prop, bool _negate, const FbTk::FbString& _xprop) :
        regstr(_regstr),
        xpropstr(_xprop),
        regexp(_regstr, true),
        prop(_prop),
        negate(_negate) {

        xprop = XInternAtom(FbTk::App::instance()->display(), xpropstr.c_str(), False);
    }

    // (title=.*bar) or (@FOO=.*bar)
    FbTk::FbString regstr;     // .*bar
    FbTk::FbString xpropstr;  // @FOO=.*bar
    Atom xprop;                // Atom of 'FOO'
    FbTk::RegExp regexp;       // compiled version of '.*bar'
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

            WinProperty prop = NAME;
            std::string expr;
            std::string xprop;
            bool negate = false;

            // need to determine the property used, potential patterns:
            //
            //  A) foo               (short for 'title=foo')
            //  B) foo=bar
            //  C) foo!=bar
            //
            //  D) @foo=bar          (xproperty 'foo' equal to 'bar')
            //

            string propstr = match;
            string::size_type eq = propstr.find_first_of('=');

            if (eq == propstr.npos) {           // A
                expr = "[current]";
            } else {                            // B or C, so strip away the '='

                // 'bar'
                expr.assign(propstr.begin() + eq + 1, propstr.end());

                // 'foo' or 'foo!'
                propstr.resize(eq);
                if (propstr.rfind("!", propstr.npos, 1) != propstr.npos) { // C 'foo!'
                    negate = true;
                    propstr.resize(propstr.size()-1);
                }
            }

            if (propstr[0] != '@') { // not D

                const Name2WinProperty* p = find_winproperty_by_name(FbTk::StringUtil::toLower(propstr));

                if (p) {
                    prop = p->prop;
                } else {
                    expr = match;
                }
            } else { // D
                prop = XPROP;
                xprop.assign(propstr, 1, propstr.size());
            }

            had_error = !addTerm(expr, prop, negate, xprop);
            pos += err;
        }
    }
    if (pos == 0 && !had_error) { // no match terms given, this is not allowed
        had_error = true;
    }

    if (!had_error) { // otherwise, we check for a number

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
    string result;
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    for (; it != it_end; ++it) {
        const Term& term = *(*it);
        result.append(" (");
        result.append(property_2_strings[term.prop].str);
        if (term.prop == XPROP)
            result.append(term.xpropstr);
        result.append(term.negate ? "!=" : "=");
        result.append(term.regstr);
        result.append(")");
    }

    if (m_matchlimit > 0) {
        result.append(" {");
        result.append(FbTk::StringUtil::number2String(m_matchlimit));
        result.append("}");
    }
    return result;
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
        if (term.prop == XPROP) {
            if (!term.negate ^ ((term.regexp.match(win.getTextProperty(term.xprop))) || term.regexp.match(FbTk::StringUtil::number2String(win.getCardinalProperty(term.xprop)))))
                return false;
        } else if (term.regstr == "[current]") {
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
        } else if (term.prop == HEAD && term.regstr == "[mouse]") {
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
            (*it)->regstr == "[current]")
            return true;
    }
    return false;
}

bool ClientPattern::dependsOnCurrentWorkspace() const {
    Terms::const_iterator it = m_terms.begin(), it_end = m_terms.end();
    for (; it != it_end; ++it) {
        if (((*it)->prop == WORKSPACE || (*it)->prop == WORKSPACENAME) &&
            (*it)->regstr == "[current]")
            return true;
    }
    return false;
}

// add an expression to match against
// The first argument is a regular expression, the second is the member
// function that we wish to match against.
bool ClientPattern::addTerm(const FbTk::FbString &str, WinProperty prop, bool negate, const FbTk::FbString& xprop) {

    bool rc = false;
    Term* term = new Term(str, prop, negate, xprop);

    if (!term)
        return rc;

    if ((rc = !term->regexp.error())) {
        m_terms.push_back(term);
    } else {
        delete term;
    }

    return rc;
}

FbTk::FbString ClientPattern::getProperty(WinProperty prop, const Focusable &client) {

    FbTk::FbString result;

    // we need this for some of the window properties
    const FluxboxWindow *fbwin = client.fbwindow();

    switch (prop) {
    case TITLE:
        result = client.title().logical();
        break;
    case CLASS:
        result = client.getWMClassClass();
        break;
    case ROLE:
        result = client.getWMRole();
        break;
    case TRANSIENT:
        result = client.isTransient() ? "yes" : "no";
        break;
    case MAXIMIZED:
        result = (fbwin && fbwin->isMaximized()) ? "yes" : "no";
        break;
    case MINIMIZED:
        result = (fbwin && fbwin->isIconic()) ? "yes" : "no";
        break;
    case FULLSCREEN:
        result = (fbwin && fbwin->isFullscreen()) ? "yes" : "no";
        break;
    case VERTMAX:
        result = (fbwin && fbwin->isMaximizedVert()) ? "yes" : "no";
        break;
    case HORZMAX:
        result = (fbwin && fbwin->isMaximizedHorz()) ? "yes" : "no";
        break;
    case SHADED:
        result = (fbwin && fbwin->isShaded()) ? "yes" : "no";
        break;
    case STUCK:
        result = (fbwin && fbwin->isStuck()) ? "yes" : "no";
        break;
    case FOCUSHIDDEN:
        result = (fbwin && fbwin->isFocusHidden()) ? "yes" : "no";
        break;
    case ICONHIDDEN:
        result = (fbwin && fbwin->isIconHidden()) ? "yes" : "no";
        break;
    case WORKSPACE: {
        unsigned int wsnum = (fbwin ? fbwin->workspaceNumber() : client.screen().currentWorkspaceID());
        result = FbTk::StringUtil::number2String(wsnum);
        break;
    }
    case WORKSPACENAME: {
        const Workspace *w = (fbwin ?
                client.screen().getWorkspace(fbwin->workspaceNumber()) :
                client.screen().currentWorkspace());
        if (w) {
            result = w->name();
        }
        break;
    }
    case HEAD:
        if (fbwin) {
            result = FbTk::StringUtil::number2String(client.screen().getHead(fbwin->fbWindow()));
        }
        break;
    case LAYER:
        if (fbwin) {
            result = ::ResourceLayer::getString(fbwin->layerNum());
        }
        break;
    case URGENT:
        result = Fluxbox::instance()->attentionHandler()
                .isDemandingAttention(client) ? "yes" : "no";
        break;
    case SCREEN:
        result = FbTk::StringUtil::number2String(client.screen().screenNumber());
        break;

    case XPROP:
        break;

    case NAME:
    default:
        result = client.getWMClassName();
        break;
    }
    return result;
}

bool ClientPattern::operator ==(const ClientPattern &pat) const {
    // we require the terms to be identical (order too)
    Terms::const_iterator it = m_terms.begin();
    Terms::const_iterator it_end = m_terms.end();
    Terms::const_iterator other_it = pat.m_terms.begin();
    Terms::const_iterator other_it_end = pat.m_terms.end();
    for (; it != it_end && other_it != other_it_end; ++it, ++other_it) {
        const Term& i = *(*it);
        const Term& o = *(*other_it);
        if (i.regstr != o.regstr ||
            i.negate != o.negate ||
            i.xpropstr != o.xpropstr)
            return false;
    }
    if (it != it_end || other_it != other_it_end)
        return false;

    return true;
}

