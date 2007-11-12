// ClientPattern.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
// Copyright (c) 2002 Xavier Brouckaert
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

#ifndef CLIENTPATTERN_HH
#define CLIENTPATTERN_HH

#include "RegExp.hh"
#include "NotCopyable.hh"

#include <string>
#include <list>

class Focusable;

/**
 * This class represents a "pattern" that we can match against a
 * Window based on various properties.
 */
class ClientPattern:private FbTk::NotCopyable {
public:
    ClientPattern();
    /**
     * Create the pattern from the given string as it would appear in the 
     * apps file. the bool value returns the character at which
     * there was a parse problem, or -1.
     */
    explicit ClientPattern(const char * str, bool default_no_transient = false);

    ~ClientPattern();

    /// @return a string representation of this pattern
    std::string toString() const;

    enum WinProperty {
        TITLE, CLASS, NAME, ROLE, TRANSIENT,
        MAXIMIZED, MINIMIZED, SHADED, STUCK, FOCUSHIDDEN, ICONHIDDEN,
        WORKSPACE, WORKSPACENAME, HEAD, LAYER
    };

    /// Does this client match this pattern?
    bool match(const Focusable &win) const;

    /// Does this pattern depend on the focused window?
    bool dependsOnFocusedWindow() const;

    /// Does this pattern depend on the current workspace?
    bool dependsOnCurrentWorkspace() const;

    /**
     * Add an expression to match against
     * @param str is a regular expression
     * @param prop is the member function that we wish to match against
     * @return false if the regexp wasn't valid
     */
    bool addTerm(const std::string &str, WinProperty prop, bool negate = false);

    inline void addMatch() { ++m_nummatches; }

    // whether this pattern has identical matching criteria
    bool operator ==(const ClientPattern &pat) const;

    /**
     * If there are no terms, then there is assumed to be an error
     * the column of the error is stored in m_matchlimit
     */
    inline int error() const { return m_terms.empty() ? 1 : 0; }

    static std::string getProperty(WinProperty prop, const Focusable &client);

private:
    /**
     * This is the type of the actual pattern we want to match against
     * We have a "term" in the whole expression which is the full pattern
     * we also need to keep track of the uncompiled regular expression
     * for final output
     */    
    struct Term {
        Term(const std::string &regstr, bool full_match) :regexp(regstr, full_match){};
        std::string orig;
        RegExp regexp;
        WinProperty prop;
        bool negate;
    };


    typedef std::list<Term *> Terms;

    Terms m_terms; ///< our pattern is made up of a sequence of terms currently we "and" them all

    int m_matchlimit, m_nummatches;
};

#endif // CLIENTPATTERN_HH
